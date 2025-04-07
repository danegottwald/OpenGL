#pragma once

#include <winsock2.h>
#pragma comment( lib, "ws2_32.lib" )

#include "../Game/Core/GUIManager.h"

#define DEFAULT_ADDRESS "127.0.0.1"
#define DEFAULT_PORT 8080

class Player;
class World;

// maybe network should be a singleton?
//    only one network instance, you are either host or client (or none)

enum NetworkType
{
   None   = 0, // this may not be needed, when might we have a network instance but not be host or client?
   Host   = 1,
   Client = 2
};

// ===================================================
//      INetwork
// ===================================================
class INetwork
{
public:
   static INetwork* Get() noexcept { return s_pInstance; }

   template< typename TNetwork >
   static TNetwork& Create();
   static void      Shutdown() noexcept
   {
      if( s_pInstance )
      {
         delete s_pInstance;
         s_pInstance = nullptr;
      }
   }

   static void RegisterGUI();

   virtual void                  Poll( World& world, Player& player ) = 0;
   virtual constexpr NetworkType GetNetworkType() const noexcept      = 0;

protected:
   INetwork()          = default;
   virtual ~INetwork() = default;

   static inline INetwork* s_pInstance = nullptr;

   SOCKET      m_socket { INVALID_SOCKET };
   sockaddr_in m_socketAddressIn {};
   uint16_t    m_port { DEFAULT_PORT };
   fd_set      m_readfds {};
   fd_set      m_writefds {};
};

// ===================================================
//      ClientConnection
// ===================================================
struct ClientConnection
{
   SOCKET      m_socket { INVALID_SOCKET };
   sockaddr_in m_socketAddressIn {};
   std::string m_ipAddress {};
};

// ===================================================
//      NetworkHost
// ===================================================
class NetworkHost final : public INetwork
{
public:
   const std::string& GetListenAddress() const noexcept { return m_ipAddress; }
   void               SetListenPort( uint16_t port ) noexcept
   {
      m_port                     = port;
      m_socketAddressIn.sin_port = htons( port );
   }

private:
   NetworkHost();
   ~NetworkHost() override;

   void ConnectClient( SOCKET clientSocket, const sockaddr_in& clientAddr );

   void                  Poll( World& world, Player& player ) override;
   constexpr NetworkType GetNetworkType() const noexcept override { return NetworkType::Host; }

   std::string                                      m_ipAddress;
   std::unordered_map< uint64_t, ClientConnection > m_clients;

   friend class INetwork;
};

// ===================================================
//      NetworkClient
// ===================================================
class NetworkClient final : public INetwork
{
public:
   //std::string GetClientAddress() const noexcept;
   //void        SetServerPort( uint16_t port ) noexcept;
   void Connect( const char* ipAddress, uint16_t port );

private:
   NetworkClient();
   ~NetworkClient() override;

   void                  Poll( World& world, Player& player ) override;
   constexpr NetworkType GetNetworkType() const noexcept override { return NetworkType::Client; }

   std::vector< uint64_t > m_relaySockets; // other clients
   bool                    m_fFirstPoll { true };

   friend class INetwork;
};

// INetwork static instance initialization
template< typename TNetwork >
static TNetwork& INetwork::Create()
{
   assert( !s_pInstance && "INetwork instance already exists!" );
   if( s_pInstance )
      throw std::runtime_error( "INetwork instance already exists!" );

   s_pInstance = new TNetwork();
   return static_cast< TNetwork& >( *s_pInstance );
}
