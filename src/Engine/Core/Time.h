#pragma once

namespace Time
{

/**
 * @brief Timestep class for managing time intervals and ticks.
 * This class provides functionality to track the time elapsed between frames as well as manage fixed update ticks.
 *
 * @example
 * FixedTimeStep timestep( 20 ); // tickrate
 * while ( game.FIsRunning() )
 * {
 *     timestep.Step(); // Update delta time
 *
 *     while ( timestep.FTryTick() )
 *         game.FixedUpdate(); // Called at fixed intervals
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

   // Step the Timestep to update delta time
   float Step( float clamp ) noexcept
   {
      const float currentTime = static_cast< float >( glfwGetTime() );
      if( m_lastTime == 0.0f )
         m_lastTime = currentTime;

      m_delta    = std::min( currentTime - m_lastTime, clamp ); // clamp to avoid spiral of death
      m_lastTime = currentTime;
      m_accumulator += m_delta;
      return m_delta;
   }
   float GetDelta() const noexcept { return m_delta; }
   float GetInterval() const noexcept { return m_interval; }

   // Returns true if enough time has passed to advance a tick
   bool FTryTick() noexcept
   {
      if( m_accumulator < m_interval )
         return false; // not enough time accumulated

      m_accumulator -= m_interval;
      m_tick++;
      return true;
   }

   // Get interpolation alpha (0.0 - 1.0) for rendering between ticks
   float GetAlpha() const noexcept { return m_accumulator / m_interval; }

   // Returns the interpolation delta in seconds (already multiplied by tick interval)
   float GetInterpolationDelta() const noexcept { return m_accumulator; }

   // Get total elapsed time since start in seconds
   float GetTotalTime() const noexcept { return m_tick * m_interval + m_accumulator; }

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
