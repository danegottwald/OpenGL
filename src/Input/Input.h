#pragma once

// Local dependencies
#include "Codes.h"

namespace Input
{

//=========================================================================
// InputManager Class
//=========================================================================
class InputManager
{
private:
   InputManager()                                 = delete;
   ~InputManager()                                = delete;
   InputManager( const InputManager& )            = delete;
   InputManager& operator=( const InputManager& ) = delete;

   friend bool IsKeyPressed( KeyCode key ) noexcept;
   friend bool IsMouseButtonPressed( MouseCode button ) noexcept;
   friend void SetKeyPressed( KeyCode key, bool fPressed ) noexcept;
   friend void SetMouseButtonPressed( MouseCode button, bool fPressed ) noexcept;

   // Holds MouseCode + KeyCode values (safe as these values do not overlap)
   inline static std::unordered_map< uint16_t, bool > m_keys;
};

//=========================================================================
// Global Input Functions
//=========================================================================
inline bool IsKeyPressed( KeyCode key ) noexcept
{
   return InputManager::m_keys[ key ];
}

inline bool IsMouseButtonPressed( MouseCode button ) noexcept
{
   return InputManager::m_keys[ button ];
}

inline void SetKeyPressed( KeyCode key, bool fPressed ) noexcept
{
   InputManager::m_keys[ key ] = fPressed;
}

inline void SetMouseButtonPressed( MouseCode button, bool fPressed ) noexcept
{
   InputManager::m_keys[ button ] = fPressed;
}

} // namespace Input
