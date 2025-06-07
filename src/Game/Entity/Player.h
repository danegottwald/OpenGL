#pragma once

#include "Entity.h"
#include "IGameObject.h"

#include "../../Events/KeyEvent.h"

// Forward Declarations
namespace Events
{
class KeyPressedEvent;
class KeyReleasedEvent;
class MouseButtonPressedEvent;
class MouseButtonReleasedEvent;
class MouseMovedEvent;
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

   // Events (fix this, make it cleaner)
   Events::EventSubscriber              m_eventSubscriber;
   std::unordered_map< uint16_t, bool > m_InputMap;
   void                                 MouseButtonPressed( const Events::MouseButtonPressedEvent& e ) noexcept;
   void                                 MouseButtonReleased( const Events::MouseButtonReleasedEvent& e ) noexcept;
   void                                 MouseMove( const Events::MouseMovedEvent& e ) noexcept;
   //  EntityState m_State inherited from Entity

   bool m_fInAir { false };
};


/**
 * @class PlayerController
 * @brief Controls the player's input and updates the player's state based on received events.
 *
 * The PlayerController class is responsible for handling user input (or other event sources)
 * and translating that input into actions or state changes for the player object it controls.
 * It listens for events (such as movement, actions, or other controls) and instructs the
 * associated Player to update its state accordingly (e.g., position, velocity, or animation).
 */
class PlayerController
{
public:
   PlayerController( std::shared_ptr< Player > spPlayer ) :
      m_spPlayer( std::move( spPlayer ) ) {
         // only register to events I care about, PlayerController doesn't need window resize events for example
         //Input::RegisterListener( []( const Events::IEvent& e )
         //    {
         //        // Dispatch event to PlayerController
         //        // PlayerController::Get().OnEvent( e );
         //    } );
      };

   void OnEvent( const Events::IEvent& e )
   {
      // Player could have been detached from the controller
      //if ( !m_spPlayer )
      //    return;
      //
      //if ( e.GetEventType() == Events::KeyPressed )
      //{
      //    const Events::KeyPressedEvent& keyEvent = static_cast< const Events::KeyPressedEvent& >( e );
      //    m_spPlayer->OnEvent( keyEvent );
      //}
      //else if ( e.GetEventType() == Events::KeyReleased )
      //{
      //    const Events::KeyReleasedEvent& keyEvent = static_cast< const Events::KeyReleasedEvent& >( e );
      //    m_spPlayer->OnEvent( keyEvent );
      //}
      //else if ( e.GetEventType() == Events::MouseButtonPressed )
      //{
      //    const Events::MouseButtonPressedEvent& mouseEvent = static_cast< const Events::MouseButtonPressedEvent& >( e );
      //    m_spPlayer->OnEvent( mouseEvent );
      //}
      //else if ( e.GetEventType() == Events::MouseButtonReleased )
      //{
      //    const Events::MouseButtonReleasedEvent& mouseEvent = static_cast< const Events::MouseButtonReleasedEvent& >( e );
      //    m_spPlayer->OnEvent( mouseEvent );
      //}
      //else if ( e.GetEventType() == Events::MouseMoved )
      //{
      //    const Events::MouseMovedEvent& mouseEvent = static_cast< const Events::MouseMovedEvent& >( e );
      //    m_spPlayer->OnEvent( mouseEvent );
      //}
      //else if ( e.GetEventType() == Events::MouseScrolled )
      //{
      //    const Events::MouseScrolledEvent& mouseEvent = static_cast< const Events::MouseScrolledEvent& >( e );
      //    m_spPlayer->OnEvent( mouseEvent );
      //}
      //else
   }

private:
   std::shared_ptr< Player > m_spPlayer;
};
