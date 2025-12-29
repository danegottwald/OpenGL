#pragma once

#include <Game/Game.h>

// Forward Declarations
namespace Entity
{
class Registry;
}

namespace Time
{
class FixedTimeStep;
}


enum ApplicationState
{
   APP_STATE_NONE = 0,
   APP_STATE_MENU,
   APP_STATE_LOADING,
   APP_STATE_INGAME,
};


class Application
{
public:
   static Application& Get() noexcept
   {
      static Application app;
      return app;
   }

   void Run();

private:
   Application()  = default;
   ~Application() = default;
   NO_COPY_MOVE( Application )

   ApplicationState m_state { APP_STATE_NONE };

   std::unique_ptr< Entity::Registry > m_pRegistry;
   std::unique_ptr< Game::Game >       m_pGame;
};
