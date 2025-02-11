
#include "Player.h"

#include "../Core/Window.h"
#include "../World.h"
#include "../../Events/KeyEvent.h"
#include "../../Events/MouseEvent.h"
#include "../../Input/Input.h"

Player::Player()
{
   m_position = { 0.0f, 0.0f, 3.0f };

   // Subscribe to events
   m_eventSubscriber.Subscribe< Events::MouseMovedEvent >( std::bind( &Player::MouseMove, this, std::placeholders::_1 ) );
   m_eventSubscriber.Subscribe< Events::MouseButtonPressedEvent >( std::bind( &Player::MouseButtonPressed, this, std::placeholders::_1 ) );
   m_eventSubscriber.Subscribe< Events::MouseButtonReleasedEvent >( std::bind( &Player::MouseButtonReleased, this, std::placeholders::_1 ) );
}

void Player::Update( float delta )
{
   // should move to polling mouse input here for perf reasons

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
   if( Input::IsKeyPressed( Input::LeftShift ) )
      speed *= 10;

   if( Input::IsKeyPressed( Input::W ) )
   {
      m_acceleration.x -= glm::cos( glm::radians( m_rotation.y + 90 ) ) * speed;
      m_acceleration.z -= glm::sin( glm::radians( m_rotation.y + 90 ) ) * speed;
   }
   if( Input::IsKeyPressed( Input::S ) )
   {
      m_acceleration.x += glm::cos( glm::radians( m_rotation.y + 90 ) ) * speed;
      m_acceleration.z += glm::sin( glm::radians( m_rotation.y + 90 ) ) * speed;
   }
   if( Input::IsKeyPressed( Input::A ) )
   {
      float yaw = glm::radians( m_rotation.y );
      m_acceleration += glm::vec3( -glm::cos( yaw ), 0, -glm::sin( yaw ) ) * speed;
   }
   if( Input::IsKeyPressed( Input::D ) )
   {
      float yaw = glm::radians( m_rotation.y );
      m_acceleration -= glm::vec3( -glm::cos( yaw ), 0, -glm::sin( yaw ) ) * speed;
   }

   // Up and Down
   if( Input::IsKeyPressed( Input::Space ) )
      m_acceleration.y += speed;
   if( Input::IsKeyPressed( Input::LeftControl ) )
      m_acceleration.y -= speed;

   //std::cout << m_position.x << " " << m_position.y << " " << m_position.z << std::endl;
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
   static Window& window       = Window::Get();
   GLFWwindow*    nativeWindow = window.GetNativeWindow();
   if( glfwGetInputMode( nativeWindow, GLFW_CURSOR ) != GLFW_CURSOR_DISABLED )
      return;

   constexpr float  sensitivity     = 1.0f / 16.0f;
   static glm::vec2 lastMousePos    = window.GetMousePosition();
   glm::vec2        currentMousePos = window.GetMousePosition();
   glm::vec2        delta           = currentMousePos - lastMousePos;

   // Update m_rotation based on mouse movement, applying sensitivity and clamping/wrapping
   // Vertical rotation (m_rotation.x) is clamped to prevent flipping of the camera (within -89.9° to 89.9°).
   // Horizontal rotation (m_rotation.y) is wrapped to keep it within 0° to 360° for continuous rotation.
   m_rotation.x = glm::clamp( m_rotation.x + ( delta.y * sensitivity ), -89.9f, 89.9f );
   m_rotation.y = glm::mod( m_rotation.y + ( delta.x * sensitivity ) + 360.0f, 360.0f );

   const WindowData& windowData = window.GetWindowData();
   lastMousePos                 = { windowData.Width * 0.5f, windowData.Height * 0.5f };
   glfwSetCursorPos( nativeWindow, lastMousePos.x, lastMousePos.y ); // Reset mouse to center of screen
}
