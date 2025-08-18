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

#include <Renderer/Shader.h>

// Component includes
#include <Entity/Registry.h>
#include <Entity/Components/Transform.h>
#include <Entity/Components/Velocity.h>
#include <Entity/Components/Input.h>
#include <Entity/Components/Camera.h>
#include <Entity/Components/PlayerTag.h>
#include <Entity/Components/CameraTag.h>
#include <Entity/Components/Parent.h>
#include <Entity/Components/Children.h>
#include <Entity/Components/Mesh.h>

// ================ SYSTEMS ================
constexpr float GRAVITY           = -32.0f;
constexpr float TERMINAL_VELOCITY = -48.0f;
constexpr float JUMP_VELOCITY     = 9.0f;
constexpr float PLAYER_HEIGHT     = 1.9f;
constexpr float GROUND_MAXSPEED   = 12.0f;
constexpr float AIR_MAXSPEED      = 10.0f;
constexpr float AIR_CONTROL       = 0.3f;
constexpr float SPRINT_MODIFIER   = 1.5f;

void PlayerInputSystem( Entity::Registry& registry, float delta )
{
   //PROFILE_SCOPE( "PlayerInputSystem" );
   for( auto [ entity, tran, vel, input, tag ] : registry.ECView< CTransform, CVelocity, CInput, CPlayerTag >() )
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

void PlayerPhysicsSystem( Entity::Registry& registry, float delta, World& world )
{
   //PROFILE_SCOPE( "PlayerPhysicsSystem" );
   for( auto [ tran, vel, tag ] : registry.CView< CTransform, CVelocity, CPlayerTag >() )
   {
      vel.velocity.y += GRAVITY * delta;
      if( vel.velocity.y < TERMINAL_VELOCITY )
         vel.velocity.y = TERMINAL_VELOCITY;

      tran.position += vel.velocity * delta;
      float edgeHeight   = ( std::max )( { world.GetHeightAtPos( tran.position.x + 0.5f, tran.position.z ),
                                           world.GetHeightAtPos( tran.position.x - 0.5f, tran.position.z ),
                                           world.GetHeightAtPos( tran.position.x, tran.position.z + 0.5f ),
                                           world.GetHeightAtPos( tran.position.x, tran.position.z - 0.5f ) } );
      float cornerHeight = ( std::max )( { world.GetHeightAtPos( tran.position.x + 0.25f, tran.position.z + 0.25f ),
                                           world.GetHeightAtPos( tran.position.x - 0.25f, tran.position.z + 0.25f ),
                                           world.GetHeightAtPos( tran.position.x + 0.25f, tran.position.z - 0.25f ),
                                           world.GetHeightAtPos( tran.position.x - 0.25f, tran.position.z - 0.25f ) } );
      float groundHeight = ( std::max )( edgeHeight, cornerHeight );
      bool  inAir        = ( tran.position.y ) > groundHeight + 0.01f;
      if( !inAir )
      {
         tran.position.y = groundHeight;
         vel.velocity.y  = 0.0f;
      }
   }
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
      cam.view = glm::rotate( cam.view, glm::radians( tran.rotation.x ), { 1, 0, 0 } ); // maybe tran rotation
      cam.view = glm::rotate( cam.view, glm::radians( tran.rotation.y ), { 0, 1, 0 } );
      cam.view = glm::rotate( cam.view, glm::radians( tran.rotation.z ), { 0, 0, 1 } );
      cam.view = glm::translate( cam.view, -tran.position );

      cam.viewProjection = cam.projection * cam.view;
   }
}

void RenderSystem( Entity::Registry& registry, const glm::vec3& camPosition, const glm::mat4& viewProjection )
{
   //PROFILE_SCOPE( "RenderSystem" );
   // TODO - refactor this system, Shader should be within component data for entity

   // hack to get working
   static std::unique_ptr< Shader > pShader = std::make_unique< Shader >();

   pShader->Bind();                                 // Bind the shader program
   pShader->SetUniform( "u_MVP", viewProjection );  // Set Model-View-Projection matrix
   pShader->SetUniform( "u_viewPos", camPosition ); // Set camera position
   //pShader->SetUniform( "u_color", 0.5f, 0.0f, 0.0f );         // Set color (example: red)
   pShader->SetUniform( "u_lightPos", camPosition ); // Set light position at camera position

   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ); // clear color and depth buffers
   for( auto [ tran, mesh ] : registry.CView< CTransform, CMesh >() )
   {
      // TODO - perform culling check, skip if culled (maybe this should be a separate system?)

      pShader->SetUniform( "u_color", mesh.mesh->GetColor() );

      mesh.mesh->SetPosition( tran.position ); // this shouldn't be here
      mesh.mesh->Render();

      //glm::mat4 modelMatrix = glm::translate( glm::mat4( 1.0f ), tran.position ) * glm::mat4_cast( tran.rotation ) *
      //                        glm::scale( glm::mat4( 1.0f ), tran.scale );
      //glm::mat4 mvpMatrix = viewProjection * modelMatrix;

      // Bind material shader and set uniforms
      //glUseProgram( pMaterial->shaderProgram );
      //glUniformMatrix4fv( glGetUniformLocation( pMaterial->shaderProgram, "uMVP" ), 1, GL_FALSE, glm::value_ptr( mvpMatrix ) );
      //glUniform3fv( glGetUniformLocation( pMaterial->shaderProgram, "uColor" ), 1, glm::value_ptr( pMaterial->color ) );

      // Bind mesh and draw
      //glBindVertexArray( pMesh->vao );
      //glDrawElements( GL_TRIANGLES, pMesh->indexCount, GL_UNSIGNED_INT, 0 );
   }

   pShader->Unbind();
}

void TickingSystem( Entity::Registry& /*registry*/, float /*delta*/ )
{
   //PROFILE_SCOPE( "TickingSystem" );
}

void Application::Run()
{
   Entity::Registry registry;

   // Create player entity
   Entity::Entity player = registry.Create();
   registry.Add< CTransform >( player, 0.0f, 128.0f, 0.0f );
   registry.Add< CVelocity >( player, 0.0f, 0.0f, 0.0f );
   registry.Add< CInput >( player );
   registry.Add< CPlayerTag >( player );

   // Create camera entity
   Entity::Entity camera = registry.Create();
   registry.Add< CTransform >( camera, 0.0f, 128.0f + PLAYER_HEIGHT, 0.0f );
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

      if( CCamera* pCameraComp = registry.Get< CCamera >( camera ); pCameraComp )
      {
         pCameraComp->projection = glm::perspective( glm::radians( pCameraComp->fov ), e.GetWidth() / static_cast< float >( e.GetHeight() ), 0.1f, 1000.0f );
      }
   } );

   eventSubscriber.Subscribe< Events::MouseScrolledEvent >( [ &registry, camera ]( const Events::MouseScrolledEvent& e ) noexcept
   {
      if( CCamera* pCameraComp = registry.Get< CCamera >( camera ); pCameraComp )
      {
         pCameraComp->fov        = std::clamp( pCameraComp->fov - ( e.GetYOffset() * 5 ), 10.0f, 90.0f );
         pCameraComp->projection = glm::perspective( glm::radians( pCameraComp->fov ),
                                                     Window::Get().GetWindowData().Width / static_cast< float >( Window::Get().GetWindowData().Height ),
                                                     0.1f,
                                                     1000.0f );
      }
   } );
   // -----------------------------------

   // Initialize things
   Window& window = Window::Get();
   window.Init();
   GUIManager::Init();
   INetwork::RegisterGUI();

   Timestep timestep( 20 /*tickrate*/ );
   GUIManager::Attach( std::make_shared< DebugGUI >( registry, player, camera, timestep ) );

   World world( registry );
   world.Setup( registry );

   eventSubscriber.Subscribe< Events::KeyPressedEvent >( [ &world, &registry ]( const Events::KeyPressedEvent& e ) noexcept
   {
      switch( e.GetKeyCode() )
      {
         case Input::R: world.Setup( registry ); break; // regen world
         default:       break;
      }
   } );

   while( window.IsOpen() )
   {
      const float delta = timestep.Step();

      CTransform* pPlayerTran = registry.Get< CTransform >( player );
      CTransform* pCameraTran = registry.Get< CTransform >( camera );
      CCamera*    pCameraComp = registry.Get< CCamera >( camera );

      {
         Events::ProcessQueuedEvents();
         // ProcessQueuedNetworkEvents(); - maybe do this here?

         PlayerInputSystem( registry, delta );
         PlayerPhysicsSystem( registry, delta, world );
         CameraSystem( registry, window );

         if( timestep.FTick() )
         {
            TickingSystem( registry, delta ); // TODO: implement this system (with CTick component)

            // remove this eventually
            if( INetwork* pNetwork = INetwork::Get(); pNetwork && pPlayerTran )
               pNetwork->Poll( pPlayerTran->position );
         }

         if( pPlayerTran && pCameraTran ) // hacky way to set camera position to follow player (update before render, will change)
         {
            pCameraTran->position = pPlayerTran->position + glm::vec3( 0.0f, PLAYER_HEIGHT, 0.0f );
            pPlayerTran->rotation = pCameraTran->rotation;
         }

         if( !window.FMinimized() && pCameraComp && pCameraTran )
         {
            RenderSystem( registry, pCameraTran->position, pCameraComp->viewProjection );
            GUIManager::Draw();
         }
      }

      window.OnUpdate();
   }

   // Shutdown everything
   GUIManager::Shutdown();
}
