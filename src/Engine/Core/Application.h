#pragma once

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
   Application() = default;
   NO_COPY_MOVE( Application )

   ApplicationState m_state { APP_STATE_NONE };
};
