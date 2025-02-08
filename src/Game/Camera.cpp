
#include "Camera.h"

#include "Core/Window.h"
#include "Entity/IGameObject.h"

Camera::Camera()
{
   const WindowData& winData = Window::Get().GetWindowData();
   float             width   = winData.Width;
   float             height  = winData.Height;
   m_projectionMatrix        = glm::perspective( glm::radians( 90.0f ) /*fovy*/, width / height /*aspect*/, 0.1f /*near*/, 100.0f /*far*/ );

   // Subscribe to events
   m_eventSubscriber.Subscribe< Events::WindowResizeEvent >( std::bind( &Camera::WindowResizeEvent, this, std::placeholders::_1 ) );
}

void Camera::Update( const IGameObject& entity )
{
   // update frustum using m_ProjectionViewMatrix

   // Set New Camera Position
   m_position = entity.GetPosition();
   m_rotation = entity.GetRotation();

   // Update m_ProjectionViewMatrix (all below)
   glm::mat4 view { 1.0f };
   view = glm::rotate( view, glm::radians( m_rotation.x ), { 1, 0, 0 } );
   view = glm::rotate( view, glm::radians( m_rotation.y ), { 0, 1, 0 } );
   view = glm::rotate( view, glm::radians( m_rotation.z ), { 0, 0, 1 } );
   view = glm::translate( view, -m_position );

   m_projectionViewMatrix = m_projectionMatrix * view;
}

const glm::mat4& Camera::GetProjectionView() const
{
   return m_projectionViewMatrix;
}

const glm::vec3& Camera::GetPosition() const
{
   return m_position;
}

void Camera::WindowResizeEvent( const Events::WindowResizeEvent& e )
{
   if( e.GetHeight() == 0 )
      return;

   float aspectRatio  = e.GetWidth() / static_cast< float >( e.GetHeight() );
   m_projectionMatrix = glm::perspective( glm::radians( 90.0f ), aspectRatio, 0.1f, 100.0f );
}
