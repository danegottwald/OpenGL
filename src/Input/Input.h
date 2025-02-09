#pragma once

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
   static bool IsKeyPressed( KeyCode key ) noexcept { return m_keys[ key ]; }

   friend bool IsMouseButtonPressed( MouseCode button ) noexcept;
   static bool IsMouseButtonPressed( MouseCode button ) noexcept { return m_keys[ button ]; }

   friend void SetKeyPressed( KeyCode key, bool fPressed ) noexcept;
   static void SetKeyPressed( KeyCode key, bool fPressed ) noexcept { m_keys[ key ] = fPressed; }

   friend void SetMouseButtonPressed( MouseCode button, bool fPressed ) noexcept;
   static void SetMouseButtonPressed( MouseCode button, bool fPressed ) noexcept { m_keys[ button ] = fPressed; }

   // Holds MouseCode + KeyCode values (safe as these values do not overlap)
   inline static std::unordered_map< uint16_t, bool > m_keys;
};

//=========================================================================
// Global Input Functions
//=========================================================================
inline bool IsKeyPressed( KeyCode key ) noexcept
{
   return InputManager::IsKeyPressed( key );
}

inline bool IsMouseButtonPressed( MouseCode button ) noexcept
{
   return InputManager::IsMouseButtonPressed( button );
}

inline void SetKeyPressed( KeyCode key, bool fPressed ) noexcept
{
   InputManager::SetKeyPressed( key, fPressed );
}

inline void SetMouseButtonPressed( MouseCode button, bool fPressed ) noexcept
{
   InputManager::SetMouseButtonPressed( button, fPressed );
}

} // namespace Input
