
#include "Player.h"

#include "../Core/Window.h"
#include "../World.h"
#include "../../Events/KeyEvent.h"
#include "../../Events/MouseEvent.h"

Player::Player()
{
   m_position = { 0.0f, 0.0f, 3.0f };

   // Subscribe to events
   m_eventSubscriber.Subscribe< Events::KeyPressedEvent >( std::bind( &Player::KeyPressed, this, std::placeholders::_1 ) );
   m_eventSubscriber.Subscribe< Events::KeyReleasedEvent >( std::bind( &Player::KeyReleased, this, std::placeholders::_1 ) );
   m_eventSubscriber.Subscribe< Events::MouseButtonPressedEvent >( std::bind( &Player::MouseButtonPressed, this, std::placeholders::_1 ) );
   m_eventSubscriber.Subscribe< Events::MouseButtonReleasedEvent >( std::bind( &Player::MouseButtonReleased, this, std::placeholders::_1 ) );
   m_eventSubscriber.Subscribe< Events::MouseMovedEvent >( std::bind( &Player::MouseMove, this, std::placeholders::_1 ) );
}

void Player::Update( float delta )
{
   // Update the player's state
   //Tick( *m_World, delta );

   // Set Player Velocity
   m_velocity += m_acceleration;
   m_acceleration = glm::vec3( 0.0f );

   // Update position based on velocity
   m_position += m_velocity * delta;
   m_velocity *= 0.9f;

   // Handle collision detection and response
   // collide(world, m_velocity * delta);
}

void Player::Render() const
{
   // Render the player
   // m_Model->Render();
}

void Player::Tick( World& world, float delta )
{
   ApplyInputs();
   UpdateState( world, delta );
}

void Player::ApplyInputs()
{
   float speed = 0.4f;
   if( m_InputMap[ Events::LeftShift ] )
      speed *= 10;

   if( m_InputMap[ Events::W ] )
   {
      m_acceleration.x -= glm::cos( glm::radians( m_rotation.y + 90 ) ) * speed;
      m_acceleration.z -= glm::sin( glm::radians( m_rotation.y + 90 ) ) * speed;
   }
   if( m_InputMap[ Events::S ] )
   {
      m_acceleration.x += glm::cos( glm::radians( m_rotation.y + 90 ) ) * speed;
      m_acceleration.z += glm::sin( glm::radians( m_rotation.y + 90 ) ) * speed;
   }
   if( m_InputMap[ Events::A ] )
   {
      float yaw = glm::radians( m_rotation.y );
      m_acceleration += glm::vec3( -glm::cos( yaw ), 0, -glm::sin( yaw ) ) * speed;
   }
   if( m_InputMap[ Events::D ] )
   {
      float yaw = glm::radians( m_rotation.y );
      m_acceleration -= glm::vec3( -glm::cos( yaw ), 0, -glm::sin( yaw ) ) * speed;
   }
   if( m_InputMap[ Events::Space ] )
   {
      m_acceleration.y += speed;
   }
   if( m_InputMap[ Events::LeftControl ] )
   {
      m_acceleration.y -= speed;
   }

   // Account for gimbal lock
   if( m_rotation.x < -89.9f )
      m_rotation.x = -89.9f;
   if( m_rotation.x > 89.9f )
      m_rotation.x = 89.9f;
   //std::cout << m_Position.x << " " << m_Position.y << " " << m_Position.z << std::endl;
}

void Player::UpdateState( World& world, float delta )
{
   // Set Player Velocity
   m_velocity += m_acceleration;
   m_acceleration = glm::vec3( 0.0f );

   // Update position based on velocity
   m_position += m_velocity * delta;
   m_velocity *= 0.9f;

   // Handle collision detection and response
   // collide(world, m_velocity * delta);
}

// --------------------------------------------------------------------
//      Player Event Handling
// --------------------------------------------------------------------
void Player::KeyPressed( const Events::KeyPressedEvent& event )
{
   Window& window = Window::Get(); // clean this up, so we dont need to check each call
   if( glfwGetInputMode( window.GetNativeWindow(), GLFW_CURSOR ) != GLFW_CURSOR_DISABLED )
      return;

   if( !event.FRepeat() )
      m_InputMap[ event.GetKeyCode() ] = true;
}

void Player::KeyReleased( const Events::KeyReleasedEvent& event )
{
   Window& window = Window::Get(); // clean this up, so we dont need to check each call
   if( glfwGetInputMode( window.GetNativeWindow(), GLFW_CURSOR ) != GLFW_CURSOR_DISABLED )
      return;

   m_InputMap[ event.GetKeyCode() ] = false;
}

void Player::MouseButtonPressed( const Events::MouseButtonPressedEvent& event )
{
   Window& window = Window::Get(); // clean this up, so we dont need to check each call
   if( glfwGetInputMode( window.GetNativeWindow(), GLFW_CURSOR ) != GLFW_CURSOR_DISABLED )
      return;

   m_InputMap[ event.GetMouseButton() ] = true;
}

void Player::MouseButtonReleased( const Events::MouseButtonReleasedEvent& event )
{
   Window& window = Window::Get(); // clean this up, so we dont need to check each call
   if( glfwGetInputMode( window.GetNativeWindow(), GLFW_CURSOR ) != GLFW_CURSOR_DISABLED )
      return;

   m_InputMap[ event.GetMouseButton() ] = false;
}

void Player::MouseMove( const Events::MouseMovedEvent& event )
{
   Window& window = Window::Get(); // clean this up, so we dont need to check each call
   if( glfwGetInputMode( window.GetNativeWindow(), GLFW_CURSOR ) != GLFW_CURSOR_DISABLED )
      return;

   constexpr float  verticalSens   = 0.5f;
   constexpr float  horizontalSens = 0.5f;
   static glm::vec2 lastMousePos   = window.GetMousePosition();
   glm::vec2        delta          = window.GetMousePosition() - lastMousePos;
   m_rotation.x += ( delta.y / 8.0f ) * verticalSens;
   m_rotation.y += ( delta.x / 8.0f ) * horizontalSens;

   WindowData& windowData = window.GetWindowData();
   lastMousePos.x         = windowData.Width / 2.0f;
   lastMousePos.y         = windowData.Height / 2.0f;

   // Reset mouse cursor to center of screen
   glfwSetCursorPos( window.GetNativeWindow(), lastMousePos.x, lastMousePos.y );
}
