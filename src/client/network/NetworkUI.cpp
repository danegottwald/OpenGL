#include "pch_shared.h"

#include <Engine/Network/Network.h>

#include <Engine/UI/UI.h>
#include <Engine/Platform/Window.h>
#include <Engine/Events/NetworkEvent.h>

#include <Engine/Network/NetworkClient.h>
#include <Engine/Network/NetworkHost.h>

// ===================================================
// NetworkUI (client-only)
// ===================================================
class NetworkUI final : public UI::IDrawable
{
public:
   NetworkUI()
   {
      m_eventSubscriber.Subscribe< Events::NetworkHostShutdownEvent >( [ this ]( const Events::NetworkHostShutdownEvent& /*e*/ ) noexcept
      {
         INetwork::Shutdown();
         m_fConnected = false;
      } );

      m_eventSubscriber.Subscribe< Events::NetworkClientConnectEvent >( [ this ]( const Events::NetworkClientConnectEvent& e ) noexcept
      { m_messages.push_back( std::format( "{} connected.", e.GetClientID() ) ); } );

      m_eventSubscriber.Subscribe< Events::NetworkClientDisconnectEvent >( [ this ]( const Events::NetworkClientDisconnectEvent& e ) noexcept
      { m_messages.push_back( std::format( "{} disconnected.", e.GetClientID() ) ); } );

      m_eventSubscriber.Subscribe< Events::NetworkChatReceivedEvent >( [ this ]( const Events::NetworkChatReceivedEvent& e ) noexcept
      { m_messages.push_back( std::format( "{}: {}", e.GetClientID(), e.GetChatMessage() ) ); } );
   }

   void Draw() override
   {
      ImGui::SetNextWindowSize( ImVec2( 650, 250 ), ImGuiCond_FirstUseEver );
      ImGui::SetNextWindowPos( ImVec2( 10, Window::Get().GetWindowState().size.y - 10 ), ImGuiCond_Always, ImVec2( 0.0f, 1.0f ) );
      ImGui::Begin( "##Network", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse );
      if( ImGui::BeginTabBar( "##Tabs" ) )
      {
         if( ImGui::BeginDisabled( m_fConnected ), ImGui::BeginTabItem( "Host" ) )
         {
            ImGui::Text( "IP: %s", INetwork::GetHostAddress() );

            ImGui::Text( "Port:" );
            ImGui::SameLine();
            ImGui::BeginDisabled( m_fHosting );
            ImGui::InputScalar( "##Port", ImGuiDataType_U16, &m_port, nullptr, nullptr, "%u", m_fHosting ? ImGuiInputTextFlags_ReadOnly : 0 );
            ImGui::EndDisabled();

            if( ImGui::Button( m_fHosting ? "Stop" : "Host" ) )
            {
               m_fHosting = !m_fHosting;
               if( m_fHosting )
               {
                  s_NetworkLogs.push_back( std::format( "Hosting on: {}:{}", INetwork::GetHostAddress(), m_port ) );
                  NetworkHost& network = INetwork::Create< NetworkHost >();
                  network.Listen( m_port );
               }
               else
               {
                  s_NetworkLogs.push_back( "Stopping host..." );
                  INetwork::Shutdown();
               }
            }

            ImGui::EndTabItem();
         }
         ImGui::EndDisabled();

         if( ImGui::BeginDisabled( m_fHosting ), ImGui::BeginTabItem( "Connect" ) )
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
               m_fConnected = !m_fConnected;
               if( m_fConnected )
               {
                  s_NetworkLogs.push_back( std::format( "Connecting to: {}:{}", m_connectAddress, m_port ) );
                  NetworkClient& network = INetwork::Create< NetworkClient >();
                  network.Connect( m_connectAddress, m_port );
               }
               else
               {
                  s_NetworkLogs.push_back( std::format( "Disconnecting from: {}:{}", m_connectAddress, m_port ) );
                  if( INetwork::Get() )
                     INetwork::Get()->SendPacket( Packet::Create< NetworkCode::ClientDisconnect >( INetwork::Get()->GetID(), {} ) );
                  INetwork::Shutdown();
               }
            }

            ImGui::EndTabItem();
         }
         ImGui::EndDisabled();

         if( ImGui::BeginTabItem( "Log" ) )
         {
            if( ImGui::BeginChild( "LogArea", ImVec2( 0, -ImGui::GetFrameHeightWithSpacing() ), true ) )
            {
               for( const std::string& log : s_NetworkLogs )
                  ImGui::TextWrapped( "%s", log.c_str() );
               ImGui::EndChild();
            }

            if( ImGui::Button( "Clear" ) )
               s_NetworkLogs.clear();

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

   std::deque< std::string > m_messages;

   Events::EventSubscriber m_eventSubscriber;
};

std::shared_ptr< UI::IDrawable > CreateNetworkUI()
{
   return std::make_shared< NetworkUI >();
}
