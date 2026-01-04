#pragma once

// Project Dependencies
#include <Engine/World/Blocks.h>

struct CItemDrop
{
   BlockId blockId;

   // System use only
   uint64_t maxTicks { 0 };
   uint64_t ticksRemaining { 0 };
};
