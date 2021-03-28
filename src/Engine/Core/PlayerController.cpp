
#include "PlayerController.h"

#include <iostream>

#include "Input.h"
#include "../../../Dependencies/GLFW/deps/linmath.h"
#include "GLFW/glfw3.h"

#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"

PlayerController* PlayerController::s_Controller = nullptr;

PlayerController::PlayerController() : m_Up(0.0f, 1.0f, 0.0f) {
    assert(s_Controller == nullptr);
    s_Controller = this;
    m_Position = glm::vec3(0.0f, 0.0f, 2.0f);
    m_ViewDirection = glm::vec3(0.0f, 0.0f, -1.0f);
    m_OldMousePos = glm::vec2(0.0f, 0.0f);
    m_FOV = 45.0f;
    m_MoveSpeed = 5.0f;
    m_LookSpeed = 0.5f;

    glfwSetCursorPosCallback(Window::Get().GetWindow(), [](GLFWwindow* window, double x, double y) {
        auto& pc = GetController();

        // Get the angles that x and y moved
        glm::vec2 mouseDelta = glm::vec2(x, y) - pc.m_OldMousePos;
        float xAngle = glm::radians(-mouseDelta.x * pc.m_LookSpeed) / 8.0f;
        float yAngle = glm::radians(-mouseDelta.y * pc.m_LookSpeed) / 8.0f;

        // Find the updated m_ViewDirection matrix
        glm::vec3 rotateAxis = glm::cross(pc.m_ViewDirection, pc.m_Up);
        pc.m_ViewDirection = glm::mat3(glm::rotate(glm::mat4(1.0f), xAngle, pc.m_Up) * 
                                         glm::rotate(glm::mat4(1.0f), yAngle, rotateAxis)) * 
                                         pc.m_ViewDirection;
        pc.m_OldMousePos = glm::vec2(x, y);
    });

}

PlayerController::~PlayerController() {

}


void PlayerController::Update(float frametime) {
    if (Input::IsKeyPressed(Key::W)) {
        m_Position += (m_MoveSpeed * frametime) * m_ViewDirection;
    }
    if (Input::IsKeyPressed(Key::S)) {
        m_Position -= (m_MoveSpeed * frametime) * m_ViewDirection;
    }
    if (Input::IsKeyPressed(Key::D)) {
        m_Position += (m_MoveSpeed * frametime) * glm::cross(m_ViewDirection, m_Up);
    }
    if (Input::IsKeyPressed(Key::A)) {
        m_Position -= (m_MoveSpeed * frametime) * glm::cross(m_ViewDirection, m_Up);
    }
    if (Input::IsKeyPressed(Key::Space)) {
        m_Position.y += (m_MoveSpeed * frametime);
    }
    if (Input::IsKeyPressed(Key::LeftShift)) {
        m_Position.y -= (m_MoveSpeed * frametime);
    }
    if (Input::IsMousePressed(Mouse::ButtonRight)) {

    }
}

glm::mat4 PlayerController::GetViewMatrix() const {
    return glm::lookAt(m_Position, m_Position + m_ViewDirection, m_Up);
}

glm::mat4 PlayerController::GetProjectionMatrix() const {
    auto& window = Window::Get();
    return glm::perspective(m_FOV, (float)(window.GetWidth() / window.GetHeight()), 0.1f, 100.0f);
}



