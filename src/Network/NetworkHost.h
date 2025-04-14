#pragma once

#include "Network.h"

// ===================================================
//      NetworkHost
// ===================================================
class NetworkHost final : public INetwork
{
public:
   void               Listen();
   const std::string& GetListenAddress() const noexcept { return m_ipAddress; }
   void               SetListenPort( uint16_t port ) noexcept { m_port = port; }


   void StartThreads( World& world, Player& player );
   void StopThreads();
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
   std::optional< Client > TryAcceptClient( _Inout_ fd_set& readfds, _Inout_ fd_set& writefds );

   void                  Poll( World& world, Player& player ) override;
   constexpr NetworkType GetNetworkType() const noexcept override { return NetworkType::Host; }

   std::string m_ipAddress;

   std::unordered_map< uint64_t, SOCKET > m_clientIDToSocket; // map clientID to socket

   std::thread         m_clientListenerThread;
   std::thread         m_dataListenerThread;
   std::mutex          m_clientsMutex;
   std::atomic< bool > m_fRunning { false }; // To control thread execution

   friend class INetwork;
};
