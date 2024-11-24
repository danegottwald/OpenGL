
#include "Game/Core/Application.h"
#include "Game/Core/Input.h"

int main( int argc, char* argv[] )
{
    std::cout << argv[ 0 ] << std::endl;

    {
        // Note: Axis: X can be default and Negate be false default
        // "Move"
        //      Key: W, Trigger: ..., Modifier: (Axis::X, Negate: false)
        //      Key: A, Trigger: ..., Modifier: (Axis::X, Negate: true)
        //      Key: S, Trigger: ..., Modifier: (Axis::Y, Negate: true)
        //      Key: D, Trigger: ..., Modifier: (Axis::Y, Negate: false)
        //

        InputController inputController;
        inputController.BindAction( "Move", EventType::KeyPressed, []( glm::vec2& input )
            {
                // player.m_forwardVec
            } );
    }

    // Global event manager? register an event then they get polled every refresh
    // https://gamedev.stackexchange.com/a/59627

    // put input controller inside game loop to be polled and fire event?


    // Run application
    Application& app = Application::Get();
    app.Run();

    return 0;
}
