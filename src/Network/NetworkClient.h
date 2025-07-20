#pragma once

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
   void SendPacket( const Packet& packet ) override;
   void SendPackets( const std::vector< Packet >& packets ) override;

   void HandleIncomingPacket( const Packet& packet, World& world, Player& player ) override;

private:
   void                  Poll( World& world, Player& player ) override;
   constexpr NetworkType GetNetworkType() const noexcept override { return NetworkType::Client; }

   // Threaded receive logic
   void                    StartRecvThread();
   void                    StopRecvThread();
   void                    RecvThread();
   std::optional< Packet > PollPacket();

   uint64_t    m_serverID { 0 }; // ID of the server
   std::string m_serverIpAddress { DEFAULT_ADDRESS };

   // Threading and queue
   std::atomic< bool >  m_fConnected { false };
   std::atomic< bool >  m_running { false };
   std::thread          m_recvThread;
   std::queue< Packet > m_incomingPackets;
   std::mutex           m_queueMutex;

   friend class INetwork;
};
