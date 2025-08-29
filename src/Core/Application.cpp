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
constexpr float GRAVITY             = -32.0f;
constexpr float TERMINAL_VELOCITY   = -48.0f;
constexpr float JUMP_VELOCITY       = 9.0f;
constexpr float PLAYER_HEIGHT       = 1.9f;
constexpr float GROUND_MAXSPEED     = 12.0f;
constexpr float SPRINT_MODIFIER     = 1.5f;
constexpr float DOUBLE_TAP_WINDOW_S = 0.5f; // seconds for space double tap
constexpr float FLIGHT_ASCEND_SPEED = 12.0f;
constexpr float FLIGHT_DESCEND_SPEED= 12.0f;
constexpr float FLIGHT_HORIZONTAL_SPEED = 14.0f; // slightly faster while flying

void PlayerInputSystem( Entity::Registry& registry, float delta )
{
   for( auto [ entity, tran, vel, input, tag ] : registry.ECView< CTransform, CVelocity, CInput, CPlayerTag >() )
   {
      // --- Double tap space to toggle flight mode ---
      if( input.spaceTapTimer > 0.0f )
         input.spaceTapTimer -= delta;

      bool spacePressed = Input::IsKeyPressed( Input::Space );
      if( spacePressed && !input.prevSpacePressed ) // rising edge
      {
         if( input.spaceTapTimer > 0.0f )
         {
            // second tap inside window -> toggle flight mode
            input.flightMode   = !input.flightMode;
            input.spaceTapTimer = 0.0f; // reset
            if( input.flightMode )
            {
               // zero vertical velocity when entering flight for smooth control
               vel.velocity.y = 0.0f;
            }
         }
         else
         {
            // first tap: start timer and perform normal jump if grounded & not already flying
            input.spaceTapTimer = DOUBLE_TAP_WINDOW_S;
            if( !input.flightMode && input.grounded )
            {
               vel.velocity.y = JUMP_VELOCITY;
               input.grounded = false;
            }
         }
      }
      input.prevSpacePressed = spacePressed;

      // Apply horizontal movement inputs
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

      float maxSpeed = input.flightMode ? FLIGHT_HORIZONTAL_SPEED : GROUND_MAXSPEED;
      if( Input::IsKeyPressed( Input::LeftShift ) )
         maxSpeed *= SPRINT_MODIFIER;

      vel.velocity.x = wishdir.x * maxSpeed;
      vel.velocity.z = wishdir.z * maxSpeed;

      // Flight vertical control
      if( input.flightMode )
      {
         // disable gravity effect while in flight (handled in physics)
         if( Input::IsKeyPressed( Input::Space ) )
            vel.velocity.y = FLIGHT_ASCEND_SPEED;
         else if( Input::IsKeyPressed( Input::LeftControl ) || Input::IsKeyPressed( Input::RightControl ) )
            vel.velocity.y = -FLIGHT_DESCEND_SPEED;
         else
            vel.velocity.y = 0.0f; // hover
      }
   }
}

void PlayerPhysicsSystem( Entity::Registry& registry, float delta, World& world )
{
   for( auto [ entity, tran, vel, input, tag ] : registry.ECView< CTransform, CVelocity, CInput, CPlayerTag >() )
   {
      // Apply gravity only when not in flight mode
      if( !input.flightMode )
      {
         vel.velocity.y += GRAVITY * delta;
         if( vel.velocity.y < TERMINAL_VELOCITY )
            vel.velocity.y = TERMINAL_VELOCITY;
      }

      // Integrate position
      tran.position += vel.velocity * delta;

      // Ground collision logic only relevant when not in flight mode
      float edgeHeight   = ( std::max )( { world.GetHeightAtPos( tran.position.x + 0.5f, tran.position.z ),
                                           world.GetHeightAtPos( tran.position.x - 0.5f, tran.position.z ),
                                           world.GetHeightAtPos( tran.position.x, tran.position.z + 0.5f ),
                                           world.GetHeightAtPos( tran.position.x, tran.position.z - 0.5f ) } );
      float cornerHeight = ( std::max )( { world.GetHeightAtPos( tran.position.x + 0.25f, tran.position.z + 0.25f ),
                                           world.GetHeightAtPos( tran.position.x - 0.25f, tran.position.z + 0.25f ),
                                           world.GetHeightAtPos( tran.position.x + 0.25f, tran.position.z - 0.25f ),
                                           world.GetHeightAtPos( tran.position.x - 0.25f, tran.position.z - 0.25f ) } );
      float groundHeight = ( std::max )( edgeHeight, cornerHeight );

      if( !input.flightMode )
      {
         bool grounded = tran.position.y <= groundHeight + 0.01f;
         if( grounded )
         {
            tran.position.y = groundHeight;
            vel.velocity.y  = 0.0f;
         }
         input.grounded = grounded;
      }
      else
      {
         // While flying we are not grounded; prevent accidental landing snap unless below ground
         if( tran.position.y < groundHeight )
            tran.position.y = groundHeight + 0.01f; // keep slightly above terrain
         input.grounded = false;
      }
   }
}

void CameraSystem( Entity::Registry& registry, Window& window )
{
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

void RenderSystem( Entity::Registry& registry, const glm::vec3& camPosition, const glm::mat4& viewProjection )
{
   static std::unique_ptr< Shader > pShader = std::make_unique< Shader >();

   pShader->Bind();
   pShader->SetUniform( "u_MVP", viewProjection );
   pShader->SetUniform( "u_viewPos", camPosition );
   pShader->SetUniform( "u_lightPos", camPosition );

   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
   for( auto [ tran, mesh ] : registry.CView< CTransform, CMesh >() )
   {
      pShader->SetUniform( "u_color", mesh.mesh->GetColor() );
      mesh.mesh->SetPosition( tran.position );
      mesh.mesh->Render();
   }

   pShader->Unbind();
}

void TickingSystem( Entity::Registry& /*registry*/, float /*delta*/ )
{
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

   Window& window = Window::Get();
   window.Init();
   GUIManager::Init();
   INetwork::RegisterGUI();

   Timestep timestep( 20 );
   GUIManager::Attach( std::make_shared< DebugGUI >( registry, player, camera, timestep ) );

   World world( registry );
   world.Setup( registry );

   eventSubscriber.Subscribe< Events::KeyPressedEvent >( [ &world, &registry ]( const Events::KeyPressedEvent& e ) noexcept
   {
      switch( e.GetKeyCode() )
      {
         case Input::R: world.Setup( registry ); break;
         default:       break;
      }
   } );

   while( window.IsOpen() )
   {
      const float delta = timestep.Step();

      CTransform* pPlayerTran = registry.TryGet< CTransform >( player );
      CTransform* pCameraTran = registry.TryGet< CTransform >( camera );
      CCamera*    pCameraComp = registry.TryGet< CCamera >( camera );

      {
         Events::ProcessQueuedEvents();

         PlayerInputSystem( registry, delta );
         PlayerPhysicsSystem( registry, delta, world );
         CameraSystem( registry, window );

         if( timestep.FTick() )
         {
            TickingSystem( registry, delta );
            if( INetwork* pNetwork = INetwork::Get(); pNetwork && pPlayerTran )
               pNetwork->Poll( pPlayerTran->position );
         }

         if( pPlayerTran && pCameraTran )
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

   GUIManager::Shutdown();
}
