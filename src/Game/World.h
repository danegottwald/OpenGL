#pragma once

#include <FastNoiseLite/FastNoiseLite.h>

#include "../Events/NetworkEvent.h"

using GLuint = unsigned int;

// Forward Declarations
class Camera;
class FastNoiseLite;
class IMesh;

class World
{
public:
   World();
   ~World();

   void Setup();
   void Tick( float delta, Camera& camera );
   void Render( const Camera& camera );

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

   std::shared_ptr< IMesh >                                 m_pSun;
   std::vector< std::shared_ptr< IMesh > >                  m_chunkMeshes;
   std::vector< std::shared_ptr< IMesh > >                  m_cubeMeshes;
   std::unordered_map< uint64_t, std::shared_ptr< IMesh > > m_otherPlayers;

   std::atomic< bool > m_fChunkThreadActive { false };
   std::atomic< bool > m_fChunkThreadTerminate { false };
   std::mutex          m_chunkMutex;
   std::thread         m_chunkThread;

   Events::EventSubscriber m_eventSubscriber;
};
