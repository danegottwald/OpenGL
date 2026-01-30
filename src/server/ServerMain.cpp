#include "pch_server.h"

#include <Server/ServerApp.h>

int main( int /*argc*/, char* /*argv*/[] )
{
   try
   {
      Server::ServerApp app;
      app.Run();
   }
   catch( const std::exception& e )
   {
      std::println( std::cerr, "Server terminated unexpectedly: {}", e.what() );
      return -1;
   }

   return 0;
}
