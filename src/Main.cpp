#include <Engine/Core/Application.h>

int main( int argc, char* argv[] )
{
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
