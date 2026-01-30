#pragma once

#include <Engine/Core/Time.h>
#include <Engine/Platform/Window.h>
#include <Engine/ECS/Registry.h>

namespace Engine
{

class GameContext
{
public:
   GameContext( Window& window, Time::FixedTimeStep& timestep, Entity::Registry& registry ) noexcept :
      m_window( window ),
      m_timestep( timestep ),
      m_registry( registry )
   {}

   Window&                    WindowRef() const noexcept { return m_window; }
   Entity::Registry&          RegistryRef() const noexcept { return m_registry; }
   const Time::FixedTimeStep& TimeRef() const noexcept { return m_timestep; }

private:
   Window&                    m_window;
   Entity::Registry&          m_registry;
   const Time::FixedTimeStep& m_timestep;
};

} // namespace Engine
