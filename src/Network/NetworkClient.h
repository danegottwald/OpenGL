#pragma once

#include "Network.h"

// ===================================================
//      NetworkClient
// ===================================================
class NetworkClient final : public INetwork
{
public:
   //std::string GetClientAddress() const noexcept;
   //void        SetServerPort( uint16_t port ) noexcept;
   void Connect( const char* ipAddress, uint16_t port );

   // SendPacket Overrides
   void SendPacket( const Packet& packet ) override;
   void SendPackets( const std::vector< Packet >& packets ) override;

private:
   NetworkClient();
   ~NetworkClient() override;

   void                  Poll( World& world, Player& player ) override;
   constexpr NetworkType GetNetworkType() const noexcept override { return NetworkType::Client; }

   uint64_t    m_serverID { 0 }; // ID of the server
   std::string m_serverIpAddress { DEFAULT_ADDRESS };
   bool        m_fConnected { false };

   friend class INetwork;
};
