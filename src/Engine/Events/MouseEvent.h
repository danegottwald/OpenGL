#pragma once

// Local dependencies
#include "Event.h"

// Project dependencies
#include <Engine/Input/Codes.h>

namespace Events
{

//=========================================================================
// MouseMovedEvent
//=========================================================================
class MouseMovedEvent final : public IEvent
{
public:
   MouseMovedEvent( const float x, const float y ) :
      m_MouseX( x ),
      m_MouseY( y )
   {}

   float GetX() const { return m_MouseX; }
   float GetY() const { return m_MouseY; }

   std::string ToString() const noexcept override { return std::format( "MouseMovedEvent: {0}x, {1}y", m_MouseX, m_MouseY ); }

   EVENT_CLASS_TYPE( EventType::MouseMoved )
   EVENT_CLASS_CATEGORY( EventCategory::Input | EventCategory::Mouse )

private:
   float m_MouseX, m_MouseY;
};

//=========================================================================
// MouseScrolledEvent
//=========================================================================
class MouseScrolledEvent final : public IEvent
{
public:
   MouseScrolledEvent( const float xOffset, const float yOffset ) :
      m_XOffset( xOffset ),
      m_YOffset( yOffset )
   {}

   float GetXOffset() const { return m_XOffset; }
   float GetYOffset() const { return m_YOffset; }

   std::string ToString() const noexcept override { return std::format( "MouseScrolledEvent: {0}x, {1}y", m_XOffset, m_YOffset ); }

   EVENT_CLASS_TYPE( EventType::MouseScrolled )
   EVENT_CLASS_CATEGORY( EventCategory::Input | EventCategory::Mouse )

private:
   float m_XOffset, m_YOffset;
};

//=========================================================================
// MouseButtonEvent
//=========================================================================
class MouseButtonEvent : public IEvent
{
public:
   Input::MouseCode GetMouseButton() const { return m_Button; }

   EVENT_CLASS_CATEGORY( EventCategory::Input | EventCategory::Mouse | EventCategory::MouseButton )

protected:
   MouseButtonEvent( const Input::MouseCode button ) :
      m_Button( button )
   {}

   Input::MouseCode m_Button;
};

//=========================================================================
// MouseButtonPressedEvent
//=========================================================================
class MouseButtonPressedEvent final : public MouseButtonEvent
{
public:
   MouseButtonPressedEvent( const Input::MouseCode button ) :
      MouseButtonEvent( button )
   {}

   std::string ToString() const noexcept override { return std::format( "MouseButtonPressedEvent: {0}", std::to_underlying( m_Button ) ); }

   EVENT_CLASS_TYPE( EventType::MouseButtonPressed )
};

//=========================================================================
// MouseButtonReleasedEvent
//=========================================================================
class MouseButtonReleasedEvent final : public MouseButtonEvent
{
public:
   MouseButtonReleasedEvent( const Input::MouseCode button ) :
      MouseButtonEvent( button )
   {}

   std::string ToString() const noexcept override { return std::format( "MouseButtonReleasedEvent: {0}", std::to_underlying( m_Button ) ); }

   EVENT_CLASS_TYPE( EventType::MouseButtonReleased )
};

} // namespace Events
