#pragma once

// Timestep Class
//      Usage
//          Use Timestep to track delta time between frames and control fixed-tick updates.
//          Call Step() once per frame to update the internal delta time.
//          Call FTick() to check if enough time has passed to advance a fixed tick.
//          Use GetDelta() for frame-based logic and GetCurrentTick() for tick-based logic.
//          Example:
//              Timestep timestep(20.0f); // 20 ticks per second
//              while (game.IsRunning())
//              {
//                  timestep.Step(); // Update delta time
//
//                  if (timestep.FTick())
//                      game.FixedUpdate(); // Called at fixed intervals
//
//                  game.Update(timestep.GetDelta()); // Called every frame
//              }
class Timestep
{
public:
   Timestep( float tickrate ) :
      m_interval( 1.0f / tickrate )
   {}

   Timestep( const Timestep& )            = delete;
   Timestep& operator=( const Timestep& ) = delete;
   Timestep( Timestep&& )                 = delete;
   Timestep& operator=( Timestep&& )      = delete;

   // Delta Related
   float Step() noexcept
   {
      const double currentTime = glfwGetTime();
      m_delta                  = currentTime - m_lastTime;
      m_lastTime               = currentTime;
      return m_delta;
   }
   float GetLastDelta() const noexcept { return m_delta; }

   // Tick Related
   bool FTick()
   {
      m_accumulator += m_delta;
      if( m_accumulator >= m_interval )
      {
         m_accumulator -= m_interval;
         m_tick++;
         return true;
      }

      return false;
   }
   uint64_t GetLastTick() const { return m_tick; }

private:
   // Delta
   float m_lastTime { 0.0f };
   float m_delta { 0.0f };

   // Tick
   float    m_interval { 0.0f };
   float    m_accumulator { 0.0f };
   uint64_t m_tick { 0 };
};
