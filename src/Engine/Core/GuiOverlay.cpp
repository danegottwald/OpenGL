#include "GuiOverlay.h"

#include "Application.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

GuiOverlay::GuiOverlay() : m_Active(false) {
}

GuiOverlay::~GuiOverlay() {
    if (m_Active) {
        Detach();
    }
}

void GuiOverlay::Attach() {
    m_Active = true;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls

    ImGui::StyleColorsDark();
    auto& app = Application::Get();
    ImGui_ImplGlfw_InitForOpenGL(Window::Get().GetWindow(), true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

void GuiOverlay::Begin() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void GuiOverlay::End() {
    ImGuiIO& io = ImGui::GetIO();
    auto& app = Application::Get();
    io.DisplaySize = ImVec2(static_cast<float>(Window::Get().GetWidth()),
                            static_cast<float>(Window::Get().GetHeight()));

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void GuiOverlay::Detach() {
    m_Active = false;

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
