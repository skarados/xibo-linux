#pragma once

#include <memory>
#include <spdlog/common.h>

#include "common/CmsSettings.hpp"

class MainLoop;
class XmdsRequestSender;
class HttpManager;
class LayoutScheduler;
class FileCacheManager;
class CollectionInterval;
class PlayerSettingsManager;
class PlayerError;
class ScreenShoter;
class XmrSubscriber;
struct PlayerSettings;

class XiboApp
{
public:
    XiboApp(const XiboApp& other) = delete;
    XiboApp& operator=(const XiboApp& other) = delete;
    ~XiboApp();

    static XiboApp& create(const std::string& name);
    static XiboApp& app();
    FileCacheManager& fileManager();
    ScreenShoter& screenShoter();

    int run();

private:
    static std::vector<spdlog::sink_ptr> createLoggerSinks();

    XiboApp(const std::string& name);

    void onCollectionFinished(const PlayerError& error);
    void updateSettings(const PlayerSettings& settings);
    void applyPlayerSettings(const PlayerSettings& settings);
    std::unique_ptr<CollectionInterval> createCollectionInterval(XmdsRequestSender& xmdsManager);

private:
    std::unique_ptr<MainLoop> m_mainLoop;
    std::unique_ptr<LayoutScheduler> m_scheduler;
    std::unique_ptr<FileCacheManager> m_fileManager;
    std::unique_ptr<CollectionInterval> m_collectionInterval;
    std::unique_ptr<XmdsRequestSender> m_xmdsManager;
    std::unique_ptr<PlayerSettingsManager> m_playerSettingsManager;
    std::unique_ptr<ScreenShoter> m_screenShoter;
    std::unique_ptr<XmrSubscriber> m_xmrSubscriber;
    CmsSettings m_cmsSettings;

    static std::unique_ptr<XiboApp> m_app;
};
