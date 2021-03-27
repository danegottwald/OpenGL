
#include "Renderer.h"

#include "imgui/imgui.h"

Renderer::Renderer() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    m_Shader.reset(new Shader("Basic.glsl"));
    m_Shader->Bind();
}

Renderer::~Renderer() {
}

void Renderer::Attach() {
}

void Renderer::Detach() {
}

// Binds the Shader, Vertex Array, and the Index Buffer, then issues a Draw Call
void Renderer::Draw() {

    // update camera
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
}

void Renderer::DrawGui() {
    ImGui::BeginMainMenuBar();
    ImGui::Text("FPS: %d (%.1f ms)", static_cast<int>(ImGui::GetIO().Framerate), 1000.0f / ImGui::GetIO().Framerate);
    ImGui::EndMainMenuBar();
}


void Renderer::SwapShader(const std::string& file) {
    m_Shader.reset(new Shader(file));
}


void Renderer::UpdateViewport(unsigned int x, unsigned int y, unsigned int width, unsigned int height) {
    glViewport(x, y, width, height);
}


