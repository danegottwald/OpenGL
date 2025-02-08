#pragma once

#include "Event.h"

namespace Events
{

enum MouseCode : uint16_t
{
   Button0 = GLFW_MOUSE_BUTTON_1,
   Button1 = GLFW_MOUSE_BUTTON_2,
   Button2 = GLFW_MOUSE_BUTTON_3,
   Button3 = GLFW_MOUSE_BUTTON_4,
   Button4 = GLFW_MOUSE_BUTTON_5,
   Button5 = GLFW_MOUSE_BUTTON_6,
   Button6 = GLFW_MOUSE_BUTTON_7,
   Button7 = GLFW_MOUSE_BUTTON_8,

   ButtonLast   = GLFW_MOUSE_BUTTON_8,
   ButtonLeft   = GLFW_MOUSE_BUTTON_1,
   ButtonRight  = GLFW_MOUSE_BUTTON_2,
   ButtonMiddle = GLFW_MOUSE_BUTTON_3
};

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

   std::string ToString() const override { return std::format( "MouseMovedEvent: {0}x, {1}y", m_MouseX, m_MouseY ); }

   EVENT_CLASS_TYPE( EventType::MouseMoved )
   EVENT_CLASS_CATEGORY( EventCategory::EventCategoryMouse | EventCategory::EventCategoryInput )

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

   std::string ToString() const override { return std::format( "MouseScrolledEvent: {0}x, {1}y", m_XOffset, m_YOffset ); }

   EVENT_CLASS_TYPE( EventType::MouseScrolled )
   EVENT_CLASS_CATEGORY( EventCategory::EventCategoryMouse | EventCategory::EventCategoryInput )

private:
   float m_XOffset, m_YOffset;
};

//=========================================================================
// MouseButtonEvent
//=========================================================================
class MouseButtonEvent : public IEvent
{
public:
   MouseCode GetMouseButton() const { return m_Button; }

   EVENT_CLASS_CATEGORY( EventCategory::EventCategoryMouse | EventCategory::EventCategoryInput | EventCategory::EventCategoryMouseButton )

protected:
   MouseButtonEvent( const MouseCode button ) :
      m_Button( button )
   {}

   MouseCode m_Button;
};

//=========================================================================
// MouseButtonPressedEvent
//=========================================================================
class MouseButtonPressedEvent final : public MouseButtonEvent
{
public:
   MouseButtonPressedEvent( const MouseCode button ) :
      MouseButtonEvent( button )
   {}

   std::string ToString() const override { return std::format( "MouseButtonPressedEvent: {0}", static_cast< int >( m_Button ) ); }

   EVENT_CLASS_TYPE( EventType::MouseButtonPressed )
};

//=========================================================================
// MouseButtonReleasedEvent
//=========================================================================
class MouseButtonReleasedEvent final : public MouseButtonEvent
{
public:
   MouseButtonReleasedEvent( const MouseCode button ) :
      MouseButtonEvent( button )
   {}

   std::string ToString() const override { return std::format( "MouseButtonReleasedEvent: {0}", static_cast< int >( m_Button ) ); }

   EVENT_CLASS_TYPE( EventType::MouseButtonReleased )
};

} // namespace Events
