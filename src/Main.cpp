#include <Core/Application.h>

//#include <fmt/format.h> // log library using this?

int main( int argc, char* argv[] )
{
   std::println( "{}", argv[ 0 ] ); // Print the executable name

   // Global event manager? register an event then they get polled every refresh
   // https://gamedev.stackexchange.com/a/59627

   // Run application
   try
   {
      Application& app = Application::Get();
      app.Run();
   }
   catch( const std::exception& e )
   {
      std::println( std::cerr, "Application terminated unexpectedly: {}", e.what() );
      return -1;
   }

   return 0;
}
