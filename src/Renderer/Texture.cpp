#include "Texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <filesystem>
#include <Renderer/Shader.h>

Texture::Texture( const std::string& file ) :
   m_RendererID( 0 ),
   m_LocalBuffer( nullptr ),
   m_Width( 0 ),
   m_Height( 0 ),
   m_BPP( 0 )
{
   stbi_set_flip_vertically_on_load( 1 );
   m_File        = "./res/textures/" + file;
   m_LocalBuffer = stbi_load( m_File.c_str(), &m_Width, &m_Height, &m_BPP, 4 );

   glGenTextures( 1, &m_RendererID );
   glBindTexture( GL_TEXTURE_2D, m_RendererID );

   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

   glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_LocalBuffer );
   glBindTexture( GL_TEXTURE_2D, 0 );

   if( m_LocalBuffer )
      stbi_image_free( m_LocalBuffer );
}

Texture::~Texture()
{
   glDeleteTextures( 1, &m_RendererID );
}

void Texture::Bind( unsigned int slot ) const
{
   glActiveTexture( GL_TEXTURE0 + slot );
   glBindTexture( GL_TEXTURE_2D, m_RendererID );
}

void Texture::Unbind() const
{
   glBindTexture( GL_TEXTURE_2D, 0 );
}


// ----------------------------------------------------------------
// SkyboxTexture
// ----------------------------------------------------------------
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


void SkyboxTexture::LoadCubemap( std::span< const std::string_view > faces )
{
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
   if( m_RendererID )
      glDeleteTextures( 1, &m_RendererID );
}


bool TextureAtlas::BuildFromDirectory( const std::string& directory, int padding )
{
   if( m_RendererID )
   {
      glDeleteTextures( 1, &m_RendererID );
      m_RendererID = 0;
   }
   m_Regions.clear();
   m_Width  = 0;
   m_Height = 0;

   namespace fs = std::filesystem;

   struct Image
   {
      std::string                  path;
      std::string                  name; // base filename, e.g. "dirt.png"
      int                          w = 0;
      int                          h = 0;
      std::vector< unsigned char > pixels; // RGBA8
   };

   std::vector< Image > images;

   // Load all .pngs
   for( const auto& entry : fs::directory_iterator( directory ) )
   {
      if( !entry.is_regular_file() )
         continue;

      const auto& p = entry.path();
      if( p.extension() != ".png" )
         continue;

      Image img;
      img.path = p.string();
      img.name = p.filename().string();

      int            bpp  = 0;
      unsigned char* data = stbi_load( img.path.c_str(), &img.w, &img.h, &bpp, 4 );
      if( !data )
      {
         std::cerr << "TextureAtlas: failed to load " << img.path << " reason: " << stbi_failure_reason() << "\n";
         continue;
      }

      img.pixels.assign( data, data + img.w * img.h * 4 );
      stbi_image_free( data );

      images.push_back( std::move( img ) );
   }

   if( images.empty() )
   {
      std::cerr << "TextureAtlas: no PNGs in " << directory << "\n";
      return false;
   }

   // Simple horizontal packing
   int totalWidth = 0;
   int maxHeight  = 0;
   for( const auto& img : images )
   {
      totalWidth += img.w + padding;
      maxHeight = std::max( maxHeight, img.h );
   }
   totalWidth -= padding; // no padding after last

   m_Width  = totalWidth;
   m_Height = maxHeight;

   std::vector< unsigned char > atlas;
   atlas.resize( static_cast< size_t >( m_Width ) * m_Height * 4, 0 );

   int xOffset = 0;
   for( const auto& img : images )
   {
      // Copy into atlas
      for( int y = 0; y < img.h; ++y )
      {
         auto*       dst = atlas.data() + ( ( y * m_Width + xOffset ) * 4 );
         const auto* src = img.pixels.data() + ( ( y * img.w ) * 4 );
         std::memcpy( dst, src, static_cast< size_t >( img.w ) * 4 );
      }

      // Compute UVs
      float u0 = static_cast< float >( xOffset ) / static_cast< float >( m_Width );
      float v0 = 0.0f;
      float u1 = static_cast< float >( xOffset + img.w ) / static_cast< float >( m_Width );
      float v1 = static_cast< float >( img.h ) / static_cast< float >( m_Height );

      m_Regions[ img.name ] = Region {
         .uMin = { u0, v0 },
           .uMax = { u1, v1 }
      };

      xOffset += img.w + padding;
   }

   // Upload atlas
   glGenTextures( 1, &m_RendererID );
   glBindTexture( GL_TEXTURE_2D, m_RendererID );
   glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, atlas.data() );

   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

   glGenerateMipmap( GL_TEXTURE_2D );
   glBindTexture( GL_TEXTURE_2D, 0 );
   return true;
}


void TextureAtlas::Bind( unsigned int slot ) const
{
   glActiveTexture( GL_TEXTURE0 + slot );
   glBindTexture( GL_TEXTURE_2D, m_RendererID );
}


void TextureAtlas::Unbind() const
{
   glBindTexture( GL_TEXTURE_2D, 0 );
}


std::optional< TextureAtlas::Region > TextureAtlas::GetRegion( const std::string& name ) const
{
   auto it = m_Regions.find( name );
   if( it == m_Regions.end() )
      return std::nullopt;

   return it->second;
}