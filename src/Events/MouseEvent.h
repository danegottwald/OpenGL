#pragma once

#include "Event.h"

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

    ButtonLast = GLFW_MOUSE_BUTTON_8,
    ButtonLeft = GLFW_MOUSE_BUTTON_1,
    ButtonRight = GLFW_MOUSE_BUTTON_2,
    ButtonMiddle = GLFW_MOUSE_BUTTON_3
};

class MouseMovedEvent : public IEvent
{
private:
    float m_MouseX, m_MouseY;

public:
    MouseMovedEvent( const float x, const float y ) : m_MouseX( x ), m_MouseY( y )
    {
    }

    float GetX() const
    {
        return m_MouseX;
    }

    float GetY() const
    {
        return m_MouseY;
    }

    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "MouseMovedEvent: " << m_MouseX << ", " << m_MouseY;
        return ss.str();
    }

    EVENT_CLASS_TYPE( EventType::MouseMoved )
        EVENT_CLASS_CATEGORY( EventCategory::EventCategoryMouse | EventCategory::EventCategoryInput )
};

class MouseScrolledEvent : public IEvent
{
private:
    float m_XOffset, m_YOffset;

public:
    MouseScrolledEvent( const float xOffset, const float yOffset )
        : m_XOffset( xOffset ), m_YOffset( yOffset )
    {
    }

    float GetXOffset() const
    {
        return m_XOffset;
    }
    float GetYOffset() const
    {
        return m_YOffset;
    }

    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "MouseScrolledEvent: " << GetXOffset() << ", " << GetYOffset();
        return ss.str();
    }

    EVENT_CLASS_TYPE( EventType::MouseScrolled )
        EVENT_CLASS_CATEGORY( EventCategory::EventCategoryMouse | EventCategory::EventCategoryInput )
};

class MouseButtonEvent : public IEvent
{
public:
    MouseCode GetMouseButton() const
    {
        return m_Button;
    }

    EVENT_CLASS_CATEGORY( EventCategory::EventCategoryMouse | EventCategory::EventCategoryInput | EventCategory::EventCategoryMouseButton )
protected:
    MouseButtonEvent( const MouseCode button ) : m_Button( button )
    {
    }

    MouseCode m_Button;
};

class MouseButtonPressedEvent : public MouseButtonEvent
{
public:
    MouseButtonPressedEvent( const MouseCode button ) : MouseButtonEvent( button )
    {
    }

    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "MouseButtonPressedEvent: " << static_cast< int >( m_Button );
        return ss.str();
    }

    EVENT_CLASS_TYPE( EventType::MouseButtonPressed )
};

class MouseButtonReleasedEvent : public MouseButtonEvent
{
public:
    MouseButtonReleasedEvent( const MouseCode button ) : MouseButtonEvent( button )
    {
    }

    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "MouseButtonReleasedEvent: " << static_cast< int >( m_Button );
        return ss.str();
    }

    EVENT_CLASS_TYPE( EventType::MouseButtonReleased )
};
