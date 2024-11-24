#pragma once

//#include <GLFW/glfw3.h>

#include "Application.h"
#include "Window.h"

// struct MousePos
//{
//     float x, y;
// };
//
// class Input
//{
// public:
//     static bool IsKeyPressed(const KeyCode key)
//     {
//         int state = glfwGetKey(Window::Get().GetNativeWindow(), key);
//         return state == GLFW_PRESS || state == GLFW_REPEAT;
//     }
//
//     static bool IsMousePressed(const MouseCode button)
//     {
//         int state = glfwGetMouseButton(Window::Get().GetNativeWindow(),
//         button); return state == GLFW_PRESS;
//     }
//
//     static MousePos GetMousePos()
//     {
//         double x, y;
//         glfwGetCursorPos(Window::Get().GetNativeWindow(), &x, &y);
//         return {static_cast<float>(x), static_cast<float>(y)};
//     }
//
//     static float GetMouseX()
//     {
//         return GetMousePos().x;
//     }
//
//     static float GetMouseY()
//     {
//         return GetMousePos().y;
//     }
// };

// input controller
//      Has map < action name string?, func ptr > for key dowm
//      Has map < action name string?, func ptr > for key up
// 
//      Input Controller should update per tick (polling)? or from glfw callback
// 
// Input Mapping Context
//      map inputs to input actions
//      multiple can be applied to the same controller to allow swapping input mappings during different parts of game
//          Each context can be given a priority
//          Ex: User has context A for walking around in game, but uses context B for inventory screen keybinds
//              context B would be prioritized during inventory screen, and removed after exiting inventory
//

class InputContextMappingValue
{

};

// https://www.youtube.com/watch?v=bIo97TLsXkY
class InputContextMapping
{
public:

    std::uint16_t GetPriority() const
    {
        return m_priority;
    }

private:
    std::uint16_t m_priority = 0;
    // < ActionName, vector< Key > >
    //      The and list of keys that can invoke such action
    //          OR
    //      < Key (from vector), ActionName >
    // Could just do < ActionName, Key >
    //      As it would only support keyboard, typically vector< Key > is for multiple different devices
    //          Actually, you bind multiple keyboard keys and it can invoke the same action

    // { 
    //   "IA_Jump" : { Space },
    //   "IA_Move" : { W, A, S, D },
    //   "IA_Look" : { vec( X, Y ) },
    // }
    // 
    //  Each key can be wrapped to allow for trigger/modifier keys
    //      Trigger: Requirements for the action to be triggered
    //      Modifier: 
    //      Ex: "IA_Jump" : { 
    //                          { Key: Space, Trigger: ..., Modifier: ... }
    //                      }
    //          Jump will be called for space key when modifier shift is also pressed (can include more modifiers and all have to be met)
    //
};

class InputContextMappingComparer
{
public:
    bool operator()( const InputContextMapping& left, const InputContextMapping& right )
    {
        return ( ( left.GetPriority() ^ 1 ) < ( right.GetPriority() ^ 1 ) );
    }
};

// define static global input controller receiver type class with all callbacks and will fire them to all registered input controllers



//glfwSetKeyCallback( m_Window,
//    []( GLFWwindow* window, int key, int scancode, int action, int mods )
//    {
//        KeyCode keyCode = static_cast< KeyCode >( key );
//        InputControllerSubsystem& icSubsystem = InputControllerSubsystem::GetSubsystem();
//        for( const& ic : icSubsystem.GetInputControllers() )
//        {
//            map[ key ]
//        }
//
//switch( action )
//{
//    case GLFW_PRESS:
//        event = &KeyPressedEvent( keyCode, 0 );
//        break;
//    case GLFW_RELEASE:
//        event = &KeyReleasedEvent( keyCode );
//        break;
//    case GLFW_REPEAT:
//        event = &KeyPressedEvent( keyCode, 1 );
//        break;
//    default:
//        throw std::runtime_error( "Unknown KeyEvent" );
//}
//Window::Get().EventCallback()( *event );
//    } );

#include <queue>

class InputController
{
public:
    // Register the InputController to enable Action and Axis callbacks, can be called in no particular order
    void Register() noexcept;

    // Maybe this should be on the InputControllerSubsystem
    void AddMappingContext( const InputContextMapping& inputContextMapping, unsigned priority )
    {
        m_inputContextMappingQueue.push( inputContextMapping );
    }

    void BindAction( const std::string& identifier, EventType eventType, std::function< void() > actionFunc );
    void BindAction( const std::string& identifier, EventType eventType, std::function< void( bool ) > actionFunc );
    void BindAction( const std::string& identifier, EventType eventType, std::function< void( glm::vec2 ) > actionFunc )
    {
        struct KeyMappingValue
        {
            int axis; // Make enum for Axis
            bool m_fNegate = false;
        };

        std::unordered_map< std::string, KeyCode > keyMap;
        std::unordered_map< KeyCode, KeyMappingValue > keyCodeMap;

        keyMap.insert( { identifier, KeyCode::W } );
        keyCodeMap.insert( { KeyCode::W, { 0, false } } );

        // InputCallbacks
        KeyCode key = KeyCode::W;
        //keyMap[ key ]
    }
    void BindAction( const std::string& identifier, EventType eventType, std::function< void( glm::vec3 ) > actionFunc );

    // Will bind specified keycode to function with parameter yielding a delta adjusted value
    void BindAxis( const std::string& identifier, EventType eventType, std::function< void( float dtValue ) > axisFunc, float value );

private:
    //static std::vector< InputController > s_inputControllers;
    std::priority_queue< InputContextMapping, std::vector< InputContextMapping >, InputContextMappingComparer > m_inputContextMappingQueue;

};
