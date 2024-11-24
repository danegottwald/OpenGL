
#include "RenderTexture.h"

#include <imgui.h>

#include "glm/gtc/matrix_transform.hpp"

TestSpace::RenderTexture::RenderTexture()
	: translation(200, 200, 0) {
	// Vertex Positions and Indices
	const std::array<float, 16> positions = {
	   -50.0f, -50.0f, 0.0f, 0.0f, // 0
		50.0f, -50.0f, 1.0f, 0.0f, // 1
		50.0f,  50.0f, 1.0f, 1.0f, // 2
	  -50.0f, 50.0f, 0.0f, 1.0f, // 3
	};
	const std::array<unsigned int, 6> indices = {
	0, 1, 2,
	2, 3, 0,
	};
	
	// Create Vertex Buffer
	vb = new VertexBuffer(positions);
	// Create Vertex Array
	va = new VertexArray();
	// Give Vertex Array the Layout of the Vertex Buffer
	layout.Push<float>(2);
	layout.Push<float>(2);
	//va->AddBuffer(*vb, layout);
	// Create Index Buffer
	ib = new IndexBuffer(indices);
	// Create and Bind the Shader
	shader = new Shader("res/shaders/Basic.glsl");
	shader->Bind();
	// Create, Bind, and Set the Texture
	texture = new Texture("res/textures/kitty.jpg");
	texture->Bind();
	shader->SetUniform1i("u_Texture", 0);
}

TestSpace::RenderTexture::~RenderTexture() {
	delete vb;
	delete va;
	delete ib;
	delete shader;
	delete texture;
}

void TestSpace::RenderTexture::OnUpdate(float deltaTime) {
	glm::mat4 proj = glm::ortho(0.0f, 1280.0f, 0.0f, 720.0f, -1.0f, 1.0f);
	glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, 0));
    glm::mat4 model = glm::translate(glm::mat4(1.0f), translation);
    glm::mat4 mvp = proj * view * model;
    shader->SetUniformMat4f("u_MVP", mvp);
}

void TestSpace::RenderTexture::OnRender() {
	glDrawElements(GL_TRIANGLES, ib->GetCount(), GL_UNSIGNED_INT, nullptr);
}

void TestSpace::RenderTexture::OnImGuiRender() {
    ImGui::SliderFloat2("Translation", &translation.x, 0.0f, 1280.0f);
}
