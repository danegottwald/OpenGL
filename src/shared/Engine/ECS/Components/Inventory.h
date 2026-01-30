#pragma once

#include <Engine/World/Blocks.h>

// Minimal inventory representation suitable for both players and block entities.
// Future: replace with item stacks, item ids, NBT-like metadata, etc.
struct CInventory
{
   struct Slot
   {
      BlockId   item { BlockId::Air };
      uint16_t  count { 0 };
   };

   std::vector< Slot > slots;

   explicit CInventory( uint32_t slotCount = 0 )
   {
      slots.resize( slotCount );
   }
};
