#pragma once

enum EventType
{
    NoType = 0,
    WindowClose,
    WindowResize,
    WindowFocus,
    WindowLostFocus,
    WindowMoved,
    AppTick,
    AppUpdate,
    AppRender,
    KeyPressed,
    KeyReleased,
    KeyTyped,
    MouseButtonPressed,
    MouseButtonReleased,
    MouseMoved,
    MouseScrolled
};

enum EventCategory
{
    NoCategory = 0,
    EventCategoryApplication = 1,
    EventCategoryInput = 1 << 1,
    EventCategoryKeyboard = 1 << 2,
    EventCategoryMouse = 1 << 3,
    EventCategoryMouseButton = 1 << 4
};

#define EVENT_CLASS_TYPE(type)       \
    static EventType GetStaticType() \
    {                                \
        return type;                 \
    }                                \
    virtual EventType GetEventType() const override \
    {                                               \
        return GetStaticType();                     \
    }                                               \
    virtual const char* GetName() const override    \
    {                                               \
        return #type;                               \
    }

#define EVENT_CLASS_CATEGORY(category)            \
    virtual int GetCategoryFlags() const override \
    {                                             \
        return static_cast<int>(category);        \
    }

//=========================================================================
// IEvent Interface
//=========================================================================
class IEvent
{
public:
    virtual ~IEvent() = default;

    // Interface Methods
    virtual const char* GetName() const = 0;
    virtual int GetCategoryFlags() const = 0;
    virtual EventType GetEventType() const = 0;

    virtual std::string ToString() const
    {
        return GetName();
    }
    bool IsInCategory( EventCategory category ) const
    {
        return GetCategoryFlags() & static_cast< int >( category );
    }

    // Event Handled
    void SetHandled( bool fHandled )
    {
        m_handled = fHandled;
    }
    bool FHandled() const
    {
        return m_handled;
    }

private:
    bool m_handled = false;

};


//=========================================================================
// EventDispatcher
//=========================================================================
class EventDispatcher
{
public:
    explicit EventDispatcher( IEvent& event ) : m_event( event )
    {
    }

    inline IEvent& GetEvent() const
    {
        return m_event;
    }

    // F will be deduced by the compiler
    template <typename T, typename F>
    bool Dispatch( const F& func )
    {
        if( m_event.GetEventType() != T::GetStaticType() )
            return false;

        m_event.SetHandled( func( static_cast< T& >( m_event ) ) );
        return true;
    }

private:
    IEvent& m_event;
};

inline std::ostream& operator<<( std::ostream& os, const IEvent& e )
{
    return os << e.ToString();
}
