#pragma once

#include <Engine/World/Blocks.h>
#include <Engine/Renderer/Shader.h>


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

   GLuint m_vao {};
   GLuint m_vbo {};
   GLuint m_textureID {};
   Shader m_shader;
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
      North  = 0,
      East   = 1,
      South  = 2,
      West   = 3,
      Top    = 4,
      Bottom = 5,
      Count
   };
   const Region& GetRegion( BlockId blockId, BlockFace face ) const;
   const Region& GetRegion( BlockState state, BlockFace face ) const;

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
   const TextureAtlas::Region& GetRegion( BlockState state, TextureAtlas::BlockFace face ) const { return m_blockAtlas.GetRegion( state, face ); }

private:
   TextureAtlasManager() noexcept = default;
   NO_COPY_MOVE( TextureAtlasManager )

   TextureAtlas m_blockAtlas;
};
