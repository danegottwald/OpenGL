#include "NetworkClient.h"

// Project dependencies
#include <Engine/Events/NetworkEvent.h>

namespace
{
constexpr size_t   RECV_CHUNK_SIZE  = 4096;
constexpr uint32_t PROTOCOL_VERSION = 1;
}

NetworkClient::NetworkClient()
{
   m_ID = 0;

   u_long mode  = 1; // non-blocking mode
   m_hostSocket = socket( AF_INET, SOCK_STREAM, 0 );
   if( m_hostSocket != INVALID_SOCKET )
      ioctlsocket( m_hostSocket, FIONBIO, &mode );

   m_socketAddressIn.sin_family      = AF_INET;
   m_socketAddressIn.sin_addr.s_addr = inet_addr( DEFAULT_ADDRESS );
   m_socketAddressIn.sin_port        = htons( DEFAULT_PORT );

   m_recvBuffer.reserve( RECV_CHUNK_SIZE );

   // Gameplay -> Network outbox (decoupled)
   static Events::EventSubscriber s_outboxSubscriber;
   s_outboxSubscriber.Subscribe< Events::NetworkRequestBlockUpdateEvent >( [ this ]( const Events::NetworkRequestBlockUpdateEvent& e ) noexcept
   {
      if( m_state != State::Connected || m_ID == 0 )
         return;

      NetBlockUpdate upd;
      upd.pos     = e.GetBlockPos();
      upd.blockId = e.GetBlockId();
      upd.action  = e.GetAction();

      QueueSend_( Packet::Create< NetworkCode::BlockUpdate >( m_ID, upd ) );
   } );
}

NetworkClient::~NetworkClient()
{
   if( m_hostSocket != INVALID_SOCKET )
      closesocket( m_hostSocket );
}

void NetworkClient::Connect( const char* ipAddress, uint16_t port )
{
   m_serverIpAddress                 = ipAddress;
   m_socketAddressIn.sin_addr.s_addr = inet_addr( m_serverIpAddress.c_str() );
   m_socketAddressIn.sin_port        = htons( port );

   m_state = State::Connecting;

   int r = connect( m_hostSocket, ( struct sockaddr* )&m_socketAddressIn, sizeof( m_socketAddressIn ) );
   if( r == SOCKET_ERROR )
   {
      const int err = WSAGetLastError();
      if( err != WSAEWOULDBLOCK && err != WSAEINPROGRESS && err != WSAEALREADY )
      {
         s_NetworkLogs.push_back( std::format( "Failed to connect to server: {}:{} (err={})", m_serverIpAddress, port, err ) );
         m_state = State::Disconnected;
         return;
      }
   }

   // Immediately start handshake attempts
   m_state         = State::AwaitingHandshakeAccept;
   m_handshakeTick = 0;
   QueueSend_( Packet::Create< NetworkCode::HandshakeInit >( 0, PROTOCOL_VERSION ) );
}

void NetworkClient::QueueSend_( const Packet& packet )
{
   auto bytes = Packet::Serialize( packet );
   m_sendBuffer.insert( m_sendBuffer.end(), bytes.begin(), bytes.end() );
}

void NetworkClient::FlushSend_()
{
   while( !m_sendBuffer.empty() )
   {
      int sent = send( m_hostSocket, reinterpret_cast< const char* >( m_sendBuffer.data() ), static_cast< int >( m_sendBuffer.size() ), 0 );

      if( sent == SOCKET_ERROR )
      {
         const int err = WSAGetLastError();
         if( err == WSAEWOULDBLOCK )
            return;

         m_state = State::Disconnected;
         return;
      }

      if( sent <= 0 )
         return;

      m_sendBuffer.erase( m_sendBuffer.begin(), m_sendBuffer.begin() + sent );
   }
}

void NetworkClient::SendPacket( const Packet& packet )
{
   QueueSend_( packet );
}

void NetworkClient::SendPackets( std::span< Packet > packets )
{
   for( const auto& p : packets )
      QueueSend_( p );
}

void NetworkClient::HandleIncomingPacket( const Packet& inPacket )
{
   switch( inPacket.m_code )
   {
      case NetworkCode::HandshakeAccept:
      {
         const uint64_t assignedID = Packet::ParseBuffer< NetworkCode::HandshakeAccept >( inPacket );
         m_serverID                = inPacket.m_sourceID;
         m_ID                      = assignedID;
         m_state                   = State::Connected;
         s_NetworkLogs.push_back( std::format( "Handshake accepted. Assigned client id: {}", assignedID ) );
         break;
      }

      case NetworkCode::ClientConnect:
      {
         const uint64_t joinedID = Packet::ParseBuffer< NetworkCode::ClientConnect >( inPacket );
         Events::Dispatch< Events::NetworkClientConnectEvent >( joinedID );
         break;
      }

      case NetworkCode::ClientDisconnect:
      {
         const uint64_t leftID = Packet::ParseBuffer< NetworkCode::ClientDisconnect >( inPacket );
         Events::Dispatch< Events::NetworkClientDisconnectEvent >( leftID );
         break;
      }

      case NetworkCode::Chat:
         Events::Dispatch< Events::NetworkChatReceivedEvent >( inPacket.m_sourceID, Packet::ParseBuffer< NetworkCode::Chat >( inPacket ) );
         break;

      case NetworkCode::PositionUpdate:
         Events::Dispatch< Events::NetworkPositionUpdateEvent >( inPacket.m_sourceID, Packet::ParseBuffer< NetworkCode::PositionUpdate >( inPacket ) );
         break;

      case NetworkCode::BlockUpdate:
      {
         const auto upd = Packet::ParseBuffer< NetworkCode::BlockUpdate >( inPacket );
         Events::Dispatch< Events::NetworkBlockUpdateEvent >( inPacket.m_sourceID, upd.pos, upd.blockId );
         break;
      }

      case NetworkCode::HostShutdown:
         Events::Dispatch< Events::NetworkHostShutdownEvent >( inPacket.m_sourceID );
         m_state = State::Disconnected;
         break;

      case NetworkCode::Heartbeat:
      default:                     break;
   }
}

void NetworkClient::ProcessStream_()
{
   size_t bytesToProcess = m_recvBuffer.size();
   size_t offset         = 0;
   size_t maxPackets     = 64;
   size_t processed      = 0;

   while( offset < bytesToProcess && processed < maxPackets )
   {
      auto pkt = Packet::Deserialize( m_recvBuffer, offset );
      if( !pkt )
         break;

      // During handshake, accept only handshake packet
      if( m_state == State::AwaitingHandshakeAccept )
      {
         if( pkt->m_code != NetworkCode::HandshakeAccept )
         {
            offset += pkt->Size();
            processed++;
            continue;
         }
      }

      HandleIncomingPacket( *pkt );

      offset += pkt->Size();
      processed++;
   }

   if( offset > 0 )
      m_recvBuffer.erase( m_recvBuffer.begin(), m_recvBuffer.begin() + offset );
}

void NetworkClient::Poll( const glm::vec3& playerPosition )
{
   if( m_hostSocket == INVALID_SOCKET )
      return;

   // recv
   std::array< uint8_t, RECV_CHUNK_SIZE > chunk {};
   int                                    bytes = recv( m_hostSocket, reinterpret_cast< char* >( chunk.data() ), static_cast< int >( chunk.size() ), 0 );
   if( bytes > 0 )
   {
      m_recvBuffer.insert( m_recvBuffer.end(), chunk.begin(), chunk.begin() + bytes );
      ProcessStream_();
   }
   else if( bytes == 0 )
   {
      m_state = State::Disconnected;
      return;
   }
   else
   {
      const int err = WSAGetLastError();
      if( err != WSAEWOULDBLOCK )
      {
         m_state = State::Disconnected;
         return;
      }
   }

   // handshake retransmit (TCP, but this covers partial-send / late connect state)
   if( m_state == State::AwaitingHandshakeAccept )
   {
      if( ( ++m_handshakeTick % 60u ) == 0u )
         QueueSend_( Packet::Create< NetworkCode::HandshakeInit >( 0, PROTOCOL_VERSION ) );
   }

   // gameplay update
   if( m_state == State::Connected && m_ID != 0 )
   {
      QueueSend_( Packet::Create< NetworkCode::PositionUpdate >( m_ID, playerPosition ) );
   }

   // send
   FlushSend_();
}
