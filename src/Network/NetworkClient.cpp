#include "NetworkClient.h"

#include "../Events/NetworkEvent.h"
#include "../Game/Entity/Player.h"
#include "../Game/World.h"
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>

namespace
{
constexpr size_t RECV_BUFFER_SIZE = 4096;
}

NetworkClient::NetworkClient()
{
   u_long mode  = 1; // non-blocking mode
   m_hostSocket = socket( AF_INET, SOCK_STREAM, 0 );
   ioctlsocket( m_hostSocket, FIONBIO, &mode );

   m_socketAddressIn.sin_family      = AF_INET;
   m_socketAddressIn.sin_addr.s_addr = inet_addr( DEFAULT_ADDRESS );
   m_socketAddressIn.sin_port        = htons( DEFAULT_PORT );
}

NetworkClient::~NetworkClient()
{
   StopRecvThread();
   closesocket( m_hostSocket );
   for( const Client& client : m_clients )
   {
      closesocket( client.m_socket );
      Events::Dispatch< Events::NetworkClientDisconnectEvent >( client.m_clientID );
   }

   if( m_fConnected )
      Events::Dispatch< Events::NetworkClientDisconnectEvent >( m_serverID );
}

void NetworkClient::StartRecvThread()
{
   m_running    = true;
   m_recvThread = std::thread( &NetworkClient::RecvThread, this );
}

void NetworkClient::StopRecvThread()
{
   m_running = false;
   if( m_recvThread.joinable() )
      m_recvThread.join();
}

void NetworkClient::RecvThread()
{
   std::vector< uint8_t > buffer( RECV_BUFFER_SIZE );
   while( m_running )
   {
      int bytesReceived = recv( m_hostSocket, reinterpret_cast< char* >( buffer.data() ), buffer.size(), 0 );
      if( bytesReceived > 0 )
      {
         size_t offset = 0;
         while( offset < static_cast< size_t >( bytesReceived ) )
         {
            Packet inPacket = Packet::Deserialize( buffer, offset );
            offset += inPacket.Size();
            std::lock_guard< std::mutex > lock( m_queueMutex );
            m_incomingPackets.push( inPacket );
         }
      }
      else if( bytesReceived == 0 || ( bytesReceived < 0 && WSAGetLastError() != WSAEWOULDBLOCK ) )
      {
         m_running = false;
         break;
      }

      std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
   }
}

bool NetworkClient::PollPacket( Packet& packet )
{
   std::lock_guard< std::mutex > lock( m_queueMutex );
   if( m_incomingPackets.empty() )
      return false; // No packets to process

   packet = m_incomingPackets.front();
   m_incomingPackets.pop();
   return true;
}

void NetworkClient::Connect( const char* ipAddress, uint16_t port )
{
   m_serverIpAddress                 = ipAddress;
   m_socketAddressIn.sin_addr.s_addr = inet_addr( m_serverIpAddress.c_str() );
   m_socketAddressIn.sin_port        = htons( port );
   if( connect( m_hostSocket, ( struct sockaddr* )&m_socketAddressIn, sizeof( m_socketAddressIn ) ) == SOCKET_ERROR )
   {
      if( WSAGetLastError() != WSAEWOULDBLOCK )
         std::cerr << "connect error: " << WSAGetLastError() << std::endl;
   }

   StartRecvThread();
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

void NetworkClient::HandleIncomingPacket( const Packet& inPacket, World& world, Player& player )
{
   switch( inPacket.m_code )
   {
      //case NetworkCode::Handshake:
      //   s_logs.push_back( std::format( "Connected to server: {}:{}", INetwork::GetHostAddress(), m_port ) );
      //   m_fConnected = true;
      //   m_serverID   = inPacket.m_sourceID;
      //   m_ID         = inPacket.m_destinationID;
      //   SendPacket( Packet::Create< NetworkCode::Handshake >( m_ID, m_serverID, {} ) );
      //   Events::Dispatch< Events::NetworkClientConnectEvent >( m_serverID );
      //   break;
      case NetworkCode::HostShutdown: Events::Dispatch< Events::NetworkHostShutdownEvent >( inPacket.m_sourceID ); break;
      case NetworkCode::ClientConnect:
      {
         if( m_fConnected )
            break; // Already connected, ignore duplicate, this shouldn't happen

         s_logs.push_back( std::format( "Connected to server: {}:{}", INetwork::GetHostAddress(), m_port ) );
         m_fConnected = true;
         m_serverID   = inPacket.m_sourceID;
         m_ID         = inPacket.m_destinationID;

         Client newClient;
         newClient.m_clientID = inPacket.m_sourceID;
         newClient.m_socket   = m_hostSocket;
         m_clients.push_back( newClient );
         Events::Dispatch< Events::NetworkClientConnectEvent >( inPacket.m_sourceID );
         break;
      }
      case NetworkCode::Chat:
         Events::Dispatch< Events::NetworkChatReceivedEvent >( inPacket.m_sourceID, Packet::ParseBuffer< NetworkCode::Chat >( inPacket ) );
         break;
      case NetworkCode::PositionUpdate:
         world.ReceivePlayerPosition( inPacket.m_sourceID, Packet::ParseBuffer< NetworkCode::PositionUpdate >( inPacket ) );
         break;
      case NetworkCode::ClientDisconnect:
         s_logs.push_back( std::format( "Client disconnected: {}", inPacket.m_sourceID ) );
         m_clients.erase( std::remove_if( m_clients.begin(),
                                          m_clients.end(),
                                          [ &inPacket ]( const Client& client ) { return client.m_clientID == inPacket.m_sourceID; } ),
                          m_clients.end() );
         Events::Dispatch< Events::NetworkClientDisconnectEvent >( inPacket.m_sourceID );
         break;
      case NetworkCode::Heartbeat:
         // Optionally handle heartbeat
         break;

      default: std::cout << "Unhandled network code" << std::endl;
   }
}

void NetworkClient::Poll( World& world, Player& player )
{
   if( m_hostSocket == INVALID_SOCKET )
      return;

   // Send position update if connected
   if( m_fConnected )
      SendPacket( Packet::Create< NetworkCode::PositionUpdate >( m_ID, m_serverID, player.GetPosition() ) );

   // Process incoming packets
   Packet inPacket;
   while( PollPacket( inPacket ) )
      HandleIncomingPacket( inPacket, world, player );
}
