#pragma once

// ========================================================================
//      IGUIElement
// ========================================================================
class IGUIElement
{
public:
   IGUIElement()  = default;
   ~IGUIElement() = default;

   virtual void Draw() = 0;
};

// ========================================================================
//      GUIManager
// ========================================================================
class GUIManager final
{
public:
   static void Init();
   static void Shutdown();

   static void Attach( std::shared_ptr< IGUIElement > element );
   static void Detach( const std::shared_ptr< IGUIElement >& element );
   static void Draw();

private:
   GUIManager()                               = delete;
   ~GUIManager()                              = delete;
   GUIManager( const GUIManager& )            = delete;
   GUIManager& operator=( const GUIManager& ) = delete;

   static inline bool                                          m_fInitialized { false };
   static inline std::vector< std::shared_ptr< IGUIElement > > m_elements;
};

// ========================================================================
//      DebugGUI
// ========================================================================
class DebugGUI final : public IGUIElement
{
public:
   DebugGUI( class Player& player, class Camera& camera );
   ~DebugGUI() = default;

   void Draw() override;

private:
   class Player& m_player;
   class Camera& m_camera;
};
