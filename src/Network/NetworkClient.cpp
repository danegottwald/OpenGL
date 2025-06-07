
#include "NetworkClient.h"

#include "../Events/NetworkEvent.h"
#include "../Game/Entity/Player.h"
#include "../Game/World.h"

// ====================================================
//      NetworkClient
// ====================================================
NetworkClient::NetworkClient()
{
   u_long mode  = 1; // non-blocking mode
   m_hostSocket = socket( AF_INET, SOCK_STREAM, 0 );
   ioctlsocket( m_hostSocket, FIONBIO, &mode );

   m_socketAddressIn.sin_family      = AF_INET;
   m_socketAddressIn.sin_addr.s_addr = inet_addr( DEFAULT_ADDRESS );
   m_socketAddressIn.sin_port        = htons( DEFAULT_PORT );

   //connect( m_hostSocket, ( struct sockaddr* )&m_socketAddressIn, sizeof( m_socketAddressIn ) );
}

NetworkClient::~NetworkClient()
{
   sockaddr_in clientAddr {};
   char        ipAddress[ INET_ADDRSTRLEN ];
   int         addrLen = sizeof( clientAddr );
   if( getpeername( m_hostSocket, ( sockaddr* )&clientAddr, &addrLen ) == 0 )
      inet_ntop( AF_INET, &clientAddr.sin_addr, ipAddress, sizeof( ipAddress ) );

   for( const Client& client : m_clients )
   {
      closesocket( client.m_socket );
      Events::Dispatch< Events::NetworkClientDisconnectEvent >( client.m_clientID );
   }

   if( m_fConnected )
      Events::Dispatch< Events::NetworkClientDisconnectEvent >( m_serverID ); // server is a "client" for now
}

void NetworkClient::Connect( const char* ipAddress, uint16_t port )
{
   m_serverIpAddress = ipAddress;

   m_socketAddressIn.sin_addr.s_addr = inet_addr( m_serverIpAddress.c_str() );
   m_socketAddressIn.sin_port        = htons( port );
   if( connect( m_hostSocket, ( struct sockaddr* )&m_socketAddressIn, sizeof( m_socketAddressIn ) ) == SOCKET_ERROR )
   {
      if( WSAGetLastError() != WSAEWOULDBLOCK )
         std::cerr << "connect error: " << WSAGetLastError() << std::endl;
   }
}

void NetworkClient::SendPacket( const Packet& packet )
{
   Send( m_hostSocket, Packet::Serialize( packet ) );
}

void NetworkClient::SendPackets( const std::vector< Packet >& packets )
{
   std::vector< uint8_t > serializedPackets;
   for( const Packet& packet : packets )
   {
      std::vector< uint8_t > serializedPacket = Packet::Serialize( packet );
      serializedPackets.insert( serializedPackets.end(), serializedPacket.begin(), serializedPacket.end() );
   }

   if( send( m_hostSocket, reinterpret_cast< char* >( serializedPackets.data() ), serializedPackets.size(), 0 ) == SOCKET_ERROR )
      s_logs.push_back( std::format( "Failed to send packet to host: {}", m_hostSocket ) );
}

void NetworkClient::Poll( World& world, Player& player )
{
   if( m_hostSocket == INVALID_SOCKET )
      return;

   fd_set readfds {}, writefds {};
   FD_ZERO( &readfds );
   FD_ZERO( &writefds );
   FD_SET( m_hostSocket, &readfds );
   FD_SET( m_hostSocket, &writefds );

   struct timeval timeout = { 0, 0 };
   if( m_fConnected && select( 0, &readfds, &writefds, nullptr, &timeout ) > 0 && FD_ISSET( m_hostSocket, &writefds ) )
   {
      // for right now, just send position updates every tick
      SendPacket( Packet::Create< NetworkCode::PositionUpdate >( m_ID, m_serverID, player.GetPosition() ) );
   }

   if( FD_ISSET( m_hostSocket, &readfds ) )
   {
      std::vector< uint8_t > buffer( PACKET_BUFFER_SIZE );
      int                    bytesReceived = recv( m_hostSocket, reinterpret_cast< char* >( buffer.data() ), buffer.size(), 0 );
      if( bytesReceived > 0 )
      {
         size_t offset = 0;
         while( offset < bytesReceived )
         {
            Packet inPacket = Packet::Deserialize( buffer, offset );
            offset += inPacket.Size();

            switch( inPacket.m_code )
            {
               case NetworkCode::Handshake:
               {
                  s_logs.push_back( std::format( "Connected to server: {}:{}", INetwork::GetHostAddress(), m_port ) );
                  m_fConnected = true;
                  m_serverID   = inPacket.m_sourceID;
                  m_ID         = inPacket.m_destinationID; // ID assigned by server
                  SendPacket( Packet::Create< NetworkCode::Handshake >( m_ID, m_serverID, {} ) );
                  Events::Dispatch< Events::NetworkClientConnectEvent >( m_serverID ); // connect server for now
                  break;
               }
               case NetworkCode::ClientConnect:
               {
                  Client newClient;
                  newClient.m_clientID = inPacket.m_sourceID;
                  newClient.m_socket   = m_hostSocket; // as a client, all other client sockets are the host socket
                  m_clients.push_back( newClient );
                  Events::Dispatch< Events::NetworkClientConnectEvent >( inPacket.m_sourceID ); // TODO: NetworkEvents should take in Client
                  break;
               }
               case NetworkCode::Chat:
               {
                  Events::Dispatch< Events::NetworkChatReceivedEvent >( inPacket.m_sourceID, Packet::ParseBuffer< NetworkCode::Chat >( inPacket ) );
                  break;
               }
               case NetworkCode::PositionUpdate:
               {
                  world.ReceivePlayerPosition( inPacket.m_sourceID, Packet::ParseBuffer< NetworkCode::PositionUpdate >( inPacket ) );
                  break;
               }
               case NetworkCode::ClientDisconnect:
               {
                  s_logs.push_back( std::format( "Client disconnected: {}", inPacket.m_sourceID ) );
                  m_clients.erase( std::remove_if( m_clients.begin(),
                                                   m_clients.end(),
                                                   [ &inPacket ]( const Client& client ) { return client.m_clientID == inPacket.m_sourceID; } ),
                                   m_clients.end() );
                  Events::Dispatch< Events::NetworkClientDisconnectEvent >( inPacket.m_sourceID );
                  break;
               }

               default: std::cout << "Unhandled network code" << std::endl;
            }
         }
      }
      else if( bytesReceived <= 0 && WSAGetLastError() != WSAEWOULDBLOCK )
      {
         s_logs.push_back( std::format( "Server closed: {}", INetwork::GetHostAddress() ) );
         m_fConnected = false;
         m_serverIpAddress.clear();

         for( const Client& client : m_clients )
         {
            closesocket( client.m_socket );
            Events::Dispatch< Events::NetworkClientDisconnectEvent >( client.m_clientID );
         }
         m_clients.clear();

         closesocket( m_hostSocket );                                            // close server socket connection
         Events::Dispatch< Events::NetworkClientDisconnectEvent >( m_serverID ); // disconnect from server for now
         Events::Dispatch< Events::NetworkHostDisconnectEvent >( m_serverID /*good enough for now*/ );
         m_hostSocket = INVALID_SOCKET;
      }
   }
}
