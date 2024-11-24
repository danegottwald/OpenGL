#include "PlayerController.h"

#include "Input.h"
#include "Window.h"

PlayerController* PlayerController::s_Controller = nullptr;

PlayerController::PlayerController()
{
    assert( s_Controller == nullptr );
    s_Controller = this;
    m_Position = glm::vec3( 0.0f, 0.0f, 2.0f );
    m_ViewDirection = glm::vec3( 0.0f, 0.0f, -1.0f );
    m_OldMousePos = glm::vec2( 0.0f, 0.0f );
    m_Up = glm::vec3( 0.0f, 1.0f, 0.0f );

    m_FOV = 45.0f;
    m_MoveSpeed = 5.0f;
    m_LookSpeed = 0.5f;

    EnableCursorCallback();
}

void PlayerController::Update( float deltaTime )
{
    //if( Input::IsKeyPressed( KeyCode::W ) )
    //{
    //    m_Position += ( m_MoveSpeed * deltaTime ) * m_ViewDirection;
    //}
    //if( Input::IsKeyPressed( KeyCode::S ) )
    //{
    //    m_Position += -( m_MoveSpeed * deltaTime ) * m_ViewDirection;
    //}
    //if( Input::IsKeyPressed( KeyCode::D ) )
    //{
    //    m_Position +=
    //        ( m_MoveSpeed * deltaTime ) * normalize( cross( m_ViewDirection,
    //            m_Up ) );
    //}
    //if( Input::IsKeyPressed( KeyCode::A ) )
    //{
    //    m_Position += -( m_MoveSpeed * deltaTime ) *
    //        normalize( cross( m_ViewDirection, m_Up ) );
    //}
    //if( Input::IsKeyPressed( KeyCode::Space ) )
    //{
    //    m_Position.y += ( m_MoveSpeed * deltaTime );
    //}
    //if( Input::IsKeyPressed( KeyCode::LeftShift ) )
    //{
    //    m_Position.y += -( m_MoveSpeed * deltaTime );
    //}
    //if( Input::IsKeyPressed( KeyCode::Up ) )
    //{
    //    m_FOV = 170.0f;
    //}
    //if( Input::IsKeyPressed( KeyCode::Down ) )
    //{
    //    m_FOV = 45.0f;
    //}
    //if( Input::IsMousePressed( MouseCode::ButtonRight ) )
}

void PlayerController::EnableCursorCallback()
{
    return;
    glfwSetCursorPosCallback(
        Window::Get().GetNativeWindow(),
        []( GLFWwindow* window, double x, double y )
        {
            auto& pc = GetController();

            static bool first = true;
            if( first )
            {
                first = false;
                pc.m_OldMousePos.x = x;
                pc.m_OldMousePos.y = y;
            }

            // Get the angles that x and y moved
            glm::vec2 mouseDelta = glm::vec2( x, y ) - pc.m_OldMousePos;
            float xAngle = glm::radians( -mouseDelta.x * pc.m_LookSpeed ) / 8.0f;
            float yAngle = glm::radians( -mouseDelta.y * pc.m_LookSpeed ) / 8.0f;
            pc.m_OldMousePos = glm::vec2( x, y );

            // Find the m_ViewDirection vector
            glm::vec3 rotateAxis = cross( pc.m_ViewDirection, pc.m_Up );
            pc.m_ViewDirection =
                glm::mat3( rotate( glm::mat4( 1.0f ), xAngle, pc.m_Up ) *
                    rotate( glm::mat4( 1.0f ), yAngle, rotateAxis ) ) *
                pc.m_ViewDirection;

            pc.m_Up = normalize(
                glm::mat3( rotate( glm::mat4( 1.0f ), yAngle, pc.m_Up ) ) * pc.m_Up );
            // pc.m_Up = glm::cross(glm::cross(pc.m_ViewDirection, {0,1,0}),
            // pc.m_ViewDirection);
            pc.m_ViewDirection = normalize( pc.m_ViewDirection );
        } );
}

glm::mat4 PlayerController::GetViewMatrix() const
{
    return lookAt( m_Position, m_Position + m_ViewDirection, m_Up );
}

glm::mat4 PlayerController::GetProjectionMatrix() const
{
    auto prop = Window::Get().GetWindowData();
    return glm::perspective(
        m_FOV, static_cast< float >( prop.Width ) / static_cast< float >( prop.Height ),
        0.01f, 200.0f );
}
