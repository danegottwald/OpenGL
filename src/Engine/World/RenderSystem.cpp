#include "RenderSystem.h"

#include <Engine/World/Level.h>
#include <Engine/Renderer/Shader.h>
#include <Engine/Renderer/Texture.h>

namespace Engine
{

namespace
{
struct WireCubeGL
{
   GLuint vao = 0;
   GLuint vbo = 0;
   GLuint ebo = 0;

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
}


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

         mesh.mesh->Render();
      }
      s_terrainShader.Unbind();
   }

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

   for( const auto& [ cc, entry ] : m_chunkRenderer.GetEntries() )
   {
      if( entry.indexCount == 0 || entry.vao == 0 )
         continue;

      const glm::vec3 chunkMin = glm::vec3( cc.x * CHUNK_SIZE_X, 0.0f, cc.z * CHUNK_SIZE_Z );
      const glm::mat4 model    = glm::translate( glm::mat4( 1.0f ), chunkMin );
      const glm::mat4 mvp      = ctx.viewProjection * model;
      s_terrainShader.SetUniform( "u_mvp", mvp );
      s_terrainShader.SetUniform( "u_model", model );

      glBindVertexArray( entry.vao );
      glDrawElements( GL_TRIANGLES, static_cast< GLsizei >( entry.indexCount ), GL_UNSIGNED_INT, nullptr );
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
      "assets/textures/skybox/px.png", "assets/textures/skybox/nx.png", "assets/textures/skybox/py.png",
      "assets/textures/skybox/ny.png", "assets/textures/skybox/pz.png", "assets/textures/skybox/nz.png",
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
