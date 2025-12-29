#pragma once

#include <Engine/ECS/Registry.h>

namespace Engine::Physics
{

enum class CollisionPhase : uint8_t
{
   Enter,
   Stay,
   Exit,
};

struct CollisionEvent
{
   Entity::Entity a { Entity::NullEntity };
   Entity::Entity b { Entity::NullEntity };
   CollisionPhase phase { CollisionPhase::Enter };
};

// Per-frame collision event buffer + previous-pair cache to produce Enter/Stay/Exit.
class CollisionEventQueue
{
public:
   // Called by collision detection each frame.
   void BeginCollect() { m_currPairs.clear(); }

   // After all AddOverlapPair calls, compute Enter/Stay/Exit and produce events.
   void EndCollect()
   {
      m_events.clear();

      // Enter/Stay
      for( uint64_t key : m_currPairs )
      {
         const bool was = m_prevPairs.contains( key );
         auto [ a, b ]  = UnpackPair( key );
         m_events.push_back( CollisionEvent { .a = a, .b = b, .phase = was ? CollisionPhase::Stay : CollisionPhase::Enter } );
      }

      // Exit
      for( uint64_t key : m_prevPairs )
      {
         if( m_currPairs.contains( key ) )
            continue;

         auto [ a, b ] = UnpackPair( key );
         m_events.push_back( CollisionEvent { .a = a, .b = b, .phase = CollisionPhase::Exit } );
      }

      m_prevPairs = m_currPairs;
   }

   void AddOverlapPair( Entity::Entity a, Entity::Entity b )
   {
      if( a == b )
         return;

      if( b < a )
         std::swap( a, b );

      m_currPairs.insert( PackPair( a, b ) );
   }

   std::span< const CollisionEvent > Events() const { return m_events; }
   void                              ClearFrame() { m_events.clear(); }

private:
   static uint64_t PackPair( Entity::Entity a, Entity::Entity b )
   {
      // Assumes Entity::Entity is 32-bit-ish (common). If it's bigger, we still work as long as it fits.
      const uint64_t aa = static_cast< uint64_t >( a );
      const uint64_t bb = static_cast< uint64_t >( b );
      return ( aa << 32 ) | ( bb & 0xFFFFFFFFull );
   }

   static std::pair< Entity::Entity, Entity::Entity > UnpackPair( uint64_t key )
   {
      const uint32_t a = static_cast< uint32_t >( key >> 32 );
      const uint32_t b = static_cast< uint32_t >( key & 0xFFFFFFFFull );
      return { static_cast< Entity::Entity >( a ), static_cast< Entity::Entity >( b ) };
   }

   std::vector< CollisionEvent >  m_events;
   std::unordered_set< uint64_t > m_prevPairs;
   std::unordered_set< uint64_t > m_currPairs;
};

} // namespace Engine::Physics
