#include "Application.h"

#include "../Timestep.h"
#include "../World.h"
#include "../Entity/Player.h"
#include "../Entity/Registry.h"
#include "../Camera.h"
#include "../../Network/Network.h"
#include "../../Input/Input.h"

#include "GUIManager.h"

struct PositionComponent
{
   glm::vec3 position { 0.0f, 0.0f, 0.0f };

   PositionComponent() = default;
   PositionComponent( float x, float y, float z ) :
      position( x, y, z )
   {}
   PositionComponent( const glm::vec3& pos ) :
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
   auto [ pPos, pVel ] = registry.GetComponents< PositionComponent, VelocityComponent >( 1 /*entity*/ );
   if( pPos && pVel )
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
         pVel->velocity.y += accel;
      else if( Input::IsKeyPressed( Input::S ) )
         pVel->velocity.y -= accel;
      else
         dampen( pVel->velocity.y, accel );

      if( Input::IsKeyPressed( Input::D ) )
         pVel->velocity.x += accel;
      else if( Input::IsKeyPressed( Input::A ) )
         pVel->velocity.x -= accel;
      else
         dampen( pVel->velocity.x, accel );

      constexpr float maxSpeed = 5.0f;
      pVel->velocity.x         = std::clamp( pVel->velocity.x, -maxSpeed, maxSpeed );
      pVel->velocity.y         = std::clamp( pVel->velocity.y, -maxSpeed, maxSpeed );

      if( fDidTick )
         std::println( "Input: Vel: ({}, {}, {})", pVel->velocity.x, pVel->velocity.y, pVel->velocity.z );
   }
}

void PhysicsSystem( Entity::Registry& registry, float delta, bool fDidTick )
{
   auto [ pPos, pVel ] = registry.GetComponents< PositionComponent, VelocityComponent >( 1 /*entity*/ );
   if( pPos && pVel )
   {
      pPos->position += pVel->velocity * delta;

      if( fDidTick )
         std::println( "Physics: Pos: ({}, {}, {}), Vel: ({}, {}, {})",
                       pPos->position.x,
                       pPos->position.y,
                       pPos->position.z,
                       pVel->velocity.x,
                       pVel->velocity.y,
                       pVel->velocity.z );
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
   {
      constexpr size_t entityCount = 1'000;

      Entity::Registry registry;
      for( size_t i = 0; i < entityCount; ++i )
      {
         Entity::Entity ent = registry.Create();

         // Assign Position to all
         registry.AddComponent< PositionComponent >( ent, 0.0f, 0.0f, 0.0f );

         // Assign Velocity to half randomly
         if( i % 2 == 0 )
            registry.AddComponent< VelocityComponent >( ent, 0.0f, 0.0f, 0.0f );
      }

      PROFILE_SCOPE( "ECS: View Iterator" );
      auto view = registry.View< PositionComponent, VelocityComponent >();
      for( auto [ pos, vel ] : view )
      {
         pos.position.x += vel.velocity.x;
         pos.position.y += vel.velocity.y;
         pos.position.z += vel.velocity.z;
      }
   }

   Init(); // Initialize the application

   // TODO:
   // Write a RenderQueue, IGameObjects will be submitted to the RenderQueue
   //      After, the RenderQueue will be rendered
   // Write a logging library (log to file)
   //    Get rid of console window, pipe output to a imgui window

   // PlayerManager::GetPlayer() - returns the player
   //    only one player should theoretically exist at a time

   // https://github.com/htmlboss/OpenGL-Renderer

   World world;
   world.Setup();

   Events::EventSubscriber eventSubscriber;
   eventSubscriber.Subscribe< Events::KeyPressedEvent >( [ & ]( const Events::KeyPressedEvent& e ) noexcept
   {
      if( e.GetKeyCode() == Input::R ) // Reload world
         world.Setup();
   } );

   Player player;
   Camera camera;

   // Attach debug UI to track player/camera
   GUIManager::Attach( std::make_shared< DebugGUI >( player, camera ) );

   // Attach Network GUI
   INetwork::RegisterGUI();

   // ECS
   Entity::Registry     registry;
   const Entity::Entity entity1 = registry.Create();
   registry.AddComponent< PositionComponent >( entity1, 1.0f, 1.0f, 1.0f );
   registry.AddComponent< VelocityComponent >( entity1, 0.1f, 0.1f, 0.0f );

   float    time = 0;
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
      world.Tick( delta, camera ); // Update world
      player.Tick( world, delta ); // Tick Player, use m_World to check collisions

      // Game Updates
      player.Update( delta );  // Update Player
      camera.Update( player ); // Update Camera to match Player

      if( timestep.FTick() )
      {
         std::println( "Tick: {}", timestep.GetLastTick() );
         if( INetwork* pNetwork = INetwork::Get() )
            pNetwork->Poll( world, player ); // Poll network events
      }

      // Render only if not minimized
      if( !window.FMinimized() )
      {
         // Render
         world.Render( camera );

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
