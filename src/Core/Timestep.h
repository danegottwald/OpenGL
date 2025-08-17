#pragma once

/**
 * @brief Timestep class for managing time intervals and ticks.
 * This class provides functionality to track the time elapsed between frames as well as manage fixed update ticks.
 *
 * @example
 * Timestep timestep( 20.0f ); // 20 ticks per second
 * while ( game.IsRunning() )
 * {
 *     timestep.Step(); // Update delta time
 *
 *     if ( timestep.FTick() )
 *         game.Update(); // Called at fixed intervals
 *
 *     game.Update( timestep.GetDelta() ); // Called every frame
 * }
 */
class Timestep
{
public:
   Timestep( uint8_t tickrate ) :
      m_interval( 1.0f / tickrate )
   {}

   Timestep( const Timestep& )            = delete;
   Timestep& operator=( const Timestep& ) = delete;
   Timestep( Timestep&& )                 = delete;
   Timestep& operator=( Timestep&& )      = delete;

   // Step the Timestep to update delta time and tick state
   float Step() noexcept
   {
      const double currentTime = glfwGetTime();
      m_delta                  = currentTime - m_lastTime;
      m_lastTime               = currentTime;

      if( m_accumulator >= m_interval )
      {
         m_accumulator -= m_interval;
         m_tick++;
      }
      else
         m_accumulator += m_delta;

      return m_delta;
   }
   float GetLastDelta() const noexcept { return m_delta; }

   // Returns true if enough time has passed to advance a tick
   bool     FTick() noexcept { return m_accumulator >= m_interval; }
   uint64_t GetLastTick() const { return m_tick; }

private:
   // Delta
   float m_lastTime { 0.0f };
   float m_delta { 0.0f };

   // Tick
   const float m_interval { 0.0f };
   float       m_accumulator { 0.0f };
   uint64_t    m_tick { 0 };
};
