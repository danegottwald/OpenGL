#include "pch_client.h"

#include <Engine/Core/Application.h>

int main( int /*argc*/, char* /*argv*/[] )
{
   try
   {
      Application::Get().Run();
   }
   catch( const std::exception& e )
   {
      std::println( std::cerr, "Client terminated unexpectedly: {}", e.what() );
      return -1;
   }

   return 0;
}
