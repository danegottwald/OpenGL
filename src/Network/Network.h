#pragma once

#include <ws2tcpip.h>
#include <winsock2.h>
#pragma comment( lib, "ws2_32.lib" )

#include "../Game/Core/GUIManager.h"
#include "Packet.h"

#define DEFAULT_ADDRESS "127.0.0.1"
#define DEFAULT_PORT 8080

class Packet;
class Player;
class World;

// maybe network should be a singleton?
//    only one network instance, you are either host or client (or none)

enum class NetworkType
{
   None   = 0, // this may not be needed, when might we have a network instance but not be host or client?
   Host   = 1,
   Client = 2
};

// ===================================================
//      Client
// ===================================================
struct Client
{
   SOCKET      m_socket { INVALID_SOCKET };
   uint64_t    m_clientID { 0 }; // TODO: use this to identify clients
   sockaddr_in m_socketAddressIn {};
   std::string m_ipAddress {};

   std::vector< uint8_t > m_receiveBuffer; // Buffer for incoming data

   static inline uint64_t GenerateClientID()
   {
      static std::mt19937_64 rng( std::chrono::steady_clock::now().time_since_epoch().count() );
      return rng();
   }
};

// ===================================================
//      INetwork
// ===================================================
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

   static void        RegisterGUI();
   static const char* GetHostAddress() noexcept;

   // As long as 'client' has a valid socket, this will set the address on 'client' and return true otherwise false
   static bool FResolvePeerAddress( Client& client );

   // add single function so Host gets its hostname, client gets peer hostname (ip address of server)

   uint64_t GetID() const noexcept { return m_ID; }
   void     SetID( uint64_t id ) noexcept { m_ID = id; }

   SOCKET GetHostSocket() const noexcept { return m_hostSocket; }

   virtual void Poll( World& world, Player& player ) = 0;
   //virtual void                  OnReceive( const uint8_t* data, size_t size ) = 0;
   virtual constexpr NetworkType GetNetworkType() const noexcept = 0;

   // Packet handling
   virtual void HandleIncomingPacket( const Packet& packet, World& world, Player& player ) = 0;

   //virtual void CheckForData( World& world, Player& player )    = 0; // check for data to read
   //virtual void CheckForClients( World& world, Player& player ) = 0; // check for new clients to accept

   // Send Helpers
   void         Send( SOCKET targetSocket, const std::vector< uint8_t >& serializedData );
   virtual void SendPacket( const Packet& packet )                  = 0;
   virtual void SendPackets( const std::vector< Packet >& packets ) = 0; // send aggregated packets (single send call)

protected:
   INetwork();
   virtual ~INetwork();

   static inline INetwork* s_pInstance = nullptr;
   static inline char*     s_address   = nullptr;

   SOCKET      m_hostSocket { INVALID_SOCKET };
   sockaddr_in m_socketAddressIn {};
   uint16_t    m_port { DEFAULT_PORT };

   uint64_t              m_ID { 0 }; // ID of this instance
   std::vector< Client > m_clients;  // clients that are connected

   static inline std::deque< std::string > s_logs;
};

// INetwork static instance initialization
template< typename TNetwork >
TNetwork& INetwork::Create()
{
   assert( !s_pInstance && "INetwork instance already exists!" );
   if( s_pInstance )
      throw std::runtime_error( "INetwork instance already exists!" );

   s_pInstance = new TNetwork();
   return static_cast< TNetwork& >( *s_pInstance );
}
