#pragma once

#include <ws2tcpip.h>
#include <winsock2.h>
#pragma comment( lib, "ws2_32.lib" )

// Local dependencies
#include "Packet.h"

#define DEFAULT_ADDRESS "127.0.0.1"
#define DEFAULT_PORT 8080

class Packet;

enum class NetworkType
{
   None   = 0,
   Host   = 1,
   Client = 2
};

struct Client
{
   SOCKET      m_socket { INVALID_SOCKET };
   uint64_t    m_clientID { 0 };
   sockaddr_in m_socketAddressIn {};
   std::string m_ipAddress {};

   std::vector< uint8_t > m_receiveBuffer;

   static inline uint64_t GenerateClientID()
   {
      static std::mt19937_64 rng( std::chrono::steady_clock::now().time_since_epoch().count() );
      return rng();
   }
};

static inline std::deque< std::string > s_NetworkLogs;

class INetwork
{
public:
   template< typename TNetwork >
   static TNetwork* Get() noexcept
   {
      return dynamic_cast< TNetwork* >( s_pInstance );
   }
   static INetwork* Get() noexcept { return s_pInstance; }

   template< typename TNetwork >
   static TNetwork& Create();
   static void      Shutdown() noexcept;

   static const char* GetHostAddress() noexcept;
   static bool        FResolvePeerAddress( Client& client );

   uint64_t GetID() const noexcept { return m_ID; }
   void     SetID( uint64_t id ) noexcept { m_ID = id; }

   SOCKET GetHostSocket() const noexcept { return m_hostSocket; }

   virtual void                  Poll( const glm::vec3& playerPosition ) = 0;
   virtual constexpr NetworkType GetNetworkType() const noexcept         = 0;

   virtual void HandleIncomingPacket( const Packet& packet ) = 0;

   void         Send( SOCKET targetSocket, const std::vector< uint8_t >& serializedData );
   virtual void SendPacket( const Packet& packet )         = 0;
   virtual void SendPackets( std::span< Packet > packets ) = 0;

protected:
   INetwork();
   virtual ~INetwork();

   static inline INetwork* s_pInstance = nullptr;
   static inline char*     s_address   = nullptr;

   SOCKET      m_hostSocket { INVALID_SOCKET };
   sockaddr_in m_socketAddressIn {};
   uint16_t    m_port { DEFAULT_PORT };

   uint64_t              m_ID { 0 };
   std::vector< Client > m_clients;
};

template< typename TNetwork >
TNetwork& INetwork::Create()
{
   assert( !s_pInstance && "INetwork instance already exists!" );
   if( s_pInstance )
      throw std::runtime_error( "INetwork instance already exists!" );

   s_pInstance = new TNetwork();
   return static_cast< TNetwork& >( *s_pInstance );
}
