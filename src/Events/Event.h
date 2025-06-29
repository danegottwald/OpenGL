#pragma once

namespace Events
{

//=========================================================================
// Thread Utilities
//=========================================================================
static const std::thread::id g_mainThreadID = std::this_thread::get_id();
inline bool                  FIsMainThread() noexcept
{
   return std::this_thread::get_id() == g_mainThreadID;
}

//=========================================================================
// Event Macros
//=========================================================================
#define EVENT_CLASS_TYPE( type )                                                         \
    static EventType GetStaticType() noexcept { return type; }                           \
    virtual EventType GetEventType() const noexcept override { return GetStaticType(); } \
    virtual const char* GetName() const noexcept override { return #type; }

#define EVENT_CLASS_CATEGORY( category ) \
    virtual int GetCategoryFlags() const noexcept override { return static_cast< int >( category ); }

//=========================================================================
// Event Type and Category Enums
//=========================================================================
enum EventType
{
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
   NetworkShutdown,
   NetworkClientConnect,
   NetworkClientDisconnect,
   NetworkClientTimeout,
   NetworkPositionUpdate,
   NetworkChatReceived,
   NetworkHostDisconnected
};

enum EventCategory
{
   None        = 0,
   Application = 1 << 0,
   Input       = 1 << 1,
   Keyboard    = 1 << 2,
   Mouse       = 1 << 3,
   MouseButton = 1 << 4,
   Network     = 1 << 5
};

//=========================================================================
// IEvent Interface
//=========================================================================
class IEvent
{
public:
   virtual ~IEvent() = default;

   virtual const char* GetName() const noexcept          = 0;
   virtual int         GetCategoryFlags() const noexcept = 0;
   virtual EventType   GetEventType() const noexcept     = 0;

   virtual std::string ToString() const noexcept { return GetName(); }
   bool                IsInCategory( EventCategory category ) const noexcept { return GetCategoryFlags() & static_cast< int >( category ); }
};

inline std::ostream& operator<<( std::ostream& os, const IEvent& e ) noexcept
{
   return os << e.ToString();
}

//=========================================================================
// EventSubscriber (Manages Subscriptions & Dispatching)
//=========================================================================
class EventSubscriber final
{
public:
   EventSubscriber() noexcept = default;
   ~EventSubscriber()
   {
      for( const auto& [ type, _ ] : m_events )
         UnregisterSubscriber( type, this );
   }

   EventSubscriber( const EventSubscriber& )            = delete;
   EventSubscriber& operator=( const EventSubscriber& ) = delete;

   template< typename TEvent,
             typename F,
             typename = std::enable_if_t< std::is_base_of_v< IEvent, TEvent > && std::is_nothrow_invocable_v< F, const TEvent& > > >
   void Subscribe( F&& callback )
   {
      RegisterSubscriber( TEvent::GetStaticType(), this );
      m_events[ TEvent::GetStaticType() ] = { true,
                                              [ cb = std::forward< F >( callback ) ]( const IEvent& event ) noexcept
      { cb( static_cast< const TEvent& >( event ) ); } };
   }

   template< typename TEvent, typename = std::enable_if_t< std::is_base_of_v< IEvent, TEvent > > >
   void Unsubscribe()
   {
      UnregisterSubscriber( TEvent::GetStaticType(), this );
      m_events.erase( TEvent::GetStaticType() );
   }

   template< typename TEvent, typename = std::enable_if_t< std::is_base_of_v< IEvent, TEvent > > >
   void SetEventState( bool enabled )
   {
      if( auto it = m_events.find( TEvent::GetStaticType() ); it != m_events.end() )
         it->second.m_fEnabled = enabled;
   }

private:
   void DispatchToCallbacks( const IEvent& event )
   {
      if( auto it = m_events.find( event.GetEventType() ); it != m_events.end() && it->second.m_fEnabled )
         it->second.m_callback( event );
   }

   struct EventData
   {
      bool                                   m_fEnabled = true;
      std::function< void( const IEvent& ) > m_callback;
   };
   std::unordered_map< EventType, EventData > m_events;

   //=====================================================================
   // Static management of event subscriptions
   //=====================================================================
   inline static std::mutex                                                       s_eventMutex;
   inline static std::queue< std::shared_ptr< IEvent > >                          s_eventQueue;
   inline static std::condition_variable                                          s_eventQueueCV;
   inline static std::unordered_map< EventType, std::vector< EventSubscriber* > > s_eventSubscribers;

   static void RegisterSubscriber( EventType type, EventSubscriber* subscriber )
   {
      std::lock_guard< std::mutex > lock( s_eventMutex );
      auto&                         subscribers = s_eventSubscribers[ type ];
      if( std::find( subscribers.begin(), subscribers.end(), subscriber ) == subscribers.end() )
         subscribers.push_back( subscriber );
   }

   static void UnregisterSubscriber( EventType type, EventSubscriber* subscriber )
   {
      std::lock_guard< std::mutex > lock( s_eventMutex );
      auto&                         subscribers = s_eventSubscribers[ type ];
      subscribers.erase( std::remove( subscribers.begin(), subscribers.end(), subscriber ), subscribers.end() );
   }

   static void DispatchToSubscribers( std::shared_ptr< IEvent > psEvent )
   {
      if( FIsMainThread() )
      {
         if( auto it = s_eventSubscribers.find( psEvent->GetEventType() ); it != s_eventSubscribers.end() )
         {
            for( EventSubscriber* subscriber : it->second )
               subscriber->DispatchToCallbacks( *psEvent );
         }
      }
      else
      {
         std::lock_guard< std::mutex > lock( s_eventMutex );
         s_eventQueue.push( psEvent );
         s_eventQueueCV.notify_one();
      }
   }

   // Friend functions for dispatching events
   template< typename TEvent, typename... Args >
   friend void Dispatch( Args&&... args );
   friend void Dispatch( std::shared_ptr< IEvent > psEvent );
   friend void ProcessQueuedEvents();
};

//=========================================================================
// Global Dispatch Functions
//=========================================================================
template< typename TEvent, typename... Args >
inline void Dispatch( Args&&... args )
{
   EventSubscriber::DispatchToSubscribers( std::make_shared< TEvent >( std::forward< Args >( args )... ) );
}

inline void Dispatch( std::shared_ptr< IEvent > psEvent )
{
   EventSubscriber::DispatchToSubscribers( psEvent );
}

inline void ProcessQueuedEvents()
{
   if( !FIsMainThread() )
      throw std::runtime_error( "ProcessQueuedEvents - must be called on the main thread" );

   std::queue< std::shared_ptr< IEvent > > localQueue;
   {
      std::lock_guard< std::mutex > lock( EventSubscriber::s_eventMutex );
      if( EventSubscriber::s_eventQueue.empty() )
         return;

      localQueue = std::move( EventSubscriber::s_eventQueue );
   }

   while( !localQueue.empty() )
   {
      std::shared_ptr< Events::IEvent > psEvent = localQueue.front();
      localQueue.pop();
      EventSubscriber::DispatchToSubscribers( psEvent );
   }
}

} // namespace Events
