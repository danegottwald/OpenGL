#pragma once

// Forward Declarations
class Shader;

class Texture
{
private:
   unsigned int   m_RendererID;
   std::string    m_File;
   unsigned char* m_LocalBuffer;
   int            m_Width, m_Height, m_BPP;

public:
   Texture( const std::string& file );
   ~Texture();

   void Bind( unsigned int slot = 0 ) const;
   void Unbind() const;

   inline int GetWidth() const { return m_Width; }
   inline int GetHeight() const { return m_Height; }
};

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

class TextureAtlas
{
public:
   struct Region
   {
      // UV rect in [0,1] range
      glm::vec2 uMin;
      glm::vec2 uMax;
   };

   TextureAtlas() noexcept;
   ~TextureAtlas();

   // Build atlas from all .png files in a directory (non-recursive)
   bool BuildFromDirectory( const std::string& directory, int padding = 1 );

   // Bind the atlas texture
   void Bind( unsigned int slot = 0 ) const;
   void Unbind() const;

   // Query the UV region for a given logical name (e.g. "dirt", "stone")
   // filename is usually "dirt.png" or the base name.
   std::optional< Region > GetRegion( const std::string& name ) const;

   int GetWidth() const { return m_Width; }
   int GetHeight() const { return m_Height; }

private:
   unsigned int m_RendererID = 0;
   int          m_Width      = 0;
   int          m_Height     = 0;

   // map from base filename (without directory) to UV region
   std::unordered_map< std::string, Region > m_Regions;
};
