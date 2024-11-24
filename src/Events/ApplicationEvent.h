#pragma once

#include "Event.h"

class WindowResizeEvent : public IEvent
{
private:
    unsigned int m_Width, m_Height;

public:
    WindowResizeEvent( unsigned int width, unsigned int height )
        : m_Width( width ), m_Height( height )
    {
    }

    unsigned int GetWidth() const
    {
        return m_Width;
    }
    unsigned int GetHeight() const
    {
        return m_Height;
    }

    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "WindowResizeEvent: " << m_Width << ", " << m_Height;
        return ss.str();
    }

    EVENT_CLASS_TYPE( EventType::WindowResize )
        EVENT_CLASS_CATEGORY( EventCategory::EventCategoryApplication )
};

class WindowCloseEvent : public IEvent
{
public:
    WindowCloseEvent() = default;

    EVENT_CLASS_TYPE( EventType::WindowClose )
        EVENT_CLASS_CATEGORY( EventCategory::EventCategoryApplication )
};

class AppTickEvent : public IEvent
{
public:
    AppTickEvent() = default;

    EVENT_CLASS_TYPE( EventType::AppTick )
        EVENT_CLASS_CATEGORY( EventCategory::EventCategoryApplication )
};

class AppUpdateEvent : public IEvent
{
public:
    AppUpdateEvent() = default;

    EVENT_CLASS_TYPE( EventType::AppUpdate )
        EVENT_CLASS_CATEGORY( EventCategory::EventCategoryApplication )
};

class AppRenderEvent : public IEvent
{
public:
    AppRenderEvent() = default;

    EVENT_CLASS_TYPE( EventType::AppRender )
        EVENT_CLASS_CATEGORY( EventCategory::EventCategoryApplication )
};
