#include "Event.hpp"

EventType DurationExpiredEvent::type() const
{
    return EventType::DurationExpired;
}

RegionDurationExpiredEvent::RegionDurationExpiredEvent(int id) : m_id{id}
{
}

int RegionDurationExpiredEvent::id() const
{
    return m_id;
}

CollectionFinishedEvent::CollectionFinishedEvent(const PlayerError& error) : m_error{error}
{
}

EventType CollectionFinishedEvent::type() const
{
    return EventType::CollectionFinished;
}

const PlayerError& CollectionFinishedEvent::error() const
{
    return m_error;
}

SettingsUpdatedEvent::SettingsUpdatedEvent(const PlayerSettings& settings) : m_settings{settings}
{
}

EventType SettingsUpdatedEvent::type() const
{
    return EventType::SettingsUpdated;
}

const PlayerSettings& SettingsUpdatedEvent::settings() const
{
    return m_settings;
}

ScheduleUpdatedEvent::ScheduleUpdatedEvent(const LayoutSchedule& schedule) : m_schedule{schedule}
{
}

EventType ScheduleUpdatedEvent::type() const
{
    return EventType::ScheduleUpdated;
}

const LayoutSchedule& ScheduleUpdatedEvent::schedule() const
{
    return m_schedule;
}

EventType WidgetShownEvent::type() const
{
    return EventType::WidgetShown;
}
