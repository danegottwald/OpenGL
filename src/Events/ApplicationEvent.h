#pragma once

#include "Event.h"

namespace Events
{

//=========================================================================
// WindowResizeEvent
//=========================================================================
class WindowResizeEvent : public IEvent
{
public:
   WindowResizeEvent( unsigned int width, unsigned int height ) :
      m_Width( width ),
      m_Height( height )
   {}

   unsigned int GetWidth() const { return m_Width; }
   unsigned int GetHeight() const { return m_Height; }

   std::string ToString() const override { return std::format( "WindowResizeEvent: {0}, {1}", m_Width, m_Height ); }

   EVENT_CLASS_TYPE( EventType::WindowResize )
   EVENT_CLASS_CATEGORY( EventCategory::EventCategoryApplication )

private:
   unsigned int m_Width, m_Height;
};

//=========================================================================
// WindowCloseEvent
//=========================================================================
class WindowCloseEvent final : public IEvent
{
public:
   WindowCloseEvent() = default;

   EVENT_CLASS_TYPE( EventType::WindowClose )
   EVENT_CLASS_CATEGORY( EventCategory::EventCategoryApplication )
};

//=========================================================================
// AppTickEvent
//=========================================================================
class AppTickEvent final : public IEvent
{
public:
   AppTickEvent() = default;

   EVENT_CLASS_TYPE( EventType::AppTick )
   EVENT_CLASS_CATEGORY( EventCategory::EventCategoryApplication )
};

//=========================================================================
// AppUpdateEvent
//=========================================================================
class AppUpdateEvent final : public IEvent
{
public:
   AppUpdateEvent() = default;

   EVENT_CLASS_TYPE( EventType::AppUpdate )
   EVENT_CLASS_CATEGORY( EventCategory::EventCategoryApplication )
};

//=========================================================================
// AppRenderEvent
//=========================================================================
class AppRenderEvent final : public IEvent
{
public:
   AppRenderEvent() = default;

   EVENT_CLASS_TYPE( EventType::AppRender )
   EVENT_CLASS_CATEGORY( EventCategory::EventCategoryApplication )
};

} // namespace Events
