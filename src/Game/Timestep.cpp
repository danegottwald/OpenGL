#include "Timestep.h"

Timestep::Timestep() : m_StartTime( glfwGetTime() )
{
}

Timestep::Timestep( Timestep& other ) : m_StartTime( glfwGetTime() )
{
    m_Delta = m_StartTime - other.m_StartTime;
    other.m_StartTime = m_StartTime;
}

float Timestep::Get() const
{
    return m_Delta;
}
