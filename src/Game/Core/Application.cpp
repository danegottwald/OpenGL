#include "Application.h"

#include "../Timestep.h"
#include "../World.h"
#include "../Entity/Player.h"
#include "../Entity/Registry.h"
#include "../Camera.h"
#include "../../Network/Network.h"
#include "../../Input/Input.h"

#include "GUIManager.h"

struct TransformComponent
{
   glm::vec3 position { 0.0f, 0.0f, 0.0f };
   glm::quat rotation { 0.0f, 0.0f, 0.0f, 1.0f };
   glm::vec3 scale { 1.0f, 1.0f, 1.0f };

   TransformComponent() = default;
   TransformComponent( float xPos, float yPos, float zPos ) :
      position( xPos, yPos, zPos )
   {}
   TransformComponent( const glm::vec3& pos ) :
      position( pos )
   {}
};

struct VelocityComponent
{
   glm::vec3 velocity { 0.0f, 0.0f, 0.0f };
   VelocityComponent() = default;
   VelocityComponent( float vx, float vy, float vz ) :
      velocity( vx, vy, vz )
   {}
   VelocityComponent( const glm::vec3& vel ) :
      velocity( vel )
   {}
};

struct InputComponent
{
};

void InputSystem( Entity::Registry& registry, float delta, bool fDidTick )
{
   for( auto [ tran, vel, input ] : registry.CView< TransformComponent, VelocityComponent, InputComponent >() )
   {
      const float accel  = 12.0f * delta;
      auto        dampen = []( float& v, float amount )
      {
         if( v > 0.0f )
            v = std::max< float >( 0.0f, v - amount );
         else if( v < 0.0f )
            v = std::min< float >( 0.0f, v + amount );
      };

      if( Input::IsKeyPressed( Input::W ) )
         vel.velocity.y += accel;
      else if( Input::IsKeyPressed( Input::S ) )
         vel.velocity.y -= accel;
      else
         dampen( vel.velocity.y, accel );

      if( Input::IsKeyPressed( Input::D ) )
         vel.velocity.x += accel;
      else if( Input::IsKeyPressed( Input::A ) )
         vel.velocity.x -= accel;
      else
         dampen( vel.velocity.x, accel );

      constexpr float maxSpeed = 5.0f;
      vel.velocity.x           = std::clamp( vel.velocity.x, -maxSpeed, maxSpeed );
      vel.velocity.y           = std::clamp( vel.velocity.y, -maxSpeed, maxSpeed );

      if( fDidTick )
         std::println( "Input: Vel: ({}, {}, {})", vel.velocity.x, vel.velocity.y, vel.velocity.z );
   }
}

void PhysicsSystem( Entity::Registry& registry, float delta, bool fDidTick )
{
   for( auto [ tran, vel ] : registry.CView< TransformComponent, VelocityComponent >() )
   {
      tran.position += vel.velocity * delta;

      if( fDidTick )
         std::println( "Physics: Pos: ({}, {}, {}), Vel: ({}, {}, {})",
                       tran.position.x,
                       tran.position.y,
                       tran.position.z,
                       vel.velocity.x,
                       vel.velocity.y,
                       vel.velocity.z );
   }
}

void MovementSystem( Entity::Registry& registry, float delta, bool fDidTick )
{
   // Iterate all entities with Transform, Velocity, and Input
   for( auto [ tran, vel, input ] : registry.CView< TransformComponent, VelocityComponent, InputComponent >() )
   {
      // Keyboard input (WASD)
      const float accel  = 12.0f * delta;
      auto        dampen = []( float& v, float amount )
      {
         if( v > 0.0f )
            v = std::max< float >( 0.0f, v - amount );
         else if( v < 0.0f )
            v = std::min< float >( 0.0f, v + amount );
      };

      if( Input::IsKeyPressed( Input::W ) )
         vel.velocity.y += accel;
      else if( Input::IsKeyPressed( Input::S ) )
         vel.velocity.y -= accel;
      else
         dampen( vel.velocity.y, accel );

      if( Input::IsKeyPressed( Input::D ) )
         vel.velocity.x += accel;
      else if( Input::IsKeyPressed( Input::A ) )
         vel.velocity.x -= accel;
      else
         dampen( vel.velocity.x, accel );

      constexpr float maxSpeed = 5.0f;
      vel.velocity.x           = std::clamp( vel.velocity.x, -maxSpeed, maxSpeed );
      vel.velocity.y           = std::clamp( vel.velocity.y, -maxSpeed, maxSpeed );

      // Mouse input (example: could be used for looking/rotation)
      // You can expand this to update orientation, etc.
      // if( Input::IsMouseMoved() ) { ... }

      // Apply velocity to position
      tran.position += vel.velocity * delta;

      if( fDidTick )
         std::println( "MovementSystem: Pos: ({}, {}, {}), Vel: ({}, {}, {})",
                       tran.position.x,
                       tran.position.y,
                       tran.position.z,
                       vel.velocity.x,
                       vel.velocity.y,
                       vel.velocity.z );
   }
}

// Ordering of initialization is critical
void Application::Init()
{
   Window::Get().Init(); // Initialize the window
   GUIManager::Init();   // Initialize the GUI Manager
}

// Cleanup should occur in reverse order of initialization
void Application::Shutdown()
{
   GUIManager::Shutdown();
}

void Application::Run()
{
   Entity::Registry registry;
   {
      PROFILE_SCOPE( "ECS: View Iterator" );

      // Create player entity
      Entity::Entity dummy  = registry.Create();
      Entity::Entity player = registry.Create();
      registry.AddComponent< TransformComponent >( player, 0.0f, 0.0f, 0.0f );
      registry.AddComponent< VelocityComponent >( player, 0.0f, 0.0f, 0.0f );
      registry.AddComponent< InputComponent >( player );

      for( auto [ entity, tran, vel ] : registry.ECView< TransformComponent, VelocityComponent >() )
      {
         std::println( "Entity {}: Pos: ({}, {}, {}), Vel: ({}, {}, {})",
                       entity,
                       tran.position.x,
                       tran.position.y,
                       tran.position.z,
                       vel.velocity.x,
                       vel.velocity.y,
                       vel.velocity.z );
      }

      for( auto [ tran, vel ] : registry.CView< TransformComponent, VelocityComponent >() )
      {
         std::println( "Transform: Pos: ({}, {}, {}), Vel: ({}, {}, {})",
                       tran.position.x,
                       tran.position.y,
                       tran.position.z,
                       vel.velocity.x,
                       vel.velocity.y,
                       vel.velocity.z );
      }

      for( Entity::Entity entity : registry.EView< TransformComponent, VelocityComponent >() )
      {
         TransformComponent& tran = *registry.GetComponent< TransformComponent >( entity );
         VelocityComponent&  vel  = *registry.GetComponent< VelocityComponent >( entity );

         std::println( "Entity {}: Pos: ({}, {}, {}), Vel: ({}, {}, {})",
                       entity,
                       tran.position.x,
                       tran.position.y,
                       tran.position.z,
                       vel.velocity.x,
                       vel.velocity.y,
                       vel.velocity.z );
      }

      // Create camera entity
      Entity::Entity camera = registry.Create();
      registry.AddComponent< TransformComponent >( camera, 0.0f, 0.0f, 0.0f );
      // Optionally: registry.AddComponent< CameraComponent >( camera, player ); // if you have a CameraComponent
   }

   Init(); // Initialize the application

   // TODO:
   // Write a logging library (log to file)
   //    Get rid of console window, pipe output to a imgui window
   // https://github.com/htmlboss/OpenGL-Renderer

   World world;
   world.Setup();

   Events::EventSubscriber eventSubscriber;
   eventSubscriber.Subscribe< Events::KeyPressedEvent >( [ & ]( const Events::KeyPressedEvent& e ) noexcept
   {
      if( e.GetKeyCode() == Input::R ) // Reload world
         world.Setup();
   } );

   Player playerObj;
   Camera cameraObj;

   // Attach debug UI to track player/camera
   GUIManager::Attach( std::make_shared< DebugGUI >( playerObj, cameraObj ) );

   // Attach Network GUI
   INetwork::RegisterGUI();

   Timestep timestep( 20 /*tickrate*/ );
   Window&  window = Window::Get();
   while( window.IsOpen() )
   {
      const float delta = timestep.Step();

      {
         Events::ProcessQueuedEvents(); // Process queued events before the next frame
         //Network::ProcessQueuedEvents(); // Process network events before the next frame

         InputSystem( registry, delta, timestep.FTick() );   // Handle input system
         PhysicsSystem( registry, delta, timestep.FTick() ); // Run physics system
      }

      // Game Ticks
      world.Tick( delta, cameraObj ); // Update world
      playerObj.Tick( world, delta ); // Tick Player, use m_World to check collisions

      // Game Updates
      cameraObj.Update( playerObj ); // Update Camera to match Player

      if( timestep.FTick() )
      {
         std::println( "Tick: {}", timestep.GetLastTick() );
         if( INetwork* pNetwork = INetwork::Get() )
            pNetwork->Poll( world, playerObj ); // Poll network events
      }

      // Render only if not minimized
      if( !window.FMinimized() )
      {
         // Render
         world.Render( cameraObj );

         // Render
         //RenderManager::Render();

         // Draw GUI
         GUIManager::Draw();
      }

      window.OnUpdate(); // Poll events and swap buffers
   }

   Shutdown(); // Cleanup the application
}

// NOTES
// https://developer.valvesoftware.com/wiki/Source_Multiplayer_Networking
//      https://github.com/Hopson97/open-builder/issues/208#issuecomment-706747718
