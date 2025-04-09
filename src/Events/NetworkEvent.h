#pragma once

#include "Event.h"

namespace Events
{

//=========================================================================
// NetworkEvent Interface
//=========================================================================
class NetworkEvent : public IEvent
{
public:
   EVENT_CLASS_CATEGORY( EventCategory::EventCategoryNetwork )

protected:
   NetworkEvent( uint64_t clientID ) :
      m_clientID( clientID ) {};

   uint64_t m_clientID {};
};

//=========================================================================
// NetworkClientConnectEvent
//=========================================================================
class NetworkClientConnectEvent final : public NetworkEvent
{
public:
   NetworkClientConnectEvent( uint64_t clientID ) :
      NetworkEvent( clientID )
   {}

   std::string ToString() const override { return std::format( "NetworkClientConnectEvent: {}", m_clientID ); }

   uint64_t GetClientID() const noexcept { return m_clientID; }

   EVENT_CLASS_TYPE( EventType::NetworkClientConnect )
};

//=========================================================================
// NetworkClientDisconnectEvent
//=========================================================================
class NetworkClientDisconnectEvent final : public NetworkEvent
{
public:
   NetworkClientDisconnectEvent( uint64_t clientID ) :
      NetworkEvent( clientID )
   {}

   std::string ToString() const override { return std::format( "NetworkClientDisconnectEvent: {}", m_clientID ); }

   uint64_t GetClientID() const noexcept { return m_clientID; }

   EVENT_CLASS_TYPE( EventType::NetworkClientDisconnect )
};

//=========================================================================
// NetworkClientTimeoutEvent
//=========================================================================
class NetworkClientTimeoutEvent final : public NetworkEvent
{
public:
   NetworkClientTimeoutEvent( uint64_t clientID ) :
      NetworkEvent( clientID )
   {}

   std::string ToString() const override { return std::format( "NetworkClientTimeoutEvent: {}", m_clientID ); }

   EVENT_CLASS_TYPE( EventType::NetworkClientTimeout )
};

} // namespace Events
