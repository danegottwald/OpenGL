
#include "PlayerController.h"

#include <iostream>


#include "Application.h"
#include "GLFW/glfw3.h"

PlayerController* PlayerController::s_Controller = nullptr;

PlayerController::PlayerController() : m_CameraPos(0.0, 0.0, 2.0),
    m_CameraRot(0.0f), m_FOV(90.0f), m_Speed(5.0f) {
    s_Controller = this;
}

PlayerController::~PlayerController() {

}

void PlayerController::UpdateCamera(float frametime) {
    int key = glfwGetKey(&Application::Get().GetWindow().GetWindow(), GLFW_KEY_W);
    if (key == GLFW_PRESS) {
        m_CameraPos.z -= (m_Speed * frametime);
    }
    key = glfwGetKey(&Application::Get().GetWindow().GetWindow(), GLFW_KEY_S);
    if (key == GLFW_PRESS) {
        m_CameraPos.z += (m_Speed * frametime);
    }
    key = glfwGetKey(&Application::Get().GetWindow().GetWindow(), GLFW_KEY_A);
    if (key == GLFW_PRESS) {
        m_CameraPos.x -= cos(glm::radians(m_CameraRot)) * (m_Speed * frametime);
        m_CameraPos.y -= sin(glm::radians(m_CameraRot)) * (m_Speed * frametime);
    }
    key = glfwGetKey(&Application::Get().GetWindow().GetWindow(), GLFW_KEY_D);
    if (key == GLFW_PRESS) {
        m_CameraPos.x += cos(glm::radians(m_CameraRot)) * (m_Speed * frametime);
        m_CameraPos.y += sin(glm::radians(m_CameraRot)) * (m_Speed * frametime);
    }
    key = glfwGetKey(&Application::Get().GetWindow().GetWindow(), GLFW_KEY_SPACE);
    if (key == GLFW_PRESS) {
        m_CameraPos.x += -sin(glm::radians(m_CameraRot)) * (m_Speed * frametime);
        m_CameraPos.y += cos(glm::radians(m_CameraRot)) * (m_Speed * frametime);
    }
    key = glfwGetKey(&Application::Get().GetWindow().GetWindow(), GLFW_KEY_LEFT_SHIFT);
    if (key == GLFW_PRESS) {
        m_CameraPos.x -= -sin(glm::radians(m_CameraRot)) * (m_Speed * frametime);
        m_CameraPos.y -= cos(glm::radians(m_CameraRot)) * (m_Speed * frametime);
    }
}



