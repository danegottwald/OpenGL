#pragma once

#include <Game/GameState.h>

#include <Engine/Core/GameContext.h>
#include <Engine/UI/UI.h>
#include <Engine/Events/Event.h>

#include <Engine/World/Level.h>
#include <Engine/World/RenderSystem.h>

#include <Engine/ECS/SystemScheduler.h>
#include <Engine/ECS/Resources/BlockInteractionResource.h>

namespace Game
{

class InGameState final : public GameState
{
public:
   InGameState() = default;

   void OnEnter() override;
   void OnExit() override;

   void Update( float dt ) override;
   void FixedUpdate( float tickInterval ) override;
   void Render() override;

   void DrawUI( UI::UIContext& ui ) override;

private:
   Entity::Entity m_player { Entity::NullEntity };
   Entity::Entity m_camera { Entity::NullEntity };

   std::unique_ptr< Level >                m_pLevel;
   std::unique_ptr< Engine::RenderSystem > m_pRenderSystem;

   std::shared_ptr< UI::IDrawable > m_pDebugUI;
   std::shared_ptr< UI::IDrawable > m_pNetworkUI;

   Events::EventSubscriber                        m_events;
   std::unordered_map< uint64_t, Entity::Entity > m_connectedPlayers;

   Engine::ECS::SystemScheduler                             m_scheduler;
   std::unique_ptr< Engine::ECS::BlockInteractionResource > m_blockRes;
};

} // namespace Game
