#include "SystemScheduler.h"

namespace Engine::ECS
{

void SystemScheduler::Add( std::unique_ptr< ISystem > sys )
{
   if( !sys )
      return;

   m_systems.push_back( std::move( sys ) );

   // Stable order by phase keeps determinism and lets game insert systems in a predictable way.
   std::stable_sort( m_systems.begin(),
                     m_systems.end(),
                     []( const auto& a, const auto& b ) { return static_cast< int >( a->Phase() ) < static_cast< int >( b->Phase() ); } );
}

void SystemScheduler::TickPhase( SystemPhase phase, TickContext& ctx )
{
   if( ( m_enabledPhases & PhaseBit( phase ) ) == 0 )
      return;

   for( const auto& s : m_systems )
      if( s->Phase() == phase )
         s->Tick( ctx );
}

void SystemScheduler::FixedTickPhase( SystemPhase phase, FixedTickContext& ctx )
{
   if( ( m_enabledPhases & PhaseBit( phase ) ) == 0 )
      return;

   for( const auto& s : m_systems )
      if( s->Phase() == phase )
         s->FixedTick( ctx );
}

} // namespace Engine::ECS
