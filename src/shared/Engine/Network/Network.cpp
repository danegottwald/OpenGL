#include "Network.h"

// Local dependencies
#include "NetworkClient.h"
#include "NetworkHost.h"
#include "Packet.h"

// Project dependencies
#include <Engine/Events/NetworkEvent.h>

// ===================================================
//      INetwork
// ===================================================
INetwork::INetwork()
{
   WSADATA wsaData;
   WSAStartup( MAKEWORD( 2, 2 ), &wsaData );
}

INetwork::~INetwork()
{
   if( m_hostSocket != INVALID_SOCKET )
      closesocket( m_hostSocket );

   WSACleanup();
}

/*static*/ void INetwork::Shutdown() noexcept
{
   if( s_pInstance )
   {
      delete s_pInstance;
      s_pInstance = nullptr;
   }
}

/*static*/ const char* INetwork::GetHostAddress() noexcept
{
   static char s_address[ INET_ADDRSTRLEN ] = { 0 };
   if( s_address[ 0 ] != '\0' )
      return s_address;

   if( !INetwork::Get() )
   {
      WSADATA wsaData;
      WSAStartup( MAKEWORD( 2, 2 ), &wsaData );

      char hostname[ 256 ];
      if( gethostname( hostname, sizeof( hostname ) ) == 0 )
      {
         addrinfo hints = {}, *pInfo = nullptr;
         hints.ai_family = AF_INET;
         if( getaddrinfo( hostname, nullptr, &hints, &pInfo ) == 0 )
         {
            for( addrinfo* p = pInfo; p != nullptr; p = p->ai_next )
            {
               if( const char* addr = inet_ntoa( reinterpret_cast< sockaddr_in* >( p->ai_addr )->sin_addr ) )
               {
                  strcpy_s( s_address, sizeof( s_address ), addr );
                  break;
               }
            }
            freeaddrinfo( pInfo );
         }
      }

      WSACleanup();
   }

   return s_address;
}

/*static*/ bool INetwork::FResolvePeerAddress( Client& client )
{
   if( client.m_socket == INVALID_SOCKET )
      return false;

   int size = sizeof( Client::m_socketAddressIn );
   if( getpeername( client.m_socket, ( sockaddr* )&client.m_socketAddressIn, &size ) == 0 )
   {
      char ipBuffer[ INET_ADDRSTRLEN ];
      if( inet_ntop( AF_INET, &client.m_socketAddressIn.sin_addr, ipBuffer, sizeof( ipBuffer ) ) == nullptr )
      {
         std::println( std::cerr, "Failed to convert IP address to string for socket: {}", client.m_socket );
         return false;
      }

      client.m_ipAddress = ipBuffer;
      return true;
   }

   std::println( std::cerr, "Failed to get peer name for socket: {}", client.m_socket );
   return false;
}

void INetwork::Send( SOCKET targetSocket, const std::vector< uint8_t >& serializedData )
{
   if( send( targetSocket, reinterpret_cast< const char* >( serializedData.data() ), serializedData.size(), 0 ) == SOCKET_ERROR )
      s_NetworkLogs.push_back( std::format( "Failed to send packet to socket: {}, WSAError: {}", targetSocket, WSAGetLastError() ) );
}
