#include "Player.h"

#include "../Core/Window.h"
#include "../World.h"
#include "../../Events/KeyEvent.h"
#include "../../Events/MouseEvent.h"
#include "../../Input/Input.h"
#include <algorithm>

constexpr float GRAVITY           = -32.0f; // units/sec^2
constexpr float TERMINAL_VELOCITY = -48.0f; // units/sec
constexpr float JUMP_VELOCITY     = 9.0f;   // units/sec
constexpr float PLAYER_HEIGHT     = 1.9f;   // just under 2 cubes
constexpr float GROUND_MAXSPEED   = 12.0f;  // units/sec
constexpr float AIR_MAXSPEED      = 10.0f;  // units/sec
constexpr float AIR_CONTROL       = 0.3f;   // 0=no air control, 1=full
constexpr float SPRINT_MODIFIER   = 1.5f;   // Sprint speed multiplier

Player::Player()
{
   m_position = { 0.0f, 128.0f, 0 };

   // Subscribe to events
   m_eventSubscriber.Subscribe< Events::MouseMovedEvent >( std::bind( &Player::MouseMove, this, std::placeholders::_1 ) );
}

void Player::Update( float delta )
{
   // No longer directly update velocity/position here; handled in Tick
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
   glm::vec3 wishdir( 0.0f );
   if( Input::IsKeyPressed( Input::W ) )
   {
      wishdir.x -= glm::cos( glm::radians( m_rotation.y + 90 ) );
      wishdir.z -= glm::sin( glm::radians( m_rotation.y + 90 ) );
   }
   if( Input::IsKeyPressed( Input::S ) )
   {
      wishdir.x += glm::cos( glm::radians( m_rotation.y + 90 ) );
      wishdir.z += glm::sin( glm::radians( m_rotation.y + 90 ) );
   }
   if( Input::IsKeyPressed( Input::A ) )
   {
      float yaw = glm::radians( m_rotation.y );
      wishdir += glm::vec3( -glm::cos( yaw ), 0, -glm::sin( yaw ) );
   }
   if( Input::IsKeyPressed( Input::D ) )
   {
      float yaw = glm::radians( m_rotation.y );
      wishdir -= glm::vec3( -glm::cos( yaw ), 0, -glm::sin( yaw ) );
   }
   if( glm::length( wishdir ) > 0.0f )
      wishdir = glm::normalize( wishdir );

   float maxSpeed = m_fInAir ? AIR_MAXSPEED : GROUND_MAXSPEED;
   if( Input::IsKeyPressed( Input::LeftShift ) )
      maxSpeed *= SPRINT_MODIFIER;

   if( m_fInAir )
   {
      // Air control: add a portion of wishdir to velocity (horizontal only)
      glm::vec3 wishvel      = wishdir * maxSpeed;
      glm::vec3 velHoriz     = glm::vec3( m_velocity.x, 0, m_velocity.z );
      glm::vec3 wishvelHoriz = glm::vec3( wishvel.x, 0, wishvel.z );
      glm::vec3 add          = wishvelHoriz - velHoriz;
      m_velocity.x += add.x * AIR_CONTROL;
      m_velocity.z += add.z * AIR_CONTROL;
   }
   else
   {
      // On ground: set velocity directly
      m_velocity.x = wishdir.x * maxSpeed;
      m_velocity.z = wishdir.z * maxSpeed;
   }

   // Jump
   if( !m_fInAir && Input::IsKeyPressed( Input::Space ) )
   {
      m_velocity.y = JUMP_VELOCITY;
      m_fInAir     = true;
   }
}

void Player::UpdateState( World& world, float delta )
{
   m_velocity.y += GRAVITY * delta;
   if( m_velocity.y < TERMINAL_VELOCITY )
      m_velocity.y = TERMINAL_VELOCITY;

   m_position += m_velocity * delta;

   float edgeHeight   = std::max( { world.GetHeightAtPos( m_position.x + 0.5f, m_position.z ),
                                    world.GetHeightAtPos( m_position.x - 0.5f, m_position.z ),
                                    world.GetHeightAtPos( m_position.x, m_position.z + 0.5f ),
                                    world.GetHeightAtPos( m_position.x, m_position.z - 0.5f ) } );
   float cornerHeight = std::max( { world.GetHeightAtPos( m_position.x + 0.25f, m_position.z + 0.25f ),
                                    world.GetHeightAtPos( m_position.x - 0.25f, m_position.z + 0.25f ),
                                    world.GetHeightAtPos( m_position.x + 0.25f, m_position.z - 0.25f ),
                                    world.GetHeightAtPos( m_position.x - 0.25f, m_position.z - 0.25f ) } );
   float groundHeight = std::max( { edgeHeight, cornerHeight } );
   bool  wasInAir     = m_fInAir;
   // Player's feet are at (m_position.y - PLAYER_HEIGHT)
   m_fInAir = ( m_position.y - PLAYER_HEIGHT ) > groundHeight + 0.01f;
   if( !m_fInAir )
   {
      m_position.y = groundHeight + PLAYER_HEIGHT;
      m_velocity.y = 0.0f;
      // No horizontal velocity stop on landing
   }
}

void Player::MouseMove( const Events::MouseMovedEvent& event ) noexcept
{
   static Window& window       = Window::Get();
   GLFWwindow*    nativeWindow = window.GetNativeWindow();
   if( glfwGetInputMode( nativeWindow, GLFW_CURSOR ) != GLFW_CURSOR_DISABLED )
      return;

   constexpr float  sensitivity     = 1.0f / 16.0f;
   static glm::vec2 lastMousePos    = window.GetMousePosition();
   glm::vec2        currentMousePos = window.GetMousePosition();
   glm::vec2        delta           = currentMousePos - lastMousePos;
   m_rotation.x                     = glm::clamp( m_rotation.x + ( delta.y * sensitivity ), -90.0f, 90.0f );
   m_rotation.y                     = glm::mod( m_rotation.y + ( delta.x * sensitivity ) + 360.0f, 360.0f );

   const WindowData& windowData = window.GetWindowData();
   lastMousePos                 = { windowData.Width * 0.5f, windowData.Height * 0.5f };
   glfwSetCursorPos( nativeWindow, lastMousePos.x, lastMousePos.y );
}
