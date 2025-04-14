#pragma once

namespace Events
{

//=========================================================================
// Thread Utilities
//=========================================================================
inline static std::thread::id g_mainThreadID = std::this_thread::get_id();
inline bool                   FIsMainThread()
{
   return std::this_thread::get_id() == g_mainThreadID;
}

//=========================================================================
// Event Macros
//=========================================================================
#define EVENT_CLASS_TYPE( type )                                                \
    static EventType GetStaticType() { return type; }                           \
    virtual EventType GetEventType() const override { return GetStaticType(); } \
    virtual const char* GetName() const override { return #type; }

#define EVENT_CLASS_CATEGORY( category ) \
    virtual int GetCategoryFlags() const override { return static_cast<int>(category); }

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

   EventSubscriber( const EventSubscriber& )            = delete;
   EventSubscriber& operator=( const EventSubscriber& ) = delete;

   template< typename TEvent, typename = std::enable_if_t< std::is_base_of_v< IEvent, TEvent > > >
   void Subscribe( EventCallbackFn< TEvent >&& callback )
   {
      EventManager::RegisterSubscriber( TEvent::GetStaticType(), this );
      m_events[ TEvent::GetStaticType() ] = { true,
                                              [ callback = std::move( callback ) ]( const IEvent& event ) noexcept
      { callback( static_cast< const TEvent& >( event ) ); } };
   }

   template< typename TEvent, typename = std::enable_if_t< std::is_base_of_v< IEvent, TEvent > > >
   void Unsubscribe()
   {
      EventManager::UnregisterSubscriber( TEvent::GetStaticType(), this );
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
      bool                      m_fEnabled = true;
      EventCallbackFn< IEvent > m_callback;
   };
   std::unordered_map< EventType, EventData > m_events;

   //=====================================================================
   // EventManager (Static Class for Managing Event Subscriptions)
   //=====================================================================
   class EventManager final
   {
   public:
      static void RegisterSubscriber( EventType type, EventSubscriber* subscriber )
      {
         auto& subscribers = s_eventSubscribers[ type ];
         if( std::find( subscribers.begin(), subscribers.end(), subscriber ) == subscribers.end() )
            subscribers.push_back( subscriber );
      }

      static void UnregisterSubscriber( EventType type, EventSubscriber* subscriber )
      {
         auto& subscribers = s_eventSubscribers[ type ];
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
            std::lock_guard< std::mutex > lock( s_eventQueueMutex );
            s_eventQueue.push( psEvent );
            s_eventQueueCV.notify_one();
         }
      }

      inline static std::queue< std::shared_ptr< IEvent > > s_eventQueue;
      inline static std::mutex                              s_eventQueueMutex;
      inline static std::condition_variable                 s_eventQueueCV;

   private:
      inline static std::unordered_map< EventType, std::vector< EventSubscriber* > > s_eventSubscribers;
   };

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
   EventSubscriber::EventManager::DispatchToSubscribers( std::make_shared< TEvent >( std::forward< Args >( args )... ) );
}

inline void Dispatch( std::shared_ptr< IEvent > psEvent )
{
   EventSubscriber::EventManager::DispatchToSubscribers( psEvent );
}

inline void ProcessQueuedEvents()
{
   if( !FIsMainThread() )
      throw std::runtime_error( "ProcessQueuedEvents - must be called on the main thread" );

   std::queue< std::shared_ptr< IEvent > > localQueue;
   {
      std::lock_guard< std::mutex > lock( EventSubscriber::EventManager::s_eventQueueMutex );
      if( EventSubscriber::EventManager::s_eventQueue.empty() )
         return;

      localQueue = std::move( EventSubscriber::EventManager::s_eventQueue );
   }

   while( !localQueue.empty() )
   {
      auto psEvent = localQueue.front();
      localQueue.pop();
      EventSubscriber::EventManager::DispatchToSubscribers( psEvent );
   }
}

} // namespace Events
