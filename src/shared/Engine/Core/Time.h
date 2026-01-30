#pragma once

namespace Time
{

/**
 * @brief FixedTimeStep provides a frame-independent fixed timestep utility.
 * 
 * Tracks both continuous (frame) time and discrete (tick) time, allowing
 * systems to advance at a fixed rate while rendering smoothly.
 * 
 * - Advance() updates the timestep each frame.
 * - TryAdvanceTick() checks if enough time has accumulated for one fixed tick.
 * - Provides safe, model-agnostic access to deltas, tick fractions, and elapsed time.
 */
class FixedTimeStep
{
public:
   explicit FixedTimeStep( uint8_t tickRate ) noexcept :
      m_tickInterval( 1.0f / tickRate )
   {}

   // Advances time by the elapsed wall-clock delta
   float Advance( float maxDelta ) noexcept
   {
      const auto  now = std::chrono::steady_clock::now();
      const float dt  = m_last.time_since_epoch().count() == 0 ? 0.0f : std::chrono::duration< float >( now - m_last ).count();

      m_last = now;

      m_frameDelta = ( std::min )( dt, maxDelta );
      m_accumulator += m_frameDelta;
      return m_frameDelta;
   }

   // Advances one fixed tick if enough time has accumulated
   bool FTryAdvanceTick() noexcept
   {
      if( m_accumulator < m_tickInterval )
         return false;

      m_accumulator -= m_tickInterval;
      ++m_tickCount;
      return true;
   }

   // Continuous time
   float GetFrameDelta() const noexcept { return m_frameDelta; }
   float GetElapsedTime() const noexcept { return m_tickCount * m_tickInterval + m_accumulator; }

   // Discrete time
   float    GetTickInterval() const noexcept { return m_tickInterval; }
   uint64_t GetTickCount() const noexcept { return m_tickCount; }

   // Relationship
   float GetTickFraction() const noexcept { return m_accumulator / m_tickInterval; }
   float GetTickAccumulator() const noexcept { return m_accumulator; }

private:
   NO_COPY_MOVE( FixedTimeStep )

   const float                           m_tickInterval { 0.0f }; // fixed tick duration
   std::chrono::steady_clock::time_point m_last {};               // last wall-clock time
   float                                 m_frameDelta { 0.0f };   // delta since last Advance()
   float                                 m_accumulator { 0.0f };  // accumulated time for ticks
   uint64_t                              m_tickCount { 0 };       // total fixed ticks
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
