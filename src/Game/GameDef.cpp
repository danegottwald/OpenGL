#include "GameDef.h"

#include "../Events/Event.h"
#include "../Events/KeyEvent.h"
#include "../Events/MouseEvent.h"

GameDef::GameDef()
{
    // Take in data to specifiy gamedef?
}

void GameDef::Setup()
{
    // Bind callback functions?

    // possibly remove if unused?
    m_World.Setup();
}

void GameDef::Tick( float delta )
{
    // m_Client.Tick(), get updates from server

    // Update World
    m_World.Tick( delta );  // Update world

    // Update Player, use m_World to check collisions
    m_Player.Tick( m_World, delta );  // Update player after world update

    // Update Camera to match Player
    m_camera.Update( m_Player );

    // send player state update to server if Multiplayer after update
    //      Only do this after a certain time passed, (ie have a tickrate)
    //          m_Client.SendState(m_Player) or something like that
}

void GameDef::Render()
{
    m_World.Render( m_camera );
    // Render each necessary member
}

bool GameDef::OnEvent( EventDispatcher& dispatcher )
{
    // Player Events
    m_Player.OnEvent( dispatcher );
    m_camera.OnEvent( dispatcher );

    // remove below? make more dispatchers for each GameDef member?
    IEvent& event = dispatcher.GetEvent();
    switch( event.GetEventType() )
    {
        case EventType::KeyPressed:
            return false;
        case EventType::KeyReleased:
            return false;
        case EventType::KeyTyped:
            return false;
        case EventType::MouseButtonPressed:
            return false;
        case EventType::MouseButtonReleased:
            return false;
        case EventType::MouseMoved:
            return false;
        case EventType::MouseScrolled:
            return false;
        default:
            return false;
    }
    // dispatcher.Dispatch<KeyPressedEvent>(
    //     std::bind(&funcHere, this, std::placeholders::_1));
}
