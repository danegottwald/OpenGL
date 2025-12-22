#include "Texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <Renderer/Shader.h>


constexpr std::string_view SKYBOX_VERT_SHADER = R"(
   #version 330 core

   layout(location = 0) in vec3 aPos;

   out vec3 v_TexDir;

   uniform mat4 u_View;
   uniform mat4 u_Projection;

   void main()
   {
       v_TexDir = aPos;

       vec4 pos = u_Projection * u_View * vec4(aPos, 1.0);
       gl_Position = pos.xyww;
   } )";


constexpr std::string_view SKYBOX_FRAG_SHADER = R"(
   #version 330 core

   in vec3 v_TexDir;
   out vec4 frag_color;

   uniform samplerCube u_Skybox;

   void main()
   {
       frag_color = texture(u_Skybox, normalize(v_TexDir));
   } )";


// ----------------------------------------------------------------
// SkyboxTexture
// ----------------------------------------------------------------
SkyboxTexture::SkyboxTexture( std::span< const std::string_view > faces ) :
   m_pShader( std::make_unique< Shader >( Shader::SOURCE, SKYBOX_VERT_SHADER.data(), SKYBOX_FRAG_SHADER.data() ) )
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


void SkyboxTexture::Draw( const glm::mat4& view, const glm::mat4& projection )
{
   glDepthFunc( GL_LEQUAL ); // skybox depth trick
   glDepthMask( GL_FALSE );

   m_pShader->Bind();
   m_pShader->SetUniform( "u_View", glm::mat4( glm::mat3( view ) ) );
   m_pShader->SetUniform( "u_Projection", projection );

   glActiveTexture( GL_TEXTURE0 );
   glBindTexture( GL_TEXTURE_CUBE_MAP, m_textureID );
   m_pShader->SetUniform( "u_Skybox", 0 );

   glBindVertexArray( m_vao );
   glDrawArrays( GL_TRIANGLES, 0, 36 );
   glBindVertexArray( 0 );

   m_pShader->Unbind();

   glDepthMask( GL_TRUE );
   glDepthFunc( GL_LESS );
}


class STBFlipVerticallyOnLoad
{
public:
   STBFlipVerticallyOnLoad( bool fValue ) :
      m_fRestore( static_cast< bool >( stbi__vertically_flip_on_load_global ) )
   {
      stbi_set_flip_vertically_on_load( fValue );
   }
   ~STBFlipVerticallyOnLoad() { stbi_set_flip_vertically_on_load( m_fRestore ); }

private:
   bool m_fRestore { false };
};


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


// ----------------------------------------------------------------
// TextureAtlas
// ----------------------------------------------------------------
TextureAtlas::TextureAtlas() noexcept = default;


TextureAtlas::~TextureAtlas()
{
   if( m_rendererID )
      glDeleteTextures( 1, &m_rendererID );
}


void TextureAtlas::PrepareTexture( BlockId blockId )
{
   std::string_view filepath = GetBlockInfo( blockId ).texturePath;
   if( filepath.empty() )
      return;

   STBFlipVerticallyOnLoad flipGuard( false );

   int            width = 0, height = 0, bpp = 0;
   unsigned char* pData = stbi_load( filepath.data(), &width, &height, &bpp, 4 );
   if( !pData )
      throw std::runtime_error( "TextureAtlas::AddTexture - failed to load texture: " + std::string( filepath ) + " reason: " + stbi_failure_reason() );

   PendingTexture texture;
   texture.blockId      = blockId;
   texture.filepath     = filepath;
   texture.width        = width;
   texture.height       = height;
   texture.pixels       = std::vector< unsigned char >( pData, pData + static_cast< size_t >( width ) * height * 4 );
   m_pending[ blockId ] = std::move( texture );

   stbi_image_free( pData );
}


void TextureAtlas::Compile()
{
   if( m_rendererID )
   {
      glDeleteTextures( 1, &m_rendererID );
      m_rendererID = 0;
   }

   m_regions.clear();
   m_width  = 0;
   m_height = 0;

   if( m_pending.empty() )
      return;

   int padding = ( std::max )( 0, m_padding );
   if( m_filtering == Filtering::MipmappedAnisotropic )
      padding = ( std::max )( padding, 16 );

   // Horizontal packing with per-image gutters
   int totalWidth = 0;
   int maxHeight  = 0;
   for( auto& [ _, texture ] : m_pending )
   {
      totalWidth += texture.width + padding * 2;
      maxHeight = ( std::max )( maxHeight, texture.height + padding * 2 );
   }

   m_width  = totalWidth;
   m_height = maxHeight;

   auto getPixel = [ & ]( const PendingTexture& texture, int x, int y ) -> const unsigned char*
   {
      x = std::clamp( x, 0, texture.width - 1 );
      y = std::clamp( y, 0, texture.height - 1 );
      return texture.pixels.data() + ( ( y * texture.width + x ) * 4 );
   };

   int                          xOffset = 0;
   std::vector< unsigned char > atlas( static_cast< size_t >( m_width ) * m_height * 4, 0 );
   for( auto& [ blockId, texture ] : m_pending )
   {
      const int dstX0 = xOffset;
      const int dstY0 = 0;

      // Copy image + extrude border into atlas
      for( int y = 0; y < texture.height + padding * 2; ++y )
      {
         for( int x = 0; x < texture.width + padding * 2; ++x )
         {
            const int            srcX       = x - padding;
            const int            srcY       = y - padding;
            const unsigned char* rgba       = getPixel( texture, srcX, srcY );
            const int            writeX     = dstX0 + x;
            const int            writeY     = dstY0 + y;
            const size_t         atlasIndex = static_cast< size_t >( writeY * m_width + writeX );
            unsigned char*       dst        = atlas.data() + atlasIndex * 4;
            dst[ 0 ]                        = rgba[ 0 ];
            dst[ 1 ]                        = rgba[ 1 ];
            dst[ 2 ]                        = rgba[ 2 ];
            dst[ 3 ]                        = rgba[ 3 ];
         }
      }

      const float invW = 1.0f / static_cast< float >( m_width );
      const float invH = 1.0f / static_cast< float >( m_height );

      float u0 = static_cast< float >( dstX0 + padding ) * invW;
      float v0 = static_cast< float >( dstY0 + padding ) * invH;
      float u1 = static_cast< float >( dstX0 + padding + texture.width ) * invW;
      float v1 = static_cast< float >( dstY0 + padding + texture.height ) * invH;

      if( m_filtering == Filtering::MipmappedAnisotropic )
      {
         const float halfTexelU = 0.5f * invW;
         const float halfTexelV = 0.5f * invH;
         u0 += halfTexelU;
         v0 += halfTexelV;
         u1 -= halfTexelU;
         v1 -= halfTexelV;
      }

      m_regions[ blockId ] = Region {
         .uvs = { glm::vec2( u0, v0 ), glm::vec2( u1, v0 ), glm::vec2( u1, v1 ), glm::vec2( u0, v1 ) }
      };

      xOffset += texture.width + padding * 2;
   }

   // Upload to OpenGL
   glGenTextures( 1, &m_rendererID );
   glBindTexture( GL_TEXTURE_2D, m_rendererID );
   glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, atlas.data() );

   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

   if( m_filtering == Filtering::PixelPerfect )
   {
      glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
      glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
      glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0 );
   }
   else
   {
      glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
      glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

      GLfloat maxAniso = 0.0f;
      glGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso );
      if( maxAniso > 0.0f )
         glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, ( std::min )( 16.0f, maxAniso ) );

      glGenerateMipmap( GL_TEXTURE_2D );
   }

   glBindTexture( GL_TEXTURE_2D, 0 );
}


void TextureAtlas::Bind( unsigned int slot ) const
{
   glActiveTexture( GL_TEXTURE0 + slot );
   glBindTexture( GL_TEXTURE_2D, m_rendererID );
}


void TextureAtlas::Unbind() const
{
   glBindTexture( GL_TEXTURE_2D, 0 );
}
