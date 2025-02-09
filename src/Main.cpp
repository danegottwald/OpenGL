
#include "Game/Core/Application.h"

//#include <fmt/format.h> // log library using this?

int main( int argc, char* argv[] )
{
   std::cout << argv[ 0 ] << std::endl;

   // Global event manager? register an event then they get polled every refresh
   // https://gamedev.stackexchange.com/a/59627

   // Run application
   Application& app = Application::Get();
   app.Run();

   return 0;
}
