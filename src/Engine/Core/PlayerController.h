
#pragma once

#include "glm/glm.hpp"

class PlayerController {
private:
    static PlayerController* s_Controller;
    glm::vec3 m_CameraPos;
    float m_CameraRot, m_FOV, m_Speed;

public:
    PlayerController();
    ~PlayerController();

    static PlayerController& GetController() { return *s_Controller; }

    void UpdateCamera(float frametime);

    const glm::vec3 &GetCameraPos() { return m_CameraPos; }
    float GetCameraRot() { return m_CameraRot; }
    float GetFOV() { return m_FOV; }

};


