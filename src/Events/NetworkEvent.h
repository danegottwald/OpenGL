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
   EVENT_CLASS_CATEGORY( EventCategory::Network )

   uint64_t GetClientID() const noexcept { return m_clientID; }

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

//=========================================================================
// NetworkChatReceivedEvent
//=========================================================================
class NetworkChatReceivedEvent final : public NetworkEvent
{
public:
   NetworkChatReceivedEvent( uint64_t clientID, const std::string& message ) :
      NetworkEvent( clientID ),
      m_message( message )
   {}

   std::string ToString() const override { return std::format( "NetworkChatReceivedEvent: {}-{}", m_clientID, m_message ); }

   const std::string& GetChatMessage() const noexcept { return m_message; }

   EVENT_CLASS_TYPE( EventType::NetworkChatReceived )

private:
   std::string m_message {};
};

//=========================================================================
// NetworkHostDisconnectEvent
//=========================================================================
class NetworkHostDisconnectEvent final : public NetworkEvent
{
public:
   NetworkHostDisconnectEvent( uint64_t hostClientID ) :
      NetworkEvent( hostClientID )
   {}

   std::string ToString() const override { return "NetworkHostDisconnectEvent"; }

   EVENT_CLASS_TYPE( EventType::NetworkHostDisconnected )
};

} // namespace Events
