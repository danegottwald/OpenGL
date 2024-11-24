#include "Application.h"

#include "../Timestep.h"

#include "../../Events/ApplicationEvent.h"
#include "../../Events/MouseEvent.h"
#include "../../Events/KeyEvent.h"


Application::Application() noexcept
    : m_Window( Window::Get() )
{
    // If a window event occurs, send to Application::OnEvent
    m_Window.SetEventCallback( std::bind( &Application::OnEvent, this, std::placeholders::_1 ) );
}


/*static*/ Application& Application::Get() noexcept
{
    static Application app;
    return app;
}


void Application::Run()
{
    m_Window.Init();
    m_Game.Setup(); // Needs to be called after Window initialization

    Timestep start;
    while( m_Window.IsOpen() )
    {
        // Compute delta
        Timestep delta( start );

        // Handle Game Tick (Update logic, Player state, etc)
        m_Game.Tick( delta.Get() );

        // Render only if not minimized
        if( !m_Window.FMinimized() )
            m_Game.Render();

        m_Window.OnUpdate();  // Poll events and swap buffers

            // EVENTUALLY MOVE TO THIS
            // stuff to render set to renderer
            // m_renderer.render();

            // GUI Update (make part of renderer?)
            //      m_GUI.OnUpdate()
            //      Bind a specific GUI from GameDef??


            // EVENTUALLY REMOVE BELOW
            // remove m_Game eventually?
        {
            // Bind World Render Target
            //      Render World
            // Bind GUI Render Target
            //      Render GUI
        }

        // RENDER() from MAIN LOOP
        // glEnable(GL_DEPTH_TEST);
        //
        //// World
        // m_worldRenderTarget.bind();
        // glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
        // m_game.render();
        //
        //// GUI
        // m_guiRenderTarget.bind();
        // glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
        // m_gui.render(m_guiRenderer);
        //
        //// Buffer to window
        // gl::unbindFramebuffers(ClientConfig::get().windowWidth,
        //                         ClientConfig::get().windowHeight);
        // glDisable(GL_DEPTH_TEST);
        // glClear(GL_COLOR_BUFFER_BIT);
        // auto drawable = m_screenBuffer.getDrawable();
        // drawable.bind();
        // m_screenShader.bind();
        //
        // m_worldRenderTarget.bindTexture();
        // drawable.draw();
        //
        // glEnable(GL_BLEND);
        //
        // m_guiRenderTarget.bindTexture();
        // drawable.draw();
        //
        // mp_window->display();
        // glDisable(GL_BLEND);
    }
}


// NOTES
// https://developer.valvesoftware.com/wiki/Source_Multiplayer_Networking
//      https://github.com/Hopson97/open-builder/issues/208#issuecomment-706747718

// EVENTS

/*! @brief Application callback function
 *  @param[in] event event to dispatch
 *  @return void
 */
bool Application::OnEvent( IEvent& e )
{
    EventDispatcher dispatcher( e );
    std::cout << e.ToString() << std::endl;

    m_Window.OnEvent( dispatcher );
    m_Game.OnEvent( dispatcher );

    return true;
}
