
#pragma once

#include "glm/glm.hpp"

class PlayerController {
private:
    static PlayerController* s_Controller;
    glm::vec3 m_Position;
    glm::vec3 m_ViewDirection;
    const glm::vec3 m_Up;
    glm::vec2 m_OldMousePos;
    float m_FOV;
    float m_MoveSpeed;
    float m_LookSpeed;

public:
    PlayerController();
    ~PlayerController();

    static PlayerController& GetController() { return *s_Controller; }

    void Update(float frametime);
    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix() const;

    const glm::vec3& GetPosition() { return m_Position; }
    float GetFOV() { return m_FOV; }

};


