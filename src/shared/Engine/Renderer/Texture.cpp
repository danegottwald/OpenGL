#include "Texture.h"

// Project Dependencies
#include <Engine/Renderer/Shader.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <json/json.h>


// ----------------------------------------------------------------
// STBFlipVerticallyOnLoad - RAII helper to flip STB image loading
// ----------------------------------------------------------------
class STBFlipVerticallyOnLoad
{
public:
   STBFlipVerticallyOnLoad( bool fValue ) noexcept :
      m_fRestore( static_cast< bool >( stbi__vertically_flip_on_load_global ) )
   {
      stbi_set_flip_vertically_on_load( fValue );
   }
   ~STBFlipVerticallyOnLoad() noexcept { stbi_set_flip_vertically_on_load( m_fRestore ); }

private:
   const bool m_fRestore { false };
};


// ----------------------------------------------------------------
// SkyboxTexture
// ----------------------------------------------------------------
static constexpr std::string_view SKYBOX_VERT_SHADER = R"(
   #version 330 core

   layout(location = 0) in vec3 aPos;

   out vec3 v_textureDir;

   uniform mat4 u_viewProjection;

   void main()
   {
       v_textureDir = aPos;

       vec4 pos = u_viewProjection * vec4(aPos, 1.0);
       gl_Position = pos.xyww;
   } )";


static constexpr std::string_view SKYBOX_FRAG_SHADER = R"(
   #version 330 core

   in vec3 v_textureDir;
   out vec4 frag_color;

   uniform samplerCube u_skybox;

   void main()
   {
       frag_color = texture(u_skybox, normalize(v_textureDir));
   } )";


SkyboxTexture::SkyboxTexture( std::span< const std::string_view > faces ) :
   m_shader( Shader::SOURCE, SKYBOX_VERT_SHADER.data(), SKYBOX_FRAG_SHADER.data() )
{
   LoadCubemap( faces );
   InitGeometry();
}


SkyboxTexture::~SkyboxTexture()
{
   glDeleteVertexArrays( 1, &m_vao );
   glDeleteBuffers( 1, &m_vbo );
   glDeleteTextures( 1, &m_textureID );
}


void SkyboxTexture::LoadCubemap( std::span< const std::string_view > faces )
{
   STBFlipVerticallyOnLoad flipGuard( false );

   glGenTextures( 1, &m_textureID );
   glBindTexture( GL_TEXTURE_CUBE_MAP, m_textureID );

   int width, height, bpp;
   for( unsigned int i = 0; i < faces.size(); i++ )
   {
      unsigned char* pData = stbi_load( faces[ i ].data(), &width, &height, &bpp, 4 );
      if( pData )
         glTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pData );
      else
         std::cerr << "SkyboxTexture::LoadCubemap - failed to load texture at " << faces[ i ] << "\n";

      stbi_image_free( pData );
   }

   glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
   glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
   glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
   glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
   glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE );
   glBindTexture( GL_TEXTURE_CUBE_MAP, 0 );
}


void SkyboxTexture::InitGeometry()
{
   // 36 (108 points) vertices (positions only) for a cube
   constexpr std::array< float, 108 > skyboxVertices {
      -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, 1.0f, -1.0f,
      -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f, 1.0f,
      1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f, 1.0f,
      1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f, -1.0f,
      1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f
   };
   glGenVertexArrays( 1, &m_vao );
   glGenBuffers( 1, &m_vbo );

   glBindVertexArray( m_vao );
   glBindBuffer( GL_ARRAY_BUFFER, m_vbo );
   glBufferData( GL_ARRAY_BUFFER, sizeof( skyboxVertices ), skyboxVertices.data(), GL_STATIC_DRAW );

   glEnableVertexAttribArray( 0 );
   glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof( float ), ( void* )0 );

   glBindVertexArray( 0 );
}


void SkyboxTexture::Draw( const glm::mat4& view, const glm::mat4& projection )
{
   glDepthFunc( GL_LEQUAL ); // skybox depth trick
   glDepthMask( GL_FALSE );

   m_shader.Bind();

   // Remove translation from the view matrix
   const glm::mat4 skyboxViewProjection = projection * glm::mat4( glm::mat3( view ) );
   m_shader.SetUniform( "u_viewProjection", skyboxViewProjection );

   glActiveTexture( GL_TEXTURE0 );
   glBindTexture( GL_TEXTURE_CUBE_MAP, m_textureID );
   m_shader.SetUniform( "u_skybox", 0 );

   glBindVertexArray( m_vao );
   glDrawArrays( GL_TRIANGLES, 0, 36 );
   glBindVertexArray( 0 );

   m_shader.Unbind();

   glDepthMask( GL_TRUE );
   glDepthFunc( GL_LESS );
}


// ----------------------------------------------------------------
// TextureAtlas
// ----------------------------------------------------------------
TextureAtlas::~TextureAtlas()
{
   if( m_rendererID )
      glDeleteTextures( 1, &m_rendererID );
}


static uint64_t HashStringViewFNV1a( std::string_view s )
{
   uint64_t h = 1469598103934665603ull;
   for( unsigned char c : s )
   {
      h ^= static_cast< uint64_t >( c );
      h *= 1099511628211ull;
   }

   return h;
}


void TextureAtlas::PrepareTexture( BlockId blockId )
{
   auto addPath = [ & ]( std::string_view path ) -> uint64_t
   {
      if( path.empty() )
         return 0ull;

      const uint64_t key = HashStringViewFNV1a( path );
      if( !m_pending.contains( key ) )
      {
         int            width = 0, height = 0, bpp = 0;
         unsigned char* pData = stbi_load( path.data(), &width, &height, &bpp, 4 );
         if( !pData )
         {
            std::cerr << "TextureAtlas::PrepareTexture - failed to load texture: " << path << " reason: " << stbi_failure_reason() << "\n";
            return 0ull;
         }

         PendingTexture texture;
         texture.filepath = path;
         texture.width    = width;
         texture.height   = height;
         texture.pixels   = std::vector< unsigned char >( pData, pData + static_cast< size_t >( width ) * height * 4 );
         m_pending[ key ] = std::move( texture );
         stbi_image_free( pData );
      }

      return key;
   };

   const BlockInfo& info = GetBlockInfo( blockId );
   if( info.json.empty() )
      return;

   std::ifstream file( info.json.data() );
   if( !file.is_open() )
   {
      std::cerr << "TextureAtlas::PrepareTexture - warning: could not open block json: " << info.json << "\n";
      return;
   }

   Json::Value             root;
   Json::CharReaderBuilder builder;
   std::string             errs;
   if( !Json::parseFromStream( builder, file, &root, &errs ) )
   {
      std::cerr << "TextureAtlas::PrepareTexture - warning: failed to parse block json: " << errs << "\n";
      return;
   }

   const Json::Value& attrTexture = root[ "textures" ];
   std::string        top, bottom, north, south, west, east;
   if( attrTexture.isString() )
      north = south = west = east = top = bottom = attrTexture.asString();
   else if( attrTexture.isObject() )
   {
      auto getString = []( const Json::Value& v, const char* k ) -> std::string { return v.isMember( k ) ? v[ k ].asString() : ""; };
      top            = getString( attrTexture, "top" );
      bottom         = getString( attrTexture, "bottom" );
      if( const std::string side = getString( attrTexture, "side" ); !side.empty() )
         north = east = south = west = side;
      else
      {
         north = getString( attrTexture, "north" );
         east  = getString( attrTexture, "east" );
         south = getString( attrTexture, "south" );
         west  = getString( attrTexture, "west" );
      }
   }
   else
   {
      std::cerr << "TextureAtlas::PrepareTexture - warning: block json 'textures' attribute is neither string nor object: " << info.json << "\n";
      return;
   }

   STBFlipVerticallyOnLoad flipGuard( true );

   // Prepare all face textures
   auto& keys                                                  = m_blockFaceKeys[ static_cast< size_t >( blockId ) ];
   keys.faceKeys[ static_cast< size_t >( BlockFace::North ) ]  = addPath( north );
   keys.faceKeys[ static_cast< size_t >( BlockFace::East ) ]   = addPath( east );
   keys.faceKeys[ static_cast< size_t >( BlockFace::South ) ]  = addPath( south );
   keys.faceKeys[ static_cast< size_t >( BlockFace::West ) ]   = addPath( west );
   keys.faceKeys[ static_cast< size_t >( BlockFace::Top ) ]    = addPath( top );
   keys.faceKeys[ static_cast< size_t >( BlockFace::Bottom ) ] = addPath( bottom );
}

void TextureAtlas::Compile()
{
   if( m_rendererID )
   {
      glDeleteTextures( 1, &m_rendererID );
      m_rendererID = 0;
   }

   m_width      = 0;
   m_height     = 0;
   m_layerCount = 0;
   m_regions.clear();
   if( m_pending.empty() )
      return;

   // Determine max dimensions (all layers must be same size)
   for( const auto& [ _, texture ] : m_pending )
   {
      m_width  = ( std::max )( m_width, texture.width );
      m_height = ( std::max )( m_height, texture.height );
   }
   m_layerCount = static_cast< int >( m_pending.size() );

   // Create texture array
   glGenTextures( 1, &m_rendererID );
   glBindTexture( GL_TEXTURE_2D_ARRAY, m_rendererID );
   glTexImage3D( GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, m_width, m_height, m_layerCount, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr );

   // Upload each texture as a layer
   uint32_t layerIndex = 0;
   for( auto& [ key, texture ] : m_pending )
   {
      // If texture is smaller than max dimensions, we need to pad it
      if( texture.width == m_width && texture.height == m_height )
      {
         glTexSubImage3D( GL_TEXTURE_2D_ARRAY, 0, 0, 0, layerIndex, m_width, m_height, 1, GL_RGBA, GL_UNSIGNED_BYTE, texture.pixels.data() );
      }
      else
      {
         // Create padded buffer (replicate edge pixels)
         std::vector< unsigned char > padded( static_cast< size_t >( m_width ) * m_height * 4, 0 );
         for( int y = 0; y < m_height; ++y )
         {
            for( int x = 0; x < m_width; ++x )
            {
               const int srcX = std::clamp( x, 0, texture.width - 1 );
               const int srcY = std::clamp( y, 0, texture.height - 1 );

               const unsigned char* src = texture.pixels.data() + ( srcY * texture.width + srcX ) * 4;
               unsigned char*       dst = padded.data() + ( y * m_width + x ) * 4;
               dst[ 0 ]                 = src[ 0 ];
               dst[ 1 ]                 = src[ 1 ];
               dst[ 2 ]                 = src[ 2 ];
               dst[ 3 ]                 = src[ 3 ];
            }
         }
         glTexSubImage3D( GL_TEXTURE_2D_ARRAY, 0, 0, 0, layerIndex, m_width, m_height, 1, GL_RGBA, GL_UNSIGNED_BYTE, padded.data() );
      }

      m_regions[ key ] = Region { .layer = layerIndex };
      ++layerIndex;
   }
   m_pending.clear();

   glGenerateMipmap( GL_TEXTURE_2D_ARRAY );

   glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
   glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
   glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR );
   glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

   glBindTexture( GL_TEXTURE_2D_ARRAY, 0 );
}


void TextureAtlas::Bind( unsigned int slot ) const
{
   glActiveTexture( GL_TEXTURE0 + slot );
   glBindTexture( GL_TEXTURE_2D_ARRAY, m_rendererID );
}


void TextureAtlas::Unbind() const
{
   glBindTexture( GL_TEXTURE_2D_ARRAY, 0 );
}


const TextureAtlas::Region& TextureAtlas::GetRegion( BlockId blockId, BlockFace face ) const
{
   return m_regions.at( m_blockFaceKeys[ static_cast< size_t >( blockId ) ].faceKeys[ static_cast< size_t >( face ) ] );
}


const TextureAtlas::Region& TextureAtlas::GetRegion( BlockState state, BlockFace face ) const
{
   BlockFace rotatedFace = face;
   if( face <= BlockFace::West )
   {
      // Table: [Orientation][Face] -> RotatedFace
      // North = 0°, East = 90°, South = 180°, West = 270°
      static constexpr BlockFace rotationTable[ 4 ][ 4 ] = {
         { BlockFace::North, BlockFace::East,  BlockFace::South, BlockFace::West  }, // North (0°)
         { BlockFace::West,  BlockFace::North, BlockFace::East,  BlockFace::South }, // East (90° clockwise)
         { BlockFace::South, BlockFace::West,  BlockFace::North, BlockFace::East  }, // South (180°)
         { BlockFace::East,  BlockFace::South, BlockFace::West,  BlockFace::North }  // West (270° clockwise)
      };

      if( const BlockOrientation ori = state.GetOrientation(); ori <= BlockOrientation::West )
         rotatedFace = rotationTable[ static_cast< uint8_t >( ori ) ][ static_cast< uint8_t >( face ) ];
   }

   return GetRegion( state.GetId(), rotatedFace );
}

// ----------------------------------------------------------------
// TextureAtlasManager
// ----------------------------------------------------------------
/*static*/ TextureAtlasManager& TextureAtlasManager::Get() noexcept
{
   static TextureAtlasManager instance;
   return instance;
}


void TextureAtlasManager::CompileBlockAtlas()
{
   try
   {
      for( int i = 0; i < static_cast< int >( BlockId::Count ); ++i )
         m_blockAtlas.PrepareTexture( static_cast< BlockId >( i ) );

      m_blockAtlas.Compile();
   }
   catch( ... )
   {
      std::cerr << "TextureManager::CompileBlockAtlas - failed to compile block atlas" << std::endl;
   }
}
