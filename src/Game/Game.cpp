#include "Game.h"

// Events
#include "../Events/Event.h"
#include "GameDef.h"

// --------------------------------------------------------------------
//      Public Methods
// --------------------------------------------------------------------
Game::Game()
{
}

void Game::Setup()
{
    m_pGameDef = std::make_unique<GameDef>(); // Initliaze m_GameDef

    _ASSERT_EXPR( m_pGameDef, L"Failed to create GameDef" );
    m_pGameDef->Setup();
}

void Game::Tick( float delta )
{
    // Only tick if game isn't paused, add if in future
    if( IsActive() )
        m_pGameDef->Tick( delta );
}

void Game::Render()
{
    // only render if not paused?
    if( IsActive() )
        m_pGameDef->Render();
}

bool Game::OnEvent( EventDispatcher& dispatcher )
{
    return IsActive() ? m_pGameDef->OnEvent( dispatcher ) : false;
}

// --------------------------------------------------------------------
//      Private Methods
// --------------------------------------------------------------------
bool Game::IsActive()
{
    return m_pGameDef != nullptr;
}
