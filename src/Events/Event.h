#pragma once

namespace Events
{

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
   NoCategory               = 0,
   EventCategoryApplication = 1,
   EventCategoryInput       = 1 << 1,
   EventCategoryKeyboard    = 1 << 2,
   EventCategoryMouse       = 1 << 3,
   EventCategoryMouseButton = 1 << 4
};

#define EVENT_CLASS_TYPE( type )       \
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

#define EVENT_CLASS_CATEGORY( category )            \
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
   virtual const char* GetName() const          = 0;
   virtual int         GetCategoryFlags() const = 0;
   virtual EventType   GetEventType() const     = 0;

   virtual std::string ToString() const { return GetName(); }
   bool                IsInCategory( EventCategory category ) const { return GetCategoryFlags() & static_cast< int >( category ); }
};

inline std::ostream& operator<<( std::ostream& os, const IEvent& e )
{
   return os << e.ToString();
}

template< typename TEvent >
using EventCallbackFn = std::function< void( const TEvent& ) >;

//=========================================================================
// EventSubscriber (Manages Subscriptions & Dispatching)
//=========================================================================
class EventSubscriber
{
public:
   EventSubscriber() { EventManager::AddSubscriber( this ); }
   ~EventSubscriber() { EventManager::RemoveSubscriber( this ); }

   template< typename TEvent, typename std::enable_if< std::is_base_of< IEvent, TEvent >::value && !std::is_same< IEvent, TEvent >::value, int >::type = 0 >
   void Subscribe( EventCallbackFn< TEvent >&& callback )
   {
      m_events[ TEvent::GetStaticType() ] = { true /*fEnabled*/, [ callback = std::move( callback ) ]( const IEvent& event ) {
         callback( static_cast< const TEvent& >( event ) );
      } };
   }

   void Unsubscribe( EventType type ) { m_events.erase( type ); }

   // Enables the specified event type if it has been subscribed to.
   // Can be used to resume processing a specific event after being disabled.
   template< typename TEvent, typename std::enable_if< std::is_base_of< IEvent, TEvent >::value && !std::is_same< IEvent, TEvent >::value, int >::type = 0 >
   void EnableEvent()
   {
      if( m_events.find( TEvent::GetStaticType() ) != m_events.end() )
         m_events[ TEvent::GetStaticType() ].fEnabled = true;
   }

   // Disables the specified event type if it has been subscribed to.
   // Useful for temporarily suspending processing of specific events (e.g., during a pause).
   template< typename TEvent, typename std::enable_if< std::is_base_of< IEvent, TEvent >::value && !std::is_same< IEvent, TEvent >::value, int >::type = 0 >
   void DisableEvent()
   {
      if( m_events.find( TEvent::GetStaticType() ) != m_events.end() )
         m_events[ TEvent::GetStaticType() ].fEnabled = false;
   }

private:
   void DispatchToCallbacks( std::unique_ptr< IEvent >& pEvent )
   {
      auto it = m_events.find( pEvent->GetEventType() );
      if( it != m_events.end() && it->second.fEnabled && it->second.callback )
         it->second.callback( *pEvent );
   }

   struct EventData
   {
      bool                      fEnabled { true };
      EventCallbackFn< IEvent > callback { nullptr };
   };
   std::unordered_map< EventType, EventData > m_events;

   //=========================================================================
   // EventManager (Static Class for Managing Event Subscriptions & Dispatching)
   //=========================================================================
   class EventManager
   {
   public:
      // Static methods for managing subscribers and dispatching events
      static void AddSubscriber( EventSubscriber* subscriber )
      {
         std::lock_guard< std::mutex > lock( m_mutex );
         m_subscribers.push_back( subscriber );
      }

      static void RemoveSubscriber( EventSubscriber* subscriber )
      {
         std::lock_guard< std::mutex > lock( m_mutex );
         m_subscribers.erase( std::remove( m_subscribers.begin(), m_subscribers.end(), subscriber ), m_subscribers.end() );
      }

      static void DispatchToSubscribers( std::unique_ptr< IEvent >& pEvent )
      {
         std::lock_guard< std::mutex > lock( m_mutex );
         for( EventSubscriber* subscriber : m_subscribers )
            subscriber->DispatchToCallbacks( pEvent );
      }

   private:
      EventManager()                                 = delete;
      ~EventManager()                                = delete;
      EventManager( const EventManager& )            = delete;
      EventManager& operator=( const EventManager& ) = delete;

      inline static std::vector< EventSubscriber* > m_subscribers;
      inline static std::mutex                      m_mutex; // Mutex to protect m_subscribers from concurrent access
   };

   template< typename TEvent, typename... Args >
   friend void Dispatch( Args&&... args );
   friend void Dispatch( std::unique_ptr< IEvent >& pEvent );
};

//=========================================================================
// Global Dispatch Functions
//=========================================================================
template< typename TEvent, typename... Args >
inline void Dispatch( Args&&... args )
{
   std::unique_ptr< IEvent > pEvent = std::make_unique< TEvent >( std::forward< Args >( args )... );
   EventSubscriber::EventManager::DispatchToSubscribers( pEvent );
}

inline void Dispatch( std::unique_ptr< IEvent >& pEvent )
{
   EventSubscriber::EventManager::DispatchToSubscribers( pEvent );
}

} // namespace Events
