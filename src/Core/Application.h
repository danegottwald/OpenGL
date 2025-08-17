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
   Application()                                = default;
   ~Application()                               = default;
   Application( const Application& )            = delete;
   Application& operator=( const Application& ) = delete;

   ApplicationState m_state { APP_STATE_NONE };
};
