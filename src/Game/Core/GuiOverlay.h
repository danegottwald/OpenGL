#pragma once

class GuiOverlay
{
private:
    bool m_Active;

public:
    GuiOverlay();
    ~GuiOverlay();

    void Attach();
    void Begin();
    void End();
    void Detach();
};

class UI
{
private:
    bool m_Active;

public:
    UI();
    virtual ~UI() = default;

    virtual void Display() = 0;

    void EnableFlag( ImGuiConfigFlags_ flag ) const
    {
        ImGui::GetIO().ConfigFlags |= flag;
    }

    void DisableFlag( ImGuiConfigFlags_ flag ) const
    {
        ImGui::GetIO().ConfigFlags ^= flag;
    };

};

//void UI::Display() {
//    // Prepare new ImGui Frame
//    ImGui_ImplOpenGL3_NewFrame();
//    ImGui_ImplGlfw_NewFrame();
//    ImGui::NewFrame();
//
//    for (const auto& e : elements) {
//        
//    }
//
//    // Adjust for possible resizing of window 
//    ImGuiIO& io = ImGui::GetIO();
//    auto prop = Window::Get().GetProperties();
//    io.DisplaySize =
//        ImVec2(static_cast<float>(prop.Width), static_cast<float>(prop.Height));
//
//    // Render frame
//    ImGui::Render();
//    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
//}
