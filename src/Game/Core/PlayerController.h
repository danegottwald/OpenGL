#pragma once

class PlayerController
{
private:
    static PlayerController* s_Controller;
    glm::vec3 m_Position;
    glm::vec3 m_ViewDirection;
    glm::vec2 m_OldMousePos;
    glm::vec3 m_Up;
    float m_FOV;
    float m_MoveSpeed;
    float m_LookSpeed;

    void EnableCursorCallback();

public:
    PlayerController();

    static PlayerController& GetController()
    {
        return *s_Controller;
    }

    void Update( float deltaTime );
    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix() const;

    const glm::vec3& GetPosition()
    {
        return m_Position;
    }
    float GetFOV() const
    {
        return m_FOV;
    }
};
