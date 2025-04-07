
#include "Player.h"

#include "../Core/Window.h"
#include "../World.h"
#include "../../Events/KeyEvent.h"
#include "../../Events/MouseEvent.h"
#include "../../Input/Input.h"

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

   // Up and Down
   if( Input::IsKeyPressed( Input::Space ) && !m_fInAir ) // Jump only if not in air
   {
      m_acceleration.y += 1.5f;
      m_velocity.y += 1.5f;
   }
   if( Input::IsKeyPressed( Input::LeftControl ) )
      m_acceleration.y -= speed;

   //std::cout << m_position.x << " " << m_position.y << " " << m_position.z << std::endl;
}

void Player::UpdateState( World& world, float delta )
{
   // Check if player is on the ground (collision check)
   float edgeHeight   = std::max( { world.GetHeightAtPos( m_position.x + 0.5f, m_position.z ),
                                    world.GetHeightAtPos( m_position.x - 0.5f, m_position.z ),
                                    world.GetHeightAtPos( m_position.x, m_position.z + 0.5f ),
                                    world.GetHeightAtPos( m_position.x, m_position.z - 0.5f ) } );
   float cornerHeight = std::max( { world.GetHeightAtPos( m_position.x + 0.25f, m_position.z + 0.25f ),
                                    world.GetHeightAtPos( m_position.x - 0.25f, m_position.z + 0.25f ),
                                    world.GetHeightAtPos( m_position.x + 0.25f, m_position.z - 0.25f ),
                                    world.GetHeightAtPos( m_position.x - 0.25f, m_position.z - 0.25f ) } );
   float groundHeight = std::max( { edgeHeight, cornerHeight } ) + 2;
   m_fInAir           = m_position.y > groundHeight;

   const glm::vec3 gravity( 0.0f, 9.81f, 0.0f );
   if( m_fInAir ) // apply gravity if player in the air
   {
      m_acceleration.y -= gravity.y * delta;        // Apply gravity to vertical acceleration
      m_velocity += m_acceleration * delta * 0.03f; // Apply general acceleration (movement)
   }
   else
      m_velocity += m_acceleration * delta; // Apply general acceleration (movement)

   m_position += m_velocity * delta; // Apply velocity to update position

   if( m_position.y <= groundHeight )
   {
      m_position.y     = groundHeight; // Correct player's position to the ground level
      m_velocity.y     = 0.0f;         // Stop downward movement (velocity)
      m_acceleration.y = 0.0f;         // Reset vertical acceleration
      m_fInAir         = false;        // Player is now on the ground
   }

   // Handle collision detection and response (optional, depending on your game logic)
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
   m_rotation.x                     = glm::clamp( m_rotation.x + ( delta.y * sensitivity ), -90.0f, 90.0f ); // Clamp vertical rotation
   m_rotation.y                     = glm::mod( m_rotation.y + ( delta.x * sensitivity ) + 360.0f, 360.0f ); // Wrap horizontal rotation

   const WindowData& windowData = window.GetWindowData();
   lastMousePos                 = { windowData.Width * 0.5f, windowData.Height * 0.5f };
   glfwSetCursorPos( nativeWindow, lastMousePos.x, lastMousePos.y ); // Reset mouse to center of screen
}
