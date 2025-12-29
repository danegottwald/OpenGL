#pragma once

struct GLFWwindow;

class RenderContext
{
public:
   explicit RenderContext( GLFWwindow& window ) noexcept;
   ~RenderContext();

   void Init( bool fVSync );
   void Present();
   void SetVSync( bool fState );

   void UpdateViewport( int x, int y, int width, int height );

private:
   NO_COPY_MOVE( RenderContext )

   void SetCallbacks();

   GLFWwindow& m_window;
};
