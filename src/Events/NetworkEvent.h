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
   NetworkEvent() = default;
};

//=========================================================================
// NetworkClientConnectEvent
//=========================================================================
class NetworkClientConnectEvent final : public NetworkEvent
{
public:
   NetworkClientConnectEvent( uint64_t clientID ) :
      m_clientID( clientID )
   {}

   std::string ToString() const override { return "NetworkClientConnectEvent"; /*std::format( "KeyPressedEvent: {0}", static_cast< int >( m_keyCode ) );*/ }

   uint64_t GetClientID() const noexcept { return m_clientID; }

   EVENT_CLASS_TYPE( EventType::NetworkClientConnect )

private:
   uint64_t m_clientID;
};

//=========================================================================
// NetworkClientDisconnectEvent
//=========================================================================
class NetworkClientDisconnectEvent final : public NetworkEvent
{
public:
   NetworkClientDisconnectEvent( uint64_t clientID ) :
      m_clientID( clientID )
   {}

   std::string ToString() const override { return "NetworkClientDisconnectEvent"; /*std::format( "KeyReleasedEvent: {0}", static_cast< int >( m_keyCode ) );*/ }

   uint64_t GetClientID() const noexcept { return m_clientID; }

   EVENT_CLASS_TYPE( EventType::NetworkClientDisconnect )

private:
   uint64_t m_clientID;
};

//=========================================================================
// NetworkClientTimeoutEvent
//=========================================================================
class NetworkClientTimeoutEvent final : public NetworkEvent
{
public:
   NetworkClientTimeoutEvent() {}

   std::string ToString() const override { return "NetworkClientTimeoutEvent"; /*std::format( "KeyTypedEvent: {0}", static_cast< int >( m_keyCode ) );*/ }

   EVENT_CLASS_TYPE( EventType::NetworkClientTimeout )
};

} // namespace Events
