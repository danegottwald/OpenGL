#pragma once

#include <Game/Game.h>

#include <Engine/Core/UI.h>
#include <Engine/Events/Event.h>
#include <Engine/World/Level.h>
#include <Engine/World/RenderSystem.h>

#include <unordered_map>

namespace Game
{

class InGameState final : public Game
{
public:
   InGameState() = default;

   void OnEnter( Engine::GameContext& ctx ) override;
   void OnExit( Engine::GameContext& ctx ) override;

   void Update( Engine::GameContext& ctx, float dt ) override;
   void FixedUpdate( Engine::GameContext& ctx, float dt ) override;
   void Render( Engine::GameContext& ctx ) override;

   void DrawUI( Engine::GameContext& ctx ) override;

private:
   Entity::Entity m_player { Entity::NullEntity };
   Entity::Entity m_camera { Entity::NullEntity };

   std::unique_ptr< Level >                m_pLevel;
   std::unique_ptr< Engine::RenderSystem > m_pRenderSystem; // game state shouldn't own this?

   std::shared_ptr< UI::IDrawable > m_pDebugUI;
   std::shared_ptr< UI::IDrawable > m_pNetworkUI;

   Events::EventSubscriber                        m_events;
   std::unordered_map< uint64_t, Entity::Entity > m_connectedPlayers;
};

} // namespace Game
