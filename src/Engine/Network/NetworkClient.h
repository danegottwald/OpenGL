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
   void SendPacket( const Packet& packet ) override;
   void SendPackets( const std::vector< Packet >& packets ) override;

   void HandleIncomingPacket( const Packet& packet ) override;

private:
   void                  Poll( const glm::vec3& playerPosition ) override;
   constexpr NetworkType GetNetworkType() const noexcept override { return NetworkType::Client; }

   // Threaded receive logic (TCP)
   void                    StartRecvThread();
   void                    StopRecvThread();
   void                    RecvThread();
   std::optional< Packet > PollPacket();

   // UDP
   void InitUDP( uint16_t port );
   void UDPRecvThread();
   void SendUDPPacket( const Packet& packet );

   uint64_t    m_serverID { 0 }; // ID of the server
   std::string m_serverIpAddress { DEFAULT_ADDRESS };

   // Threading and queue (TCP)
   std::atomic< bool >  m_fConnected { false };
   std::atomic< bool >  m_running { false };
   std::thread          m_recvThread;
   std::queue< Packet > m_incomingPackets;
   std::mutex           m_queueMutex;

   // UDP
   SOCKET              m_udpSocket { INVALID_SOCKET };
   std::thread         m_udpRecvThread;
   std::atomic< bool > m_udpRunning { false };
   sockaddr_in         m_serverUdpAddr {};

   friend class INetwork;
};
