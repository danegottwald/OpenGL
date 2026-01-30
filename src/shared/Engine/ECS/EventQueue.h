#pragma once

#include <vector>

template< typename TEvent >
class EventQueue
{
public:
   void Push( const TEvent& e ) { m_events.push_back( e ); }
   void Push( TEvent&& e ) { m_events.push_back( std::move( e ) ); }

   std::span< const TEvent > Events() const noexcept { return m_events; }

   void Clear() { m_events.clear(); }
   bool Empty() const noexcept { return m_events.empty(); }

private:
   std::vector< TEvent > m_events;
};
