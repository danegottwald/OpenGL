#pragma once

#include <cstdint>

namespace Engine
{

// Helpers for fixed-timestep games.
// Professional practice: represent gameplay timing in ticks in the simulation layer.
struct TickCounter
{
   uint64_t value { 0 };

   constexpr TickCounter() = default;
   constexpr explicit TickCounter( uint64_t v ) :
      value( v )
   {}

   constexpr operator uint64_t() const noexcept { return value; }
};

struct TickTimer
{
   uint64_t remaining { 0 };

   void Start( uint64_t ticks ) noexcept { remaining = ticks; }
   void Stop() noexcept { remaining = 0; }

   [[nodiscard]] bool Active() const noexcept { return remaining > 0; }

   // Returns true exactly once when the timer reaches 0.
   bool Tick() noexcept { return remaining && ( --remaining == 0 ); }
};

} // namespace Engine
