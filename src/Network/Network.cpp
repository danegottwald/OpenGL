
#include "Network.h"

// Local dependencies
#include "Packet.h"

// Relative dependencies
#include "../Events/NetworkEvent.h"
#include "../Game/Entity/Player.h"
#include "../Game/World.h"
#include "../Game/Core/GUIManager.h"
#include "../Input/Input.h"

// External dependencies
#include <ws2tcpip.h>
#include <winsock2.h>
#pragma comment( lib, "ws2_32.lib" )

// TODO
//    need to define schema/design for every packet type
//       EX: Relay packet is { uint8_t code, uint64_t clientID, uint16_t dataLength, uint8_t data[] }
//          OR the clientID is contained inside the data, we just prepend it

// every packet should have a clientid!!!!!!!!!!


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
   if( m_socket != INVALID_SOCKET )
      closesocket( m_socket );

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
   if( s_address[ 0 ] != '\0' ) // already set
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

// ===================================================
//      NetworkHost
// ===================================================
NetworkHost::NetworkHost()
{
   //Packet                 packet = PacketMaker< NetworkCode::Chat >( 0, "Hello World this is a long string of text!" );
   Packet                 packet = PacketMaker< NetworkCode::Chat >( 0, { 1, 1, 1, 1 } );
   std::vector< uint8_t > buffer = Packet::Serialize( packet );

   Packet deserializedPacket = Packet::Deserialize( buffer );
   //std::string packetData         = PacketParseBuffer< NetworkCode::Chat >( packet );
   std::string packetData = PacketParseBuffer< NetworkCode::Chat >( packet );

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
   // Close all outstanding client sockets, dispatch disconnect events
   for( auto& [ _, client ] : m_clients )
   {
      closesocket( client.m_socket );
      Events::Dispatch< Events::NetworkClientDisconnectEvent >( client.m_socket );
   }
}

void NetworkHost::Listen()
{
   u_long mode = 1; // non-blocking mode
   m_socket    = socket( AF_INET, SOCK_STREAM /*tcp*/, 0 );
   ioctlsocket( m_socket, FIONBIO, &mode );

   m_socketAddressIn.sin_family      = AF_INET;
   m_socketAddressIn.sin_addr.s_addr = INADDR_ANY;
   m_socketAddressIn.sin_port        = htons( m_port ); //htons( DEFAULT_PORT );

   bind( m_socket, ( struct sockaddr* )&m_socketAddressIn, sizeof( m_socketAddressIn ) );

   // check if returns SOCKET_ERROR, handle
   listen( m_socket, SOMAXCONN );
}

void NetworkHost::ConnectClient( SOCKET clientSocket, const sockaddr_in& clientAddr )
{
   char ipAddress[ INET_ADDRSTRLEN ];
   inet_ntop( AF_INET, &clientAddr.sin_addr, ipAddress, sizeof( ipAddress ) );
   // Handle connections through this event
   Events::Dispatch< Events::NetworkClientConnectEvent >( clientSocket );
}

void NetworkHost::Poll( World& world, Player& player )
{
   FD_ZERO( &m_readfds );
   FD_SET( m_socket, &m_readfds );
   timeval timeout = { 0, 0 };
   if( select( 0, &m_readfds, nullptr, nullptr, &timeout ) > 0 && FD_ISSET( m_socket, &m_readfds ) )
   {
      ClientConnection newClient;
      newClient.m_socket = accept( m_socket, nullptr, nullptr );
      if( newClient.m_socket == INVALID_SOCKET )
         return;

      u_long mode = 1;
      ioctlsocket( newClient.m_socket, FIONBIO, &mode );

      int size = sizeof( ClientConnection::m_socketAddressIn );
      if( getpeername( newClient.m_socket, ( sockaddr* )&newClient.m_socketAddressIn, &size ) == 0 )
      {
         char ipBuffer[ INET_ADDRSTRLEN ];
         if( inet_ntop( AF_INET, &newClient.m_socketAddressIn.sin_addr, ipBuffer, sizeof( ipBuffer ) ) == nullptr )
            throw std::runtime_error( "inet_ntop error" );

         newClient.m_ipAddress = ipBuffer;
      }

      s_logs.push_back( std::format( "New client connected: {}:{}", newClient.m_ipAddress, ntohs( newClient.m_socketAddressIn.sin_port ) ) );

      // Inform other clients about the new client
      m_clients[ newClient.m_socket ] = newClient; // keep here and handle in loop, change socket to server socket
      for( const auto& [ _, otherClient ] : m_clients )
      {
         // Inform other clients about the new client
         if( otherClient.m_socket != newClient.m_socket )
         {
            Packet                 outPacketToOthers = PacketMaker< NetworkCode::ClientConnect >( otherClient.m_socket, newClient.m_socket );
            std::vector< uint8_t > outBufferToOthers = Packet::Serialize( outPacketToOthers );
            send( otherClient.m_socket, reinterpret_cast< char* >( outBufferToOthers.data() ), outBufferToOthers.size(), 0 );
         }

         // Inform the new client about all other clients
         if( otherClient.m_socket != newClient.m_socket )
         {
            Packet                 outPacketToNewClient = PacketMaker< NetworkCode::ClientConnect >( newClient.m_socket, otherClient.m_socket );
            std::vector< uint8_t > outBufferToNewClient = Packet::Serialize( outPacketToNewClient );
            send( newClient.m_socket, reinterpret_cast< char* >( outBufferToNewClient.data() ), outBufferToNewClient.size(), 0 );
         }
      }

      Events::Dispatch< Events::NetworkClientConnectEvent >( newClient.m_socket );
   }

   // Prepare readfds for all clients
   FD_ZERO( &m_readfds );
   for( const auto& [ _, client ] : m_clients )
      FD_SET( client.m_socket, &m_readfds );

   timeout = { 0, 0 };
   if( select( 0, &m_readfds, nullptr, nullptr, &timeout ) > 0 ) // check if there's data to read
   {
      std::vector< SOCKET > disconnectSockets;
      for( auto& [ _, client ] : m_clients )
      {
         if( !FD_ISSET( client.m_socket, &m_readfds ) )
            continue; // client has no data to read

         std::vector< uint8_t > buffer( PACKET_BUFFER_SIZE );
         int                    bytesReceived = recv( client.m_socket, reinterpret_cast< char* >( buffer.data() ), buffer.size(), 0 );
         if( bytesReceived > 0 )
         {
            //   OnReceive( buffer.data(), bytes ); // Handle received data
            // Append received data to the client's persistent buffer
            client.m_receiveBuffer.insert( client.m_receiveBuffer.end(), buffer.begin(), buffer.begin() + bytesReceived );

            size_t offset = 0;
            while( offset < client.m_receiveBuffer.size() )
            {
               if( client.m_receiveBuffer.size() - offset < Packet::HeaderSize() )
                  break; // not enough data for a complete packet header

               // Peek at the fixed fields to determine the total packet size
               uint16_t bufferLength;
               std::memcpy( &bufferLength, client.m_receiveBuffer.data() + offset + sizeof( uint16_t ) + sizeof( NetworkCode ), sizeof( uint16_t ) );
               size_t totalPacketSize = Packet::HeaderSize() + bufferLength;
               if( client.m_receiveBuffer.size() - offset < totalPacketSize ) // Check if we have the full packet
                  break;                                                      // Wait for more data

               Packet inPacket = Packet::Deserialize( buffer, offset );
               offset += inPacket.Size();

               switch( inPacket.m_code )
               {
                  case NetworkCode::Chat: break;
                  case NetworkCode::PositionUpdate:
                  {
                     glm::vec3 receivedVec3;
                     std::memcpy( &receivedVec3, inPacket.m_buffer.data(), sizeof( glm::vec3 ) );
                     world.ReceivePlayerPosition( client.m_socket, receivedVec3 );
                     break;
                  }
                  case NetworkCode::ClientDisconnect: break;
                  default:                            std::cout << "Unhandled network code" << std::endl;
               }

               // only support relaying position updates for now, TODO CHANGE
               if( inPacket.m_code == NetworkCode::PositionUpdate )
               {
                  inPacket.m_clientID = client.m_socket; // set the clientID to the sender's socket

                  std::vector< uint8_t > relayBuffer = Packet::Serialize( inPacket );
                  for( const auto& [ _, otherClient ] : m_clients )
                  {
                     if( otherClient.m_socket == client.m_socket )
                        continue; // don't send to self

                     send( otherClient.m_socket, reinterpret_cast< char* >( relayBuffer.data() ), relayBuffer.size(), 0 );
                  }
               }
            }

            // Remove processed data from the buffer
            client.m_receiveBuffer.erase( client.m_receiveBuffer.begin(), client.m_receiveBuffer.begin() + offset );
         }
         else if( bytesReceived <= 0 )
         {
            int lastError = WSAGetLastError();
            switch( lastError )
            {
               case WSAEWOULDBLOCK: break; // No data available, this is expected for non-blocking sockets

               case WSAENOTCONN:     // Socket is not connected
               case WSAESHUTDOWN:    // Socket has been shut down
               case WSAECONNRESET:   // Connection reset by peer
               case WSAECONNABORTED: // Connection aborted
                  s_logs.push_back( std::format( "Client disconnected: {}:{}", client.m_ipAddress, ntohs( client.m_socketAddressIn.sin_port ) ) );
                  disconnectSockets.push_back( client.m_socket );
                  closesocket( client.m_socket );
                  Events::Dispatch< Events::NetworkClientDisconnectEvent >( client.m_socket );
                  continue;

               default: std::cerr << "RECV Error on socket " << client.m_socket << ": " << lastError << std::endl; break;
            }
         }

         Packet                 positionUpdatePacket = PacketMaker< NetworkCode::PositionUpdate >( client.m_socket, player.GetPosition() );
         std::vector< uint8_t > outBuffer            = Packet::Serialize( positionUpdatePacket );
         send( client.m_socket, reinterpret_cast< char* >( outBuffer.data() ), outBuffer.size(), 0 );
      }

      // Cleanup disconnected clients
      for( SOCKET s : disconnectSockets )
         m_clients.erase( s );
   }
}

// ====================================================
//      NetworkClient
// ====================================================
NetworkClient::NetworkClient()
{
   u_long mode = 1; // non-blocking mode
   m_socket    = socket( AF_INET, SOCK_STREAM, 0 );
   ioctlsocket( m_socket, FIONBIO, &mode );

   m_socketAddressIn.sin_family      = AF_INET;
   m_socketAddressIn.sin_addr.s_addr = inet_addr( DEFAULT_ADDRESS );
   m_socketAddressIn.sin_port        = htons( DEFAULT_PORT );

   //connect( m_socket, ( struct sockaddr* )&m_socketAddressIn, sizeof( m_socketAddressIn ) );
}

NetworkClient::~NetworkClient()
{
   sockaddr_in clientAddr {};
   char        ipAddress[ INET_ADDRSTRLEN ];
   int         addrLen = sizeof( clientAddr );
   if( getpeername( m_socket, ( sockaddr* )&clientAddr, &addrLen ) == 0 )
      inet_ntop( AF_INET, &clientAddr.sin_addr, ipAddress, sizeof( ipAddress ) );

   Events::Dispatch< Events::NetworkClientDisconnectEvent >( m_socket );
}

void NetworkClient::Connect( const char* ipAddress, uint16_t port )
{
   m_socketAddressIn.sin_addr.s_addr = inet_addr( ipAddress );
   m_socketAddressIn.sin_port        = htons( port );
   if( connect( m_socket, ( struct sockaddr* )&m_socketAddressIn, sizeof( m_socketAddressIn ) ) == SOCKET_ERROR )
   {
      if( WSAGetLastError() != WSAEWOULDBLOCK )
         std::cerr << "connect error: " << WSAGetLastError() << std::endl;
   }
}

void NetworkClient::Poll( World& world, Player& player )
{
   if( m_socket == INVALID_SOCKET )
      return;

   FD_ZERO( &m_writefds );
   FD_ZERO( &m_readfds );
   FD_SET( m_socket, &m_writefds );
   FD_SET( m_socket, &m_readfds );

   struct timeval timeout = { 0, 0 };
   select( 0, &m_readfds, &m_writefds, nullptr, &timeout );
   if( FD_ISSET( m_socket, &m_writefds ) )
   {
      Packet                 outPacket = PacketMaker< NetworkCode::PositionUpdate >( m_socket, player.GetPosition() );
      std::vector< uint8_t > outBuffer = Packet::Serialize( outPacket );
      send( m_socket, reinterpret_cast< char* >( outBuffer.data() ), outBuffer.size(), 0 );
   }

   if( FD_ISSET( m_socket, &m_readfds ) )
   {
      std::vector< uint8_t > buffer( PACKET_BUFFER_SIZE );
      int                    bytesReceived = recv( m_socket, reinterpret_cast< char* >( buffer.data() ), buffer.size(), 0 );
      if( bytesReceived > 0 )
      {
         size_t offset = 0;
         while( offset < bytesReceived )
         {
            Packet inPacket = Packet::Deserialize( buffer, offset );
            offset += inPacket.Size();

            switch( inPacket.m_code )
            {
               case NetworkCode::ClientConnect: m_relaySockets.push_back( inPacket.m_clientID ); break;

               case NetworkCode::Chat: break;
               case NetworkCode::PositionUpdate:
               {
                  if( std::find( m_relaySockets.begin(), m_relaySockets.end(), inPacket.m_clientID ) == m_relaySockets.end() )
                     continue; // bad bad bad, we should always have the clientID if we are processing a packet from it

                  const glm::vec3 pos = PacketParseBuffer< NetworkCode::PositionUpdate >( inPacket );
                  world.ReceivePlayerPosition( inPacket.m_clientID, pos );
                  break;
               }
               case NetworkCode::ClientDisconnect: break;

               default: std::cout << "Unhandled network code" << std::endl;
            }
         }
      }
      else if( bytesReceived <= 0 )
      {
         int lastError = WSAGetLastError();
         switch( lastError )
         {
            case WSAEWOULDBLOCK: break; // No data available, this is expected for non-blocking sockets

            case WSAENOTCONN:     // Socket is not connected
            case WSAESHUTDOWN:    // Socket has been shut down
            case WSAECONNRESET:   // Connection reset by peer
            case WSAECONNABORTED: // Connection aborted
               s_logs.push_back( std::format( "Server closed: {}:{}", INetwork::GetHostAddress(), ntohs( m_socketAddressIn.sin_port ) ) );
               closesocket( m_socket );
               m_socket = INVALID_SOCKET;
               Events::Dispatch< Events::NetworkClientDisconnectEvent >( m_socket );
               break;

            default: std::cerr << "RECV Error on socket " << m_socket << ": " << lastError << std::endl; break;
         }
      }
   }
}

// ===================================================
//      NetworkGUI
// ===================================================
class NetworkGUI final : public IGUIElement
{
public:
   NetworkGUI( std::deque< std::string >& sharedLogs ) :
      m_logs( sharedLogs )
   {}
   ~NetworkGUI() = default;

   void Draw() override
   {
      ImGui::SetNextWindowSize( ImVec2( 400, 120 ), ImGuiCond_FirstUseEver );
      ImGui::SetNextWindowPos( ImVec2( ImGui::GetIO().DisplaySize.x - 10, ImGui::GetIO().DisplaySize.y - 10 ), ImGuiCond_Always, ImVec2( 1.0f, 1.0f ) );
      ImGui::Begin( "Network", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse );
      if( ImGui::BeginTabBar( "Tabs" ) )
      {
         if( ImGui::BeginDisabled( m_fConnected ), ImGui::BeginTabItem( "Host" ) ) // Host Tab
         {
            ImGui::Text( "IP: %s", INetwork::GetHostAddress() );

            ImGui::Text( "Port:" );
            ImGui::SameLine();
            ImGui::BeginDisabled( m_fHosting );
            ImGui::InputScalar( "##Port", ImGuiDataType_U16, &m_port, nullptr, nullptr, "%u", m_fHosting ? ImGuiInputTextFlags_ReadOnly : 0 );
            ImGui::EndDisabled();

            if( ImGui::Button( m_fHosting ? "Stop" : "Host" ) )
            {
               m_fHosting = !m_fHosting; // Toggle hosting state
               if( m_fHosting )
               {
                  m_logs.push_back( std::format( "Hosting on: {}:{}", INetwork::GetHostAddress(), m_port ) );
                  NetworkHost& network = INetwork::Create< NetworkHost >(); // Create a new network host instance
                  network.SetListenPort( m_port );
                  network.Listen();
               }
               else
               {
                  m_logs.push_back( "Stopping host..." );
                  INetwork::Shutdown();
               }
            }

            ImGui::EndTabItem();
         }
         ImGui::EndDisabled();

         if( ImGui::BeginDisabled( m_fHosting ), ImGui::BeginTabItem( "Connect" ) ) // Connect Tab
         {
            ImGui::Text( "IP:" );
            ImGui::SameLine();
            ImGui::BeginDisabled( m_fConnected );
            ImGui::InputText( "##IP", m_connectAddress, sizeof( m_connectAddress ), m_fConnected ? ImGuiInputTextFlags_ReadOnly : 0 );
            ImGui::EndDisabled();

            ImGui::Text( "Port:" );
            ImGui::SameLine();
            ImGui::BeginDisabled( m_fConnected );
            ImGui::InputScalar( "##Port", ImGuiDataType_U16, &m_port, nullptr, nullptr, "%u", m_fConnected ? ImGuiInputTextFlags_ReadOnly : 0 );
            ImGui::EndDisabled();

            if( ImGui::Button( m_fConnected ? "Disconnect" : "Connect" ) )
            {
               m_fConnected = !m_fConnected; // Toggle connection state
               if( m_fConnected )
               {
                  m_logs.push_back( std::format( "Connecting to: {}:{}", m_connectAddress, m_port ) );
                  NetworkClient& network = INetwork::Create< NetworkClient >();
                  network.Connect( m_connectAddress, m_port );
               }
               else
               {
                  m_logs.push_back( std::format( "Disconnecting from: {}:{}", m_connectAddress, m_port ) );
                  INetwork::Shutdown();
               }
            }

            ImGui::EndTabItem();
         }
         ImGui::EndDisabled();

         if( ImGui::BeginTabItem( "Log" ) ) // Log Tab
         {
            if( ImGui::BeginChild( "LogArea", ImVec2( 0, -ImGui::GetFrameHeightWithSpacing() ), true ) )
            {
               for( const std::string& log : m_logs )
                  ImGui::TextWrapped( "%s", log.c_str() );

               if( m_fAutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() )
                  ImGui::SetScrollHereY( 1.0f );

               ImGui::EndChild();
            }

            if( ImGui::Button( "Clear" ) )
               m_logs.clear();

            ImGui::SameLine();
            ImGui::Checkbox( "Auto-scroll", &m_fAutoScroll );
            ImGui::EndTabItem();
         }

         ImGui::EndTabBar();
      }
      ImGui::End();
   }

private:
   char     m_connectAddress[ INET_ADDRSTRLEN ] { DEFAULT_ADDRESS };
   uint16_t m_port { DEFAULT_PORT };

   bool m_fConnected { false };
   bool m_fHosting { false };

   // Logging Related
   std::deque< std::string >& m_logs;
   bool                       m_fAutoScroll { true };
};

/*static*/ void INetwork::RegisterGUI()
{
   GUIManager::Attach( std::make_shared< NetworkGUI >( s_logs ) );
}
