#pragma once

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
