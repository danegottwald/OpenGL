#include "NetworkHost.h"

#include "../Events/NetworkEvent.h"
#include "../Game/World.h"

constexpr size_t RECV_BUFFER_SIZE = 4096;

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
}

NetworkHost::~NetworkHost()
{
   Shutdown();
}

void NetworkHost::Listen( uint16_t port )
{
   m_hostSocket                      = socket( AF_INET, SOCK_STREAM, 0 );
   m_socketAddressIn.sin_family      = AF_INET;
   m_socketAddressIn.sin_addr.s_addr = INADDR_ANY;
   m_socketAddressIn.sin_port        = htons( port );
   bind( m_hostSocket, ( struct sockaddr* )&m_socketAddressIn, sizeof( m_socketAddressIn ) );
   listen( m_hostSocket, SOMAXCONN );

   u_long mode = 1; // Set host socket to non-blocking mode
   ioctlsocket( m_hostSocket, FIONBIO, &mode );

   m_fRunning     = true;
   m_acceptThread = std::thread( &NetworkHost::AcceptThread, this );
}

void NetworkHost::AcceptThread()
{
   while( m_fRunning )
   {
      // Check for new connections
      if( SOCKET clientSocket = accept( m_hostSocket, nullptr, nullptr ); clientSocket != INVALID_SOCKET )
      {
         Client newClient;
         newClient.m_clientID = Client::GenerateClientID();
         newClient.m_socket   = clientSocket;
         if( !FResolvePeerAddress( newClient ) )
         {
            closesocket( clientSocket );
            continue;
         }

         {
            std::lock_guard< std::mutex > lock( m_clientsMutex );
            m_clientSockets[ newClient.m_clientID ]  = clientSocket;
            m_outgoingQueues[ newClient.m_clientID ] = std::queue< Packet >();
         }

         // Start IO thread for this client
         m_clientThreads[ newClient.m_clientID ] = std::thread( &NetworkHost::ClientIOThread, this, newClient.m_clientID );

         // Send handshake
         //std::vector< uint8_t > data = Packet::Serialize( Packet::Create< NetworkCode::ClientConnect >( m_ID, newClient.m_clientID, 0 /*data*/ ) );
         //send( clientSocket, reinterpret_cast< const char* >( data.data() ), data.size(), 0 );

         SendPacket( Packet::Create< NetworkCode::ClientConnect >( m_ID, newClient.m_clientID, 0 /*data*/ ) ); // Inform new client of the host
         Events::Dispatch< Events::NetworkClientConnectEvent >( newClient.m_clientID );                        // Notify that a new client connected

         // TODO: Need to inform all other clients that a new client connected
      }

      // Sleep accept thread to avoid busy waiting
      std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
   }
}

void NetworkHost::ClientIOThread( uint64_t clientID )
{
   SOCKET sock;
   {
      std::lock_guard< std::mutex > lock( m_clientsMutex );
      sock = m_clientSockets[ clientID ];
   }

   // Set this client socket to non-blocking mode
   u_long mode = 1;
   ioctlsocket( sock, FIONBIO, &mode );

   std::vector< uint8_t > recvBuffer( RECV_BUFFER_SIZE );
   while( m_fRunning )
   {
      // Receive
      int bytesReceived = recv( sock, reinterpret_cast< char* >( recvBuffer.data() ), recvBuffer.size(), 0 );
      if( bytesReceived > 0 )
      {
         size_t offset = 0;
         while( offset < static_cast< size_t >( bytesReceived ) )
         {
            if( auto inPacket = Packet::Deserialize( recvBuffer, offset ) )
            {
               offset += inPacket->Size();

               // Handle the packet
               // TODO: Pass World/Player for game logic
               // For now, pass nullptrs (should be refactored for real use)

               World* dummyWorld = nullptr;
               HandleIncomingPacket( *inPacket, *( World* )dummyWorld );
            }
            else
            {
               s_logs.push_back( std::format( "Failed to deserialize packet from client {}: {}", clientID, std::to_underlying( inPacket.error() ) ) );
               break;
            }
         }
      }
      else if( bytesReceived == 0 || ( bytesReceived < 0 && WSAGetLastError() != WSAEWOULDBLOCK ) )
      {
         Events::Dispatch< Events::NetworkClientDisconnectEvent >( clientID );
         break;
      }

      // Send
      Packet outPacket;
      while( DequeueOutgoing( clientID, outPacket ) )
      {
         std::vector< uint8_t > data = Packet::Serialize( outPacket );
         send( sock, reinterpret_cast< char* >( data.data() ), data.size(), 0 );
      }

      std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
   }
}

void NetworkHost::HandleIncomingPacket( const Packet& packet, World& world )
{
   switch( packet.m_code )
   {
      case NetworkCode::Heartbeat:
         // Optionally handle heartbeat logic
         break;
      //case NetworkCode::ClientConnect:
      //   // Optionally handle client connect logic
      //   BroadcastPacket( packet );
      //   Events::Dispatch< Events::NetworkClientConnectEvent >( packet.m_sourceID );
      //   break;
      case NetworkCode::ClientDisconnect:
         // Optionally handle client disconnect logic
         break;
      case NetworkCode::Chat:
         // Broadcast chat to all except sender
         BroadcastPacket( packet, packet.m_sourceID );
         Events::Dispatch< Events::NetworkChatReceivedEvent >( packet.m_sourceID, Packet::ParseBuffer< NetworkCode::Chat >( packet ) );
         break;
      case NetworkCode::PositionUpdate:
         // Broadcast position update to all except sender
         BroadcastPacket( packet, packet.m_sourceID );
         // Dispatch event for main thread to update world
         Events::Dispatch< Events::NetworkPositionUpdateEvent >( packet.m_sourceID, Packet::ParseBuffer< NetworkCode::PositionUpdate >( packet ) );
         break;

      default: std::println( "Error: Unhandled packet code {}", static_cast< int >( packet.m_code ) ); break;
   }
}

void NetworkHost::SendPacket( const Packet& packet )
{
   if( packet.m_destinationID == 0 )
   {
      BroadcastPacket( packet );
      return;
   }

   EnqueueOutgoing( packet.m_destinationID, packet );
}

void NetworkHost::SendPackets( const std::vector< Packet >& packets )
{
   for( const auto& packet : packets )
      SendPacket( packet );
}

void NetworkHost::BroadcastPacket( const Packet& packet, uint64_t exceptID )
{
   std::lock_guard< std::mutex > lock( m_clientsMutex );
   for( const auto& [ clientID, _ ] : m_clientSockets )
   {
      if( clientID == exceptID )
         continue;

      EnqueueOutgoing( clientID, packet );
   }
}

void NetworkHost::EnqueueOutgoing( uint64_t clientID, const Packet& packet )
{
   std::lock_guard< std::mutex > lock( m_queueMutexes[ clientID ] );
   m_outgoingQueues[ clientID ].push( packet );
   m_queueCV.notify_all();
}

bool NetworkHost::DequeueOutgoing( uint64_t clientID, Packet& packet )
{
   std::lock_guard< std::mutex > lock( m_queueMutexes[ clientID ] );
   if( m_outgoingQueues[ clientID ].empty() )
      return false;

   packet = m_outgoingQueues[ clientID ].front();
   m_outgoingQueues[ clientID ].pop();
   return true;
}

void NetworkHost::DisconnectClient( uint64_t clientID )
{
   std::lock_guard< std::mutex > lock( m_clientsMutex ); // lock
   auto                          it = m_clientSockets.find( clientID );
   if( it == m_clientSockets.end() )
      return; // Client not found

   closesocket( it->second );
   m_clientSockets.erase( it );
   m_outgoingQueues.erase( clientID );
   m_queueMutexes.erase( clientID );
   auto threadIt = m_clientThreads.find( clientID );
   if( threadIt != m_clientThreads.end() )
   {
      if( threadIt->second.joinable() )
         threadIt->second.join();

      m_clientThreads.erase( threadIt );
   }
}

void NetworkHost::Shutdown()
{
   // Broadcast shutdown packet to all clients
   BroadcastPacket( Packet::Create< NetworkCode::HostShutdown >( m_ID, 0 /*destID*/, 0 /*data*/ ) );

   m_fRunning = false;
   if( m_acceptThread.joinable() )
      m_acceptThread.join();

   std::lock_guard< std::mutex > lock( m_clientsMutex );
   for( auto& [ clientID, sock ] : m_clientSockets )
      closesocket( sock );

   for( auto& [ clientID, thread ] : m_clientThreads )
   {
      if( thread.joinable() )
         thread.join();
   }

   m_clientSockets.clear();
   m_clientThreads.clear();
   m_outgoingQueues.clear();
   m_queueMutexes.clear();
}

void NetworkHost::Poll( World& world, const glm::vec3& playerPosition )
{
   // No-op: all work is done in threads
   SendPacket( Packet::Create< NetworkCode::PositionUpdate >( m_ID, 0, playerPosition ) );
}

// TODO: Implement ProcessPacket to handle game logic, relay, etc.
// void NetworkHost::ProcessPacket(const Packet& packet, uint64_t clientID, World& world, Player& player) { ... }