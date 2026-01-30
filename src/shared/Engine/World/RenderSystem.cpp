#include "RenderSystem.h"

#include <Engine/World/Level.h>
#include <Engine/Renderer/Shader.h>
#include <Engine/Renderer/Texture.h>
#include <Engine/ECS/Components.h>

namespace Engine
{

struct ReticleGL
{
   GLuint vao = 0;
   GLuint vbo = 0;

   void Ensure()
   {
      if( vao )
         return;

      const float                      r     = 0.01f;
      const std::array< glm::vec3, 4 > verts = {
         glm::vec3( -r, 0.0f, 0.0f ), glm::vec3( r, 0.0f, 0.0f ), glm::vec3( 0.0f, -r, 0.0f ), glm::vec3( 0.0f, r, 0.0f )
      };

      glGenVertexArrays( 1, &vao );
      glGenBuffers( 1, &vbo );

      glBindVertexArray( vao );
      glBindBuffer( GL_ARRAY_BUFFER, vbo );
      glBufferData( GL_ARRAY_BUFFER, sizeof( glm::vec3 ) * verts.size(), verts.data(), GL_STATIC_DRAW );

      glEnableVertexAttribArray( 0 );
      glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof( float ), ( void* )0 );

      glBindVertexArray( 0 );
   }

   void Destroy()
   {
      if( vbo )
         glDeleteBuffers( 1, &vbo );
      if( vao )
         glDeleteVertexArrays( 1, &vao );

      vbo = 0;
      vao = 0;
   }

   ~ReticleGL() { Destroy(); }
};


// ----------------------------------------------------------------
// ViewFrustum - frustum culling helper
// ----------------------------------------------------------------
class ViewFrustum
{
public:
   explicit ViewFrustum( const glm::mat4& pv )
   {
      m_planes[ 0 ] = glm::vec4( pv[ 0 ][ 3 ] + pv[ 0 ][ 0 ], pv[ 1 ][ 3 ] + pv[ 1 ][ 0 ], pv[ 2 ][ 3 ] + pv[ 2 ][ 0 ], pv[ 3 ][ 3 ] + pv[ 3 ][ 0 ] );
      m_planes[ 1 ] = glm::vec4( pv[ 0 ][ 3 ] - pv[ 0 ][ 0 ], pv[ 1 ][ 3 ] - pv[ 1 ][ 0 ], pv[ 2 ][ 3 ] - pv[ 2 ][ 0 ], pv[ 3 ][ 3 ] - pv[ 3 ][ 0 ] );
      m_planes[ 2 ] = glm::vec4( pv[ 0 ][ 3 ] - pv[ 0 ][ 1 ], pv[ 1 ][ 3 ] - pv[ 1 ][ 1 ], pv[ 2 ][ 3 ] - pv[ 2 ][ 1 ], pv[ 3 ][ 3 ] - pv[ 3 ][ 1 ] );
      m_planes[ 3 ] = glm::vec4( pv[ 0 ][ 3 ] + pv[ 0 ][ 1 ], pv[ 1 ][ 3 ] + pv[ 1 ][ 1 ], pv[ 2 ][ 3 ] + pv[ 2 ][ 1 ], pv[ 3 ][ 3 ] + pv[ 3 ][ 1 ] );
      m_planes[ 4 ] = glm::vec4( pv[ 0 ][ 3 ] + pv[ 0 ][ 2 ], pv[ 1 ][ 3 ] + pv[ 1 ][ 2 ], pv[ 2 ][ 3 ] + pv[ 2 ][ 2 ], pv[ 3 ][ 3 ] + pv[ 3 ][ 2 ] );
      m_planes[ 5 ] = glm::vec4( pv[ 0 ][ 3 ] - pv[ 0 ][ 2 ], pv[ 1 ][ 3 ] - pv[ 1 ][ 2 ], pv[ 2 ][ 3 ] - pv[ 2 ][ 2 ], pv[ 3 ][ 3 ] - pv[ 3 ][ 2 ] );
      for( glm::vec4& plane : m_planes )
         plane /= glm::length( glm::vec3( plane ) );
   }

   bool FInFrustum( const glm::vec3& min, const glm::vec3& max ) const
   {
      for( const glm::vec4& plane : m_planes )
      {
         const glm::vec3 point( plane.x >= 0 ? max.x : min.x, plane.y >= 0 ? max.y : min.y, plane.z >= 0 ? max.z : min.z );
         if( glm::dot( glm::vec3( plane ), point ) + plane.w < 0 )
            return false;
      }

      return true;
   }

private:
   std::array< glm::vec4, 6 > m_planes;
};

struct TerrainLighting
{
   glm::vec3 sunDir { glm::normalize( glm::vec3( -0.35f, 0.85f, -0.25f ) ) };
   glm::vec3 sunColor { glm::vec3( 1.0f, 0.98f, 0.92f ) * 3.0f };
   glm::vec3 ambientColor { glm::vec3( 0.12f, 0.16f, 0.22f ) };
};

static void SetTerrainCommonUniforms( Shader& shader, const glm::vec3& viewPos, const TerrainLighting& light )
{
   shader.SetUniform( "u_sunDirection", light.sunDir );
   shader.SetUniform( "u_sunColor", light.sunColor );
   shader.SetUniform( "u_ambientColor", light.ambientColor );
   shader.SetUniform( "u_blockTextures", 0 );
   shader.SetUniform( "u_viewPos", viewPos );
}


// ----------------------------------------------------------------
// RenderSystem
// ----------------------------------------------------------------
RenderSystem::RenderSystem( Level& level ) noexcept :
   m_level( level )
{}


void RenderSystem::Update( const glm::vec3& playerPos, uint8_t viewRadius )
{
   m_chunkRenderer.Update( m_level, playerPos, viewRadius );
}


void RenderSystem::Run( const FrameContext& ctx )
{
   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

   RenderQueues queues;
   BuildQueues( ctx, queues );

   DrawTerrainPass( ctx );
   DrawOpaquePass( ctx, queues );

   if( m_fHighlightEnabled )
      DrawBlockHighlight( ctx );

   if( m_fSkyboxEnabled )
      DrawSkybox( ctx );

   DrawOverlayPass( ctx, queues );

   if( m_fReticleEnabled )
      DrawReticle( ctx );
}


void RenderSystem::BuildQueues( const FrameContext& ctx, RenderQueues& outQueues )
{
   outQueues.Clear();

   QueueItemDrops( ctx, outQueues );

   outQueues.Sort();
}


void RenderSystem::DrawTerrainPass( const FrameContext& ctx )
{
   TextureAtlasManager::Get().Bind();

   static Shader s_terrainShader( Shader::FILE, "terrain_vert.glsl", "terrain_frag.glsl" );
   s_terrainShader.Bind();

   TerrainLighting lighting;
   SetTerrainCommonUniforms( s_terrainShader, ctx.viewPos, lighting );

   ViewFrustum frustum( ctx.viewProjection );
   for( const auto& [ cc, chunkEntry ] : m_chunkRenderer.GetEntries() )
   {
      const float worldX0 = static_cast< float >( cc.x * CHUNK_SIZE_X );
      const float worldZ0 = static_cast< float >( cc.z * CHUNK_SIZE_Z );
      for( const auto& [ i, sec ] : chunkEntry.sections | std::views::enumerate )
      {
         if( sec.fEmpty || sec.indexCount == 0 || sec.vao == 0 )
            continue;

         const float     y0 = static_cast< float >( i * CHUNK_SECTION_SIZE );
         const glm::vec3 secMin( worldX0, y0, worldZ0 );
         const glm::vec3 secMax( worldX0 + CHUNK_SIZE_X, y0 + CHUNK_SECTION_SIZE, worldZ0 + CHUNK_SIZE_Z );
         if( !frustum.FInFrustum( secMin, secMax ) )
            continue;

         // Section mesh has world-space Y baked in already. Only translate by chunk XZ.
         const glm::mat4 model = glm::translate( glm::mat4( 1.0f ), glm::vec3( worldX0, 0.0f, worldZ0 ) );
         const glm::mat4 mvp   = ctx.viewProjection * model;

         s_terrainShader.SetUniform( "u_mvp", mvp );
         s_terrainShader.SetUniform( "u_model", model );

         glBindVertexArray( sec.vao );
         glDrawElements( GL_TRIANGLES, static_cast< GLsizei >( sec.indexCount ), GL_UNSIGNED_INT, nullptr );
      }
   }

   glBindVertexArray( 0 );
   TextureAtlasManager::Get().Unbind();

   s_terrainShader.Unbind();
}


void RenderSystem::QueueItemDrops( const FrameContext& ctx, RenderQueues& outQueues )
{
   // Animation constants
   constexpr float ROTATION_SPEED = 45.0f; // degrees/sec
   constexpr float BOB_SPEED      = 4.0f;  // frequency
   constexpr float BOB_HEIGHT     = 0.1f;  // amplitude

   // Culling
   constexpr float MAX_CHUNK_DISTANCE        = 4.0f;
   constexpr float MAX_ITEM_DROP_DISTANCE    = CHUNK_SECTION_SIZE * MAX_CHUNK_DISTANCE;
   constexpr float MAX_ITEM_DROP_DISTANCE_SQ = MAX_ITEM_DROP_DISTANCE * MAX_ITEM_DROP_DISTANCE;

   ViewFrustum frustum( ctx.viewProjection );
   for( auto [ tran, mesh, phys, drop ] : ctx.registry.CView< CTransform, CMesh, CPhysics, CItemDrop >() )
   {
      // Culling: distance and frustum
      const glm::vec3 toDrop = tran.position - ctx.viewPos;
      if( glm::dot( toDrop, toDrop ) > MAX_ITEM_DROP_DISTANCE_SQ || !frustum.FInFrustum( tran.position + phys.bbMin, tran.position + phys.bbMax ) )
         continue;

      // Interpolate position
      glm::vec3 renderPos = tran.prevPosition + ( tran.position - tran.prevPosition ) * ctx.time.GetTickFraction();

      // Bobbing effect
      const float t = ( drop.maxTicks - drop.ticksRemaining ) * ctx.time.GetTickInterval() + ctx.time.GetTickAccumulator();
      renderPos.y += std::sin( t * BOB_SPEED ) * BOB_HEIGHT + BOB_HEIGHT;

      // Rotation around Y axis
      const float rotationY = t * ROTATION_SPEED;

      glm::mat4 model = glm::translate( glm::mat4( 1.0f ), renderPos );
      model           = glm::rotate( model, glm::radians( tran.rotation.x ), glm::vec3( 1, 0, 0 ) );
      model           = glm::rotate( model, glm::radians( rotationY ), glm::vec3( 0, 1, 0 ) );
      model           = glm::rotate( model, glm::radians( tran.rotation.z ), glm::vec3( 0, 0, 1 ) );

      outQueues.SubmitOpaque( RenderQueues::IndexedDraw { .key           = RenderKey { 0 },
                                                          .vertexArrayId = mesh.mesh->GetMeshBuffer().GetVertexArrayID(),
                                                          .textureId     = 0,
                                                          .indexCount    = mesh.mesh->GetMeshBuffer().GetIndexCount(),
                                                          .model         = model } );
   }
}


void RenderSystem::DrawOpaquePass( const FrameContext& ctx, const RenderQueues& queues )
{
   if( queues.GetOpaqueIndexed().empty() )
      return;

   TextureAtlasManager::Get().Bind();

   static Shader s_terrainShader( Shader::FILE, "terrain_vert.glsl", "terrain_frag.glsl" );
   s_terrainShader.Bind();

   TerrainLighting lighting;
   SetTerrainCommonUniforms( s_terrainShader, ctx.viewPos, lighting );

   uint32_t currentVao = 0;
   for( const auto& item : queues.GetOpaqueIndexed() )
   {
      const glm::mat4 mvp = ctx.viewProjection * item.model;
      s_terrainShader.SetUniform( "u_mvp", mvp );
      s_terrainShader.SetUniform( "u_model", item.model );

      if( item.vertexArrayId != currentVao )
      {
         glBindVertexArray( item.vertexArrayId );
         currentVao = item.vertexArrayId;
      }

      glDrawElements( GL_TRIANGLES, static_cast< GLsizei >( item.indexCount ), GL_UNSIGNED_INT, nullptr );
   }

   glBindVertexArray( 0 );
   TextureAtlasManager::Get().Unbind();

   s_terrainShader.Unbind();
}


void RenderSystem::DrawOverlayPass( const FrameContext& /*ctx*/, const RenderQueues& queues )
{
   if( queues.GetOverlayIndexed().empty() )
      return;

   // Reserved for future: transparent objects, UI meshes, debug.
   // Currently unused.
}


void RenderSystem::DrawBlockHighlight( const FrameContext& ctx )
{
   if( !ctx.optHighlightBlock.has_value() )
      return;

   // Static shader
   static Shader s_shader( Shader::SOURCE,
                           R"(
      #version 330 core
      layout(location = 0) in vec3 a_position;
      uniform mat4 u_mvp;
      uniform float u_depthBias;
      void main()
      {
         gl_Position = u_mvp * vec4(a_position, 1.0);
         gl_Position.z -= u_depthBias * gl_Position.w;
      }
      )",
                           R"(
      #version 330 core
      uniform vec3 u_color;
      out vec4 FragColor;
      void main(){ FragColor = vec4(u_color,1.0); }
      )" );

   // Static OpenGL resources for wire cube
   static GLuint s_vao = 0, s_vbo = 0, s_ebo = 0;
   if( s_vao == 0 )
   {
      constexpr glm::vec3 vertices[ 8 ] = {
         { -0.5f, -0.5f, -0.5f },
         { 0.5f,  -0.5f, -0.5f },
         { 0.5f,  0.5f,  -0.5f },
         { -0.5f, 0.5f,  -0.5f },
         { -0.5f, -0.5f, 0.5f  },
         { 0.5f,  -0.5f, 0.5f  },
         { 0.5f,  0.5f,  0.5f  },
         { -0.5f, 0.5f,  0.5f  },
      };

      glGenVertexArrays( 1, &s_vao );
      glGenBuffers( 1, &s_vbo );
      glGenBuffers( 1, &s_ebo );

      glBindVertexArray( s_vao );
      glBindBuffer( GL_ARRAY_BUFFER, s_vbo );
      glBufferData( GL_ARRAY_BUFFER, sizeof( vertices ), vertices, GL_STATIC_DRAW );
      glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, s_ebo );
      glEnableVertexAttribArray( 0 );
      glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof( glm::vec3 ), nullptr );
      glBindVertexArray( 0 );
   }

   // Face edge indices for exposed faces
   // Vertices: 0(-X,-Y,-Z) 1(+X,-Y,-Z) 2(+X,+Y,-Z) 3(-X,+Y,-Z) 4(-X,-Y,+Z) 5(+X,-Y,+Z) 6(+X,+Y,+Z) 7(-X,+Y,+Z)
   struct FaceEdges
   {
      glm::ivec3   offset;
      unsigned int edges[ 8 ];
   };
   static constexpr FaceEdges s_faces[ 6 ] = {
      { { -1, 0, 0 }, { 0, 3, 3, 7, 7, 4, 4, 0 } }, // -X
      { { 1, 0, 0 },  { 1, 2, 2, 6, 6, 5, 5, 1 } }, // +X
      { { 0, -1, 0 }, { 0, 1, 1, 5, 5, 4, 4, 0 } }, // -Y
      { { 0, 1, 0 },  { 3, 2, 2, 6, 6, 7, 7, 3 } }, // +Y
      { { 0, 0, -1 }, { 0, 1, 1, 2, 2, 3, 3, 0 } }, // -Z
      { { 0, 0, 1 },  { 4, 5, 5, 6, 6, 7, 7, 4 } }, // +Z
   };

   // Build indices for exposed faces only
   std::vector< unsigned int > indices;
   indices.reserve( 48 );

   const glm::ivec3 blockPos = ctx.optHighlightBlock.value();
   for( const FaceEdges& face : s_faces )
   {
      const BlockState neighbor = m_level.GetBlock( WorldBlockPos( blockPos + face.offset ) );
      if( !FHasFlag( GetBlockInfo( neighbor ).flags, BlockFlag::Opaque ) )
      {
         for( unsigned int idx : face.edges )
            indices.push_back( idx );
      }
   }

   if( indices.empty() )
      return;

   s_shader.Bind();
   s_shader.SetUniform( "u_depthBias", 0.0005f );

   // MVP matrix
   const glm::mat4 model = glm::translate( glm::mat4( 1.0f ), glm::vec3( blockPos ) + glm::vec3( 0.5f ) );
   const glm::mat4 mvp   = ctx.viewProjection * model;
   s_shader.SetUniform( "u_mvp", mvp );

   // Pulsating color
   constexpr float pulsePeriod    = 2.0f;
   const float     pulse          = 0.5f + 0.5f * std::sin( ctx.time.GetElapsedTime() * ( 2.0f * glm::pi< float >() / pulsePeriod ) );
   const glm::vec3 highlightColor = glm::vec3( 1.0f, 0.5f, 0.0f ) * pulse;
   s_shader.SetUniform( "u_color", highlightColor );

   glBindVertexArray( s_vao );
   glBufferData( GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof( unsigned int ), indices.data(), GL_DYNAMIC_DRAW );
   glDrawElements( GL_LINES, static_cast< GLsizei >( indices.size() ), GL_UNSIGNED_INT, nullptr );
   glBindVertexArray( 0 );

   s_shader.Unbind();
}


void RenderSystem::DrawSkybox( const FrameContext& ctx )
{
   constexpr std::array< std::string_view, 6 > faceFiles = {
      "assets/textures/skybox/sky2/px.png", "assets/textures/skybox/sky2/nx.png", "assets/textures/skybox/sky2/py.png",
      "assets/textures/skybox/sky2/ny.png", "assets/textures/skybox/sky2/pz.png", "assets/textures/skybox/sky2/nz.png",
   };

   static SkyboxTexture s_skyboxTexture( faceFiles );
   s_skyboxTexture.Draw( ctx.view, ctx.projection );
}


void RenderSystem::DrawReticle( const FrameContext& ctx )
{
   static Shader s_reticleShader( Shader::SOURCE,
                                  R"(
       #version 330 core
       layout(location = 0) in vec3 a_position;
       uniform float u_aspect;
       void main() { gl_Position = vec4(a_position.x / u_aspect, a_position.y, a_position.z, 1.0); }
       )",
                                  R"(
       #version 330 core
       uniform vec3 u_color;
       out vec4 FragColor;
       void main() { FragColor = vec4(u_color, 1.0); }
       )" );

   static ReticleGL s_reticle;
   s_reticle.Ensure();

   s_reticleShader.Bind();
   s_reticleShader.SetUniform( "u_color", glm::vec3( 1.0f ) ); // white reticle

   GLint vp[ 4 ];
   glGetIntegerv( GL_VIEWPORT, vp );
   s_reticleShader.SetUniform( "u_aspect", float( vp[ 2 ] ) / float( vp[ 3 ] ) );

   glDisable( GL_DEPTH_TEST ); // disable depth testing, render over everything

   glBindVertexArray( s_reticle.vao );
   glDrawArrays( GL_LINES, 0, 4 );
   glBindVertexArray( 0 );

   glEnable( GL_DEPTH_TEST ); // restore depth testing

   s_reticleShader.Unbind();
}

} // namespace Engine
