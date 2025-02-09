#pragma once

#include "../Events/ApplicationEvent.h"

// Forward declarations
class IGameObject;

class Camera
{
public:
   Camera();

   void Update( const IGameObject& entity );

   const glm::mat4& GetProjectionView() const;
   const glm::vec3& GetPosition() const;

private:
   // Events
   Events::EventSubscriber m_eventSubscriber;
   void                    WindowResizeEvent( const Events::WindowResizeEvent& e );

   glm::mat4 m_projectionMatrix { 1.0f };
   glm::mat4 m_projectionViewMatrix { 1.0f };
   glm::vec3 m_position { 1.0f };
   glm::vec3 m_rotation { 1.0f };
};
