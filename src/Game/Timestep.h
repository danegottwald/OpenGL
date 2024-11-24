#pragma once

// Timestep Class
//      Usage
//          Declare baseline Timestep where you want to track start time
//          To get the elapsed time from the baseline Timestep, create another
//          Timestep in the following manner
//              Timestep start; // Baseline
//              ...
//              ...
//              Timestep delta(start); // Will contain delta time
//
class Timestep
{
public:
    Timestep();
    Timestep(Timestep& other);

    Timestep(const Timestep&) = delete;
    Timestep& operator=(const Timestep&) = delete;
    Timestep(Timestep&&) = delete;
    Timestep& operator=(Timestep&&) = delete;

    float Get() const;

private:
    float m_StartTime;
    float m_Delta = 0;
};
