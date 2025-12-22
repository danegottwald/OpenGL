#pragma once

#include <Engine/World/Blocks.h>

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
   TextureAtlas() noexcept = default;
   ~TextureAtlas();

   int GetWidth() const { return m_width; }
   int GetHeight() const { return m_height; }

   // Prepares the textures referenced by this block (top/bottom/side).
   void PrepareTexture( BlockId blockId );
   void Compile();
   void Bind( unsigned int slot = 0 ) const;
   void Unbind() const;

   struct Region
   {
      std::array< glm::vec2, 4 > uvs;
   };

   enum class BlockFace : uint8_t
   {
      East,
      West,
      Top,
      Bottom,
      South,
      North,
      Count
   };
   const Region& GetRegion( BlockId blockId, BlockFace face ) const
   {
      return m_regions.at( m_blockFaceKeys[ static_cast< size_t >( blockId ) ].faceKeys[ static_cast< size_t >( face ) ] );
   }

private:
   struct PendingTexture
   {
      std::string_view             filepath;
      int                          width { 0 };
      int                          height { 0 };
      std::vector< unsigned char > pixels; // RGBA8
   };

   unsigned int m_rendererID { 0 };
   int          m_width { 0 };
   int          m_height { 0 };

   std::unordered_map< uint64_t, PendingTexture > m_pending;
   std::unordered_map< uint64_t, Region >         m_regions;

   struct BlockFaceKeys
   {
      std::array< uint64_t, static_cast< size_t >( BlockFace::Count ) > faceKeys { 0 };
   };
   std::array< BlockFaceKeys, static_cast< size_t >( BlockId::Count ) > m_blockFaceKeys { 0 };
};


// ----------------------------------------------------------------
// TextureAtlasManager - Manages Texture Atlases
// ----------------------------------------------------------------
class TextureAtlasManager
{
public:
   static TextureAtlasManager& Get() noexcept;
   void                        CompileBlockAtlas();
   void                        Bind( unsigned int slot = 0 ) const { m_blockAtlas.Bind( slot ); }
   void                        Unbind() const { m_blockAtlas.Unbind(); }

   const TextureAtlas::Region& GetRegion( BlockId blockId, TextureAtlas::BlockFace face ) const { return m_blockAtlas.GetRegion( blockId, face ); }

private:
   TextureAtlasManager() noexcept = default;
   NO_COPY_MOVE( TextureAtlasManager )

   TextureAtlas m_blockAtlas;
};
