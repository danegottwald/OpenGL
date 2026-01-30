#pragma once

#include <Engine/World/Blocks.h>
#include <Engine/World/Level.h>

namespace World
{

struct BlockDef
{
   BlockId id { BlockId::Air };

   // Tick-based breaking.
   // `breakTicks == 0` => instant break.
   // Very large values can be used for unbreakable.
   uint32_t breakTicks { 20 };

   bool hasBlockEntity { false };
   bool openable { false };

   void ( *OnBroken )( Level& level, WorldBlockPos pos ) { nullptr };
};

class BlockDefRegistry
{
public:
   static const BlockDef& Get( BlockId id ) noexcept;
};

} // namespace World
