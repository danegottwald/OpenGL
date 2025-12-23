#include "Application.h"

// Local dependencies
#include "GUIManager.h"
#include "Timestep.h"
#include "Window.h"

// Project dependencies
#include <Engine/Events/ApplicationEvent.h>
#include <Engine/Events/KeyEvent.h>
#include <Engine/Events/MouseEvent.h>
#include <Engine/Input/Input.h>
#include <Engine/Network/Network.h>

#include <Engine/World/Level.h>
#include <Engine/Renderer/Shader.h>
#include <Engine/Renderer/Texture.h>
#include <Engine/World/Raycast.h>

//#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// Component includes
#include <Engine/ECS/Components.h>
#include <Engine/ECS/Registry.h>

// ================ SYSTEMS ================
constexpr float GRAVITY           = -32.0f;
constexpr float TERMINAL_VELOCITY = -48.0f;
constexpr float JUMP_VELOCITY     = 9.0f;
constexpr float PLAYER_HEIGHT     = 1.8f;
constexpr float PLAYER_EYE_HEIGHT = 1.62f;
constexpr float GROUND_MAXSPEED   = 4.3f;
constexpr float AIR_MAXSPEED      = 10.0f;
constexpr float AIR_CONTROL       = 0.3f;
constexpr float SPRINT_MODIFIER   = 1.3f;

static void MouseLookSystem( Entity::Registry& registry, Window& window )
{
   //PROFILE_SCOPE( "MouseLookSystem" );
   GLFWwindow* nativeWindow   = window.GetNativeWindow();
   const bool  fMouseCaptured = glfwGetInputMode( nativeWindow, GLFW_CURSOR ) == GLFW_CURSOR_DISABLED;

   glm::vec2 mouseDelta { 0.0f };
   if( fMouseCaptured )
   {
      static glm::vec2 lastMousePos = window.GetMousePosition();
      mouseDelta                    = window.GetMousePosition() - lastMousePos;

      const WindowData& windowData = window.GetWindowData();
      lastMousePos                 = { windowData.Width * 0.5f, windowData.Height * 0.5f };
      glfwSetCursorPos( nativeWindow, lastMousePos.x, lastMousePos.y );
   }

   // Write intent only (no transform mutation)
   for( auto [ look ] : registry.CView< CLookInput >() )
   {
      look.yawDelta   = 0.0f;
      look.pitchDelta = 0.0f;
   }

   // Only apply mouse delta if captured
   if( fMouseCaptured )
   {
      for( auto [ cam, look ] : registry.CView< CCamera, CLookInput >() )
      {
         look.pitchDelta += mouseDelta.y * cam.sensitivity;
         look.yawDelta += mouseDelta.x * cam.sensitivity;
      }
   }
}

static void CameraRigSystem( Entity::Registry& registry )
{
   //PROFILE_SCOPE( "CameraRigSystem" );
   for( auto [ camEntity, camTran, rig ] : registry.ECView< CTransform, CCameraRig >() )
   {
      CTransform* pTargetTran = registry.TryGet< CTransform >( rig.targetEntity );
      if( !pTargetTran )
         continue;

      // Apply look input to camera
      if( CCamera* cam = registry.TryGet< CCamera >( camEntity ) )
      {
         if( CLookInput* look = registry.TryGet< CLookInput >( camEntity ) )
         {
            camTran.rotation.x = glm::clamp( camTran.rotation.x + look->pitchDelta, -90.0f, 90.0f );
            camTran.rotation.y = glm::mod( camTran.rotation.y + look->yawDelta + 360.0f, 360.0f );
         }
      }

      // Apply camera yaw/pitch to target
      if( rig.followYaw )
         pTargetTran->rotation.y = camTran.rotation.y;
      if( rig.followPitch )
         pTargetTran->rotation.x = camTran.rotation.x;

      // Position the camera (with offset if any)
      camTran.position = pTargetTran->position + rig.offset;
   }
}

static void PlayerInputSystem( Entity::Registry& registry, float delta )
{
   //PROFILE_SCOPE( "PlayerInputSystem" );
   for( auto [ tran, vel, input, tag ] : registry.CView< CTransform, CVelocity, CInput, CPlayerTag >() )
   {
      // Apply Player Movement Inputs
      glm::vec3 wishdir( 0.0f );
      if( Input::IsKeyPressed( Input::W ) )
      {
         wishdir.x -= glm::cos( glm::radians( tran.rotation.y + 90 ) );
         wishdir.z -= glm::sin( glm::radians( tran.rotation.y + 90 ) );
      }
      if( Input::IsKeyPressed( Input::S ) )
      {
         wishdir.x += glm::cos( glm::radians( tran.rotation.y + 90 ) );
         wishdir.z += glm::sin( glm::radians( tran.rotation.y + 90 ) );
      }
      if( Input::IsKeyPressed( Input::A ) )
      {
         float y = glm::radians( tran.rotation.y );
         wishdir += glm::vec3( -glm::cos( y ), 0, -glm::sin( y ) );
      }
      if( Input::IsKeyPressed( Input::D ) )
      {
         float y = glm::radians( tran.rotation.y );
         wishdir -= glm::vec3( -glm::cos( y ), 0, -glm::sin( y ) );
      }
      if( glm::length( wishdir ) > 0.0f )
         wishdir = glm::normalize( wishdir );

      float maxSpeed = GROUND_MAXSPEED;
      if( Input::IsKeyPressed( Input::LeftShift ) )
         maxSpeed *= SPRINT_MODIFIER;

      vel.velocity.x = wishdir.x * maxSpeed;
      vel.velocity.z = wishdir.z * maxSpeed;

      // this needs to be per-component, not static which affects all
      static bool wasJumping = false;
      if( Input::IsKeyPressed( Input::Space ) )
      {
         if( !wasJumping )
         {
            vel.velocity.y = JUMP_VELOCITY;
            wasJumping     = true;
         }
      }
      else
         wasJumping = false;
   }
}

static void PlayerPhysicsSystem( Entity::Registry& registry, Level& level, float delta )
{
   //PROFILE_SCOPE( "PlayerPhysicsSystem" );

   // World vs Entity collision
   {
      auto getVoxelBounds = [ & ]( const glm::vec3& pos, const glm::vec3& bbMin, const glm::vec3& bbMax ) -> std::tuple< glm::ivec3, glm::ivec3 >
      {
         constexpr float EPS = 1e-4f; // for stable voxel range calc
         return {
            glm::ivec3 { static_cast< int >( std::floor( pos.x + bbMin.x + EPS ) ), // min x
                         static_cast< int >( std::floor( pos.y + bbMin.y + EPS ) ), // min y
                         static_cast< int >( std::floor( pos.z + bbMin.z + EPS ) ) }, // min z
            glm::ivec3 { static_cast< int >( std::floor( pos.x + bbMax.x - EPS ) ), // max x
                         static_cast< int >( std::floor( pos.y + bbMax.y - EPS ) ), // max y
                         static_cast< int >( std::floor( pos.z + bbMax.z - EPS ) ) }  // max z
         };
      };

      auto computeGrounded = [ & ]( const glm::vec3& pos, const glm::vec3& bbMin, const glm::vec3& bbMax ) -> bool
      {
         constexpr float GROUND_PROBE = 0.05f; // how far below to probe for ground

         glm::vec3 probePos = pos;
         probePos.y -= GROUND_PROBE;

         auto [ min, max ] = getVoxelBounds( probePos, bbMin, bbMax );
         for( int y = min.y; y <= max.y; ++y )
            for( int x = min.x; x <= max.x; ++x )
               for( int z = min.z; z <= max.z; ++z )
                  if( FSolid( level.GetBlock( x, y, z ) ) )
                     return true;

         return false;
      };

      enum Axis
      {
         X = 0,
         Y = 1,
         Z = 2
      };
      auto moveAndCollideAxis = [ & ]( glm::vec3& pos, glm::vec3& vel, CPhysics& phys, float d, Axis axis )
      {
         if( d == 0.0f )
            return;

         pos[ axis ] += d;
         auto [ min, max ]   = getVoxelBounds( pos, phys.bbMin, phys.bbMax );
         const bool positive = d > 0.0f;
         int        hit      = positive ? INT_MAX : INT_MIN;

         // Scan overlapped voxels, tracking closest hit plane along `axis`
         for( int y = min.y; y <= max.y; ++y )
         {
            for( int x = min.x; x <= max.x; ++x )
            {
               for( int z = min.z; z <= max.z; ++z )
               {
                  if( !FSolid( level.GetBlock( x, y, z ) ) )
                     continue;

                  switch( axis )
                  {
                     case Axis::X: hit = positive ? ( std::min )( hit, x ) : ( std::max )( hit, x ); break;
                     case Axis::Y: hit = positive ? ( std::min )( hit, y ) : ( std::max )( hit, y ); break;
                     case Axis::Z: hit = positive ? ( std::min )( hit, z ) : ( std::max )( hit, z ); break;
                  }
               }
            }
         }

         constexpr float SKIN = 0.001f; // small separation to prevent re-penetration
         if( positive )
         {
            if( hit == INT_MAX )
               return;

            // clamp so our max face sits just before `hit` voxel's min face
            pos[ axis ] = static_cast< float >( hit ) - phys.bbMax[ axis ] - SKIN;
         }
         else
         {
            if( hit == INT_MIN )
               return;

            // clamp so our min face sits just after (`hit + 1`) voxel's max face
            pos[ axis ] = static_cast< float >( hit + 1 ) - phys.bbMin[ axis ] + SKIN;
         }

         // Apply restitution on collision for this axis.
         // If bounciness is 0, this matches the old behavior (velocity becomes 0).
         const float restitution = std::clamp( phys.bounciness, 0.0f, 1.0f );
         vel[ axis ]             = -vel[ axis ] * restitution;

         // Avoid tiny jitter bounces
         if( std::abs( vel[ axis ] ) < 0.01f )
            vel[ axis ] = 0.0f;

         // Consider on-ground only if we hit while moving downward and we're not bouncing significantly.
         if( axis == Axis::Y && !positive && restitution < 0.5f )
            phys.fOnGround = true;
      };

      for( auto [ tran, vel, phys ] : registry.CView< CTransform, CVelocity, CPhysics >() )
      {
         // Determine if entity is on the ground and only apply gravity if not.
         phys.fOnGround = computeGrounded( tran.position, phys.bbMin, phys.bbMax );
         if( !phys.fOnGround )
            vel.velocity.y = ( std::max )( vel.velocity.y + ( GRAVITY * delta ), TERMINAL_VELOCITY );

         // Per-axis movement + collision
         glm::vec3 pos  = tran.position;
         glm::vec3 step = vel.velocity * delta;
         moveAndCollideAxis( pos, vel.velocity, phys, step.y, Axis::Y );
         moveAndCollideAxis( pos, vel.velocity, phys, step.x, Axis::X );
         moveAndCollideAxis( pos, vel.velocity, phys, step.z, Axis::Z );
         tran.position = pos;
      }
   }

   // ---- Optional: entity-vs-entity stays after world, but note it can fight world resolution ----
   //for( auto [ tranA, velA, physA ] : registry.CView< CTransform, CVelocity, CPhysics >() )
   //{
   //   for( auto [ tranB, velB, physB ] : registry.CView< CTransform, CVelocity, CPhysics >() )
   //   {
   //      if( &tranA == &tranB )
   //         continue;
   //
   //      // AABB vs AABB collision
   //      glm::vec3 minA     = tranA.position - physA.halfExtents;
   //      glm::vec3 maxA     = tranA.position + physA.halfExtents;
   //      glm::vec3 minB     = tranB.position - physB.halfExtents;
   //      glm::vec3 maxB     = tranB.position + physB.halfExtents;
   //      bool      overlapX = ( minA.x <= maxB.x ) && ( maxA.x >= minB.x );
   //      bool      overlapY = ( minA.y <= maxB.y ) && ( maxA.y >= minB.y );
   //      bool      overlapZ = ( minA.z <= maxB.z ) && ( maxA.z >= minB.z );
   //      if( overlapX && overlapY && overlapZ )
   //      {
   //         // Simple resolution: push A back along its velocity vector
   //         glm::vec3 normVelA = glm::normalize( velA.velocity );
   //         tranA.position -= normVelA * 0.1f; // arbitrary small pushback
   //         // Zero A's velocity
   //         velA.velocity = glm::vec3( 0.0f );
   //      }
   //   }
   //}
}


static void CameraViewSystem( Entity::Registry& registry )
{
   //PROFILE_SCOPE( "CameraViewSystem" );
   for( auto [ tran, cam ] : registry.CView< CTransform, CCamera >() )
   {
      cam.view = glm::mat4( 1.0f );
      cam.view = glm::rotate( cam.view, glm::radians( tran.rotation.x ), { 1, 0, 0 } );
      cam.view = glm::rotate( cam.view, glm::radians( tran.rotation.y ), { 0, 1, 0 } );
      cam.view = glm::rotate( cam.view, glm::radians( tran.rotation.z ), { 0, 0, 1 } );
      cam.view = glm::translate( cam.view, -tran.position );

      cam.viewProjection = cam.projection * cam.view;
   }
}


class ViewFrustum
{
public:
   ViewFrustum( const glm::mat4& pv )
   {
      m_planes[ 0 ] = glm::vec4( pv[ 0 ][ 3 ] + pv[ 0 ][ 0 ], pv[ 1 ][ 3 ] + pv[ 1 ][ 0 ], pv[ 2 ][ 3 ] + pv[ 2 ][ 0 ], pv[ 3 ][ 3 ] + pv[ 3 ][ 0 ] );
      m_planes[ 1 ] = glm::vec4( pv[ 0 ][ 3 ] - pv[ 0 ][ 0 ], pv[ 1 ][ 3 ] - pv[ 1 ][ 0 ], pv[ 2 ][ 3 ] - pv[ 2 ][ 0 ], pv[ 3 ][ 3 ] - pv[ 3 ][ 0 ] );
      m_planes[ 2 ] = glm::vec4( pv[ 0 ][ 3 ] - pv[ 0 ][ 1 ], pv[ 1 ][ 3 ] - pv[ 1 ][ 1 ], pv[ 2 ][ 3 ] - pv[ 2 ][ 1 ], pv[ 3 ][ 3 ] - pv[ 3 ][ 1 ] );
      m_planes[ 3 ] = glm::vec4( pv[ 0 ][ 3 ] + pv[ 0 ][ 1 ], pv[ 1 ][ 3 ] + pv[ 1 ][ 1 ], pv[ 2 ][ 3 ] + pv[ 2 ][ 1 ], pv[ 3 ][ 3 ] + pv[ 3 ][ 1 ] );
      m_planes[ 4 ] = glm::vec4( pv[ 0 ][ 3 ] + pv[ 0 ][ 2 ], pv[ 1 ][ 3 ] + pv[ 1 ][ 2 ], pv[ 2 ][ 3 ] + pv[ 2 ][ 2 ], pv[ 3 ][ 3 ] + pv[ 3 ][ 2 ] );
      m_planes[ 5 ] = glm::vec4( pv[ 0 ][ 3 ] - pv[ 0 ][ 2 ], pv[ 1 ][ 3 ] - pv[ 1 ][ 2 ], pv[ 2 ][ 3 ] - pv[ 2 ][ 2 ], pv[ 3 ][ 3 ] - pv[ 3 ][ 2 ] );
      for( glm::vec4& plane : m_planes )
         plane /= glm::length( glm::vec3( plane ) );
   }

   // Check if point is inside frustum
   bool FInFrustum( const glm::vec3& point ) const
   {
      return std::none_of( m_planes.begin(), m_planes.end(), [ & ]( const glm::vec4& plane ) { return glm::dot( glm::vec3( plane ), point ) + plane.w < 0; } );
   }

   // Check if AABB is inside/intersecting frustum
   bool FInFrustum( const glm::vec3& min, const glm::vec3& max ) const
   {
      for( const glm::vec4& plane : m_planes )
      {
         const glm::vec3 point( plane.x >= 0 ? max.x : min.x, plane.y >= 0 ? max.y : min.y, plane.z >= 0 ? max.z : min.z );
         if( glm::dot( glm::vec3( plane ), point ) + plane.w < 0 )
            return false;
      }

      return true; // inside or intersecting
   }

private:
   std::array< glm::vec4, 6 > m_planes; // planes: left, right, top, bottom, near, far
};


std::optional< glm::ivec3 > g_highlightBlock;
static void                 RenderSystem( Entity::Registry& registry, Level& level, const CCamera& camera, const glm::vec3& camPosition )
{
   //PROFILE_SCOPE( "RenderSystem" );
   // TODO - refactor this system, Shader should be within component data for entity

   // hack to get working
   static std::unique_ptr< Shader > pTerrainShader = std::make_unique< Shader >( Shader::FILE, "terrain_vert.glsl", "terrain_frag.glsl" );

   pTerrainShader->Bind();                                       // Bind the shader program
   pTerrainShader->SetUniform( "u_MVP", camera.viewProjection ); // Set Model-View-Projection matrix
   pTerrainShader->SetUniform( "u_Model", glm::mat4( 1.0f ) );
   pTerrainShader->SetUniform( "u_viewPos", camPosition ); // Set camera position

   // Sun lighting (directional)
   {
      glm::vec3 sunDir       = glm::normalize( glm::vec3( -0.35f, 0.85f, -0.25f ) ); // points from fragment toward sun
      glm::vec3 sunColor     = glm::vec3( 1.0f, 0.98f, 0.92f ) * 3.0f;
      glm::vec3 ambientColor = glm::vec3( 0.12f, 0.16f, 0.22f );

      pTerrainShader->SetUniform( "u_sunDirection", sunDir );
      pTerrainShader->SetUniform( "u_sunColor", sunColor );
      pTerrainShader->SetUniform( "u_ambientColor", ambientColor );
   }

   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ); // clear color and depth buffers

   // Frustum for culling
   ViewFrustum frustum( camera.viewProjection );

   {
      TextureAtlasManager::Get().Bind();

      pTerrainShader->SetUniform( "u_blockTextures", 0 );

      for( const auto& [ coord, chunk ] : level.GetChunks() )
      {
         const ChunkRender* pRender = level.GetChunkRender( coord );
         if( !pRender )
            continue;

         const glm::vec3 chunkMin = glm::vec3( coord.x * CHUNK_SIZE_X, 0.0f, coord.z * CHUNK_SIZE_Z );
         const glm::vec3 chunkMax = chunkMin + glm::vec3( CHUNK_SIZE_X, CHUNK_SIZE_Y, CHUNK_SIZE_Z );
         if( !frustum.FInFrustum( chunkMin, chunkMax ) )
            continue;

         const glm::mat4 model = glm::translate( glm::mat4( 1.0f ), chunkMin );
         const glm::mat4 mvp   = camera.viewProjection * model;
         pTerrainShader->SetUniform( "u_MVP", mvp );
         pTerrainShader->SetUniform( "u_Model", model );
         pRender->Render();
      }

      TextureAtlasManager::Get().Unbind();
   }

   // Render all entities with CMesh component
   pTerrainShader->SetUniform( "u_MVP", camera.viewProjection );
   for( auto [ e, tran, mesh ] : registry.ECView< CTransform, CMesh >() )
   {
      if( CPhysics* pPhys = registry.TryGet< CPhysics >( e ) )
      {
         if( !frustum.FInFrustum( tran.position + pPhys->bbMin, tran.position + pPhys->bbMax ) )
            continue;
      }

      const glm::mat4 model = glm::translate( glm::mat4( 1.0f ), tran.position );
      const glm::mat4 mvp   = camera.viewProjection * model;
      //pTerrainShader->SetUniform( "u_color", mesh.mesh->GetColor() );
      pTerrainShader->SetUniform( "u_MVP", mvp );
      pTerrainShader->SetUniform( "u_Model", model );

      mesh.mesh->Render();
   }

   // Render CAABB bounds for debugging
   for( auto [ tran, phys ] : registry.CView< CTransform, CPhysics >() )
   {
      const glm::mat4 model = glm::translate( glm::mat4( 1.0f ), tran.position );
      const glm::mat4 mvp   = camera.viewProjection * model; // MVP = Projection * View * Model
      pTerrainShader->SetUniform( "u_MVP", mvp );

      // Build vertices relative to origin
      const glm::vec3 vertices[ 8 ] = {
         { phys.bbMin.x, phys.bbMin.y, phys.bbMin.z },
         { phys.bbMax.x, phys.bbMin.y, phys.bbMin.z },
         { phys.bbMax.x, phys.bbMax.y, phys.bbMin.z },
         { phys.bbMin.x, phys.bbMax.y, phys.bbMin.z },
         { phys.bbMin.x, phys.bbMin.y, phys.bbMax.z },
         { phys.bbMax.x, phys.bbMin.y, phys.bbMax.z },
         { phys.bbMax.x, phys.bbMax.y, phys.bbMax.z },
         { phys.bbMin.x, phys.bbMax.y, phys.bbMax.z }
      };

      constexpr unsigned int indices[ 24 ] = { 0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6, 6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7 };

      unsigned int vao, vbo, ebo;
      glGenVertexArrays( 1, &vao );
      glGenBuffers( 1, &vbo );
      glGenBuffers( 1, &ebo );

      glBindVertexArray( vao );
      glBindBuffer( GL_ARRAY_BUFFER, vbo );
      glBufferData( GL_ARRAY_BUFFER, sizeof( vertices ), vertices, GL_STATIC_DRAW );

      glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ebo );
      glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( indices ), indices, GL_STATIC_DRAW );

      glEnableVertexAttribArray( 0 );
      glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof( float ), ( void* )0 );

      glDrawElements( GL_LINES, 24, GL_UNSIGNED_INT, 0 );

      glDeleteBuffers( 1, &vbo );
      glDeleteBuffers( 1, &ebo );
      glDeleteVertexArrays( 1, &vao );
   }

   // Highlight block under crosshair
   {
      // Highlight current block
      if( g_highlightBlock.has_value() )
      {
         // Use the same shader; we draw a simple wireframe cube around the block
         const glm::vec3 blockPos = *g_highlightBlock;

         // Model: translate to block center, scale to slightly larger than 1x1x1
         glm::mat4 model = glm::translate( glm::mat4( 1.0f ), blockPos + glm::vec3( 0.5f ) );
         model           = glm::scale( model, glm::vec3( 1.02f ) ); // tiny inflation

         const glm::mat4 mvp = camera.viewProjection * model;
         pTerrainShader->SetUniform( "u_MVP", mvp );
         //pTerrainShader->SetUniform( "u_color", glm::vec3( 1.0f, 1.0f, 1.0f ) ); // white

         // Unit cube wireframe in local space [-0.5,0.5]^3
         const glm::vec3 min           = glm::vec3( -0.5f );
         const glm::vec3 max           = glm::vec3( 0.5f );
         const glm::vec3 vertices[ 8 ] = {
            { min.x, min.y, min.z },
            { max.x, min.y, min.z },
            { max.x, max.y, min.z },
            { min.x, max.y, min.z },
            { min.x, min.y, max.z },
            { max.x, min.y, max.z },
            { max.x, max.y, max.z },
            { min.x, max.y, max.z },
         };

         constexpr unsigned int indices[ 24 ] = {
            0, 1, 1, 2, 2, 3, 3, 0, // bottom
            4, 5, 5, 6, 6, 7, 7, 4, // top
            0, 4, 1, 5, 2, 6, 3, 7  // verticals
         };

         GLuint vao = 0, vbo = 0, ebo = 0;
         glGenVertexArrays( 1, &vao );
         glGenBuffers( 1, &vbo );
         glGenBuffers( 1, &ebo );

         glBindVertexArray( vao );
         glBindBuffer( GL_ARRAY_BUFFER, vbo );
         glBufferData( GL_ARRAY_BUFFER, sizeof( vertices ), vertices, GL_STATIC_DRAW );

         glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ebo );
         glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( indices ), indices, GL_STATIC_DRAW );

         glEnableVertexAttribArray( 0 );
         glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof( float ), ( void* )0 );

         glDrawElements( GL_LINES, 24, GL_UNSIGNED_INT, 0 );

         glDeleteBuffers( 1, &vbo );
         glDeleteBuffers( 1, &ebo );
         glDeleteVertexArrays( 1, &vao );
      }
   }

   pTerrainShader->Unbind();

   // Draw Skybox
   {
      constexpr std::string_view faceFiles[ 6 ] = { "assets/textures/skybox/px.png", "assets/textures/skybox/nx.png", "assets/textures/skybox/py.png",
                                                    "assets/textures/skybox/ny.png", "assets/textures/skybox/pz.png", "assets/textures/skybox/nz.png" };
      static SkyboxTexture       skyboxTexture( faceFiles );
      skyboxTexture.Draw( camera.view, camera.projection );
   }

   // Draw simple reticle
   {
      pTerrainShader->Bind();

      glDisable( GL_DEPTH_TEST );
      pTerrainShader->SetUniform( "u_MVP", glm::mat4( 1.0f ) );
      //pTerrainShader->SetUniform( "u_color", glm::vec3( 1.0f, 1.0f, 1.0f ) );

      const float                r            = 0.01f; // reticle arm length in NDC
      std::array< glm::vec3, 4 > reticleVerts = {
         { { -r, 0.0f, 0.0f }, { r, 0.0f, 0.0f }, { 0.0f, -r, 0.0f }, { 0.0f, r, 0.0f } }
      };

      GLuint reticleVao = 0, reticleVbo = 0;
      glGenVertexArrays( 1, &reticleVao );
      glGenBuffers( 1, &reticleVbo );
      glBindVertexArray( reticleVao );
      glBindBuffer( GL_ARRAY_BUFFER, reticleVbo );
      glBufferData( GL_ARRAY_BUFFER, sizeof( reticleVerts ), reticleVerts.data(), GL_STATIC_DRAW );
      glEnableVertexAttribArray( 0 );
      glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof( float ), ( void* )0 );
      glDrawArrays( GL_LINES, 0, reticleVerts.size() );
      glDeleteBuffers( 1, &reticleVbo );
      glDeleteVertexArrays( 1, &reticleVao );
      glEnable( GL_DEPTH_TEST );

      pTerrainShader->Unbind();
   }
}

struct CTick
{
   uint32_t currentTick = 0;
   uint32_t maxTicks    = 0;
};

static void TickingSystem( Entity::Registry& registry, float /*delta*/ )
{
   //PROFILE_SCOPE( "TickingSystem" );
   std::unordered_set< Entity::Entity > entitiesToDestory;
   for( auto [ e, tick ] : registry.ECView< CTick >() )
   {
      if( tick.currentTick++ >= tick.maxTicks )
         entitiesToDestory.insert( e );
   }

   for( Entity::Entity e : entitiesToDestory )
      registry.Destroy( e );
}


static float RandRange( float min, float max )
{
   thread_local static std::mt19937                            rng( std::random_device {}() );
   thread_local static std::uniform_real_distribution< float > dist( 0.0f, 1.0f );
   return min + ( dist( rng ) * ( max - min ) );
}


static glm::mat4 CreateProjectionMatrix( uint16_t screenWidth, uint16_t screenHeight, float fov, float nearPlane, float farPlane )
{
   return glm::perspective( glm::radians( fov ), static_cast< float >( screenWidth ) / static_cast< float >( screenHeight ), nearPlane, farPlane );
}


void Application::Run()
{
   Window&     window     = Window::Get();
   WindowData& windowData = window.GetWindowData();
   window.Init();

   Level level;

   // ECS Registry
   Entity::Registry registry;

   Entity::Entity player = registry.Create();
   registry.Add< CTransform >( player, 0.0f, 128.0f, 0.0f );
   registry.Add< CVelocity >( player, 0.0f, 0.0f, 0.0f );
   registry.Add< CInput >( player );
   registry.Add< CPlayerTag >( player );
   registry.Add< CPhysics >( player, CPhysics { .bbMin = glm::vec3( -0.3f, 0.0f, -0.3f ), .bbMax = glm::vec3( 0.3f, PLAYER_HEIGHT, 0.3f ) } );

   Entity::Entity camera = registry.Create();
   registry.Add< CTransform >( camera, 0.0f, 128.0f + PLAYER_EYE_HEIGHT, 0.0f );
   registry.Add< CLookInput >( camera );
   if( CCamera* pCamera = &registry.Add< CCamera >( camera ) )
      pCamera->projection = CreateProjectionMatrix( windowData.Width, windowData.Height, pCamera->fov, 0.1f, 1000.0f );
   registry.Add< CCameraRig >( camera,
                               CCameraRig {
                                  .targetEntity = player,
                                  .offset       = glm::vec3( 0.0f, PLAYER_EYE_HEIGHT, 0.0f ),
                                  .followYaw    = true,
                                  .followPitch  = false,
                               } );
   //registry.Add< CParent >( camera, player, glm::vec3( 0.0f, PLAYER_EYE_HEIGHT, 0.0f ) );

   std::shared_ptr< Entity::EntityHandle > psBall = registry.CreateWithHandle();
   registry.Add< CTransform >( psBall->Get(), 5.0f, 128.0f, 5.0f );
   registry.Add< CMesh >( psBall->Get(), std::make_shared< SphereMesh >() );
   registry.Add< CVelocity >( psBall->Get(), glm::vec3( 0.0f, 0.0f, 0.0f ) );
   registry.Add< CPhysics >( psBall->Get(), CPhysics { .bbMin = glm::vec3( -0.5f ), .bbMax = glm::vec3( 0.5f ) } );

   std::shared_ptr< Entity::EntityHandle > psCube = registry.CreateWithHandle();
   registry.Add< CTransform >( psCube->Get(), 6.0f, 128.0f, 8.0f );
   registry.Add< CMesh >( psCube->Get(), std::make_shared< CubeMesh >() );
   registry.Add< CVelocity >( psCube->Get(), glm::vec3( 0.0f, 0.0f, 0.0f ) );
   registry.Add< CPhysics >( psCube->Get(), CPhysics { .bbMin = glm::vec3( -0.5f ), .bbMax = glm::vec3( 0.5f ) } );

   Events::EventSubscriber eventSubscriber;
   eventSubscriber.Subscribe< Events::WindowResizeEvent >( [ & ]( const auto& e ) noexcept
   {
      if( e.GetHeight() == 0 )
         return;

      if( CCamera* pCamera = registry.TryGet< CCamera >( camera ) )
         pCamera->projection = CreateProjectionMatrix( e.GetWidth(), e.GetHeight(), pCamera->fov, 0.1f, 1000.0f );
   } );

   eventSubscriber.Subscribe< Events::MouseScrolledEvent >( [ & ]( const auto& e ) noexcept
   {
      if( CCamera* pCamera = registry.TryGet< CCamera >( camera ) )
      {
         pCamera->fov        = std::clamp( pCamera->fov - ( e.GetYOffset() * 5 ), 10.0f, 90.0f );
         pCamera->projection = CreateProjectionMatrix( windowData.Width, windowData.Height, pCamera->fov, 0.1f, 1000.0f );
      }
   } );

   eventSubscriber.Subscribe< Events::KeyPressedEvent >( [ & ]( const auto& e ) noexcept
   {
      switch( e.GetKeyCode() )
      {
         case Input::KeyCode::R: registry.Get< CTransform >( player ).position.y = 128.0f; break;
      }
   } );

   eventSubscriber.Subscribe< Events::MouseButtonPressedEvent >( [ & ]( const auto& e ) noexcept
   {
      //if( e.GetMouseButton() != Input::ButtonLeft && e.GetMouseButton() != Input::ButtonRight && e.GetMouseButton() != Input::ButtonMiddle )
      //   return;

      CTransform* pCamTran = registry.TryGet< CTransform >( camera );
      if( !pCamTran )
         return;

      const float pitch = glm::radians( pCamTran->rotation.x );
      const float yaw   = glm::radians( pCamTran->rotation.y );

      glm::vec3 direction { glm::cos( pitch ) * glm::sin( yaw ), -glm::sin( pitch ), -glm::cos( pitch ) * glm::cos( yaw ) };
      direction = glm::normalize( direction );

      const glm::vec3 rayOrigin   = pCamTran->position;
      constexpr float maxDistance = 64.0f;
      if( e.GetMouseButton() == Input::ButtonLeft )
      {
         constexpr int   NUM_PROJECTILES = 500;
         constexpr float CONE_ANGLE_DEG  = 8.0f;
         constexpr float SPEED           = 50.0f;

         glm::vec3 right = glm::normalize( ( glm::abs( direction.y ) < 0.99f ) ? glm::cross( direction, glm::vec3( 0, 1, 0 ) )
                                                                               : glm::cross( direction, glm::vec3( 1, 0, 0 ) ) );
         glm::vec3 up    = glm::cross( right, direction );

         float     cosMax   = glm::cos( glm::radians( CONE_ANGLE_DEG ) );
         glm::vec3 spawnPos = rayOrigin + direction;
         for( int i = 0; i < NUM_PROJECTILES; ++i )
         {
            // Uniform random direction in cone
            float u        = RandRange( 0.0f, 1.0f );
            float v        = RandRange( 0.0f, 1.0f );
            float cosTheta = glm::mix( cosMax, 1.0f, u );
            float sinTheta = std::sqrt( 1.0f - cosTheta * cosTheta );
            float phi      = glm::two_pi< float >() * v;

            glm::vec3 spreadDir = direction * cosTheta + right * ( std::cos( phi ) * sinTheta ) + up * ( std::sin( phi ) * sinTheta );

            Entity::Entity projectile = registry.Create();
            registry.Add< CTransform >( projectile, spawnPos );
            registry.Add< CMesh >( projectile, std::make_shared< SphereMesh >( 0.1f ) );
            registry.Add< CVelocity >( projectile, spreadDir * SPEED );
            registry.Add< CPhysics >( projectile, CPhysics { .bbMin = glm::vec3( -0.05f ), .bbMax = glm::vec3( 0.05f ), .bounciness = 0.8f } );
            registry.Add< CTick >( projectile, 0, 200 );
         }

         return;
      }

      if( std::optional< RaycastResult > result = TryRaycast( level, Ray { rayOrigin, direction, maxDistance } ) )
      {
         std::cout << "Raycast hit at position: " << result->point.x << ", " << result->point.y << ", " << result->point.z << "\n";
         switch( e.GetMouseButton() )
         {
            case Input::ButtonLeft:   level.SetBlock( result->block, BlockId::Air /*id*/ ); break;
            case Input::ButtonRight:  level.SetBlock( result->block + result->faceNormal, BlockId::Grass /*id*/ ); break;
            case Input::ButtonMiddle: level.Explode( result->block, 36 /*radius*/ ); break;
         }
      }
      else
         std::cout << "Raycast did not hit any block within " << maxDistance << " units.\n";
   } );
   // -----------------------------------

   // Initialize things
   TextureAtlasManager::Get().CompileBlockAtlas();

   GUIManager::Init();
   INetwork::RegisterGUI();

   Timestep timestep( 20 /*tickrate*/ );
   GUIManager::Attach( std::make_shared< DebugGUI >( registry, player, camera, timestep ) );

   while( window.IsOpen() )
   {
      const float delta = std::clamp( timestep.Step(), 0.0f, 0.05f /*50ms*/ ); // time since last frame, max 50ms

      CTransform* pPlayerTran = registry.TryGet< CTransform >( player );
      CTransform* pCameraTran = registry.TryGet< CTransform >( camera );
      CCamera*    pCameraComp = registry.TryGet< CCamera >( camera );

      {
         if( pPlayerTran )
            level.UpdateVisibleRegion( pPlayerTran->position, 8 /*viewRadius*/ );

         Events::ProcessQueuedEvents();

         MouseLookSystem( registry, window ); // input -> look intent
         CameraRigSystem( registry );         // apply look to camera, drive player yaw/pitch, position camera

         // Player systems - input and physics
         PlayerInputSystem( registry, delta );
         PlayerPhysicsSystem( registry, level, delta );

         if( timestep.FDidTick() )
         {
            TickingSystem( registry, delta );

            if( INetwork* pNetwork = INetwork::Get(); pNetwork && pPlayerTran )
               pNetwork->Poll( pPlayerTran->position );
         }

         if( window.FMinimized() || !( pCameraComp && pCameraTran ) )
            continue;

         CameraViewSystem( registry ); // compute view matrices for render (maybe merge with render system)

         {
            // Update which block is being looked at
            auto optResult   = TryRaycast( level, CreateRay( pCameraTran->position, pCameraTran->rotation, 64.0f /*maxDistance*/ ) );
            g_highlightBlock = optResult ? std::optional< glm::ivec3 >( optResult->block ) : std::nullopt;
         }

         // Render
         RenderSystem( registry, level, *pCameraComp, pCameraTran->position );
         GUIManager::Draw();
      }

      window.OnUpdate();
   }

   GUIManager::Shutdown();
}
