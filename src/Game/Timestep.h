#pragma once

// Timestep Class
//      Usage
//          Declare a Timestep instance to track elapsed time between frames.
//          Call Step() at the start of each frame to update the delta time.
//          Use GetDelta() to retrieve the elapsed time (delta time) since the last Step().
//          Example:
//              Timestep timestep; // Create a Timestep instance
//              while (game.IsRunning())
//              {
//                  timestep.Step(); // Update delta time
//                  float delta = timestep.GetDelta(); // Get the delta time
//                  game.Tick(delta); // Pass delta time to game logic
//              }

class Timestep
{
public:
   Timestep() noexcept {};

   Timestep( const Timestep& )            = delete;
   Timestep& operator=( const Timestep& ) = delete;
   Timestep( Timestep&& )                 = delete;
   Timestep& operator=( Timestep&& )      = delete;

   void Step() noexcept
   {
      const double currentTime = glfwGetTime();
      m_delta                  = currentTime - m_lastTime;
      m_lastTime               = currentTime;
   }
   float GetDelta() const noexcept { return m_delta; }

private:
   float m_lastTime = 0;
   float m_delta    = 0;
};
