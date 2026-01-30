#pragma once

#include <Engine/ECS/ISystem.h>

namespace Engine::ECS
{

using PhaseMask = uint32_t;

constexpr PhaseMask PhaseBit( SystemPhase phase ) noexcept
{
   return 1u << static_cast< uint32_t >( phase );
}

class SystemScheduler
{
public:
   void Add( std::unique_ptr< ISystem > sys );

   void SetEnabledPhases( PhaseMask mask ) noexcept { m_enabledPhases = mask; }
   [[nodiscard]] PhaseMask EnabledPhases() const noexcept { return m_enabledPhases; }

   void TickPhase( SystemPhase phase, TickContext& ctx );
   void FixedTickPhase( SystemPhase phase, FixedTickContext& ctx );

private:
   PhaseMask                               m_enabledPhases { 0xFFFFFFFFu };
   std::vector< std::unique_ptr< ISystem > > m_systems;
};

} // namespace Engine::ECS
