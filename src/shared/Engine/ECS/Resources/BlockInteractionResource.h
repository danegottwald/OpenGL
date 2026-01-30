#pragma once

#include <Engine/ECS/BlockEntityRegistry.h>
#include <Engine/ECS/EventQueue.h>
#include <Engine/ECS/BlockInteractionEvents.h>

namespace Engine::ECS
{

// Tracks per-player mining state to emulate Minecraft behavior:
// - progress resets when the targeted block changes
// - progress can be re-used if you keep mining the same block
struct PlayerMiningState
{
   bool     fHasTarget { false };
   BlockPos target { 0, 0, 0 };

   // Player-owned mining progress (authoritative on server).
   uint32_t accumulatedTicks { 0 };
   uint64_t lastHitTick { 0 };
};

// Per-world gameplay resource for block interaction.
struct BlockInteractionResource
{
   // Intent queues
   EventQueue< BlockHitEvent >         hit;
   EventQueue< BlockBreakEvent >       brk;
   EventQueue< BlockUseEvent >         use;
   EventQueue< OpenBlockEntityEvent >  open;

   // Per-player mining state (target + progress)
   std::unordered_map< Entity::Entity, PlayerMiningState > mining;

   // Block entity mapping
   BlockEntityRegistry blockEntities;

   void ClearIntents()
   {
      hit.Clear();
      brk.Clear();
      use.Clear();
      open.Clear();
   }
};

} // namespace Engine::ECS
