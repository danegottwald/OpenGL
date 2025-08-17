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

      // Apply Mouse Look Inputs
   }
}

void PlayerPhysicsSystem( Entity::Registry& registry, float delta, World& world )
{
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

      cam.viewMatrix = glm::mat4( 1.0f );
      cam.viewMatrix = glm::rotate( cam.viewMatrix, glm::radians( tran.rotation.x ), { 1, 0, 0 } ); // maybe tran rotation
      cam.viewMatrix = glm::rotate( cam.viewMatrix, glm::radians( tran.rotation.y ), { 0, 1, 0 } );
      cam.viewMatrix = glm::rotate( cam.viewMatrix, glm::radians( tran.rotation.z ), { 0, 0, 1 } );
      cam.viewMatrix = glm::translate( cam.viewMatrix, -tran.position );

      cam.projectionViewMatrix = cam.projectionMatrix * cam.viewMatrix;
   }
}

void RenderSystem( Entity::Registry& registry )
{
   //PROFILE_SCOPE( "RenderSystem" );
   //for( auto [ tran, mesh ] : registry.ECView< CTransform, CMesh >() )
   //{
   //   if( !mesh.mesh )
   //      continue;
   //
   //   mesh.mesh->SetPosition( tran.position );
   //   mesh.mesh->SetRotation( tran.rotation );
   //   mesh.mesh->SetScale( tran.scale );
   //   mesh.mesh->Render( projectionView );
   //}
}

void Application::Run()
{
   Events::EventSubscriber eventSubscriber;

   // ----------------------------------- ECS
   Entity::Registry registry;

   // Create player entity
   Entity::Entity player = registry.Create();
   registry.AddComponent< CTransform >( player, 0.0f, 128.0f, 0.0f );
   registry.AddComponent< CVelocity >( player, 0.0f, 0.0f, 0.0f );
   registry.AddComponent< CInput >( player );
   registry.AddComponent< CPlayerTag >( player );

   // Create camera entity
   Entity::Entity camera = registry.Create();
   registry.AddComponent< CTransform >( camera, 0.0f, 128.0f + PLAYER_HEIGHT, 0.0f );
   registry.AddComponent< CCameraTag >( camera );

   {
      std::shared_ptr< Entity::EntityHandle > psEntity = registry.CreateWithHandle();
      registry.AddComponent< CTransform >( psEntity->Get(), 0.0f, 128.0f, 0.0f );
      registry.AddComponent< CVelocity >( psEntity->Get(), 0.0f, 0.0f, 0.0f );
      registry.AddComponent< CInput >( psEntity->Get() );

      //Entity::EntityHandle entitySame1( *psEntity );  // Create a reference to the same entity
      //Entity::EntityHandle entitySame2 = entitySame1; // Create a reference to the same entity
   }

   if( CCamera* pCameraComp = &registry.AddComponent< CCamera >( camera ) )
      pCameraComp->projectionMatrix = glm::perspective( glm::radians( pCameraComp->fov ),
                                                        Window::Get().GetWindowData().Width / static_cast< float >( Window::Get().GetWindowData().Height ),
                                                        0.1f,
                                                        1000.0f );

   eventSubscriber.Subscribe< Events::WindowResizeEvent >( [ &registry, camera ]( const Events::WindowResizeEvent& e ) noexcept
   {
      if( e.GetHeight() == 0 )
         return;

      if( CCamera* pCameraComp = registry.GetComponent< CCamera >( camera ); pCameraComp )
      {
         float aspectRatio             = e.GetWidth() / static_cast< float >( e.GetHeight() );
         pCameraComp->projectionMatrix = glm::perspective( glm::radians( pCameraComp->fov ), aspectRatio, 0.1f, 1000.0f );
      }
   } );

   eventSubscriber.Subscribe< Events::MouseScrolledEvent >( [ &registry, camera ]( const Events::MouseScrolledEvent& e ) noexcept
   {
      if( CCamera* pCameraComp = registry.GetComponent< CCamera >( camera ); pCameraComp )
      {
         pCameraComp->fov += -e.GetYOffset() * 5;
         pCameraComp->fov              = std::clamp( pCameraComp->fov, 10.0f, 90.0f );
         pCameraComp->projectionMatrix = glm::perspective( glm::radians( pCameraComp->fov ),
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

   Timestep timestep( 20 );
   GUIManager::Attach( std::make_shared< DebugGUI >( registry, player, camera, timestep ) );

   World world;
   world.Setup( registry );

   eventSubscriber.Subscribe< Events::KeyPressedEvent >( [ &world, &registry ]( const Events::KeyPressedEvent& e ) noexcept
   {
      if( e.GetKeyCode() == Input::R )
         world.Setup( registry );
   } );

   while( window.IsOpen() )
   {
      const float delta = timestep.Step();

      Events::ProcessQueuedEvents();

      PlayerInputSystem( registry, delta );
      PlayerPhysicsSystem( registry, delta, world );
      CameraSystem( registry, window );
      RenderSystem( registry );

      // Camera follows player
      auto* pPlayerTran = registry.GetComponent< CTransform >( player );
      auto* pCameraTran = registry.GetComponent< CTransform >( camera );
      if( pPlayerTran && pCameraTran )
      {
         pCameraTran->position = pPlayerTran->position + glm::vec3( 0.0f, PLAYER_HEIGHT, 0.0f );
         pPlayerTran->rotation = pCameraTran->rotation;
      }

      // Game Ticks
      auto* pCameraComp = registry.GetComponent< CCamera >( camera );
      if( pCameraTran /*pCameraComp*/ )
         world.Tick( delta, pCameraTran->position );

      if( timestep.FTick() )
      {
         if( INetwork* pNetwork = INetwork::Get(); pNetwork && pPlayerTran )
            pNetwork->Poll( world, pPlayerTran->position );
      }

      if( !window.FMinimized() )
      {
         if( pCameraTran && pCameraComp )
            world.Render( registry, pCameraTran->position, pCameraComp->projectionViewMatrix );

         GUIManager::Draw();
      }

      window.OnUpdate();
   }

   // Shutdown everything
   GUIManager::Shutdown();
}
