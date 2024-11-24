#pragma once

class Entity;
class EventDispatcher;

class Camera
{
public:
    Camera();

    void Update( const Entity& entity );
    bool OnEvent( EventDispatcher& dispatcher );

    const glm::mat4& GetProjectionView() const;
    const glm::vec3& GetPosition() const;

private:
    glm::mat4 m_projectionMatrix{ 1.0f };
    glm::mat4 m_projectionViewMatrix{ 1.0f };
    glm::vec3 m_position{ 1.0f };
    glm::vec3 m_rotation{ 1.0f };
};