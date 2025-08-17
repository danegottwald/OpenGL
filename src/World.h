#pragma once

// Project dependencies
#include <Events/NetworkEvent.h>
#include <FastNoiseLite/FastNoiseLite.h>

using GLuint = unsigned int;

// Forward Declarations
class Camera;
class FastNoiseLite;
class IMesh;
namespace Entity
{
class Registry;
class EntityHandle;
}

class World
{
public:
   explicit World( Entity::Registry& registry );
   ~World() = default;

   void Setup( Entity::Registry& registry );

   float GetHeightAtPos( float x, float z ) const
   {
      return ( static_cast< int >( ( m_mapNoise.GetNoise( x, z ) + 1 ) * 127.5 ) ) - 64; // TODO: make this a constant
   }

private:
   // All relevant world data
   //      Generate terrain on construction?
   // Maybe list of all entities? then Tick them in Tick()?
   // std::vector< Entity > m_EntityList;

   FastNoiseLite m_mapNoise; // https://auburn.github.io/FastNoiseLite/

   std::vector< std::shared_ptr< Entity::EntityHandle > >                  m_entityHandles;
   std::unordered_map< uint64_t, std::shared_ptr< Entity::EntityHandle > > m_playerHandles; // Maps player ID to their entity handle

   Events::EventSubscriber m_eventSubscriber;
};
