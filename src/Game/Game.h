#pragma once

#include "GameDef.h"

// Forward Declarations
class EventDispatcher;

class Game
{
public:
    Game();

    void Setup();
    void Tick( float delta );
    void Render();

    // Events
    bool OnEvent( EventDispatcher& dispatcher );

private:
    bool IsActive();

    std::unique_ptr<GameDef> m_pGameDef;  // pointer to allow gamedef swapping
};
