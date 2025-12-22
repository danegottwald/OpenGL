#pragma once

namespace Entity
{
using Entity = uint64_t;
class Registry;
}

class Timestep;

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
   DebugGUI( const Entity::Registry& registry, Entity::Entity player, Entity::Entity camera, Timestep& timestep ) :
      m_registry( registry ),
      m_player( player ),
      m_camera( camera ),
      m_timestep( timestep )
   {}
   ~DebugGUI() = default;

   void Draw() override;

private:
   const Entity::Registry& m_registry;
   Entity::Entity          m_player;
   Entity::Entity          m_camera;
   Timestep&               m_timestep;
};
