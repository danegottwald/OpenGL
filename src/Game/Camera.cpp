
#include "Camera.h"

#include "Core/Window.h"
#include "Entity/Entity.h"

Camera::Camera()
{
    const WindowData& winData = Window::Get().GetWindowData();
    float width = winData.Width;
    float height = winData.Height;

    m_projectionMatrix = glm::perspective( glm::radians( 90.0f ) /*fovy*/, width / height /*aspect*/, 0.1f /*near*/, 100.0f /*far*/ );
}

void Camera::Update( const Entity& entity )
{
    // update frustum using m_ProjectionViewMatrix

    // Set New Camera Position
    m_position = entity.m_Position;
    m_rotation = entity.m_Rotation;

    // Update m_ProjectionViewMatrix (all below)
    glm::mat4 view{ 1.0f };
    view = glm::rotate( view, glm::radians( m_rotation.x ), { 1, 0, 0 } );
    view = glm::rotate( view, glm::radians( m_rotation.y ), { 0, 1, 0 } );
    view = glm::rotate( view, glm::radians( m_rotation.z ), { 0, 0, 1 } );
    view = glm::translate( view, -m_position );

    m_projectionViewMatrix = m_projectionMatrix * view;
}

bool Camera::OnEvent( EventDispatcher& dispatcher )
{
    // remove below? make more dispatchers for each GameDef member?
    IEvent& event = dispatcher.GetEvent();
    switch( event.GetEventType() )
    {
        case EventType::KeyPressed:
        case EventType::KeyReleased:
        case EventType::KeyTyped:
        case EventType::MouseButtonPressed:
        case EventType::MouseButtonReleased:
        case EventType::MouseMoved:
        case EventType::MouseScrolled:
            return false;
        case EventType::WindowResize:
        {
            WindowResizeEvent& resizeEvent = static_cast< WindowResizeEvent& >( event );
            if( resizeEvent.GetHeight() == 0 )
                return true;

            float aspectRatio = resizeEvent.GetWidth() / resizeEvent.GetHeight();
            m_projectionMatrix = glm::perspective( glm::radians( 90.0f ), aspectRatio, 0.1f, 100.0f );
            return true;
        }
        default:
            return false;
    }
    // dispatcher.Dispatch<KeyPressedEvent>(
    //     std::bind(&funcHere, this, std::placeholders::_1));
}

const glm::mat4& Camera::GetProjectionView() const
{
    return m_projectionViewMatrix;
}

const glm::vec3& Camera::GetPosition() const
{
    return m_position;
}
