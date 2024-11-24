
#include "Player.h"

#include "../Core/Input.h"
#include "../Core/Window.h"
#include "../World.h"
#include "../../Events/KeyEvent.h"
#include "../../Events/MouseEvent.h"

Player::Player()
{
}

void Player::Tick( World& world, float delta )
{
    ApplyInputs();
    UpdateState( world, delta );
}

void Player::ApplyInputs()
{
    float speed = 0.4f;
    if( m_InputMap[ KeyCode::LeftShift ] ) speed *= 10;

    if( m_InputMap[ KeyCode::W ] )
    {
        m_Acceleration.x -= glm::cos( glm::radians( m_Rotation.y + 90 ) ) * speed;
        m_Acceleration.z -= glm::sin( glm::radians( m_Rotation.y + 90 ) ) * speed;
    }
    if( m_InputMap[ KeyCode::S ] )
    {
        m_Acceleration.x += glm::cos( glm::radians( m_Rotation.y + 90 ) ) * speed;
        m_Acceleration.z += glm::sin( glm::radians( m_Rotation.y + 90 ) ) * speed;
    }
    if( m_InputMap[ KeyCode::A ] )
    {
        float yaw = glm::radians( m_Rotation.y );
        m_Acceleration += glm::vec3( -glm::cos( yaw ), 0, -glm::sin( yaw ) ) * speed;
    }
    if( m_InputMap[ KeyCode::D ] )
    {
        float yaw = glm::radians( m_Rotation.y );
        m_Acceleration -= glm::vec3( -glm::cos( yaw ), 0, -glm::sin( yaw ) ) * speed;
    }
    if( m_InputMap[ KeyCode::Space ] )
    {
        m_Acceleration.y += speed;
    }
    if( m_InputMap[ KeyCode::LeftControl ] )
    {
        m_Acceleration.y -= speed;
    }

    if( m_Rotation.x < -90.0f ) m_Rotation.x = -89.9f;
    if( m_Rotation.x > 90.0f ) m_Rotation.x = 89.9f;
    //std::cout << m_Position.x << " " << m_Position.y << " " << m_Position.z << std::endl;
}

void Player::UpdateState( World& world, float delta )
{
    // Set Player Velocity
    m_Velocity += m_Acceleration;
    m_Acceleration = glm::vec3( 0.0f );

    // Update position based on velocity
    m_Position += m_Velocity * delta;

    m_Velocity *= 0.9f;

    // Handle collision detection and response
    // collide(world, m_Velocity * delta);
}

bool Player::OnEvent( EventDispatcher& dispatcher )
{
    Window& window = Window::Get();
    if( glfwGetInputMode( window.GetNativeWindow(), GLFW_CURSOR ) != GLFW_CURSOR_DISABLED )
        return false;

    IEvent& event = dispatcher.GetEvent();
    switch( event.GetEventType() )
    {
        case EventType::KeyPressed:
            return dispatcher.Dispatch< KeyPressedEvent >(
                std::bind( &Player::KeyPressed, this, std::placeholders::_1 ) );
        case EventType::KeyReleased:
            return dispatcher.Dispatch< KeyReleasedEvent >(
                std::bind( &Player::KeyReleased, this, std::placeholders::_1 ) );
        case EventType::KeyTyped:
            return false;
        case EventType::MouseButtonPressed:
            return false;
        case EventType::MouseButtonReleased:
            return false;
        case EventType::MouseMoved:
            return dispatcher.Dispatch< MouseMovedEvent >(
                std::bind( &Player::MouseMove, this, std::placeholders::_1 ) );
        case EventType::MouseScrolled:
            return false;
        default:
            return false;
    }
}

// --------------------------------------------------------------------
//      Player Event Handling
// --------------------------------------------------------------------
bool Player::KeyPressed( KeyPressedEvent& e )
{
    if( !e.FRepeat() )
        m_InputMap[ e.GetKeyCode() ] = true;

    return true;
}

bool Player::KeyReleased( KeyReleasedEvent& e )
{
    m_InputMap[ e.GetKeyCode() ] = false;
    return true;
}

bool Player::MouseButtonPressed( MouseButtonPressedEvent& e )
{
    m_InputMap[ e.GetMouseButton() ] = true;
    return true;
}

bool Player::MouseButtonReleased( MouseButtonReleasedEvent& e )
{
    m_InputMap[ e.GetMouseButton() ] = false;
    return true;
}

bool Player::MouseMove( MouseMovedEvent& e )
{
    Window& window = Window::Get();
    static glm::vec2 lastMousePos = window.GetMousePosition();

    float horizontalSens = 0.5f;
    float verticalSens = 0.5f;

    glm::vec2 delta = window.GetMousePosition() - lastMousePos;
    m_Rotation.x += delta.y / 8.0f * verticalSens;
    m_Rotation.y += delta.x / 8.0f * horizontalSens;

    WindowData& windowData = window.GetWindowData();
    lastMousePos.x = windowData.Width / 2;
    lastMousePos.y = windowData.Height / 2;

    // Reset mouse cursor to center of screen
    glfwSetCursorPos( window.GetNativeWindow(), lastMousePos.x, lastMousePos.y );

    return true;
}
