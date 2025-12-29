#pragma once

namespace Time
{

/**
 * @brief Timestep class for managing time intervals and ticks.
 * This class provides functionality to track the time elapsed between frames as well as manage fixed update ticks.
 *
 * @example
 * FixedTimeStep timestep( 20.0f ); // 20 ticks per second
 * while ( game.IsRunning() )
 * {
 *     timestep.Step(); // Update delta time
 *
 *     if ( timestep.FShouldTick() )
 *         game.Update(); // Called at fixed intervals
 *
 *     game.Update( timestep.GetDelta() ); // Called every frame
 * }
 */
class FixedTimeStep
{
public:
   explicit FixedTimeStep( uint8_t tickrate ) noexcept :
      m_interval( 1.0f / tickrate )
   {}

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
   float GetDelta() const noexcept { return m_delta; }

   // Returns true if enough time has passed to advance a tick
   bool     FShouldTick() const noexcept { return m_accumulator >= m_interval; }
   uint64_t GetTickCount() const noexcept { return m_tick; }

private:
   NO_COPY_MOVE( FixedTimeStep )

   // Delta
   float m_lastTime { 0.0f };
   float m_delta { 0.0f };

   // Tick
   const float m_interval { 0.0f };
   float       m_accumulator { 0.0f };
   uint64_t    m_tick { 0 };
};


/**
 * @brief Utility class for accumulating time and triggering actions at fixed intervals.
 * 
 * This class tracks elapsed time and determines when a certain interval has passed.
 * Useful for implementing fixed-timestep updates, periodic events, or throttled operations.
 *
 * @example
 * IntervalTimer timer(0.5f); // Trigger every 0.5 seconds
 * 
 * while (game.IsRunning())
 * {
 *     float dt = GetDeltaTime();
 *     if (timer.FTick(dt))
 *         game.Update(); // Called every 0.5 seconds
 * }
 */
class IntervalTimer
{
public:
   explicit IntervalTimer( float interval ) noexcept :
      m_interval( interval )
   {}

   bool FTick( float dt ) noexcept
   {
      m_accumulator += dt;
      if( m_accumulator < m_interval )
         return false;

      m_accumulator -= m_interval; // make sure we keep remainder to it carries over
      return true;
   }

private:
   NO_COPY_MOVE( IntervalTimer )

   const float m_interval { 0.0f };
   float       m_accumulator { 0.0f };
};

} // namespace Time
