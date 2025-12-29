#include "RenderSystem.h"

#include <Engine/World/Level.h>
#include <Engine/Renderer/Shader.h>
#include <Engine/Renderer/Texture.h>

namespace Engine
{

struct WireCubeGL
{
   GLuint vao { 0 };
   GLuint vbo { 0 };
   GLuint ebo { 0 };

   void Ensure()
   {
      if( vao )
         return;

      constexpr glm::vec3 min           = glm::vec3( -0.5f );
      constexpr glm::vec3 max           = glm::vec3( 0.5f );
      constexpr glm::vec3 vertices[ 8 ] = {
         { min.x, min.y, min.z },
         { max.x, min.y, min.z },
         { max.x, max.y, min.z },
         { min.x, max.y, min.z },
         { min.x, min.y, max.z },
         { max.x, min.y, max.z },
         { max.x, max.y, max.z },
         { min.x, max.y, max.z },
      };

      constexpr unsigned int indices[ 24 ] = { 0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6, 6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7 };

      glGenVertexArrays( 1, &vao );
      glGenBuffers( 1, &vbo );
      glGenBuffers( 1, &ebo );

      glBindVertexArray( vao );

      glBindBuffer( GL_ARRAY_BUFFER, vbo );
      glBufferData( GL_ARRAY_BUFFER, sizeof( vertices ), vertices, GL_STATIC_DRAW );

      glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ebo );
      glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( indices ), indices, GL_STATIC_DRAW );

      glEnableVertexAttribArray( 0 );
      glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof( float ), ( void* )0 );

      glBindVertexArray( 0 );
   }

   void Destroy()
   {
      if( vbo )
         glDeleteBuffers( 1, &vbo );
      if( ebo )
         glDeleteBuffers( 1, &ebo );
      if( vao )
         glDeleteVertexArrays( 1, &vao );
      vao = vbo = ebo = 0;
   }

   ~WireCubeGL() { Destroy(); }
};

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
      vao = vbo = 0;
   }

   ~ReticleGL() { Destroy(); }
};

static WireCubeGL s_wireCube;
static ReticleGL  s_reticle;


RenderSystem::RenderSystem( Level& level ) noexcept :
   m_level( level )
{}


void RenderSystem::Update( const glm::vec3& playerPos, uint8_t viewRadius )
{
   m_chunkRenderer.Update( m_level, playerPos, viewRadius );
}


class ViewFrustum
{
public:
   ViewFrustum( const glm::mat4& pv )
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

   // Check if point is inside frustum
   bool FInFrustum( const glm::vec3& point ) const
   {
      return std::none_of( m_planes.begin(), m_planes.end(), [ & ]( const glm::vec4& plane ) { return glm::dot( glm::vec3( plane ), point ) + plane.w < 0; } );
   }

   // Check if AABB is inside/intersecting frustum
   bool FInFrustum( const glm::vec3& min, const glm::vec3& max ) const
   {
      for( const glm::vec4& plane : m_planes )
      {
         const glm::vec3 point( plane.x >= 0 ? max.x : min.x, plane.y >= 0 ? max.y : min.y, plane.z >= 0 ? max.z : min.z );
         if( glm::dot( glm::vec3( plane ), point ) + plane.w < 0 )
            return false;
      }

      return true; // inside or intersecting
   }

private:
   std::array< glm::vec4, 6 > m_planes; // planes: left, right, top, bottom, near, far
};


void RenderSystem::Run( const FrameContext& ctx )
{
   //PROFILE_SCOPE( "RenderSystem::Run" );
   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

   DrawTerrain( ctx );

   { // render entities
      static Shader s_terrainShader( Shader::FILE, "terrain_vert.glsl", "terrain_frag.glsl" );
      s_terrainShader.Bind();

      ViewFrustum frustum( ctx.viewProjection );
      for( auto [ e, tran, mesh ] : ctx.registry.ECView< CTransform, CMesh >() )
      {
         if( CPhysics* pPhys = ctx.registry.TryGet< CPhysics >( e ) )
         {
            if( !frustum.FInFrustum( tran.position + pPhys->bbMin, tran.position + pPhys->bbMax ) )
               continue;
         }

         const glm::mat4 model = glm::translate( glm::mat4( 1.0f ), tran.position );
         const glm::mat4 mvp   = ctx.viewProjection * model;
         s_terrainShader.SetUniform( "u_mvp", mvp );
         s_terrainShader.SetUniform( "u_model", model );

         //mesh.mesh->Render();
         g_renderBuffer.Submit( { .key           = RenderKey { 0 },
                                  .vertexArrayId = mesh.mesh->GetMeshBuffer().GetVertexArrayID(),
                                  .textureId     = 0,
                                  .indexCount    = mesh.mesh->GetMeshBuffer().GetIndexCount(),
                                  .model         = model } );
      }
      s_terrainShader.Unbind();
   }

   uint32_t currentShader  = 0;
   uint32_t currentTexture = 0;
   uint32_t currentVao     = 0;
   for( const auto& item : g_renderBuffer.GetItems() )
   {
      static Shader s_terrainShader( Shader::FILE, "terrain_vert.glsl", "terrain_frag.glsl" );
      s_terrainShader.Bind();

      glm::mat4 mvp = ctx.viewProjection * item.model;
      s_terrainShader.SetUniform( "u_mvp", mvp );
      s_terrainShader.SetUniform( "u_model", item.model );

      // 1. Only switch shader if it changes
      //if( item.shaderId != currentShader )
      //{
      //   UseShader( item.shaderId );
      //   currentShader = item.shaderId;
      //}

      // 2. Only bind texture if it changes
      //if( item.textureId != currentTexture )
      //{
      //   BindTexture( item.textureId );
      //   currentTexture = item.textureId;
      //}

      // 3. Only bind geometry if it changes
      if( item.vertexArrayId != currentVao )
      {
         glBindVertexArray( item.vertexArrayId );
         currentVao = item.vertexArrayId;
      }

      glDrawElements( GL_TRIANGLES, static_cast< GLsizei >( item.indexCount ), GL_UNSIGNED_INT, nullptr );
      s_terrainShader.Unbind();
   }
   glBindVertexArray( 0 );
   g_renderBuffer.Clear();

   if( m_fHighlightEnabled )
      DrawBlockHighlight( ctx );

   if( m_fSkyboxEnabled )
      DrawSkybox( ctx );

   if( m_fReticleEnabled )
      DrawReticle( ctx );
}


void RenderSystem::DrawTerrain( const FrameContext& ctx )
{
   static Shader s_terrainShader( Shader::FILE, "terrain_vert.glsl", "terrain_frag.glsl" );
   s_terrainShader.Bind();

   {
      glm::vec3 sunDir       = glm::normalize( glm::vec3( -0.35f, 0.85f, -0.25f ) );
      glm::vec3 sunColor     = glm::vec3( 1.0f, 0.98f, 0.92f ) * 3.0f;
      glm::vec3 ambientColor = glm::vec3( 0.12f, 0.16f, 0.22f );

      s_terrainShader.SetUniform( "u_sunDirection", sunDir );
      s_terrainShader.SetUniform( "u_sunColor", sunColor );
      s_terrainShader.SetUniform( "u_ambientColor", ambientColor );
   }

   TextureAtlasManager::Get().Bind();
   s_terrainShader.SetUniform( "u_blockTextures", 0 );
   s_terrainShader.SetUniform( "u_viewPos", ctx.viewPos );

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


void RenderSystem::DrawBlockHighlight( const FrameContext& ctx )
{
   if( !ctx.optHighlightBlock.has_value() )
      return;

   static Shader s_terrainShader( Shader::FILE, "terrain_vert.glsl", "terrain_frag.glsl" );

   s_wireCube.Ensure();

   const glm::vec3 blockPos = glm::vec3( ctx.optHighlightBlock.value() );
   const glm::mat4 model    = glm::scale( glm::translate( glm::mat4( 1.0f ), blockPos + glm::vec3( 0.5f ) ), glm::vec3( 1.02f ) );
   const glm::mat4 mvp      = ctx.viewProjection * model;

   s_terrainShader.Bind();
   s_terrainShader.SetUniform( "u_mvp", mvp );
   s_terrainShader.SetUniform( "u_model", model );

   glBindVertexArray( s_wireCube.vao );
   glDrawElements( GL_LINES, 24, GL_UNSIGNED_INT, 0 );
   glBindVertexArray( 0 );

   s_terrainShader.Unbind();
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


void RenderSystem::DrawReticle( const FrameContext& /*ctx*/ )
{
   static Shader s_terrainShader( Shader::FILE, "terrain_vert.glsl", "terrain_frag.glsl" );

   s_reticle.Ensure();

   glDisable( GL_DEPTH_TEST );

   s_terrainShader.Bind();
   s_terrainShader.SetUniform( "u_mvp", glm::mat4( 1.0f ) );

   glBindVertexArray( s_reticle.vao );
   glDrawArrays( GL_LINES, 0, 4 );
   glBindVertexArray( 0 );

   s_terrainShader.Unbind();

   glEnable( GL_DEPTH_TEST );
}

} // namespace Engine
