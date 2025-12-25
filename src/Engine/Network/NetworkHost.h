#pragma once

// Local dependencies
#include "Network.h"

// Project dependencies
#include <Engine/Events/NetworkEvent.h>

// ===================================================
//      NetworkHost
// ===================================================
class NetworkHost final : public INetwork
{
public:
   NetworkHost();
   ~NetworkHost() override;

   void               Listen( uint16_t port );
   const std::string& GetListenAddress() const noexcept { return m_ipAddress; }

   void                  Poll( const glm::vec3& playerPosition ) override;
   constexpr NetworkType GetNetworkType() const noexcept override { return NetworkType::Host; }

   void SendPacket( const Packet& packet ) override;            // broadcast
   void SendPackets( std::span< Packet > packets ) override;    // broadcast

   void HandleIncomingPacket( const Packet& packet ) override;  // host-side processing

private:
   enum class PeerState : uint8_t
   {
      AwaitingHandshakeInit,
      Connected
   };

   struct Peer
   {
      uint64_t            clientID { 0 };
      SOCKET              socket { INVALID_SOCKET };
      PeerState           state { PeerState::AwaitingHandshakeInit };
      std::vector<uint8_t> recvBuffer; // TCP stream buffer
      std::vector<uint8_t> sendBuffer; // queued outgoing bytes (handles partial sends)
   };

   void AcceptNewClients_();
   void PollClients_();
   void FlushSends_( Peer& peer );
   void QueueSend_( Peer& peer, const Packet& packet );
   void QueueSendBytes_( Peer& peer, std::span< const uint8_t > bytes );

   // Broadcast helpers
   void Broadcast_( const Packet& packet );

   void DisconnectClient( uint64_t clientID );
   void Shutdown();

   std::string         m_ipAddress;
   std::atomic< bool > m_fRunning { false };

   std::unordered_map< uint64_t, Peer > m_peers;

   Events::EventSubscriber m_eventSubscriber;

protected:
   using INetwork::m_clients;
   using INetwork::m_hostSocket;
   using INetwork::m_ID;
   using INetwork::m_port;
   using INetwork::m_socketAddressIn;
};
