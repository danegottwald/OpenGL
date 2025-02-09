#pragma once

#include "Window.h"

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
   ~Application()                               = default;
   Application( const Application& )            = delete;
   Application& operator=( const Application& ) = delete;
   Application( Application&& )                 = delete;
   Application& operator=( Application&& )      = delete;

   void Run();

   static Application& Get() noexcept;

private:
   Application() noexcept = default;

private:
   ApplicationState m_state { APP_STATE_NONE };
};
