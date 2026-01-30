#include "FurnaceSystem.h"

#include <Engine/ECS/Components.h>

namespace Engine::ECS
{

static bool TryGetRecipe( BlockId in, BlockId& out )
{
   // Placeholder recipes. Professional design: data-driven recipe table.
   // Example: stone "smelts" into grass just to prove ticking works.
   if( in == BlockId::Stone )
   {
      out = BlockId::Grass;
      return true;
   }

   return false;
}

static uint64_t FuelTicks( BlockId fuel )
{
   // Placeholder fuels.
   if( fuel == BlockId::Dirt )
      return 80;
   return 0;
}

void FurnaceSystem::FixedTick( FixedTickContext& ctx )
{
   auto& reg = ctx.game.RegistryRef();

   for( auto [ e, furnace, inv ] : reg.ECView< CFurnace, CInventory >() )
   {
      // Interpret inventory: slot0=input, slot1=fuel, slot2=output.
      if( inv.slots.size() < 3 )
         continue;

      auto& inSlot  = inv.slots[ 0 ];
      auto& fuelSlot = inv.slots[ 1 ];
      auto& outSlot = inv.slots[ 2 ];

      // Consume fuel when needed.
      if( furnace.burnTicksRemaining == 0 )
      {
         const uint64_t fuelTime = FuelTicks( fuelSlot.item );
         if( fuelTime > 0 && fuelSlot.count > 0 )
         {
            fuelSlot.count--;
            if( fuelSlot.count == 0 )
               fuelSlot.item = BlockId::Air;

            furnace.burnTicksRemaining = fuelTime;
         }
      }

      const bool burning = furnace.burnTicksRemaining > 0;
      if( burning )
         furnace.burnTicksRemaining--;

      // If no recipe or no input, reset cook progress.
      BlockId recipeOut = BlockId::Air;
      if( inSlot.count == 0 || !TryGetRecipe( inSlot.item, recipeOut ) )
      {
         furnace.cookTicks = 0;
         continue;
      }

      // Need fuel/burning to cook.
      if( !burning )
         continue;

      constexpr uint64_t COOK_TIME = 200; // 10 seconds @ 20tps
      furnace.cookTicks++;

      if( furnace.cookTicks < COOK_TIME )
         continue;

      // Produce output.
      furnace.cookTicks = 0;

      if( outSlot.count > 0 && outSlot.item != recipeOut )
         continue; // output blocked

      if( outSlot.count == 0 )
         outSlot.item = recipeOut;

      outSlot.count++;

      // Consume one input.
      inSlot.count--;
      if( inSlot.count == 0 )
         inSlot.item = BlockId::Air;
   }
}

} // namespace Engine::ECS
