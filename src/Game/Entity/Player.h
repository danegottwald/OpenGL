#pragma once

#include "Entity.h"

#include "../../Events/KeyEvent.h"

// Forward Declarations
class EventDispatcher;
class KeyPressedEvent;
class KeyReleasedEvent;
class MouseButtonPressedEvent;
class MouseButtonReleasedEvent;
class MouseMovedEvent;
class World;

class Player : public Entity
{
public:
    Player();

    void Tick( World& world, float delta );
    bool OnEvent( EventDispatcher& dispatcher );

private:
    void ApplyInputs();
    void UpdateState( World& world, float delta );

    // Events (fix this, make it cleaner)
    std::unordered_map<uint16_t, bool> m_InputMap;
    bool KeyPressed( KeyPressedEvent& e );
    bool KeyReleased( KeyReleasedEvent& e );
    bool MouseButtonPressed( MouseButtonPressedEvent& e );
    bool MouseButtonReleased( MouseButtonReleasedEvent& e );
    bool MouseMove( MouseMovedEvent& e );
    //  EntityState m_State inherited from Entity
};
