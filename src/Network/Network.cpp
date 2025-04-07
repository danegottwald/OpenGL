
#include "Network.h"

#include <ws2tcpip.h>
#include <winsock2.h>
#pragma comment( lib, "ws2_32.lib" )

#include "../Events/NetworkEvent.h"
#include "../Game/Entity/Player.h"
#include "../Game/World.h"
#include "../Game/Core/GUIManager.h"
#include "../Input/Input.h"

// TODO
//    need to define schema/design for every packet type
//       EX: Relay packet is { uint8_t code, uint64_t clientID, uint16_t dataLength, uint8_t data[] }
//          OR the clientID is contained inside the data, we just prepend it

// every packet should have a clientid!!!!!!!!!!

enum class NetworkCode : uint8_t
{
   // Control Messages
   Invalid    = 0x00, // Invalid packet
   Handshake  = 0x01,
   Relay      = 0x02, // Relay message to other clients
   Heartbeat  = 0x03,
   NewClient  = 0x04,
   Disconnect = 0x05,

   // Game Messages
   Chat           = 0x20,
   PositionUpdate = 0x21,
};

struct Packet
{
   NetworkCode                 m_code { NetworkCode::Invalid };
   uint16_t                    m_bufferLength {};
   std::array< uint8_t, 1024 > m_buffer;

   size_t Size() const noexcept { return sizeof( m_code ) + sizeof( m_bufferLength ) + m_bufferLength; }

   std::vector< uint8_t > Serialize() const
   {
      std::vector< uint8_t > data( sizeof( m_code ) + sizeof( m_bufferLength ) + m_bufferLength );
      std::memcpy( data.data(), &m_code, sizeof( m_code ) );
      std::memcpy( data.data() + sizeof( m_code ), &m_bufferLength, sizeof( m_bufferLength ) );
      std::memcpy( data.data() + sizeof( m_code ) + sizeof( m_bufferLength ), m_buffer.data(), m_bufferLength );
      return data;
   }

   //static Packet Deserialize( const std::vector< uint8_t >& data )
   //{
   //   Packet packet;
   //   std::memcpy( &packet.m_code, data.data(), sizeof( packet.m_code ) );
   //   std::memcpy( &packet.m_bufferLength, data.data() + sizeof( packet.m_code ), sizeof( packet.m_bufferLength ) );
   //   std::memcpy( packet.m_buffer.data(), data.data() + sizeof( packet.m_code ) + sizeof( packet.m_bufferLength ), packet.m_bufferLength );
   //   return packet;
   //}

   static Packet Deserialize( const std::vector< uint8_t >& data, size_t offset )
   {
      Packet packet;
      std::memcpy( &packet.m_code, data.data() + offset, sizeof( packet.m_code ) );
      std::memcpy( &packet.m_bufferLength, data.data() + sizeof( packet.m_code ) + offset, sizeof( packet.m_bufferLength ) );
      std::memcpy( packet.m_buffer.data(), data.data() + sizeof( packet.m_code ) + sizeof( packet.m_bufferLength ) + offset, packet.m_bufferLength );
      return packet;
   }
};

// ===================================================
//      NetworkHost
// ===================================================
NetworkHost::NetworkHost()
{
   WSADATA wsaData;
   WSAStartup( MAKEWORD( 2, 2 ), &wsaData );

   u_long mode = 1; // non-blocking mode
   m_socket    = socket( AF_INET, SOCK_STREAM /*tcp*/, 0 );
   ioctlsocket( m_socket, FIONBIO, &mode );

   m_socketAddressIn.sin_family      = AF_INET;
   m_socketAddressIn.sin_addr.s_addr = INADDR_ANY;
   m_socketAddressIn.sin_port        = htons( DEFAULT_PORT );
   m_port                            = m_socketAddressIn.sin_port;

   bind( m_socket, ( struct sockaddr* )&m_socketAddressIn, sizeof( m_socketAddressIn ) );
   //listen( m_socket, SOMAXCONN );
   listen( m_socket, SOMAXCONN );

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

   closesocket( m_socket ); // close server socket last
   WSACleanup();
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

      // Inform other clients about the new client
      m_clients[ newClient.m_socket ] = newClient; // keep here and handle in loop, change socket to server socket
      for( const auto& [ _, otherClient ] : m_clients )
      {
         Packet outPacket;
         outPacket.m_code         = NetworkCode::NewClient;
         outPacket.m_bufferLength = sizeof( otherClient.m_socket );

         if( otherClient.m_socket == newClient.m_socket )
         {
            // just send this for now since its stable
            uint64_t stableID = 0;
            std::memcpy( outPacket.m_buffer.data(), &stableID, sizeof( stableID ) ); // send this server socket to the new client
         }
         else
            std::memcpy( outPacket.m_buffer.data(), &newClient.m_socket, sizeof( newClient.m_socket ) );

         std::vector< uint8_t > outBuffer = outPacket.Serialize();
         send( otherClient.m_socket, reinterpret_cast< char* >( outBuffer.data() ), outBuffer.size(), 0 );
      }

      // Inform the new client about all other clients
      for( const auto& [ _, otherClient ] : m_clients )
      {
         if( otherClient.m_socket == newClient.m_socket )
            continue; // don't send to self

         Packet outPacket;
         outPacket.m_code         = NetworkCode::NewClient;
         outPacket.m_bufferLength = sizeof( otherClient.m_socket );
         std::memcpy( outPacket.m_buffer.data(), &otherClient.m_socket, sizeof( otherClient.m_socket ) );
         std::vector< uint8_t > outBuffer = outPacket.Serialize();
         send( newClient.m_socket, reinterpret_cast< char* >( outBuffer.data() ), outBuffer.size(), 0 );
      }

      // Inform the new client about the server
      //Packet outPacket;
      //outPacket.m_code = NetworkCode::NewClient;
      //outPacket.m_bufferLength = sizeof( m_socket );
      //std::memcpy( outPacket.m_buffer.data(), &m_socket, sizeof( m_socket ) );
      //std::vector< uint8_t > outBuffer = outPacket.Serialize();
      //send( newClient.m_socket, reinterpret_cast< char* >( outBuffer.data() ), outBuffer.size(), 0 );

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

         std::vector< uint8_t > buffer( 1024 );
         int                    bytesReceived = recv( client.m_socket, reinterpret_cast< char* >( buffer.data() ), buffer.size(), 0 );
         if( bytesReceived > 0 )
         {
            size_t offset = 0;
            while( offset < bytesReceived )
            {
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
                  case NetworkCode::Disconnect: break;
                  case NetworkCode::Relay:      break; // a client should never send a relay message, only receive
                  default:                      std::cout << "Unhandled network code" << std::endl;
               }

               // Relay the packet to other clients
               // we probably want to select which packets we relay (define some list of codes we want to relay)
               Packet relayPacket;
               relayPacket.m_code         = NetworkCode::Relay;
               relayPacket.m_bufferLength = inPacket.m_bufferLength;
               std::memcpy( relayPacket.m_buffer.data(), &client.m_socket, sizeof( SOCKET ) );
               std::memcpy( relayPacket.m_buffer.data() + sizeof( SOCKET ), inPacket.m_buffer.data(), inPacket.m_bufferLength );
               relayPacket.m_bufferLength += sizeof( SOCKET );

               std::vector< uint8_t > relayBuffer = relayPacket.Serialize();
               for( const auto& [ _, otherClient ] : m_clients )
               {
                  if( inPacket.m_code != NetworkCode::PositionUpdate )
                     continue; // only support relaying position updates for now, TODO CHANGE

                  if( otherClient.m_socket != client.m_socket )
                     send( otherClient.m_socket, reinterpret_cast< char* >( relayBuffer.data() ), relayBuffer.size(), 0 );
               }
            }
         }
         else if( bytesReceived == 0 || WSAGetLastError() == WSAECONNRESET || WSAGetLastError() == WSAECONNABORTED )
         {
            std::cout << "Client disconnected." << std::endl;
            disconnectSockets.push_back( client.m_socket );
            closesocket( client.m_socket );
            Events::Dispatch< Events::NetworkClientDisconnectEvent >( client.m_socket );
         }
         else if( WSAGetLastError() != WSAEWOULDBLOCK )
         {
            std::cerr << "recv error: " << WSAGetLastError() << std::endl;
         }

         if( client.m_socket != INVALID_SOCKET )
         {
            Packet outPacket;
            outPacket.m_code         = NetworkCode::PositionUpdate;
            outPacket.m_bufferLength = sizeof( glm::vec3 );
            std::memcpy( outPacket.m_buffer.data(), &player.GetPosition(), sizeof( glm::vec3 ) );

            std::vector< uint8_t > outBuffer = outPacket.Serialize();
            send( client.m_socket, reinterpret_cast< char* >( outBuffer.data() ), outBuffer.size(), 0 );
         }
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
   WSADATA wsaData;
   WSAStartup( MAKEWORD( 2, 2 ), &wsaData );

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
   sockaddr_in clientAddr;
   char        ipAddress[ INET_ADDRSTRLEN ];
   int         addrLen = sizeof( clientAddr );
   if( getpeername( m_socket, ( sockaddr* )&clientAddr, &addrLen ) == 0 )
      inet_ntop( AF_INET, &clientAddr.sin_addr, ipAddress, sizeof( ipAddress ) );

   Events::Dispatch< Events::NetworkClientDisconnectEvent >( m_socket );

   closesocket( m_socket );
   WSACleanup();
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
      Packet outPacket;
      outPacket.m_code         = NetworkCode::PositionUpdate;
      outPacket.m_bufferLength = sizeof( glm::vec3 );
      std::memcpy( outPacket.m_buffer.data(), &player.GetPosition(), sizeof( glm::vec3 ) );

      std::vector< uint8_t > outBuffer = outPacket.Serialize();
      send( m_socket, reinterpret_cast< char* >( outBuffer.data() ), outBuffer.size(), 0 );
   }

   if( FD_ISSET( m_socket, &m_readfds ) )
   {
      std::vector< uint8_t > buffer( 1024 );
      int                    bytesReceived = recv( m_socket, reinterpret_cast< char* >( buffer.data() ), buffer.size(), 0 );
      if( bytesReceived > 0 )
      {
         sockaddr_in clientAddr;
         char        ipAddress[ INET_ADDRSTRLEN ];
         int         addrLen = sizeof( clientAddr );
         if( getpeername( m_socket, ( sockaddr* )&clientAddr, &addrLen ) == 0 )
            inet_ntop( AF_INET, &clientAddr.sin_addr, ipAddress, sizeof( ipAddress ) );

         if( m_fFirstPoll )
         {
            // Handle connections through this event
            //Events::Dispatch< Events::NetworkClientConnectEvent >( m_socket );
            m_fFirstPoll = false;
         }

         size_t offset = 0;
         while( offset < bytesReceived )
         {
            Packet inPacket = Packet::Deserialize( buffer, offset );
            offset += inPacket.Size();
            switch( inPacket.m_code )
            {
               case NetworkCode::NewClient:
               {
                  SOCKET clientSocket;
                  std::memcpy( &clientSocket, inPacket.m_buffer.data(), sizeof( SOCKET ) );
                  Events::Dispatch< Events::NetworkClientConnectEvent >( clientSocket );
                  if( clientSocket != 0 )
                     m_relaySockets.push_back( clientSocket );
                  break;
               }
               case NetworkCode::Chat: break;
               case NetworkCode::PositionUpdate:
               {
                  glm::vec3 receivedVec3;
                  std::memcpy( &receivedVec3, inPacket.m_buffer.data(), sizeof( glm::vec3 ) );
                  //world.ReceivePlayerPosition( m_socket, receivedVec3 );
                  world.ReceivePlayerPosition( 0 /*m_socket*/, receivedVec3 ); // use host id of 0 for now
                  break;
               }
               case NetworkCode::Disconnect: break;
               case NetworkCode::Relay:
               {
                  // TODO - WE NEED TO RELAY THE OP CODE AS WELL
                  uint64_t clientID;
                  std::memcpy( &clientID, inPacket.m_buffer.data(), sizeof( SOCKET ) );
                  if( std::find( m_relaySockets.begin(), m_relaySockets.end(), clientID ) == m_relaySockets.end() )
                     continue; // force connect client for now, bad bad bad

                  //std::memcpy( &inPacket.m_buffer[ sizeof( SOCKET ) ], inPacket.m_buffer.data(), inPacket.m_bufferLength - sizeof( SOCKET ) );
                  glm::vec3 receivedVec3;
                  std::memcpy( &receivedVec3, inPacket.m_buffer.data() + sizeof( SOCKET ), sizeof( glm::vec3 ) );
                  world.ReceivePlayerPosition( clientID, receivedVec3 );
                  break;
               }
               default: std::cout << "Unhandled network code" << std::endl;
            }
         }
      }
      else if( bytesReceived == 0 /*graceful disconnect*/ || WSAGetLastError() == WSAECONNRESET /*forceful disconnect*/ ||
               WSAGetLastError() == WSAECONNABORTED /*socket was killed*/ || WSAGetLastError() == WSAENOTSOCK /*socket not valid*/ )
      {
         std::cout << "Server disconnected." << std::endl;

         sockaddr_in clientAddr;
         char        ipAddress[ INET_ADDRSTRLEN ];
         int         addrLen = sizeof( clientAddr );
         if( getpeername( m_socket, ( sockaddr* )&clientAddr, &addrLen ) == 0 )
            inet_ntop( AF_INET, &clientAddr.sin_addr, ipAddress, sizeof( ipAddress ) );

         // Handle disconnects through this event
         Events::Dispatch< Events::NetworkClientDisconnectEvent >( 0 /*m_socket*/ ); // use host id of 0 for now

         closesocket( m_socket );
         m_socket = INVALID_SOCKET;
      }
      else if( WSAGetLastError() != WSAEWOULDBLOCK )
         std::cerr << "recv error: " << WSAGetLastError() << std::endl;
   }
}

// ===================================================
//      NetworkGUI
// ===================================================
class NetworkGUI final : public IGUIElement
{
public:
   NetworkGUI()  = default;
   ~NetworkGUI() = default;

   void Draw() override
   {
      static bool                      fHosting         = false;
      static bool                      fConnected       = false;
      static const char*               hostIP           = "127.0.0.1"; // Temporary IP for Host
      static char                      connectIP[ 128 ] = "127.0.0.1"; // Temporary IP for Connect
      static int32_t                   port             = 8080;        // Temporary Port for Connect
      static std::deque< std::string > logs;

      ImGui::SetNextWindowSize( ImVec2( 400, 120 ), ImGuiCond_FirstUseEver );
      ImGui::SetNextWindowPos( ImVec2( ImGui::GetIO().DisplaySize.x - 10, ImGui::GetIO().DisplaySize.y - 10 ), ImGuiCond_Always, ImVec2( 1.0f, 1.0f ) );
      ImGui::Begin( "Network", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse //|
                    /*ImGuiWindowFlags_NoBackground |*/ /*ImGuiWindowFlags_AlwaysAutoResize*/ );
      if( ImGui::BeginTabBar( "Tabs" ) )
      {
         ImGui::BeginDisabled( fConnected );
         if( ImGui::BeginTabItem( "Host" ) ) // Host Tab
         {
            ImGui::Text( "IP: %s", hostIP );
            ImGui::BeginDisabled( fHosting );
            ImGui::Text( "Port:" );
            ImGui::SameLine();
            ImGui::InputInt( "##Port", &port, 0, 0, fHosting ? ImGuiInputTextFlags_ReadOnly : 0 );
            ImGui::EndDisabled();
            if( ImGui::Button( !fHosting ? "Host" : "Stop" ) )
            {
               fHosting = !fHosting; // toggle hosting state
               if( fHosting )
               {
                  NetworkHost& network = INetwork::Create< NetworkHost >(); // Create a new network host instance
                  network.SetListenPort( port );
                  hostIP = network.GetListenAddress().c_str();
                  logs.push_back( "Hosting on: " + std::string( hostIP ) + ":" + std::to_string( port ) );
               }
               else
               {
                  logs.push_back( "Stopping host..." );
                  INetwork::Shutdown();
               }
            }
            ImGui::EndTabItem();
         }
         ImGui::EndDisabled();

         ImGui::BeginDisabled( fHosting );
         if( ImGui::BeginTabItem( "Connect" ) ) // Connect Tab
         {
            ImGui::BeginDisabled( fConnected );
            ImGui::Text( "IP:" );
            ImGui::SameLine();
            ImGui::InputText( "##IP", connectIP, sizeof( connectIP ), fConnected ? ImGuiInputTextFlags_ReadOnly : 0 );
            ImGui::Text( "Port:" );
            ImGui::SameLine();
            ImGui::InputInt( "##Port", &port, 0, 0, fConnected ? ImGuiInputTextFlags_ReadOnly : 0 );
            ImGui::EndDisabled();

            if( ImGui::Button( !fConnected ? "Connect" : "Disconnect" ) )
            {
               fConnected = !fConnected; // toggle connection state
               if( fConnected )
               {
                  NetworkClient& network = INetwork::Create< NetworkClient >(); // Create a new network client instance
                  network.Connect( connectIP, port );
                  logs.push_back( "Connecting to: " + std::string( connectIP ) + ":" + std::to_string( port ) );
               }
               else
               {
                  logs.push_back( "Disconnecting from: " + std::string( connectIP ) + ":" + std::to_string( port ) );
                  INetwork::Shutdown();
               }
            }

            ImGui::EndTabItem();
         }
         ImGui::EndDisabled();

         if( ImGui::BeginTabItem( "Log" ) ) // Log Tab
         {
            for( const std::string& log : logs )
               ImGui::TextWrapped( "%s", log.c_str() );

            if( logs.size() > 10 ) // Keep only the last 10 logs
               logs.pop_front();

            ImGui::EndTabItem();
         }

         ImGui::EndTabBar();
      }
      ImGui::End();
   }
};

/*static*/ void INetwork::RegisterGUI()
{
   GUIManager::Attach( std::make_shared< NetworkGUI >() );
}
