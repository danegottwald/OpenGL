#pragma once

#include "Entity.h"
#include "IGameObject.h"

#include <unordered_map>
#include <glm/glm.hpp>
#include "../../Events/KeyEvent.h"

// Forward Declarations
namespace Events
{
class KeyPressedEvent;
class KeyReleasedEvent;
class MouseButtonPressedEvent;
class MouseButtonReleasedEvent;
class MouseMovedEvent;
class EventSubscriber;
};
class World;

class Player : public IGameObject
{
public:
   Player();

   void Update( float delta ) override;
   void Render() const override;

   void Tick( World& world, float delta );

private:
   void ApplyInputs();
   void UpdateState( World& world, float delta );

   Events::EventSubscriber m_eventSubscriber;
   std::unordered_map< uint16_t, bool > m_InputMap;
   void MouseButtonPressed( const Events::MouseButtonPressedEvent& e ) noexcept;
   void MouseButtonReleased( const Events::MouseButtonReleasedEvent& e ) noexcept;
   void MouseMove( const Events::MouseMovedEvent& e ) noexcept;

   bool m_fInAir { false };
};
