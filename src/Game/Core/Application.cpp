
#include "Application.h"

#include "../Timestep.h"
#include "../World.h"
#include "../Entity/Player.h"
#include "../Camera.h"
#include "../../Network/Network.h"
#include "../../Input/Input.h"

#include "GUIManager.h"

class ScopeTimer
{
public:
   ScopeTimer( const std::string& label ) :
      m_label( label ),
      m_start( std::chrono::high_resolution_clock::now() )
   {}

   ~ScopeTimer()
   {
      double ms = std::chrono::duration< double, std::milli >( std::chrono::high_resolution_clock::now() - m_start ).count();
      std::cout << m_label << " took " << std::fixed << std::setprecision( 3 ) << ms << "ms\n";
   }

private:
   std::string                                    m_label;
   std::chrono::high_resolution_clock::time_point m_start;
};
#define PROFILE_SCOPE( name ) ScopeTimer timer##__LINE__{name}

namespace AEntity
{

using Entity = uint64_t;

class Registry
{
private:
   using EntityIndex      = uint32_t;
   using ComponentTypeKey = void*; // Unique identifier for component types, can be a typeid or similar

public:
   Registry()  = default;
   ~Registry() = default;

   [[nodiscard]] Entity Create()
   {
      Entity entity = 0;
      if( !m_recycledEntities.empty() )
      {
         entity = m_recycledEntities.front();
         m_recycledEntities.pop();
      }
      else
         entity = m_nextEntity++;

      m_entityComponentTypes[ entity ] = std::unordered_set< ComponentTypeKey >(); // Initialize with an empty set of component types
      return entity;
   }

   void Destroy( Entity entity )
   {
      if( auto compTypesIter = m_entityComponentTypes.find( entity ); compTypesIter != m_entityComponentTypes.end() )
      {
         for( ComponentTypeKey typeKey : compTypesIter->second )
         {
            // The pool MUST exist at this point; otherwise, why do we have a record of the typeKey?
            Pool* pPool = m_storagePools.at( typeKey ).get();
            pPool->Remove( entity );

            // Will invalidate pPool pointer, do not use it after this point
            if( pPool->FEmpty() )
               m_storagePools.erase( typeKey ); // If the pool is empty, we can remove it from the storage pools
         }

         m_entityComponentTypes.erase( entity );
         m_recycledEntities.push( entity ); // Recycle the entity ID for future use
      }
   }

   // FValid
   //   Checks if the entity is valid, i.e., it has not been destroyed.
   [[nodiscard]] bool FValid( Entity entity ) const { return m_entityComponentTypes.contains( entity ); }

   // AddComponent
   //   Adds a component to the entity, if the entity already has the component, it will throw an error.
   //   Enforces that each entity can only have one instance of a component type.
   template< typename TComponent, typename... TArgs >
   void AddComponent( Entity entity, TArgs&&... args )
   {
      // If the entity already has this component type, throw an error
      ComponentTypeKey typeKey = GetTypeKey< TComponent >();
      if( m_entityComponentTypes.contains( entity ) && m_entityComponentTypes[ entity ].contains( typeKey ) )
         throw std::runtime_error( std::string( "Entity already has component: " ) + typeid( TComponent ).name() );

      // Otherwise, ensure the storage exists for this component type and associate it with the entity
      ComponentStorage< TComponent >& storage = EnsureStorage< TComponent >();
      storage.m_components.emplace_back( std::forward< TArgs >( args )... );
      storage.m_entityAtIndex.push_back( entity );
      storage.m_entityIndexMap[ entity ] = storage.m_components.size() - 1;

      // Record that this entity has this component type
      m_entityComponentTypes[ entity ].insert( typeKey );
   }

   // RemoveComponent
   //   Removes the component from the entity if the entity has the component; otherwise, it does nothing.
   template< typename TComponent >
   void RemoveComponent( Entity entity )
   {
      bool             fEmpty  = false;
      ComponentTypeKey typeKey = GetTypeKey< TComponent >();
      if( ComponentStorage< TComponent >* pStorage = GetStorage< TComponent >() )
      {
         pStorage->Remove( entity );
         fEmpty = pStorage->FEmpty();

         m_entityComponentTypes[ entity ].erase( typeKey );
      }

      // If the storage is empty, we can remove it from the storage pools
      if( fEmpty )
         m_storagePools.erase( typeKey );
   }

   // RemoveComponents
   //   Removes multiple components from the entity. If the entity does not have a component, it will do nothing for that component.
   template< typename... TComponents >
   void RemoveComponents( Entity entity )
   {
      ( RemoveComponent< TComponents >( entity ), ... );
   }

   // GetComponent
   //   Returns a pointer to the component of the entity. If the entity does not have the component, it returns nullptr.
   template< typename TComponent >
   [[nodiscard]] TComponent* GetComponent( Entity entity )
   {
      if( ComponentStorage< TComponent >* pStorage = GetStorage< TComponent >() )
      {
         if( auto it = pStorage->m_entityIndexMap.find( entity ); it != pStorage->m_entityIndexMap.end() )
            return &pStorage->m_components.at( it->second );
      }

      return nullptr; // Entity does not have the component
   }

   // GetComponents
   //   Returns a tuple of pointers to the components of the entity. If the entity does not have a component, the pointer will be nullptr.
   template< typename... TComponents >
   [[nodiscard]] auto GetComponents( Entity entity )
   {
      return std::tuple< TComponents*... > { GetComponent< TComponents >( entity )... };
   }

private:
   struct Pool
   {
      virtual ~Pool() noexcept             = default;
      virtual void Remove( Entity )        = 0;
      virtual bool FEmpty() const noexcept = 0;
   };

   template< typename TComponent >
   struct ComponentStorage : public Pool
   {
      std::vector< TComponent >            m_components;     // dense array of components
      std::vector< EntityIndex >           m_entityAtIndex;  // maps index -> entity
      std::unordered_map< Entity, size_t > m_entityIndexMap; // maps entity -> index

      // Remove the entity from this storage
      void Remove( Entity entity ) override
      {
         auto it = m_entityIndexMap.find( entity );
         if( m_components.empty() || it == m_entityIndexMap.end() )
            return;

         const EntityIndex index = it->second;

         // swap and pop to remove the component (overwrite with last component)
         m_components.at( index ) = std::move( m_components.back() );
         m_components.pop_back();

         // swap and pop to remove the entity from the index (overwrite with last entity)
         m_entityAtIndex.at( index ) = std::move( m_entityAtIndex.back() );
         m_entityAtIndex.pop_back();

         // erase entity from index map and update the index of the swapped entity
         m_entityIndexMap.erase( entity );
         if( !m_entityAtIndex.empty() && index < m_entityAtIndex.size() )
            m_entityIndexMap[ m_entityAtIndex.at( index ) ] = index;
      }

      bool FEmpty() const noexcept override { return m_components.empty(); }
   };

   // Uses the unique type address as a key to identify the component type
   template< typename TComponent >
   ComponentTypeKey GetTypeKey()
   {
      static int typeKey;
      return &typeKey; // Return the address of the static variable as a unique key for the TComponent type
   }

   template< typename TComponent >
   ComponentStorage< TComponent >& EnsureStorage() // Will create the storage if it doesn't exist
   {
      ComponentTypeKey typeKey = GetTypeKey< TComponent >();
      if( auto it = m_storagePools.find( typeKey ); it != m_storagePools.end() )
         return static_cast< ComponentStorage< TComponent >& >( *it->second );

      return static_cast< ComponentStorage< TComponent >& >( *m_storagePools.emplace( typeKey, std::make_unique< ComponentStorage< TComponent > >() )
                                                                 .first->second ); // Create a new storage pool for the component type
   }

   template< typename TComponent >
   ComponentStorage< TComponent >* GetStorage() // Will return nullptr if the storage does not exist
   {
      ComponentTypeKey typeKey = GetTypeKey< TComponent >();
      if( auto it = m_storagePools.find( typeKey ); it != m_storagePools.end() )
         return static_cast< ComponentStorage< TComponent >* >( it->second.get() );

      return nullptr;
   }

   Entity               m_nextEntity { 1 }; // Start from 1 to avoid zero ID (could be reserved for invalid entity)
   std::queue< Entity > m_recycledEntities; // Recycled entities for reuse

   std::unordered_map< ComponentTypeKey, std::unique_ptr< Pool > >      m_storagePools;
   std::unordered_map< Entity, std::unordered_set< ComponentTypeKey > > m_entityComponentTypes;
};

}

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

void InputSystem( AEntity::Registry& registry, float delta, bool fDidTick )
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

void PhysicsSystem( AEntity::Registry& registry, float delta, bool fDidTick )
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
      AEntity::Registry     registry;
      const AEntity::Entity entity1 = registry.Create();
      const AEntity::Entity entity2 = registry.Create();

      std::println( "E1 Valid after Create - {}", registry.FValid( entity1 ) );
      std::println( "E2 Valid after Create - {}", registry.FValid( entity2 ) );

      {
         registry.AddComponent< PositionComponent >( entity1, 1.0f, 2.0f, 3.0f );
         registry.AddComponent< VelocityComponent >( entity1, 0.1f, 0.2f, 0.3f );

         if( PositionComponent* pPos = registry.GetComponent< PositionComponent >( entity1 ) )
            std::println( "E1 - Position: ({}, {}, {})", pPos->position.x, pPos->position.y, pPos->position.z );

         if( VelocityComponent* pVel = registry.GetComponent< VelocityComponent >( entity1 ) )
            std::println( "E1 - Velocity: ({}, {}, {})", pVel->velocity.x, pVel->velocity.y, pVel->velocity.z );

         auto [ pPos, pVel ] = registry.GetComponents< PositionComponent, VelocityComponent >( entity1 );
         if( pPos && pVel )
            std::println( "E1 - Has Position and Velocity" );
      }

      {
         registry.AddComponent< PositionComponent >( entity2, 3.0f, 4.0f, 5.0f );
         if( PositionComponent* pPos = registry.GetComponent< PositionComponent >( entity1 ) )
            std::println( "E2 - Position: ({}, {}, {})", pPos->position.x, pPos->position.y, pPos->position.z );
      }

      {
         registry.RemoveComponent< PositionComponent >( entity2 );
         //registry.RemoveComponent< PositionComponent >( entity1 );
         registry.RemoveComponents< VelocityComponent, PositionComponent >( entity1 );

         std::println( "E1 Valid after RemoveComponent - {}", registry.FValid( entity1 ) );
         std::println( "E2 Valid after RemoveComponent - {}", registry.FValid( entity2 ) );
      }

      registry.Destroy( entity2 );
      registry.Destroy( entity1 );

      std::println( "E1 Valid after Destroy - {}", registry.FValid( entity1 ) );
      std::println( "E2 Valid after Destroy - {}", registry.FValid( entity2 ) );

      AEntity::Entity entity1Recycled = registry.Create();
      AEntity::Entity entity2Recycled = registry.Create();
      std::println( "E1 Recycled ID - {}", entity1Recycled );
      std::println( "E2 Recycled ID - {}", entity2Recycled );
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
   AEntity::Registry     registry;
   const AEntity::Entity entity1 = registry.Create();
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
