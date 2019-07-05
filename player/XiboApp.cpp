﻿#include "XiboApp.hpp"

#include "MainLoop.hpp"
#include "config.hpp"

#include "utils/ScreenShoter.hpp"
#include "utils/Resources.hpp"
#include "xmlsink/XmlLoggerSink.hpp"

#include "control/MainWindowController.hpp"
#include "control/MainWindow.hpp"
#include "control/StatusScreenFormatter.hpp"
#include "control/StatusScreen.hpp"
#include "control/media/player/video/XiboVideoSink.hpp"
#include "control/media/creators/MediaParsersRepo.hpp"
#include "control/media/creators/MediaFactoriesRepo.hpp"

#include "managers/CollectionInterval.hpp"
#include "managers/XiboLayoutScheduler.hpp"
#include "managers/FileCacheManager.hpp"
#include "managers/PlayerSettingsManager.hpp"
#include "managers/XmrManager.hpp"
#include "managers/ScheduleManager.hpp"

#include "networking/HttpManager.hpp"
#include "networking/xmds/XmdsRequestSender.hpp"
#include "networking/xmds/SoapRequestSender.hpp"

#include "common/CmsSettingsManager.hpp"
#include "common/RsaManager.hpp"

#include <gst/gst.h>
#include <glibmm/main.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <boost/date_time/time_clock.hpp>
#include <gdk/gdkx.h>
#include <X11/extensions/scrnsaver.h>

std::unique_ptr<XiboApp> XiboApp::m_app;

XiboApp& XiboApp::create(const std::string& name)
{
    auto logger = XiboLogger::create(SpdLogger, createLoggerSinks());
    logger->setLevel(LoggingLevel::Debug);
    logger->setPattern("[%H:%M:%S.%e] [%t] [%l]: %v");

    gst_init(nullptr, nullptr);
    registerVideoSink();
    Resources::setDirectory(ProjectResources::defaultResourcesDir());

    m_app = std::unique_ptr<XiboApp>(new XiboApp(name));
    return *m_app;
}

std::vector<spdlog::sink_ptr> XiboApp::createLoggerSinks()
{
    std::vector<spdlog::sink_ptr> sinks;

    sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_mt>());
    sinks.push_back(std::make_shared<LoggerXmlSinkMt>(XmlLogsRepo::get()));

    return sinks;
}

void XiboApp::registerVideoSink()
{
    if(!gst_plugin_register_static(GST_VERSION_MAJOR, GST_VERSION_MINOR, "Xibo Video Sink", "Video Sink Plugin for gstreamer",
                                   pluginInit, "0.5", "GPL", "source", "package", "http://github.com/Stivius"))
    {
        throw std::runtime_error("XiboVideoSink was not registered");
    }
}

XiboApp::XiboApp(const std::string& name) :
    m_mainLoop(std::make_unique<MainLoop>(name)),
    m_scheduler(std::make_unique<XiboLayoutScheduler>()),
    m_fileManager(std::make_unique<FileCacheManager>()),
    m_playerSettingsManager(std::make_unique<PlayerSettingsManager>(ProjectResources::playerSettings())),
    m_xmrManager(std::make_unique<XmrManager>()),
    m_scheduleManager(std::make_unique<ScheduleManager>())
{
    if(!FileSystem::exists(ProjectResources::cmsSettings()))
        throw std::runtime_error("Update CMS settings using player options app");

    CmsSettingsManager cmsSettingsManager{ProjectResources::cmsSettings()};
    m_cmsSettings = cmsSettingsManager.load();
    Resources::setDirectory(FilePath{m_cmsSettings.resourcesPath});

    m_playerSettingsManager->load();
    m_scheduleManager->load(ProjectResources::configDirectory() / DefaultScheduleFile);
    m_scheduler->reloadSchedule(m_scheduleManager->schedule());
    m_fileManager->loadCache(Resources::resDirectory() / DefaultCacheFile);
    HttpManager::instance().setProxyServer(m_cmsSettings.domain, m_cmsSettings.username, m_cmsSettings.password);
    RsaManager::instance().load();
    setupXmrManager();

    MediaParsersRepo::init();
    MediaFactoriesRepo::init();

    m_mainLoop->setShutdownAction([this](){
        m_windowController.reset();
        m_xmrManager->stop();
        HttpManager::instance().shutdown();
        if(m_collectionInterval)
        {
            m_collectionInterval->stop();
        }
    });
}

void XiboApp::setupXmrManager()
{
    m_xmrManager->collectionInterval().connect([this](){
        Log::info("Start unscheduled collection");

        m_collectionInterval->collect([this](const PlayerError& error){
            onCollectionFinished(error);
        });
    });

    m_xmrManager->screenshot().connect([this](){
        Log::info("Taking unscheduled screenshot");

        Managers::screenShoter().takeBase64([this](const std::string& screenshot){
            m_xmdsManager->submitScreenShot(screenshot).then([](auto future){
                auto [error, result] = future.get();
                if(error)
                {
                    Log::error("SubmitScreenShot: {}", error);
                }
            });
        });
    });
}

XiboApp::~XiboApp()
{
    if(gst_is_initialized())
    {
        gst_deinit();
    }
}

XiboApp& XiboApp::app()
{
    return *m_app;
}

FileCacheManager& XiboApp::fileManager()
{
    return *m_fileManager;
}

ScreenShoter& XiboApp::screenShoter()
{
    return *m_screenShoter;
}

int XiboApp::run()
{
    m_mainWindow = std::make_shared<MainWindow>();
    m_windowController = std::make_unique<MainWindowController>(m_mainWindow, *m_scheduler);

    GdkDisplay* display = m_mainWindow->get().get_display()->gobj();
    auto x11Display = gdk_x11_display_get_xdisplay(display);
    XScreenSaverSuspend(x11Display, true);

    auto statusScreen = std::make_shared<StatusScreen>(640, 480);
    m_windowController->statusScreenRequested().connect([this, statusScreen](){
        StatusInfo info{collectGeneralInfo(), m_collectionInterval->status(), m_scheduler->status(), m_xmrManager->status()};

        statusScreen->setText(StatusScreenFormatter::formatInfo(info));
        statusScreen->show();
    });

    m_screenShoter = std::make_unique<ScreenShoter>(*m_mainWindow);
    m_xmdsManager = std::make_unique<XmdsRequestSender>(m_cmsSettings.cmsAddress, m_cmsSettings.key, m_cmsSettings.displayId);
    m_collectionInterval = createCollectionInterval(*m_xmdsManager);

    updateSettings(m_playerSettingsManager->settings());

    m_collectionInterval->startRegularCollection();
    m_windowController->updateLayout(m_scheduler->nextLayoutId());

    return m_mainLoop->run(*m_mainWindow);
}

GeneralInfo XiboApp::collectGeneralInfo()
{
    GeneralInfo info;
    auto&& settings = m_playerSettingsManager->settings();

    info.currentDT = boost::posix_time::second_clock::local_time();
    info.cmsAddress = m_cmsSettings.cmsAddress;
    info.resourcesPath = m_cmsSettings.resourcesPath;
    info.codeVersion = ProjectResources::codeVersion();
    info.projectVersion = ProjectResources::version();
    info.screenShotInterval = settings.collectInterval;
    info.displayName = settings.displayName;
    info.windowWidth = m_mainWindow->width();
    info.windowHeight = m_mainWindow->height();

    return info;
}


std::unique_ptr<CollectionInterval> XiboApp::createCollectionInterval(XmdsRequestSender& xmdsManager)
{
    auto interval = std::make_unique<CollectionInterval>(xmdsManager);

    interval->collectionFinished().connect(sigc::mem_fun(this, &XiboApp::onCollectionFinished));
    interval->settingsUpdated().connect(sigc::mem_fun(this, &XiboApp::updateSettings));
    interval->scheduleUpdated().connect([this](const Schedule::Result& result){
        m_scheduleManager->update(result.scheduleXml);
        m_scheduler->reloadSchedule(m_scheduleManager->schedule());
    });

    return interval;
}

void XiboApp::onCollectionFinished(const PlayerError& error)
{
    if(error)
    {
        Log::error("[Collection interval] {}", error);
    }
    else
    {
        if(m_scheduler->currentLayoutId() == EmptyLayoutId)
        {
            m_windowController->updateLayout(m_scheduler->nextLayoutId());
        }
    }
}

void XiboApp::updateSettings(const PlayerSettings& settings)
{
    m_playerSettingsManager->update(settings);
    applyPlayerSettings(settings);
}

void XiboApp::applyPlayerSettings(const PlayerSettings& settings)
{
    Log::logger()->setLevel(settings.logLevel);
    m_collectionInterval->updateInterval(settings.collectInterval);
    m_xmrManager->connect(settings.xmrNetworkAddress);
    m_windowController->updateWindowDimensions(settings.dimensions);

    Log::debug("Player settings updated");
}
