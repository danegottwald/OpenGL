#pragma once

// Local dependencies
#include "Network.h"

// Project dependencies
#include <Events/NetworkEvent.h>

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

   void SendPacket( const Packet& packet ) override;
   void SendPackets( const std::vector< Packet >& packets ) override;

   // Packet handling
   void HandleIncomingPacket( const Packet& packet ) override;

private:
   // New architecture
   void AcceptThread();
   void ClientIOThread( uint64_t clientID );
   void BroadcastPacket( const Packet& packet, uint64_t exceptID = 0 );
   void EnqueueOutgoing( uint64_t clientID, const Packet& packet );
   bool DequeueOutgoing( uint64_t clientID, Packet& packet );
   void DisconnectClient( uint64_t clientID );
   void Shutdown();

   // UDP specific
   void InitUDP( uint16_t port );
   void UDPRecvThread();
   void SendUDPPacket( const Packet& packet, const sockaddr_in& addr );

   std::string                                          m_ipAddress;
   std::atomic< bool >                                  m_fRunning { false };
   std::thread                                          m_acceptThread;
   std::unordered_map< uint64_t, SOCKET >               m_clientSockets;  // TCP sockets
   std::unordered_map< uint64_t, sockaddr_in >          m_clientUdpAddrs; // learned via UDPRegister
   std::unordered_map< uint64_t, std::thread >          m_clientThreads;
   std::unordered_map< uint64_t, std::queue< Packet > > m_outgoingQueues; // TCP outgoing
   std::unordered_map< uint64_t, std::mutex >           m_queueMutexes;
   std::condition_variable                              m_queueCV;
   std::mutex                                           m_clientsMutex;

   // UDP
   SOCKET      m_udpSocket { INVALID_SOCKET };
   std::thread m_udpRecvThread;

   Events::EventSubscriber m_eventSubscriber;

   // Inherited/protected from INetwork
protected:
   using INetwork::m_clients;
   using INetwork::m_hostSocket;
   using INetwork::m_ID;
   using INetwork::m_port;
   using INetwork::m_socketAddressIn;
};
