#pragma once

// Forward declarations
class Shader;
using GLuint = unsigned int;

class Renderer
{
public:
   Renderer();
   ~Renderer();

   // Member Functions
   void Attach();
   void Detach();
   void Draw();
   void DrawGui();

   void SwapShader( const std::string& file );

   void UpdateViewport( unsigned int x, unsigned int y, unsigned int width, unsigned int height );

private:
   std::unique_ptr< Shader > m_Shader;

   GLuint m_VA {}, m_VB {}, m_IB {};

   //std::vector< GUI::Window* > m_guiList;
};
