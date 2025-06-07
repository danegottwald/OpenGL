#pragma once

#include "Network.h"

// ===================================================
//      NetworkHost
// ===================================================
class NetworkHost final : public INetwork
{
public:
   void               Listen( uint16_t port );
   const std::string& GetListenAddress() const noexcept { return m_ipAddress; }

   void StartThreads( World& world, Player& player );
   void ListenForClients();
   void ListenForData( World& world, Player& player );
   void HandlePacket( const Packet& packet, Client& client, World& world, Player& player );

   // SendPacket Overrides
   void SendPacket( const Packet& packet ) override;
   void SendPackets( const std::vector< Packet >& packets ) override;

private:
   NetworkHost();
   ~NetworkHost() override;

   // Checks for and attempts to accept incoming client connection
   std::optional< Client > TryAcceptClient( _Inout_ fd_set& readfds );

   void                  Poll( World& world, Player& player ) override;
   constexpr NetworkType GetNetworkType() const noexcept override { return NetworkType::Host; }

   std::string m_ipAddress;

   std::unordered_map< uint64_t, SOCKET > m_clientSockets; // map clientID to socket

   std::atomic< bool > m_fRunning { false }; // To control thread execution
   std::mutex          m_clientsMutex;
   std::thread         m_clientListenerThread;
   std::thread         m_dataListenerThread;

   friend class INetwork;
};
