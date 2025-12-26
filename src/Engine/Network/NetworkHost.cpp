#include "NetworkHost.h"

// Project dependencies
#include <Engine/Events/NetworkEvent.h>

namespace
{
constexpr size_t   RECV_CHUNK_SIZE  = 4096;
constexpr uint32_t PROTOCOL_VERSION = 1;
}

NetworkHost::NetworkHost()
{
   SetID( Client::GenerateClientID() );

   char hostname[ 256 ];
   if( gethostname( hostname, sizeof( hostname ) ) == 0 )
   {
      addrinfo hints = {}, *info = nullptr;
      hints.ai_family = AF_INET;
      if( getaddrinfo( hostname, nullptr, &hints, &info ) == 0 )
      {
         for( addrinfo* p = info; p != nullptr; p = p->ai_next )
         {
            m_ipAddress = inet_ntoa( reinterpret_cast< sockaddr_in* >( p->ai_addr )->sin_addr );
            break;
         }

         freeaddrinfo( info );
      }
   }

   m_eventSubscriber.Subscribe< Events::NetworkClientDisconnectEvent >( [ & ]( const Events::NetworkClientDisconnectEvent& e ) noexcept
   { DisconnectClient( e.GetClientID() ); } );

   m_eventSubscriber.Subscribe< Events::NetworkRequestBlockUpdateEvent >( [ & ]( const Events::NetworkRequestBlockUpdateEvent& e ) noexcept
   {
      NetBlockUpdate upd;
      upd.pos     = e.GetBlockPos();
      upd.blockId = e.GetBlockId();
      upd.action  = e.GetAction();

      // Apply locally via incoming-style event
      Events::Dispatch< Events::NetworkBlockUpdateEvent >( m_ID, upd.pos, upd.blockId );

      // Broadcast to clients
      Broadcast_( Packet::Create< NetworkCode::BlockUpdate >( m_ID, 0 /*broadcast*/, upd ) );
   } );
}

NetworkHost::~NetworkHost()
{
   Shutdown();
}

void NetworkHost::Listen( uint16_t port )
{
   m_hostSocket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
   if( m_hostSocket == INVALID_SOCKET )
   {
      s_NetworkLogs.push_back( std::format( "Failed to create TCP socket: {}", WSAGetLastError() ) );
      return;
   }

   sockaddr_in addr {};
   addr.sin_family      = AF_INET;
   addr.sin_addr.s_addr = INADDR_ANY;
   addr.sin_port        = htons( port );
   if( bind( m_hostSocket, reinterpret_cast< sockaddr* >( &addr ), sizeof( addr ) ) == SOCKET_ERROR )
   {
      s_NetworkLogs.push_back( std::format( "Failed to bind TCP socket: {}", WSAGetLastError() ) );
      closesocket( m_hostSocket );
      m_hostSocket = INVALID_SOCKET;
      return;
   }

   if( listen( m_hostSocket, SOMAXCONN ) == SOCKET_ERROR )
   {
      s_NetworkLogs.push_back( std::format( "Failed to listen on TCP socket: {}", WSAGetLastError() ) );
      closesocket( m_hostSocket );
      m_hostSocket = INVALID_SOCKET;
      return;
   }

   // Non-blocking accept()
   u_long nonBlocking = 1;
   ioctlsocket( m_hostSocket, FIONBIO, &nonBlocking );

   m_fRunning = true;
}

void NetworkHost::Shutdown()
{
   m_fRunning = false;

   for( auto& [ id, peer ] : m_peers )
   {
      if( peer.socket != INVALID_SOCKET )
         closesocket( peer.socket );
   }
   m_peers.clear();
}

void NetworkHost::DisconnectClient( uint64_t clientID )
{
   auto it = m_peers.find( clientID );
   if( it == m_peers.end() )
      return;

   if( it->second.socket != INVALID_SOCKET )
      closesocket( it->second.socket );

   m_peers.erase( it );

   // Broadcast disconnect (best-effort)
   Broadcast_( Packet::Create< NetworkCode::ClientDisconnect >( m_ID, 0 /*broadcast*/, clientID ) );
}

void NetworkHost::QueueSendBytes_( Peer& peer, std::span< const uint8_t > bytes )
{
   peer.sendBuffer.insert( peer.sendBuffer.end(), bytes.begin(), bytes.end() );
}

void NetworkHost::QueueSend_( Peer& peer, const Packet& packet )
{
   auto bytes = Packet::Serialize( packet );
   QueueSendBytes_( peer, bytes );
}

void NetworkHost::FlushSends_( Peer& peer )
{
   while( !peer.sendBuffer.empty() )
   {
      int sent = send( peer.socket, reinterpret_cast< const char* >( peer.sendBuffer.data() ), static_cast< int >( peer.sendBuffer.size() ), 0 );

      if( sent == SOCKET_ERROR )
      {
         int err = WSAGetLastError();
         if( err == WSAEWOULDBLOCK )
            return;

         DisconnectClient( peer.clientID );
         return;
      }

      if( sent <= 0 )
         return;

      peer.sendBuffer.erase( peer.sendBuffer.begin(), peer.sendBuffer.begin() + sent );
   }
}

void NetworkHost::Broadcast_( const Packet& packet )
{
   for( auto& [ id, peer ] : m_peers )
      QueueSend_( peer, packet );
}

void NetworkHost::SendPacket( const Packet& packet )
{
   Broadcast_( packet );
}

void NetworkHost::SendPackets( std::span< Packet > packets )
{
   for( const auto& p : packets )
      Broadcast_( p );
}

void NetworkHost::HandleIncomingPacket( const Packet& packet )
{
   switch( packet.m_code )
   {
      case NetworkCode::Heartbeat:
      case NetworkCode::ClientDisconnect: break;

      case NetworkCode::Chat:
         Events::Dispatch< Events::NetworkChatReceivedEvent >( packet.m_sourceID, Packet::ParseBuffer< NetworkCode::Chat >( packet ) );
         break;

      case NetworkCode::PositionUpdate:
         Events::Dispatch< Events::NetworkPositionUpdateEvent >( packet.m_sourceID, Packet::ParseBuffer< NetworkCode::PositionUpdate >( packet ) );
         break;

      case NetworkCode::BlockUpdate:
      {
         const auto upd = Packet::ParseBuffer< NetworkCode::BlockUpdate >( packet );
         Events::Dispatch< Events::NetworkBlockUpdateEvent >( packet.m_sourceID, upd.pos, upd.blockId );
         Broadcast_( Packet::Create< NetworkCode::BlockUpdate >( packet.m_sourceID, 0 /*broadcast*/, upd ) );
         break;
      }

      default: std::println( "Error: Unhandled packet code {}", static_cast< int >( packet.m_code ) ); break;
   }
}

void NetworkHost::AcceptNewClients_()
{
   for( ;; )
   {
      SOCKET clientSocket = accept( m_hostSocket, nullptr, nullptr );
      if( clientSocket == INVALID_SOCKET )
      {
         const int err = WSAGetLastError();
         if( err == WSAEWOULDBLOCK )
            break;

         continue;
      }

      u_long nonBlocking = 1;
      if( ioctlsocket( clientSocket, FIONBIO, &nonBlocking ) == SOCKET_ERROR )
      {
         closesocket( clientSocket );
         continue;
      }

      const uint64_t clientID = Client::GenerateClientID();

      Peer peer;
      peer.clientID = clientID;
      peer.socket   = clientSocket;
      peer.state    = PeerState::AwaitingHandshakeInit;
      peer.recvBuffer.reserve( RECV_CHUNK_SIZE );

      auto [ it, inserted ] = m_peers.emplace( clientID, std::move( peer ) );
      Peer& newPeer         = it->second;

      // Enqueue handshake accept immediately (reliable via sendBuffer + flush)
      QueueSend_( newPeer, Packet::Create< NetworkCode::HandshakeAccept >( m_ID, clientID, clientID ) );
      QueueSend_( newPeer, Packet::Create< NetworkCode::ClientConnect >( m_ID, clientID, m_ID ) );

      // Inform existing clients
      //Broadcast_( Packet::Create< NetworkCode::ClientConnect >( m_ID, 0 /*broadcast*/, clientID ) );
      Events::Dispatch< Events::NetworkClientConnectEvent >( clientID );

      // Best-effort flush now
      FlushSends_( newPeer );
   }
}

void NetworkHost::PollClients_()
{
   std::vector< uint64_t > toDisconnect;

   for( auto& [ clientID, peer ] : m_peers )
   {
      // recv
      std::array< uint8_t, RECV_CHUNK_SIZE > chunk {};
      int                                    bytes = recv( peer.socket, reinterpret_cast< char* >( chunk.data() ), static_cast< int >( chunk.size() ), 0 );

      if( bytes > 0 )
      {
         peer.recvBuffer.insert( peer.recvBuffer.end(), chunk.begin(), chunk.begin() + bytes );

         size_t bytesToProcess = peer.recvBuffer.size();
         size_t offset         = 0;
         size_t maxPackets     = 64;
         size_t processed      = 0;

         while( offset < bytesToProcess && processed < maxPackets )
         {
            auto pkt = Packet::Deserialize( peer.recvBuffer, offset );
            if( !pkt )
               break;

            // Socket-authoritative identity
            const uint64_t effectiveSource = peer.clientID;

            // Handshake gate / verification
            if( peer.state == PeerState::AwaitingHandshakeInit )
            {
               if( pkt->m_code != NetworkCode::HandshakeInit )
               {
                  toDisconnect.push_back( clientID );
                  break;
               }

               const uint32_t version = Packet::ParseBuffer< NetworkCode::HandshakeInit >( *pkt );
               if( version != PROTOCOL_VERSION )
               {
                  toDisconnect.push_back( clientID );
                  break;
               }

               peer.state = PeerState::Connected;
               // No event here; connect event already fired on accept.
            }
            else
            {
               if( pkt->m_sourceID != effectiveSource )
               {
                  toDisconnect.push_back( clientID );
                  break;
               }
            }

            pkt->m_sourceID = effectiveSource;
            HandleIncomingPacket( *pkt );

            offset += pkt->Size();
            processed++;
         }

         if( offset > 0 )
            peer.recvBuffer.erase( peer.recvBuffer.begin(), peer.recvBuffer.begin() + offset );
      }
      else if( bytes == 0 )
      {
         toDisconnect.push_back( clientID );
      }
      else
      {
         int err = WSAGetLastError();
         if( err != WSAEWOULDBLOCK )
            toDisconnect.push_back( clientID );
      }

      // send
      FlushSends_( peer );
   }

   for( uint64_t id : toDisconnect )
      DisconnectClient( id );
}

void NetworkHost::Poll( const glm::vec3& playerPosition )
{
   if( m_hostSocket == INVALID_SOCKET )
      return;

   AcceptNewClients_();
   PollClients_();

   // Keep position updates in Poll (broadcast)
   Broadcast_( Packet::Create< NetworkCode::PositionUpdate >( m_ID, 0 /*broadcast*/, playerPosition ) );

   // Flush any queued broadcasts
   for( auto& [ id, peer ] : m_peers )
      FlushSends_( peer );
}
