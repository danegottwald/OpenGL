#pragma once

namespace Events
{

#define EVENT_CLASS_TYPE( type )                                                \
    static EventType GetStaticType() { return type; }                           \
    virtual EventType GetEventType() const override { return GetStaticType(); } \
    virtual const char* GetName() const override { return #type; }

#define EVENT_CLASS_CATEGORY( category ) \
    virtual int GetCategoryFlags() const override { return static_cast<int>(category); }

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
   MouseScrolled,
   NetworkClientConnect,
   NetworkClientDisconnect,
   NetworkClientTimeout,
};

enum EventCategory
{
   NoCategory               = 0,
   EventCategoryApplication = 1,
   EventCategoryInput       = 1 << 1,
   EventCategoryKeyboard    = 1 << 2,
   EventCategoryMouse       = 1 << 3,
   EventCategoryMouseButton = 1 << 4,
   EventCategoryNetwork     = 1 << 5,
};

//=========================================================================
// IEvent Interface
//=========================================================================
class IEvent
{
public:
   IEvent()                           = default;
   virtual ~IEvent()                  = default;
   IEvent( const IEvent& )            = delete;
   IEvent& operator=( const IEvent& ) = delete;

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
class EventSubscriber final
{
public:
   EventSubscriber()  = default;
   ~EventSubscriber() = default;

   // Shouldn't be copying or moving subscribers, relationship with callbacks will be lost
   EventSubscriber( const EventSubscriber& )            = delete;
   EventSubscriber& operator=( const EventSubscriber& ) = delete;

   // Subscribe to an event type with callback (will overwrite any existing callback if same type)
   template< typename TEvent, typename = std::enable_if_t< std::is_base_of< IEvent, TEvent >::value && !std::is_same< IEvent, TEvent >::value > >
   void Subscribe( EventCallbackFn< TEvent >&& callback )
   {
      EventManager::RegisterSubscriber( TEvent::GetStaticType(), this );
      m_events[ TEvent::GetStaticType() ] = { true,
                                              [ callback = std::move( callback ) ]( const IEvent& event ) noexcept
      { callback( static_cast< const TEvent& >( event ) ); } };
   }

   template< typename TEvent, typename = std::enable_if_t< std::is_base_of< IEvent, TEvent >::value && !std::is_same< IEvent, TEvent >::value > >
   void Unsubscribe()
   {
      EventManager::UnregisterSubscriber( TEvent::GetStaticType(), this );
      m_events.erase( TEvent::GetStaticType() );
   }

   // Enable or disable a subscribed event type
   template< typename TEvent, typename = std::enable_if_t< std::is_base_of< IEvent, TEvent >::value && !std::is_same< IEvent, TEvent >::value > >
   void SetEventState( bool enabled )
   {
      if( auto it = m_events.find( TEvent::GetStaticType() ); it != m_events.end() )
         it->second.fEnabled = enabled;
   }

private:
   void DispatchToCallbacks( const IEvent& event )
   {
      if( auto it = m_events.find( event.GetEventType() ); it != m_events.end() && it->second.fEnabled )
         it->second.callback( event );
   }

   struct EventData
   {
      bool                      fEnabled = true;
      EventCallbackFn< IEvent > callback;
   };
   std::unordered_map< EventType, EventData > m_events;

   //=========================================================================
   // EventManager (Static Class for Managing Event Subscriptions & Dispatching)
   //=========================================================================
   class EventManager final
   {
   public:
      static void RegisterSubscriber( EventType type, EventSubscriber* subscriber )
      {
         std::lock_guard< std::mutex > lock( m_mutex );

         // Only add the subscriber if it's not already in the list
         auto& subscribers = m_eventSubscribers[ type ];
         if( std::find( subscribers.begin(), subscribers.end(), subscriber ) == subscribers.end() )
            subscribers.push_back( subscriber );
      }

      static void UnregisterSubscriber( EventType type, EventSubscriber* subscriber )
      {
         std::lock_guard< std::mutex > lock( m_mutex );
         auto&                         subscribers = m_eventSubscribers[ type ];
         subscribers.erase( std::remove( subscribers.begin(), subscribers.end(), subscriber ), subscribers.end() );
      }

      static void DispatchToSubscribers( IEvent&& event )
      {
         std::lock_guard< std::mutex > lock( m_mutex );
         if( auto it = m_eventSubscribers.find( event.GetEventType() ); it != m_eventSubscribers.end() )
         {
            for( EventSubscriber* subscriber : it->second )
               subscriber->DispatchToCallbacks( event );
         }
      }

   private:
      EventManager()                                 = delete;
      ~EventManager()                                = delete;
      EventManager( const EventManager& )            = delete;
      EventManager& operator=( const EventManager& ) = delete;

      inline static std::mutex                                                       m_mutex;
      inline static std::unordered_map< EventType, std::vector< EventSubscriber* > > m_eventSubscribers;
   };

   template< typename TEvent, typename... Args >
   friend void Dispatch( Args&&... args );
   friend void Dispatch( IEvent&& pEvent );
};

//=========================================================================
// Global Dispatch Functions
//=========================================================================
template< typename TEvent, typename... Args >
inline void Dispatch( Args&&... args )
{
   EventSubscriber::EventManager::DispatchToSubscribers( std::move( TEvent( std::forward< Args >( args )... ) ) );
}

inline void Dispatch( IEvent&& event )
{
   EventSubscriber::EventManager::DispatchToSubscribers( std::move( event ) );
}

} // namespace Events
