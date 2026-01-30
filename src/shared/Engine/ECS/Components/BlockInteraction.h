#pragma once

#include <Engine/ECS/Registry.h>
#include <Engine/World/Level.h>

// Player-side interaction state for intent generation.
struct CBlockInteractor
{
   float reach { 6.0f };

   // input edge tracking
   bool fWasLeftDown { false };
   bool fWasRightDown { false };
};

// Tag component for entities used as block entities (furnace/chest/etc.).
struct CBlockEntity
{
   WorldBlockPos pos { 0, 0, 0 };
   BlockId       blockId { BlockId::Air };
};

// Tick-based furnace simulation.
struct CFurnace
{
   uint64_t burnTicksRemaining { 0 };
   uint64_t cookTicks { 0 };

   // optional: could track what recipe is active / last
   BlockId lastInput { BlockId::Air };
};
