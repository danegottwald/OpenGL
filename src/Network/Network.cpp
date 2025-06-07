
#include "Network.h"

// Local dependencies
#include "NetworkClient.h"
#include "NetworkHost.h"
#include "Packet.h"

// Relative dependencies
#include "../Events/NetworkEvent.h"
#include "../Game/Entity/Player.h"
#include "../Game/World.h"
#include "../Game/Core/GUIManager.h"
#include "../Game/Core/Window.h"
#include "../Input/Input.h"

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

   //Events::Dispatch< Events::NetworkShutdownEvent >( GetNetworkType() );
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

/* static */ bool INetwork::FResolvePeerAddress( Client& client )
{
   if( client.m_socket == INVALID_SOCKET )
      return false; // invalid socket on client

   int size = sizeof( Client::m_socketAddressIn );
   if( getpeername( client.m_socket, ( sockaddr* )&client.m_socketAddressIn, &size ) == 0 )
   {
      char ipBuffer[ INET_ADDRSTRLEN ];
      if( inet_ntop( AF_INET, &client.m_socketAddressIn.sin_addr, ipBuffer, sizeof( ipBuffer ) ) == nullptr )
      {
         std::cout << "Failed to get client IP address from socket: " << client.m_socket << std::endl;
         return false;
      }

      client.m_ipAddress = ipBuffer;
      return true;
   }

   std::cout << "Failed to get peer name for socket: " << client.m_socket << std::endl;
   return false;
}

void INetwork::Send( SOCKET targetSocket, const std::vector< uint8_t >& serializedData )
{
   if( send( targetSocket, reinterpret_cast< const char* >( serializedData.data() ), serializedData.size(), 0 ) == SOCKET_ERROR )
      s_logs.push_back( std::format( "Failed to send packet to socket: {}, WSAError: {}", targetSocket, WSAGetLastError() ) );
}

// ===================================================
//      NetworkGUI
// ===================================================
class NetworkGUI final : public IGUIElement
{
public:
   NetworkGUI( std::deque< std::string >& sharedLogs ) :
      m_logs( sharedLogs )
   {
      m_eventSubscriber.Subscribe< Events::NetworkHostDisconnectEvent >( [ this ]( const Events::NetworkHostDisconnectEvent& /*e*/ )
      {
         INetwork::Shutdown();
         m_fConnected = false; // Set connected state to false
      } );

      m_eventSubscriber.Subscribe< Events::NetworkClientConnectEvent >( [ this ]( const Events::NetworkClientConnectEvent& e )
      { m_messages.push_back( std::format( "{} connected.", e.GetClientID() ) ); } );

      m_eventSubscriber.Subscribe< Events::NetworkClientDisconnectEvent >( [ this ]( const Events::NetworkClientDisconnectEvent& e )
      { m_messages.push_back( std::format( "{} disconnected.", e.GetClientID() ) ); } );

      m_eventSubscriber.Subscribe< Events::NetworkChatReceivedEvent >( [ this ]( const Events::NetworkChatReceivedEvent& e )
      { m_messages.push_back( std::format( "{}: {}", e.GetClientID(), e.GetChatMessage() ) ); } );
   }

   ~NetworkGUI() = default;

   void Draw() override
   {
      ImGui::SetNextWindowSize( ImVec2( 400, 170 ), ImGuiCond_FirstUseEver );
      ImGui::SetNextWindowPos( ImVec2( 10, Window::Get().GetWindowData().Height - 10 ), ImGuiCond_Always, ImVec2( 0.0f, 1.0f ) );
      ImGui::Begin( "##Network", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse );
      if( ImGui::BeginTabBar( "##Tabs" ) )
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
                  network.Listen( m_port );
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
                  INetwork::Get()->SendPacket( Packet::Create< NetworkCode::ClientDisconnect >( INetwork::Get()->GetID(), 0 /*destID*/, {} ) );
                  INetwork::Shutdown();
               }
            }

            ImGui::EndTabItem();
         }
         ImGui::EndDisabled();

         if( ImGui::BeginTabItem( "Chat" ) ) // Chat Tab
         {
            ImGui::BeginDisabled( !m_fHosting && !m_fConnected );
            // Chat log area
            if( ImGui::BeginChild( "ChatLog", ImVec2( 0, -ImGui::GetFrameHeightWithSpacing() ), true, ImGuiWindowFlags_HorizontalScrollbar ) )
            {
               for( const std::string& message : m_messages )
                  ImGui::TextWrapped( "%s", message.c_str() );

               // Auto-scroll to the bottom if enabled
               if( m_fChatAutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() )
                  ImGui::SetScrollHereY( 1.0f ); // Auto scroll to bottom

               ImGui::EndChild();
            }

            // Input area
            if( ImGui::InputText( "##ChatInput", m_inputBuffer, sizeof( m_inputBuffer ), ImGuiInputTextFlags_EnterReturnsTrue ) )
            {
               if( strlen( m_inputBuffer ) > 0 )
               {
                  m_messages.push_back( std::format( "You: {}", m_inputBuffer ) );

                  if( INetwork* pNetwork = INetwork::Get() )
                     pNetwork->SendPacket( Packet::Create< NetworkCode::Chat >( pNetwork->GetID(), 0 /*destID*/, m_inputBuffer ) );

                  memset( m_inputBuffer, 0, sizeof( m_inputBuffer ) );
                  ImGui::SetKeyboardFocusHere( -1 ); // keep focus on the input box
               }
            }

            // Auto-scroll toggle
            ImGui::SameLine();
            ImGui::Checkbox( "Auto-scroll", &m_fChatAutoScroll );
            ImGui::EndDisabled();
            ImGui::EndTabItem();
         }

         if( ImGui::BeginTabItem( "Log" ) ) // Log Tab
         {
            if( ImGui::BeginChild( "LogArea", ImVec2( 0, -ImGui::GetFrameHeightWithSpacing() ), true ) )
            {
               for( const std::string& log : m_logs )
                  ImGui::TextWrapped( "%s", log.c_str() );

               if( m_fLogAutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() )
                  ImGui::SetScrollHereY( 1.0f );

               ImGui::EndChild();
            }

            if( ImGui::Button( "Clear" ) )
               m_logs.clear();

            ImGui::SameLine();
            ImGui::Checkbox( "Auto-scroll", &m_fLogAutoScroll );
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

   // Chat Related
   std::deque< std::string > m_messages;
   char                      m_inputBuffer[ 256 ] = "";
   bool                      m_fChatAutoScroll { true };

   // Logging Related
   std::deque< std::string >& m_logs;
   bool                       m_fLogAutoScroll { true };

   // Events
   Events::EventSubscriber m_eventSubscriber;
};

/*static*/ void INetwork::RegisterGUI()
{
   GUIManager::Attach( std::make_shared< NetworkGUI >( s_logs ) );
}
