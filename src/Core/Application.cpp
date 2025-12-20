#include "Application.h"

// Local dependencies
#include "GUIManager.h"
#include "Timestep.h"
#include "Window.h"

// Project dependencies
#include <Events/ApplicationEvent.h>
#include <Events/KeyEvent.h>
#include <Events/MouseEvent.h>
#include <Input/Input.h>
#include <Network/Network.h>
#include <World.h>

#include <World/Level.h>
#include <Renderer/Shader.h>

// Component includes
#include <Entity/Components.h>
#include <Entity/Registry.h>

// ================ SYSTEMS ================
constexpr float GRAVITY           = -32.0f;
constexpr float TERMINAL_VELOCITY = -48.0f;
constexpr float JUMP_VELOCITY     = 9.0f;
constexpr float PLAYER_HEIGHT     = 1.8f;
constexpr float PLAYER_EYE_HEIGHT = 1.62f;
constexpr float GROUND_MAXSPEED   = 4.3f;
constexpr float AIR_MAXSPEED      = 10.0f;
constexpr float AIR_CONTROL       = 0.3f;
constexpr float SPRINT_MODIFIER   = 1.5f;

void PlayerInputSystem( Entity::Registry& registry, float delta )
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

void PlayerPhysicsSystem( Entity::Registry& registry, Level& level, float delta )
{
   // Integration System
   {
      //PROFILE_SCOPE( "PlayerPhysicsSystem::Integration" );
      for( auto [ pTran, pVel ] : registry.CView< CTransform, CVelocity >() )
      {
         // Apply gravity
         pVel.velocity.y = ( std::max )( pVel.velocity.y + ( GRAVITY * delta ), TERMINAL_VELOCITY );

         // Integrate position
         pTran.position += pVel.velocity * delta;
      }
   }

   // Entity vs World Collision System
   {
      //PROFILE_SCOPE( "PlayerPhysicsSystem::EntityVsWorldCollision" );
      auto overlapsSolid = [ & ]( const glm::vec3& pos, const glm::vec3& halfExtents )
      {
         int minX = ( int )std::floor( pos.x - halfExtents.x );
         int maxX = ( int )std::floor( pos.x + halfExtents.x );
         int minY = ( int )std::floor( pos.y - halfExtents.y );
         int maxY = ( int )std::floor( pos.y + halfExtents.y );
         int minZ = ( int )std::floor( pos.z - halfExtents.z );
         int maxZ = ( int )std::floor( pos.z + halfExtents.z );

         for( int x = minX; x <= maxX; ++x )
            for( int y = minY; y <= maxY; ++y )
               for( int z = minZ; z <= maxZ; ++z )
                  if( level.GetBlock( x, y, z ) != BLOCK_AIR )
                     return true;

         return false;
      };

      for( auto [ tran, vel, box ] : registry.CView< CTransform, CVelocity, CAABB >() )
      {
         glm::vec3 newPos = tran.position;
         glm::vec3 step   = vel.velocity * delta;

         // Y axis
         newPos.y += step.y;
         if( overlapsSolid( newPos, box.halfExtents ) )
         {
            if( step.y > 0.0f )
               newPos.y = std::floor( newPos.y + box.halfExtents.y ) - box.halfExtents.y;
            else
               newPos.y = std::floor( newPos.y - box.halfExtents.y ) + 1.0f + box.halfExtents.y;
            vel.velocity.y = 0.0f;
         }

         // X axis
         newPos.x += step.x;
         if( overlapsSolid( newPos, box.halfExtents ) )
         {
            if( step.x > 0.0f )
               newPos.x = std::floor( newPos.x + box.halfExtents.x ) - box.halfExtents.x;
            else
               newPos.x = std::floor( newPos.x - box.halfExtents.x ) + 1.0f + box.halfExtents.x;
            vel.velocity.x = 0.0f;
         }

         // Z axis
         newPos.z += step.z;
         if( overlapsSolid( newPos, box.halfExtents ) )
         {
            if( step.z > 0.0f )
               newPos.z = std::floor( newPos.z + box.halfExtents.z ) - box.halfExtents.z;
            else
               newPos.z = std::floor( newPos.z - box.halfExtents.z ) + 1.0f + box.halfExtents.z;
            vel.velocity.z = 0.0f;
         }

         tran.position = newPos;
      }
   }

   // Collision System
   {
      //PROFILE_SCOPE( "PlayerPhysicsSystem::Collision" );
      auto aabbOverlap = []( const glm::vec3& aPos, const CAABB& a, const glm::vec3& bPos, const CAABB& b ) -> bool
      {
         return std::abs( aPos.x - bPos.x ) <= ( a.halfExtents.x + b.halfExtents.x ) && std::abs( aPos.y - bPos.y ) <= ( a.halfExtents.y + b.halfExtents.y ) &&
                std::abs( aPos.z - bPos.z ) <= ( a.halfExtents.z + b.halfExtents.z );
      };

      for( auto [ cTran1, cVel1, cBox1 ] : registry.CView< CTransform, CVelocity, CAABB >() )
      {
         for( auto [ cTran2, cBox2 ] : registry.CView< CTransform, CAABB >() )
         {
            if( &cBox1 == &cBox2 ) // skip self
               continue;

            if( !aabbOverlap( cTran1.position, cBox1, cTran2.position, cBox2 ) )
               continue;

            float bottom = cTran1.position.y - cBox1.halfExtents.y;
            float top    = cTran2.position.y + cBox2.halfExtents.y;
            if( cVel1.velocity.y <= 0.0f && bottom < top )
            {
               cTran1.position.y = top + cBox1.halfExtents.y;
               cVel1.velocity.y  = 0.0f;
               break;
            }
         }
      }
   }

   // Dynamic vs Static
   //    Player and other moving objects collide against world blocks.
   // Dynamic vs Dynamic
   //    Player vs moving platforms, other players, etc.
}

void CameraSystem( Entity::Registry& registry, Window& window )
{
   //PROFILE_SCOPE( "CameraSystem" );
   GLFWwindow* nativeWindow   = window.GetNativeWindow();
   bool        fMouseCaptured = glfwGetInputMode( nativeWindow, GLFW_CURSOR ) == GLFW_CURSOR_DISABLED;

   glm::vec2 mouseDelta { 0.0f };
   if( fMouseCaptured )
   {
      static glm::vec2 lastMousePos = window.GetMousePosition();
      mouseDelta                    = window.GetMousePosition() - lastMousePos;

      const WindowData& windowData = window.GetWindowData();
      lastMousePos                 = { windowData.Width * 0.5f, windowData.Height * 0.5f };
      glfwSetCursorPos( nativeWindow, lastMousePos.x, lastMousePos.y );
   }

   for( auto [ tran, cam, tag ] : registry.CView< CTransform, CCamera, CCameraTag >() )
   {
      if( fMouseCaptured )
      {
         tran.rotation.x = glm::clamp( tran.rotation.x + ( mouseDelta.y * cam.sensitivity ), -90.0f, 90.0f );
         tran.rotation.y = glm::mod( tran.rotation.y + ( mouseDelta.x * cam.sensitivity ) + 360.0f, 360.0f );
      }

      cam.view = glm::mat4( 1.0f );
      cam.view = glm::rotate( cam.view, glm::radians( tran.rotation.x ), { 1, 0, 0 } );
      cam.view = glm::rotate( cam.view, glm::radians( tran.rotation.y ), { 0, 1, 0 } );
      cam.view = glm::rotate( cam.view, glm::radians( tran.rotation.z ), { 0, 0, 1 } );
      cam.view = glm::translate( cam.view, -tran.position );

      cam.viewProjection = cam.projection * cam.view;
   }
}

void RenderSystem( Entity::Registry& registry, Level& level, const glm::vec3& camPosition, const glm::mat4& viewProjection )
{
   //PROFILE_SCOPE( "RenderSystem" );
   // TODO - refactor this system, Shader should be within component data for entity

   // hack to get working
   static std::unique_ptr< Shader > pShader = std::make_unique< Shader >();

   pShader->Bind();                                  // Bind the shader program
   pShader->SetUniform( "u_MVP", viewProjection );   // Set Model-View-Projection matrix
   pShader->SetUniform( "u_viewPos", camPosition );  // Set camera position
   pShader->SetUniform( "u_lightPos", camPosition ); // Set light position at camera position

   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ); // clear color and depth buffers

   {
      pShader->SetUniform( "u_color", glm::vec3( 1.0f, 1.0f, 1.0f ) );
      for( const auto& [ coord, pChunk ] : level.GetChunks() )
      {
         if( !pChunk )
            continue;

         const ChunkRender* pRender = level.GetChunkRender( coord );
         if( !pRender )
            continue;

         glm::vec3 chunkOffset( coord.x * CHUNK_SIZE_X, 0 /*y*/, coord.z * CHUNK_SIZE_Z );
         glm::mat4 model = glm::translate( glm::mat4( 1.0f ), chunkOffset );
         glm::mat4 mvp   = viewProjection * model;
         pShader->SetUniform( "u_MVP", mvp );
         pRender->Render();
      }
   }

   // Render all entities with CMesh component
   pShader->SetUniform( "u_MVP", viewProjection ); // Set Model-View-Projection matrix
   for( auto [ tran, mesh ] : registry.CView< CTransform, CMesh >() )
   {
      pShader->SetUniform( "u_color", mesh.mesh->GetColor() );

      mesh.mesh->SetPosition( tran.position ); // this shouldn't be here
      mesh.mesh->Render();
   }

   // Render CAABB bounds for debugging
   for( auto [ tran, box ] : registry.CView< CTransform, CAABB >() )
   {
      glm::mat4 model = glm::translate( glm::mat4( 1.0f ), tran.position );
      glm::mat4 mvp   = viewProjection * model; // MVP = Projection * View * Model
      pShader->SetUniform( "u_MVP", mvp );

      // Build vertices relative to origin
      glm::vec3 min           = -box.halfExtents;
      glm::vec3 max           = box.halfExtents;
      glm::vec3 vertices[ 8 ] = {
         { min.x, min.y, min.z },
         { max.x, min.y, min.z },
         { max.x, max.y, min.z },
         { min.x, max.y, min.z },
         { min.x, min.y, max.z },
         { max.x, min.y, max.z },
         { max.x, max.y, max.z },
         { min.x, max.y, max.z }
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

   // Draw simple plus-shaped reticle at the center of the screen in NDC.
   // Assumes the main shader uses clip-space positions when u_MVP is identity.
   glDisable( GL_DEPTH_TEST );
   pShader->SetUniform( "u_MVP", glm::mat4( 1.0f ) );
   pShader->SetUniform( "u_color", glm::vec3( 1.0f, 1.0f, 1.0f ) );

   const float r                 = 0.01f; // reticle arm length in NDC
   glm::vec3   reticleVerts[ 4 ] = {
      { -r,   0.0f, 0.0f },
      { r,    0.0f, 0.0f }, // horizontal
      { 0.0f, -r,   0.0f },
      { 0.0f, r,    0.0f }  // vertical
   };

   GLuint reticleVao = 0, reticleVbo = 0;
   glGenVertexArrays( 1, &reticleVao );
   glGenBuffers( 1, &reticleVbo );
   glBindVertexArray( reticleVao );
   glBindBuffer( GL_ARRAY_BUFFER, reticleVbo );
   glBufferData( GL_ARRAY_BUFFER, sizeof( reticleVerts ), reticleVerts, GL_STATIC_DRAW );
   glEnableVertexAttribArray( 0 );
   glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof( float ), ( void* )0 );
   glDrawArrays( GL_LINES, 0, 4 );
   glDeleteBuffers( 1, &reticleVbo );
   glDeleteVertexArrays( 1, &reticleVao );
   glEnable( GL_DEPTH_TEST );

   pShader->Unbind();
}

void TickingSystem( Entity::Registry& /*registry*/, float /*delta*/ )
{
   //PROFILE_SCOPE( "TickingSystem" );
}

struct Ray
{
   glm::vec3 origin;
   glm::vec3 direction;
};

struct RaycastResult
{
   glm::vec3  position;
   glm::ivec3 normal;
   float      distance;
};

// add debug rendering for Raycasts

std::optional< RaycastResult > DoRaycast( const Level& level, const Ray& ray, float maxDistance )
{
   PROFILE_SCOPE( "DoRaycast" );

   glm::vec3 dir = glm::normalize( ray.direction );
   if( glm::dot( dir, dir ) < 1e-12f )
      return std::nullopt;

   // Current voxel
   glm::ivec3 blockPos = glm::floor( ray.origin );

   // If we start inside a block, hit it immediately
   // if( FIsSolid( level.GetBlock( blockPos.x, blockPos.y, blockPos.z ) ) )
   if( level.GetBlock( blockPos.x, blockPos.y, blockPos.z ) != BLOCK_AIR )
      return RaycastResult { glm::vec3( blockPos ), glm::ivec3( 0 ) /*normal*/, 0.0f /*distance*/ };

   // DDA setup
   glm::ivec3 step( dir.x < 0.0f ? -1 : 1, dir.y < 0.0f ? -1 : 1, dir.z < 0.0f ? -1 : 1 );
   glm::vec3  invDir( dir.x != 0.0f ? 1.0f / dir.x : std::numeric_limits< float >::infinity(),
                     dir.y != 0.0f ? 1.0f / dir.y : std::numeric_limits< float >::infinity(),
                     dir.z != 0.0f ? 1.0f / dir.z : std::numeric_limits< float >::infinity() );

   // Distance from origin to first voxel boundary on each axis
   glm::vec3 tMax;
   for( int i = 0; i < 3; ++i )
      tMax[ i ] = ( static_cast< float >( blockPos[ i ] ) + ( step[ i ] > 0 ? 1.0f : 0.0f ) - ray.origin[ i ] ) * invDir[ i ];

   float      dist      = 0.0f;
   glm::vec3  tDelta    = glm::abs( invDir );
   glm::ivec3 hitNormal = glm::ivec3( 0 );
   while( dist <= maxDistance )
   {
      // TODO: integrate world access and solidity check here.
      // if( FIsSolid( level.GetBlock( blockPos.x, blockPos.y, blockPos.z ) ) )
      if( level.GetBlock( blockPos.x, blockPos.y, blockPos.z ) != BLOCK_AIR )
         return RaycastResult { glm::vec3( blockPos ), hitNormal, dist };

      // Determine next axis to step
      int axis = 0;
      if( tMax.y < tMax.x )
         axis = 1;
      if( tMax.z < tMax[ axis ] )
         axis = 2;

      dist = tMax[ axis ];
      tMax[ axis ] += tDelta[ axis ];
      blockPos[ axis ] += step[ axis ];
      hitNormal         = glm::ivec3( 0 );
      hitNormal[ axis ] = -step[ axis ];
   }

   return std::nullopt;
}

void Application::Run()
{
   Entity::Registry registry;

   // Create player entity
   Entity::Entity player = registry.Create();
   registry.Add< CTransform >( player, 0.0f, 128.0f + ( PLAYER_HEIGHT / 2 ), 0.0f );
   registry.Add< CVelocity >( player, 0.0f, 0.0f, 0.0f );
   registry.Add< CInput >( player );
   registry.Add< CPlayerTag >( player );
   registry.Add< CAABB >( player, glm::vec3( 0.3f, 0.9f, 0.3f ) ); // center at body

   // Create camera entity
   Entity::Entity camera = registry.Create();
   registry.Add< CTransform >( camera, 0.0f, 128.0f + PLAYER_EYE_HEIGHT, 0.0f );
   registry.Add< CCameraTag >( camera );

   if( CCamera* pCameraComp = &registry.Add< CCamera >( camera ) )
      pCameraComp->projection = glm::perspective( glm::radians( pCameraComp->fov ),
                                                  Window::Get().GetWindowData().Width / static_cast< float >( Window::Get().GetWindowData().Height ),
                                                  0.1f,
                                                  1000.0f );

   Events::EventSubscriber eventSubscriber;
   eventSubscriber.Subscribe< Events::WindowResizeEvent >( [ &registry, camera ]( const Events::WindowResizeEvent& e ) noexcept
   {
      if( e.GetHeight() == 0 )
         return;

      if( CCamera* pCameraComp = registry.TryGet< CCamera >( camera ) )
         pCameraComp->projection = glm::perspective( glm::radians( pCameraComp->fov ), e.GetWidth() / static_cast< float >( e.GetHeight() ), 0.1f, 1000.0f );
   } );

   eventSubscriber.Subscribe< Events::MouseScrolledEvent >( [ &registry, camera ]( const Events::MouseScrolledEvent& e ) noexcept
   {
      if( CCamera* pCameraComp = registry.TryGet< CCamera >( camera ) )
      {
         pCameraComp->fov        = std::clamp( pCameraComp->fov - ( e.GetYOffset() * 5 ), 10.0f, 90.0f );
         pCameraComp->projection = glm::perspective( glm::radians( pCameraComp->fov ),
                                                     Window::Get().GetWindowData().Width / static_cast< float >( Window::Get().GetWindowData().Height ),
                                                     0.1f,
                                                     1000.0f );
      }
   } );

   Level level;
   eventSubscriber.Subscribe< Events::MouseButtonPressedEvent >( [ & ]( const Events::MouseButtonPressedEvent& e ) noexcept
   {
      if( e.GetMouseButton() != Input::ButtonLeft )
         return;

      // Get camera transform
      CTransform* pCamTran = registry.TryGet< CTransform >( camera );
      CCamera*    pCamComp = registry.TryGet< CCamera >( camera );
      if( !pCamTran || !pCamComp )
         return;

      // Camera forward vector from yaw (y) and pitch (x). This is equivalent
      // to rotating the default forward (0,0,-1) by pitch then yaw.
      float pitch = glm::radians( pCamTran->rotation.x );
      float yaw   = glm::radians( pCamTran->rotation.y );

      glm::vec3 dir;
      dir.x = glm::cos( pitch ) * glm::sin( yaw );
      dir.y = -glm::sin( pitch );
      dir.z = -glm::cos( pitch ) * glm::cos( yaw );
      dir   = glm::normalize( dir );

      glm::vec3 rayOrigin   = pCamTran->position;
      float     maxDistance = 64.0f;

      if( std::optional< RaycastResult > result = DoRaycast( level, Ray { rayOrigin, dir }, maxDistance ) )
      {
         std::cout << "Raycast hit at position: " << result->position.x << ", " << result->position.y << ", " << result->position.z << "\n";
         level.SetBlock( static_cast< int >( result->position.x ),
                         static_cast< int >( result->position.y ),
                         static_cast< int >( result->position.z ),
                         BLOCK_AIR );
      }
      else
         std::cout << "Raycast did not hit any block within " << maxDistance << " units.\n";
   } );
   // -----------------------------------

   // Initialize things
   Window& window = Window::Get();
   window.Init();
   GUIManager::Init();
   INetwork::RegisterGUI();

   Timestep timestep( 20 /*tickrate*/ );
   GUIManager::Attach( std::make_shared< DebugGUI >( registry, player, camera, timestep ) );

   std::shared_ptr< Entity::EntityHandle > psBall = registry.CreateWithHandle();
   registry.Add< CTransform >( psBall->Get(), 5.0f, 128.0f, 5.0f );
   registry.Add< CMesh >( psBall->Get(), std::make_shared< SphereMesh >() );
   registry.Add< CVelocity >( psBall->Get(), glm::vec3( 0.0f, 0.0f, 0.0f ) );
   registry.Add< CAABB >( psBall->Get(), CAABB { glm::vec3( 0.5f ) } );

   std::shared_ptr< Entity::EntityHandle > psCube = registry.CreateWithHandle();
   registry.Add< CTransform >( psCube->Get(), 6.0f, 128.0f, 8.0f );
   registry.Add< CMesh >( psCube->Get(), std::make_shared< CubeMesh >() );
   registry.Add< CVelocity >( psCube->Get(), glm::vec3( 0.0f, 0.0f, 0.0f ) );
   registry.Add< CAABB >( psCube->Get(), CAABB { glm::vec3( 1.0f ) } );

   while( window.IsOpen() )
   {
      const float delta = timestep.Step();

      CTransform* pPlayerTran = registry.TryGet< CTransform >( player );
      CTransform* pCameraTran = registry.TryGet< CTransform >( camera );
      CCamera*    pCameraComp = registry.TryGet< CCamera >( camera );

      {
         if( pPlayerTran )
            level.UpdateVisibleRegion( pPlayerTran->position, 2 /*viewRadius*/ );

         Events::ProcessQueuedEvents();
         // ProcessQueuedNetworkEvents(); - maybe do this here?

         PlayerInputSystem( registry, delta );
         PlayerPhysicsSystem( registry, level, delta );
         CameraSystem( registry, window );

         if( timestep.FTick() )
         {
            TickingSystem( registry, delta ); // TODO: implement this system (with CTick component)

            // remove this eventually
            if( INetwork* pNetwork = INetwork::Get(); pNetwork && pPlayerTran )
               pNetwork->Poll( pPlayerTran->position );
         }

         if( pPlayerTran && pCameraTran ) // hacky way to set camera position to follow player (update before render, should change)
         {
            pCameraTran->position = pPlayerTran->position + glm::vec3( 0.0f, PLAYER_EYE_HEIGHT - ( PLAYER_HEIGHT / 2 ), 0.0f );
            pPlayerTran->rotation = pCameraTran->rotation;
         }

         if( !window.FMinimized() && pCameraComp && pCameraTran )
         {
            RenderSystem( registry, level, pCameraTran->position, pCameraComp->viewProjection );
            GUIManager::Draw();
         }
      }

      window.OnUpdate();
   }

   // Shutdown everything
   GUIManager::Shutdown();
}
