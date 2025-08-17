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
}

class World
{
public:
   World();
   ~World() = default;

   void Setup( Entity::Registry& registry );
   void Tick( float delta, const glm::vec3& position );
   void Render( Entity::Registry& registry, const glm::vec3& position, const glm::mat4& projectionView );

   void ReceivePlayerPosition( uint64_t clientID, const glm::vec3& position );

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

   std::unordered_map< uint64_t, std::shared_ptr< IMesh > > m_otherPlayers;

   Events::EventSubscriber m_eventSubscriber;
};
