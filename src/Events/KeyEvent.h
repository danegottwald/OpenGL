#pragma once

#include "Event.h"
#include "../Input/Codes.h"

namespace Events
{

//=========================================================================
// KeyEvent Interface
//=========================================================================
class KeyEvent : public IEvent
{
public:
   Input::KeyCode GetKeyCode() const { return m_keyCode; }

   EVENT_CLASS_CATEGORY( EventCategory::Input | EventCategory::Keyboard )

protected:
   KeyEvent( const Input::KeyCode keycode ) :
      m_keyCode( keycode )
   {}

   Input::KeyCode m_keyCode;
};

//=========================================================================
// KeyPressedEvent
//=========================================================================
class KeyPressedEvent final : public KeyEvent
{
public:
   KeyPressedEvent( const Input::KeyCode keycode, bool fRepeated ) :
      KeyEvent( keycode ),
      m_fRepeated( fRepeated )
   {}

   bool FRepeat() const { return m_fRepeated; }

   std::string ToString() const noexcept override
   {
      return std::format( "KeyPressedEvent: {0} (repeated: {1})", std::to_underlying( m_keyCode ), m_fRepeated );
   }

   EVENT_CLASS_TYPE( EventType::KeyPressed )

private:
   bool m_fRepeated;
};

//=========================================================================
// KeyReleasedEvent
//=========================================================================
class KeyReleasedEvent final : public KeyEvent
{
public:
   KeyReleasedEvent( const Input::KeyCode keycode ) :
      KeyEvent( keycode )
   {}

   std::string ToString() const noexcept override { return std::format( "KeyReleasedEvent: {0}", std::to_underlying( m_keyCode ) ); }

   EVENT_CLASS_TYPE( EventType::KeyReleased )
};

//=========================================================================
// KeyTypedEvent
//=========================================================================
class KeyTypedEvent final : public KeyEvent
{
public:
   KeyTypedEvent( const Input::KeyCode keycode ) :
      KeyEvent( keycode )
   {}

   std::string ToString() const noexcept override { return std::format( "KeyTypedEvent: {0}", std::to_underlying( m_keyCode ) ); }

   EVENT_CLASS_TYPE( EventType::KeyTyped )
};

} // namespace Events
