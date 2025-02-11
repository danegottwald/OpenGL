#pragma once

// ========================================================================
//      IGuiElement
// ========================================================================
class IGuiElement
{
public:
   IGuiElement()  = default;
   ~IGuiElement() = default;

   virtual void Draw() = 0;
};

// ========================================================================
//      GUIManager
// ========================================================================
class GUIManager final
{
public:
   static void Init();
   static void Reset();

   static void Attach( std::shared_ptr< IGuiElement > element );
   static void Detach( std::shared_ptr< IGuiElement > element );
   static void Draw();

private:
   GUIManager()                               = delete;
   ~GUIManager()                              = delete;
   GUIManager( const GUIManager& )            = delete;
   GUIManager& operator=( const GUIManager& ) = delete;

   static bool                                          m_fInitialized;
   static std::vector< std::shared_ptr< IGuiElement > > m_elements;
};

// ========================================================================
//      DebugGUI
// ========================================================================
class DebugGUI final : public IGuiElement
{
public:
   DebugGUI()  = default;
   ~DebugGUI() = default;

   void Draw() override;
};
