#pragma once

#include "Camera.h"
#include "Entity/Player.h"
#include "World.h"

// Forward Declarations
class EventDispatcher;

class GameDef
{
public:
    GameDef();

    void Setup();
    void Tick( float delta );
    void Render();

    // Events
    bool OnEvent( EventDispatcher& dispatcher );

private:
    World m_World;
    //      World will contain World Data, other entities, etc
    Player m_Player;
    Camera m_camera;

    //  std::vector< Clients > m_Clients; or just Client m_Client which holds
    //  all other clients
    // PlayerController& m_PlayerController;
    //   etc related to GameDefinition
};