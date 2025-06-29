#pragma once

#include "Network.h"
#include "../Events/NetworkEvent.h"

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

   void                  Poll( World& world, Player& player ) override;
   constexpr NetworkType GetNetworkType() const noexcept override { return NetworkType::Host; }

   void SendPacket( const Packet& packet ) override;
   void SendPackets( const std::vector< Packet >& packets ) override;

   // Packet handling
   void HandleIncomingPacket( const Packet& packet, World& world, Player& player ) override;

private:
   // New architecture
   void AcceptThread();
   void ClientIOThread( uint64_t clientID );
   void BroadcastPacket( const Packet& packet, uint64_t exceptID = 0 );
   void EnqueueOutgoing( uint64_t clientID, const Packet& packet );
   bool DequeueOutgoing( uint64_t clientID, Packet& packet );
   void DisconnectClient( uint64_t clientID );
   void Shutdown();

   std::string                                          m_ipAddress;
   std::atomic< bool >                                  m_fRunning { false };
   std::thread                                          m_acceptThread;
   std::unordered_map< uint64_t, SOCKET >               m_clientSockets;
   std::unordered_map< uint64_t, std::thread >          m_clientThreads;
   std::unordered_map< uint64_t, std::queue< Packet > > m_outgoingQueues;
   std::unordered_map< uint64_t, std::mutex >           m_queueMutexes;
   std::condition_variable                              m_queueCV;
   std::mutex                                           m_clientsMutex;

   Events::EventSubscriber m_eventSubscriber;

   // Inherited/protected from INetwork
protected:
   using INetwork::m_clients;
   using INetwork::m_hostSocket;
   using INetwork::m_ID;
   using INetwork::m_port;
   using INetwork::m_socketAddressIn;
};
