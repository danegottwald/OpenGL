#pragma once

#include <World/Blocks.h>

// Forward Declarations
class Shader;


// ----------------------------------------------------------------
// SkyboxTexture - Cubemap Texture for Skyboxes
// ----------------------------------------------------------------
class SkyboxTexture
{
public:
   SkyboxTexture( std::span< const std::string_view > faces );
   ~SkyboxTexture();

   void Draw( const glm::mat4& view, const glm::mat4& projection );

private:
   void LoadCubemap( std::span< const std::string_view > faces );
   void InitGeometry();

   GLuint                    m_vao {};
   GLuint                    m_vbo {};
   GLuint                    m_textureID {};
   std::unique_ptr< Shader > m_pShader;
};


// ----------------------------------------------------------------
// TextureAtlas - Atlas for 2D Textures
// ----------------------------------------------------------------
class TextureAtlas
{
public:
   struct Region
   {
      std::array< glm::vec2, 4 > uvs;
   };

   enum class Filtering
   {
      PixelPerfect,        // no mipmaps
      MipmappedAnisotropic // mipmaps (and optional aniso)
   };

   TextureAtlas() noexcept;
   ~TextureAtlas();

   // Incremental build: call AddTexture() N times then Compile() once.
   void SetPadding( int pixels ) { m_padding = ( std::max )( 0, pixels ); }
   void SetFiltering( Filtering filtering ) { m_filtering = filtering; }

   // Prepares the texture to be packed during Compile().
   void PrepareTexture( BlockId blockId );

   // Pack all added textures, compute UVs/regions, and upload to OpenGL.
   void Compile();

   void Bind( unsigned int slot = 0 ) const;
   void Unbind() const;

   const Region& GetRegion( BlockId blockId ) const { return m_regions.at( blockId ); }
   //std::optional< Region > GetRegion( const std::string& name ) const;

   int GetWidth() const { return m_width; }
   int GetHeight() const { return m_height; }

private:
   struct PendingTexture
   {
      BlockId                      blockId;
      std::string_view             filepath;
      int                          width  = 0;
      int                          height = 0;
      std::vector< unsigned char > pixels; // RGBA8
   };

   unsigned int m_rendererID = 0;
   int          m_width      = 0;
   int          m_height     = 0;

   int       m_padding   = 16;
   Filtering m_filtering = Filtering::MipmappedAnisotropic;

   std::unordered_map< BlockId, PendingTexture > m_pending;
   std::unordered_map< BlockId, Region >         m_regions;
};


// ----------------------------------------------------------------
// TextureAtlasManager - Global Texture Atlas Manager
// ----------------------------------------------------------------
class TextureAtlasManager
{
public:
   TextureAtlasManager() noexcept
   {
      try
      {
         for( int i = 0; i < static_cast< int >( BlockId::Count ); ++i )
            m_atlas.PrepareTexture( static_cast< BlockId >( i ) );

         //m_atlas.Compile();
      }
      catch( ... )
      {
         std::cerr << "TextureAtlasManager - failed to initialize texture atlas\n";
         std::abort();
      }
   }

   void                        Compile() { m_atlas.Compile(); }
   const TextureAtlas::Region& GetRegion( BlockId blockId ) const { return m_atlas.GetRegion( blockId ); }

   void Bind( unsigned int slot = 0 ) const { m_atlas.Bind( slot ); }
   void Unbind() const { m_atlas.Unbind(); }

private:
   TextureAtlas m_atlas;
};

inline TextureAtlasManager g_textureAtlasManager;
