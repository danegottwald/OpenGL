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
   InputManager()  = delete;
   ~InputManager() = delete;

   // Key
   friend bool FKeyPressed( KeyCode key ) noexcept;
   friend void SetKeyPressed( KeyCode key, bool fPressed ) noexcept;

   // Mouse
   friend bool FMouseButtonPressed( MouseCode button ) noexcept;
   friend void SetMouseButtonPressed( MouseCode button, bool fPressed ) noexcept;

   // Holds MouseCode + KeyCode values (safe as these values do not overlap)
   inline static std::unordered_map< uint16_t, bool > m_keys;
};


//=========================================================================
// Global Input Functions
//=========================================================================
[[nodiscard]] inline bool FKeyPressed( KeyCode key ) noexcept
{
   return InputManager::m_keys[ key ];
}


inline void SetKeyPressed( KeyCode key, bool fPressed ) noexcept
{
   InputManager::m_keys[ key ] = fPressed;
}


[[nodiscard]] inline bool FMouseButtonPressed( MouseCode button ) noexcept
{
   return InputManager::m_keys[ button ];
}


inline void SetMouseButtonPressed( MouseCode button, bool fPressed ) noexcept
{
   InputManager::m_keys[ button ] = fPressed;
}

} // namespace Input
