#include "GuiOverlay.h"

#include "Application.h"

GuiOverlay::GuiOverlay() : m_Active( false )
{
}

GuiOverlay::~GuiOverlay()
{
    if( m_Active )
    {
        Detach();
    }
}

void GuiOverlay::Attach()
{
    m_Active = true;

    // IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    io.IniFilename = "settings.ini";

    ( void )io;
    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();

    auto& app = Application::Get();
    ImGui_ImplGlfw_InitForOpenGL( Window::Get().GetNativeWindow(), true );
    ImGui_ImplOpenGL3_Init( "#version 330" );
}

void GuiOverlay::Begin()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void GuiOverlay::End()
{
    ImGuiIO& io = ImGui::GetIO();
    auto prop = Window::Get().GetWindowData();
    io.DisplaySize =
        ImVec2( static_cast< float >( prop.Width ), static_cast< float >( prop.Height ) );

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData( ImGui::GetDrawData() );
}

void GuiOverlay::Detach()
{
    m_Active = false;

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
