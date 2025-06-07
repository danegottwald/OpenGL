
#include "Camera.h"

#include "Core/Window.h"
#include "Entity/IGameObject.h"

Camera::Camera()
{
   const WindowData& winData = Window::Get().GetWindowData();
   float             width   = winData.Width;
   float             height  = winData.Height;
   m_projectionMatrix        = glm::perspective( glm::radians( m_fov ), width / height, 0.1f /*near*/, 1000.0f /*far*/ );

   // Subscribe to events
   m_eventSubscriber.Subscribe< Events::WindowResizeEvent >( std::bind( &Camera::WindowResizeEvent, this, std::placeholders::_1 ) );
   m_eventSubscriber.Subscribe< Events::MouseScrolledEvent >( [ this ]( const Events::MouseScrolledEvent& e ) noexcept
   {
      m_fov += -e.GetYOffset() * 5;
      m_fov              = std::clamp( m_fov, 10.0f, 90.0f );
      m_projectionMatrix = glm::perspective( glm::radians( m_fov ),
                                             Window::Get().GetWindowData().Width / static_cast< float >( Window::Get().GetWindowData().Height ),
                                             0.1f,
                                             1000.0f );
   } );
}

void Camera::Update( const IGameObject& entity )
{
   // update frustum using m_ProjectionViewMatrix

   // Set New Camera Position
   m_position = entity.GetPosition();
   m_rotation = entity.GetRotation();

   // Update m_ProjectionViewMatrix (all below)
   m_viewMatrix = glm::mat4( 1.0f );
   m_viewMatrix = glm::rotate( m_viewMatrix, glm::radians( m_rotation.x ), { 1, 0, 0 } );
   m_viewMatrix = glm::rotate( m_viewMatrix, glm::radians( m_rotation.y ), { 0, 1, 0 } );
   m_viewMatrix = glm::rotate( m_viewMatrix, glm::radians( m_rotation.z ), { 0, 0, 1 } );
   m_viewMatrix = glm::translate( m_viewMatrix, -m_position );

   m_projectionViewMatrix = m_projectionMatrix * m_viewMatrix;
}

const glm::mat4& Camera::GetView() const
{
   return m_viewMatrix;
}

const glm::mat4& Camera::GetProjectionView() const
{
   return m_projectionViewMatrix;
}

const glm::vec3& Camera::GetPosition() const
{
   return m_position;
}

void Camera::WindowResizeEvent( const Events::WindowResizeEvent& e ) noexcept
{
   if( e.GetHeight() == 0 )
      return;

   float aspectRatio  = e.GetWidth() / static_cast< float >( e.GetHeight() );
   m_projectionMatrix = glm::perspective( glm::radians( m_fov ), aspectRatio, 0.1f, 1000.0f );
}
