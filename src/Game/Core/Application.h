#pragma once

#include "../Game.h"
#include "Window.h"

enum ApplicationState
{
    APP_STATE_NONE = 0,
    APP_STATE_MENU,
    APP_STATE_LOADING,
    APP_STATE_INGAME,
};

// Forward Declarations

class Application
{
public:
    ~Application() = default;
    Application( const Application& ) = delete;
    Application& operator=( const Application& ) = delete;
    Application( Application&& ) = delete;
    Application& operator=( Application&& ) = delete;

    void Run();

    static Application& Get() noexcept;

private:
    Application() noexcept;

    // Application Events
    bool OnEvent( IEvent& e ); // Make an IEventable/IEvent as base class

private:
    Window& m_Window;
    Game m_Game;

    ApplicationState m_state{ APP_STATE_NONE };
};
