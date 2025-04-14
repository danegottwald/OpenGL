
#include "Player.h"

#include "../Core/Window.h"
#include "../World.h"
#include "../../Events/KeyEvent.h"
#include "../../Events/MouseEvent.h"
#include "../../Input/Input.h"

constexpr glm::vec3 GRAVITY( 0.0f, -9.8f, 0.0f ); // Gravity in meters per second squared
constexpr float     JUMP_VELOCITY = 0.7f;         // Initial jump velocity to reach 1 meter

Player::Player()
{
   m_position = { 0.0f, 128.0f, 0 };

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
   if( m_fInAir )
   {
      m_acceleration.x = 0;
      m_acceleration.z = 0;
   }
   else
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
      speed *= 1.8;

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

   // Jump
   if( !m_fInAir && Input::IsKeyPressed( Input::Space ) ) // Jump only if not in air
   {
      m_acceleration.y = JUMP_VELOCITY; // Set upward velocity
      m_fInAir         = true;          // Player is now in the air
   }
}

void Player::UpdateState( World& world, float delta )
{
   m_acceleration.y += -9.8f * ( delta * 0.33f );
   m_acceleration.y = glm::clamp( m_acceleration.y, -9.8f, 9.8f ); // Clamp vertical acceleration
   m_velocity += m_acceleration * delta;
   m_position += m_velocity * delta; // Update position based on velocity

   // Check if player has landed
   float edgeHeight   = std::max( { world.GetHeightAtPos( m_position.x + 0.5f, m_position.z ),
                                    world.GetHeightAtPos( m_position.x - 0.5f, m_position.z ),
                                    world.GetHeightAtPos( m_position.x, m_position.z + 0.5f ),
                                    world.GetHeightAtPos( m_position.x, m_position.z - 0.5f ) } );
   float cornerHeight = std::max( { world.GetHeightAtPos( m_position.x + 0.25f, m_position.z + 0.25f ),
                                    world.GetHeightAtPos( m_position.x - 0.25f, m_position.z + 0.25f ),
                                    world.GetHeightAtPos( m_position.x + 0.25f, m_position.z - 0.25f ),
                                    world.GetHeightAtPos( m_position.x - 0.25f, m_position.z - 0.25f ) } );
   float groundHeight = std::max( { edgeHeight, cornerHeight } ) + 2.0f; // Add buffer for ground height
   m_fInAir           = m_position.y > groundHeight;
   if( !m_fInAir )
   {
      m_position.y = groundHeight; // Snap to ground height
      m_velocity.y = 0.0f;         // Stop vertical velocity
   }
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
   m_rotation.x                     = glm::clamp( m_rotation.x + ( delta.y * sensitivity ), -90.0f, 90.0f ); // Clamp vertical rotation
   m_rotation.y                     = glm::mod( m_rotation.y + ( delta.x * sensitivity ) + 360.0f, 360.0f ); // Wrap horizontal rotation

   const WindowData& windowData = window.GetWindowData();
   lastMousePos                 = { windowData.Width * 0.5f, windowData.Height * 0.5f };
   glfwSetCursorPos( nativeWindow, lastMousePos.x, lastMousePos.y ); // Reset mouse to center of screen
}
