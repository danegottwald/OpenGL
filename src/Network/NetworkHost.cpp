
#include "NetworkHost.h"

#include "../Events/NetworkEvent.h"
#include "../Game/Entity/Player.h"
#include "../Game/World.h"

// ===================================================
//      NetworkHost
// ===================================================
NetworkHost::NetworkHost()
{
   SetID( Client::GenerateClientID() ); // Create a unique ID for the host

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
}

NetworkHost::~NetworkHost()
{
   m_fRunning = false;

   // Close all outstanding client sockets, dispatch disconnect events
   for( const Client& client : m_clients )
   {
      closesocket( client.m_socket );
      Events::Dispatch< Events::NetworkClientDisconnectEvent >( client.m_clientID );
   }

   // need to close the host socket prior to joining the listener threads, this will signal for them to stop
   closesocket( m_hostSocket );

   if( m_clientListenerThread.joinable() )
      m_clientListenerThread.join();

   if( m_dataListenerThread.joinable() )
      m_dataListenerThread.join();
}

void NetworkHost::Listen( uint16_t port )
{
   m_hostSocket = socket( AF_INET, SOCK_STREAM /*tcp*/, 0 );

   //u_long mode = 1; // non-blocking mode
   //ioctlsocket( m_hostSocket, FIONBIO, &mode );

   m_socketAddressIn.sin_family      = AF_INET;
   m_socketAddressIn.sin_addr.s_addr = INADDR_ANY;
   m_socketAddressIn.sin_port        = htons( m_port );

   bind( m_hostSocket, ( struct sockaddr* )&m_socketAddressIn, sizeof( m_socketAddressIn ) );
   listen( m_hostSocket, SOMAXCONN );
}

void NetworkHost::StartThreads( World& world, Player& player )
{
   m_clientListenerThread = std::thread( &NetworkHost::ListenForClients, this );
   m_dataListenerThread   = std::thread( &NetworkHost::ListenForData, this, std::ref( world ), std::ref( player ) );
}

std::optional< Client > NetworkHost::TryAcceptClient( _Inout_ fd_set& readfds )
{
   FD_ZERO( &readfds );
   {
      std::lock_guard< std::mutex > lock( m_clientsMutex );
      FD_SET( m_hostSocket, &readfds );
   }

   if( select( 0, &readfds, nullptr, nullptr, nullptr ) > 0 && FD_ISSET( m_hostSocket, &readfds ) && m_fRunning )
   {
      Client newClient;
      newClient.m_clientID = Client::GenerateClientID();
      newClient.m_socket   = accept( m_hostSocket, nullptr, nullptr );
      if( newClient.m_socket == INVALID_SOCKET )
         return std::nullopt;

      if( !FResolvePeerAddress( newClient ) )
      {
         closesocket( newClient.m_socket );
         return std::nullopt; // failed to resolve client address
      }

      //u_long mode = 1; // non-blocking mode for socket
      //ioctlsocket( newClient.m_socket, FIONBIO, &mode );
      return newClient;
   }

   return std::nullopt;
}

void NetworkHost::ListenForClients()
{
   fd_set readfds {};
   if( std::optional< Client > optNewClient = TryAcceptClient( readfds ) )
   {
      {
         std::lock_guard< std::mutex > lock( m_clientsMutex );
         s_logs.push_back( std::format( "New client connection attempt: {}", optNewClient->m_ipAddress ) );
         m_clientSockets[ optNewClient->m_clientID ] = optNewClient->m_socket;
      }

      // Begin handshake process
      SendPacket( Packet::Create< NetworkCode::Handshake >( m_ID, optNewClient->m_clientID, 0 ) );

      auto cleanupClient = [ this, optNewClient ]()
      {
         std::lock_guard< std::mutex > lock( m_clientsMutex );
         closesocket( optNewClient->m_socket );
         m_clientSockets.erase( optNewClient->m_clientID );
      };

      // Wait for handshake response
      FD_ZERO( &readfds );
      FD_SET( optNewClient->m_socket, &readfds );

      timeval timeout      = { 2, 5000000 }; // 2.5 seconds timeout
      int     selectResult = select( 0, &readfds, nullptr, nullptr, &timeout );
      if( selectResult > 0 )
      {
         std::vector< uint8_t > buffer( PACKET_BUFFER_SIZE );
         int                    bytesReceived = recv( optNewClient->m_socket, reinterpret_cast< char* >( buffer.data() ), buffer.size(), 0 );
         if( bytesReceived > 0 )
         {
            const Packet inPacket = Packet::Deserialize( buffer, 0 /*dataOffset*/ );
            if( inPacket.m_code == NetworkCode::Handshake && inPacket.m_sourceID == optNewClient->m_clientID )
            {
               u_long mode = 0; // non-blocking mode for socket
               ioctlsocket( optNewClient->m_socket, FIONBIO, &mode );

               // Finalize client connection after successful handshake
               s_logs.push_back( std::format( "New client connected: {}", optNewClient->m_ipAddress ) );
               std::vector< Packet > clientConnectPackets;
               {
                  // Notify other clients
                  std::lock_guard< std::mutex > lock( m_clientsMutex );
                  for( const Client& otherClient : m_clients )
                  {
                     clientConnectPackets.push_back( Packet::Create< NetworkCode::ClientConnect >( optNewClient->m_clientID, otherClient.m_clientID, {} ) );
                     clientConnectPackets.push_back( Packet::Create< NetworkCode::ClientConnect >( otherClient.m_clientID, optNewClient->m_clientID, {} ) );
                  }
                  m_clients.push_back( *optNewClient );
               }
               Events::Dispatch< Events::NetworkClientConnectEvent >( optNewClient->m_clientID );
               SendPackets( clientConnectPackets );
            }
            else
            {
               s_logs.push_back( std::format( "Handshake failed: invalid packet code or source ID" ) );
               cleanupClient();
            }
         }
         else
         {
            s_logs.push_back( std::format( "Handshake failed: recv error {}", WSAGetLastError() ) );
            cleanupClient();
         }
      }
      else
      {
         if( selectResult == 0 )
            s_logs.push_back( "Handshake timeout: No response from client." );
         else
            s_logs.push_back( std::format( "Handshake failed: select error: {}, selectResult: {}, readfds: {}",
                                           WSAGetLastError(),
                                           selectResult,
                                           FD_ISSET( optNewClient->m_socket, &readfds ) ? "true" : "false" ) );

         cleanupClient();
      }
   }

   if( m_fRunning )
      ListenForClients(); // Call again to continue listening for clients
}

void NetworkHost::ListenForData( World& world, Player& player )
{
   fd_set readfds {};
   while( m_fRunning )
   {
      {
         FD_ZERO( &readfds );
         std::lock_guard< std::mutex > lock( m_clientsMutex );
         for( Client& client : m_clients )
            FD_SET( client.m_socket, &readfds );
      }

      std::vector< uint64_t > disconnectedClients;
      timeval                 timeout = { 0, 0 }; // non-blocking timeout
      if( select( 0, &readfds, nullptr, nullptr, &timeout ) > 0 )
      {
         std::lock_guard< std::mutex > lock( m_clientsMutex );
         for( Client& client : m_clients )
         {
            if( !FD_ISSET( client.m_socket, &readfds ) )
               continue;

            std::vector< uint8_t > buffer( PACKET_BUFFER_SIZE );
            int                    bytesReceived = recv( client.m_socket, reinterpret_cast< char* >( buffer.data() ), buffer.size(), 0 );
            if( bytesReceived > 0 )
            {
               client.m_receiveBuffer.insert( client.m_receiveBuffer.end(), buffer.begin(), buffer.begin() + bytesReceived );

               size_t offset = 0;
               while( offset < client.m_receiveBuffer.size() )
               {
                  if( client.m_receiveBuffer.size() - offset < Packet::HeaderSize() )
                     break;

                  uint16_t bufferLength;
                  std::memcpy( &bufferLength,
                               client.m_receiveBuffer.data() + offset + sizeof( uint64_t ) + sizeof( uint64_t ) + sizeof( NetworkCode ),
                               sizeof( uint16_t ) );
                  size_t totalPacketSize = Packet::HeaderSize() + bufferLength;
                  if( client.m_receiveBuffer.size() - offset < totalPacketSize )
                     break;

                  Packet inPacket = Packet::Deserialize( client.m_receiveBuffer, offset );
                  offset += inPacket.Size();

                  // Handle packet
                  bool fRelayPacket = false;
                  switch( inPacket.m_code )
                  {
                     case NetworkCode::Chat:
                     {
                        fRelayPacket                  = true;
                        const std::string chatMessage = Packet::ParseBuffer< NetworkCode::Chat >( inPacket );
                        Events::Dispatch< Events::NetworkChatReceivedEvent >( inPacket.m_sourceID, chatMessage );
                        break;
                     }
                     case NetworkCode::PositionUpdate:
                     {
                        fRelayPacket             = true;
                        const glm::vec3 position = Packet::ParseBuffer< NetworkCode::PositionUpdate >( inPacket );
                        // this won't work since we are on background thread
                        //world.ReceivePlayerPosition( client.m_clientID, position );
                        break;
                     }
                     case NetworkCode::ClientDisconnect:
                     {
                        fRelayPacket = true;
                        s_logs.push_back( std::format( "Client disconnected: {}", client.m_ipAddress ) );
                        disconnectedClients.push_back( client.m_clientID );
                        closesocket( client.m_socket );

                        Events::Dispatch< Events::NetworkClientDisconnectEvent >( inPacket.m_sourceID );
                        break;
                     }

                     default: std::cout << "Unhandled network code" << std::endl; break;
                  }

                  if( fRelayPacket )
                  {
                     for( Client& otherClient : m_clients )
                     {
                        if( otherClient.m_clientID == client.m_clientID )
                           continue; // don't send to self

                        inPacket.m_destinationID = otherClient.m_clientID; // update destination ID for the relay packet
                        SendPacket( inPacket );
                     }
                  }
               }

               client.m_receiveBuffer.erase( client.m_receiveBuffer.begin(), client.m_receiveBuffer.begin() + offset );
            }
            else if( bytesReceived <= 0 && WSAGetLastError() != WSAEWOULDBLOCK )
            {
               s_logs.push_back( std::format( "Client disconnected: {}", client.m_ipAddress ) );
               disconnectedClients.push_back( client.m_clientID );
               closesocket( client.m_socket );
               Events::Dispatch< Events::NetworkClientDisconnectEvent >( client.m_clientID );
            }

            // Just send server position updates, move to queue'd system
            if( std::find( disconnectedClients.begin(), disconnectedClients.end(), client.m_clientID ) == disconnectedClients.end() )
               SendPacket( Packet::Create< NetworkCode::PositionUpdate >( m_ID, client.m_clientID, player.GetPosition() ) );
         }
      }

      if( disconnectedClients.size() > 0 )
      {
         std::lock_guard< std::mutex > lock( m_clientsMutex );
         m_clients.erase( std::remove_if( m_clients.begin(),
                                          m_clients.end(),
                                          [ &disconnectedClients ]( const Client& client )
         { return std::find( disconnectedClients.begin(), disconnectedClients.end(), client.m_clientID ) != disconnectedClients.end(); } ),
                          m_clients.end() );
      }
   }
}

void NetworkHost::SendPacket( const Packet& packet )
{
   std::vector< uint8_t > serializedPacket = Packet::Serialize( packet );
   if( packet.m_destinationID == 0 )
   {
      for( const auto& [ clientID, socket ] : m_clientSockets )
      {
         if( clientID == packet.m_sourceID )
            continue; // don't send to self

         Send( socket, serializedPacket );
      }
      return;
   }
   else
   {
      auto it = m_clientSockets.find( packet.m_destinationID );
      if( it == m_clientSockets.end() )
         throw std::runtime_error( "NetworkHost::SendPacket - Client not found" );

      Send( it->second, Packet::Serialize( packet ) );
   }
}

void NetworkHost::SendPackets( const std::vector< Packet >& packets )
{
   // Aggregate packets into a single buffer for each client
   std::unordered_map< uint64_t, std::vector< uint8_t > > clientBuffers;
   for( const Packet& packet : packets )
   {
      if( packet.m_destinationID == 0 )
      {
         // Broadcast to all clients
         std::vector< uint8_t > serializedPacket = Packet::Serialize( packet );
         for( const auto& [ clientID, socket ] : m_clientSockets )
         {
            if( clientID == packet.m_sourceID )
               continue; // don't send to self

            clientBuffers[ clientID ].insert( clientBuffers[ clientID ].end(), serializedPacket.begin(), serializedPacket.end() );
         }
      }
      else
      {
         auto it = m_clientSockets.find( packet.m_destinationID );
         if( it == m_clientSockets.end() )
            throw std::runtime_error( "NetworkHost::SendPackets - Client not found" );

         std::vector< uint8_t > serializedPacket = Packet::Serialize( packet );
         clientBuffers[ packet.m_destinationID ].insert( clientBuffers[ packet.m_destinationID ].end(), serializedPacket.begin(), serializedPacket.end() );
      }
   }

   // Send the aggregated buffer to all clients
   for( auto& [ clientID, buffer ] : clientBuffers )
   {
      if( send( m_clientSockets[ clientID ], reinterpret_cast< char* >( buffer.data() ), buffer.size(), 0 ) == SOCKET_ERROR )
         s_logs.push_back( std::format( "Failed to send packet to client: {}", clientID ) );
   }
}

void NetworkHost::Poll( World& world, Player& player )
{
   if( !m_fRunning )
   {
      m_fRunning = true; // Start the threads
      StartThreads( world, player );
   }

   return;
}