#include "NetworkClient.h"

// Project dependencies
#include <Engine/Events/NetworkEvent.h>

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
   if( m_udpRunning )
   {
      m_udpRunning = false;
      if( m_udpRecvThread.joinable() )
         m_udpRecvThread.join();
   }
   if( m_udpSocket != INVALID_SOCKET )
      closesocket( m_udpSocket );

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

void NetworkClient::InitUDP( uint16_t port )
{
   m_udpSocket = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
   if( m_udpSocket == INVALID_SOCKET )
   {
      s_logs.push_back( std::format( "Failed to create UDP socket (client): {}", WSAGetLastError() ) );
      return;
   }

   sockaddr_in localAddr {};
   localAddr.sin_family      = AF_INET;
   localAddr.sin_addr.s_addr = INADDR_ANY; // any local interface
   localAddr.sin_port        = 0;          // let OS pick a port
   if( bind( m_udpSocket, reinterpret_cast< sockaddr* >( &localAddr ), sizeof( localAddr ) ) == SOCKET_ERROR )
   {
      s_logs.push_back( std::format( "Failed to bind UDP client socket: {}", WSAGetLastError() ) );
      closesocket( m_udpSocket );
      m_udpSocket = INVALID_SOCKET;
      return;
   }

   u_long mode = 1;
   ioctlsocket( m_udpSocket, FIONBIO, &mode );

   // Setup server UDP address
   m_serverUdpAddr.sin_family      = AF_INET;
   m_serverUdpAddr.sin_addr.s_addr = inet_addr( m_serverIpAddress.c_str() );
   m_serverUdpAddr.sin_port        = htons( port );

   m_udpRunning    = true;
   m_udpRecvThread = std::thread( &NetworkClient::UDPRecvThread, this );
}

void NetworkClient::UDPRecvThread()
{
   std::vector< uint8_t > buffer( RECV_BUFFER_SIZE );
   while( m_udpRunning )
   {
      sockaddr_in fromAddr {};
      int         fromLen = sizeof( fromAddr );
      int         bytes   = recvfrom(
         m_udpSocket, reinterpret_cast< char* >( buffer.data() ), ( int )buffer.size(), 0, reinterpret_cast< sockaddr* >( &fromAddr ), &fromLen );
      if( bytes > 0 )
      {
         size_t offset = 0;
         while( offset < static_cast< size_t >( bytes ) )
         {
            auto inPacket = Packet::Deserialize( buffer, offset );
            if( !inPacket )
               break;
            offset += inPacket->Size();

            // Only handle real-time packets expected over UDP
            if( inPacket->m_code == NetworkCode::PositionUpdate )
            {
               Events::Dispatch< Events::NetworkPositionUpdateEvent >( inPacket->m_sourceID, Packet::ParseBuffer< NetworkCode::PositionUpdate >( *inPacket ) );
            }
         }
      }
      std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
   }
}

void NetworkClient::RecvThread()
{
   std::vector< uint8_t > buffer( RECV_BUFFER_SIZE );
   while( m_running )
   {
      int bytesReceived = recv( m_hostSocket, reinterpret_cast< char* >( buffer.data() ), ( int )buffer.size(), 0 );
      if( bytesReceived > 0 )
      {
         size_t offset = 0;
         while( offset < static_cast< size_t >( bytesReceived ) )
         {
            if( auto inPacket = Packet::Deserialize( buffer, offset ) )
            {
               offset += inPacket->Size();
               std::lock_guard< std::mutex > lock( m_queueMutex );
               m_incomingPackets.push( *inPacket );
            }
            else
            {
               s_logs.push_back( std::format( "Failed to deserialize packet: {}", std::to_underlying( inPacket.error() ) ) );
               break;
            }
         }
      }
      else if( bytesReceived == 0 || ( bytesReceived < 0 && WSAGetLastError() != WSAEWOULDBLOCK ) )
      {
         m_fConnected = false;
         m_running    = false;
         break;
      }

      std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
   }
}

std::optional< Packet > NetworkClient::PollPacket()
{
   std::lock_guard< std::mutex > lock( m_queueMutex );
   if( m_incomingPackets.empty() )
      return std::nullopt; // No packets to process

   const Packet& packet = m_incomingPackets.front();
   m_incomingPackets.pop();
   return packet;
}

void NetworkClient::Connect( const char* ipAddress, uint16_t port )
{
   m_serverIpAddress                 = ipAddress;
   m_socketAddressIn.sin_addr.s_addr = inet_addr( m_serverIpAddress.c_str() );
   m_socketAddressIn.sin_port        = htons( port );
   if( connect( m_hostSocket, ( struct sockaddr* )&m_socketAddressIn, sizeof( m_socketAddressIn ) ) == SOCKET_ERROR )
   {
      if( WSAGetLastError() != WSAEWOULDBLOCK )
      {
         std::println( std::cerr, "Failed to connect to server: {}:{}", m_serverIpAddress, port );
         std::println( std::cerr, "connect error: {}", WSAGetLastError() );
         m_fConnected = false;
         return;
      }
   }

   //m_fConnected = true; // Set to true only if connect did not fail
   StartRecvThread();
   InitUDP( port );
}

void NetworkClient::SendUDPPacket( const Packet& packet )
{
   if( m_udpSocket == INVALID_SOCKET )
      return;

   auto data = Packet::Serialize( packet );
   sendto( m_udpSocket,
           reinterpret_cast< const char* >( data.data() ),
           ( int )data.size(),
           0,
           reinterpret_cast< const sockaddr* >( &m_serverUdpAddr ),
           sizeof( m_serverUdpAddr ) );
}

void NetworkClient::SendPacket( const Packet& packet )
{
   if( packet.m_code == NetworkCode::PositionUpdate )
   {
      // Send unreliable over UDP
      SendUDPPacket( packet );
      return;
   }

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

   if( send( m_hostSocket, reinterpret_cast< char* >( serializedPackets.data() ), ( int )serializedPackets.size(), 0 ) == SOCKET_ERROR )
      s_logs.push_back( std::format( "Failed to send packet to host: {}", m_hostSocket ) );
}

void NetworkClient::HandleIncomingPacket( const Packet& inPacket )
{
   switch( inPacket.m_code )
   {
      case NetworkCode::HostShutdown:
         m_fConnected = false;
         Events::Dispatch< Events::NetworkHostShutdownEvent >( inPacket.m_sourceID );
         break;
      case NetworkCode::ClientConnect:
      {
         if( m_fConnected )
            break; // Already connected

         s_logs.push_back( std::format( "Connected to server: {}:{}", INetwork::GetHostAddress(), m_port ) );
         m_fConnected = true;
         m_serverID   = inPacket.m_sourceID;
         m_ID         = inPacket.m_destinationID;

         Client newClient;
         newClient.m_clientID = inPacket.m_sourceID;
         newClient.m_socket   = m_hostSocket;
         m_clients.push_back( newClient );
         Events::Dispatch< Events::NetworkClientConnectEvent >( inPacket.m_sourceID );

         // Register UDP endpoint (unreliable channel)
         SendUDPPacket( Packet::Create< NetworkCode::UDPRegister >( m_ID, m_serverID, m_ID ) );
         break;
      }
      case NetworkCode::Chat:
         Events::Dispatch< Events::NetworkChatReceivedEvent >( inPacket.m_sourceID, Packet::ParseBuffer< NetworkCode::Chat >( inPacket ) );
         break;
      case NetworkCode::PositionUpdate:
         // These normally come over UDP, but allow TCP fallback
         Events::Dispatch< Events::NetworkPositionUpdateEvent >( inPacket.m_sourceID, Packet::ParseBuffer< NetworkCode::PositionUpdate >( inPacket ) );
         break;
      case NetworkCode::ClientDisconnect:
         s_logs.push_back( std::format( "Client disconnected: {}", inPacket.m_sourceID ) );
         m_clients.erase( std::remove_if( m_clients.begin(),
                                          m_clients.end(),
                                          [ &inPacket ]( const Client& client ) { return client.m_clientID == inPacket.m_sourceID; } ),
                          m_clients.end() );
         Events::Dispatch< Events::NetworkClientDisconnectEvent >( inPacket.m_sourceID );
         break;
      case NetworkCode::Heartbeat: break;
      default:                     std::println( std::cerr, "Unhandled network code: {}", std::to_underlying( inPacket.m_code ) );
   }
}

void NetworkClient::Poll( const glm::vec3& playerPosition )
{
   if( m_hostSocket == INVALID_SOCKET )
      return;

   if( m_fConnected )
      SendPacket( Packet::Create< NetworkCode::PositionUpdate >( m_ID, m_serverID, playerPosition ) );

   while( std::optional< Packet > optInPacket = PollPacket() )
      HandleIncomingPacket( *optInPacket );
}
