#pragma once

// Local dependencies
#include "Network.h"

// ===================================================
//      NetworkClient
// ===================================================
class NetworkClient final : public INetwork
{
public:
   NetworkClient();
   ~NetworkClient() override;

   void Connect( const char* ipAddress, uint16_t port );

   void SendPacket( const Packet& packet ) override;         // send to server
   void SendPackets( std::span< Packet > packets ) override; // send to server

   void HandleIncomingPacket( const Packet& packet ) override;

private:
   void                  Poll( const glm::vec3& playerPosition ) override;
   constexpr NetworkType GetNetworkType() const noexcept override { return NetworkType::Client; }

   enum class State : uint8_t
   {
      Disconnected,
      Connecting,
      AwaitingHandshakeAccept,
      Connected
   };

   void FlushSend_();
   void QueueSend_( const Packet& packet );
   void ProcessStream_();

   uint64_t    m_serverID { 0 };
   std::string m_serverIpAddress { DEFAULT_ADDRESS };

   State                m_state { State::Disconnected };
   std::vector< uint8_t > m_recvBuffer;
   std::vector< uint8_t > m_sendBuffer;

   // handshake
   uint32_t m_handshakeTick { 0 };

   friend class INetwork;
};
