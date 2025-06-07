#pragma once

#include "../Events/ApplicationEvent.h"

// Forward declarations
class IGameObject;

class Camera
{
public:
   Camera();

   void Update( const IGameObject& entity );

   const glm::mat4& GetView() const;
   const glm::mat4& GetProjectionView() const;
   const glm::vec3& GetPosition() const;

private:
   // Events
   Events::EventSubscriber m_eventSubscriber;
   void                    WindowResizeEvent( const Events::WindowResizeEvent& e ) noexcept;

   float     m_fov { 90.0f };
   glm::mat4 m_viewMatrix { 1.0f };
   glm::mat4 m_projectionMatrix { 1.0f };
   glm::mat4 m_projectionViewMatrix { 1.0f };
   glm::vec3 m_position { 1.0f };
   glm::vec3 m_rotation { 1.0f };
};

class ViewFrustum
{
public:
   ViewFrustum( const Camera& camera )
   {
      glm::mat4 vp  = camera.GetProjectionView();
      m_planes[ 0 ] = glm::vec4( vp[ 0 ][ 3 ] + vp[ 0 ][ 0 ], vp[ 1 ][ 3 ] + vp[ 1 ][ 0 ], vp[ 2 ][ 3 ] + vp[ 2 ][ 0 ], vp[ 3 ][ 3 ] + vp[ 3 ][ 0 ] ); // Left
      m_planes[ 1 ] = glm::vec4( vp[ 0 ][ 3 ] - vp[ 0 ][ 0 ], vp[ 1 ][ 3 ] - vp[ 1 ][ 0 ], vp[ 2 ][ 3 ] - vp[ 2 ][ 0 ], vp[ 3 ][ 3 ] - vp[ 3 ][ 0 ] ); // Right
      m_planes[ 2 ] = glm::vec4( vp[ 0 ][ 3 ] - vp[ 0 ][ 1 ], vp[ 1 ][ 3 ] - vp[ 1 ][ 1 ], vp[ 2 ][ 3 ] - vp[ 2 ][ 1 ], vp[ 3 ][ 3 ] - vp[ 3 ][ 1 ] ); // Top
      m_planes[ 3 ] = glm::vec4( vp[ 0 ][ 3 ] + vp[ 0 ][ 1 ], vp[ 1 ][ 3 ] + vp[ 1 ][ 1 ], vp[ 2 ][ 3 ] + vp[ 2 ][ 1 ], vp[ 3 ][ 3 ] + vp[ 3 ][ 1 ] ); // Bottom
      m_planes[ 4 ] = glm::vec4( vp[ 0 ][ 3 ] + vp[ 0 ][ 2 ], vp[ 1 ][ 3 ] + vp[ 1 ][ 2 ], vp[ 2 ][ 3 ] + vp[ 2 ][ 2 ], vp[ 3 ][ 3 ] + vp[ 3 ][ 2 ] ); // Near
      m_planes[ 5 ] = glm::vec4( vp[ 0 ][ 3 ] - vp[ 0 ][ 2 ], vp[ 1 ][ 3 ] - vp[ 1 ][ 2 ], vp[ 2 ][ 3 ] - vp[ 2 ][ 2 ], vp[ 3 ][ 3 ] - vp[ 3 ][ 2 ] ); // Far

      for( glm::vec4& plane : m_planes )
         plane = glm::normalize( plane );
   }

   bool FInFrustum( const glm::vec3& point, float padding = 0.0f ) const
   {
      return std::none_of( m_planes.begin(),
                           m_planes.end(),
                           [ & ]( const glm::vec4& plane ) { return glm::dot( glm::vec3( plane ), point ) + plane.w + padding < 0; } );
   }

private:
   std::array< glm::vec4, 6 > m_planes;
};
