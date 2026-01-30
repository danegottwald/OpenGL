#include "BlockDefs.h"

namespace World
{

constexpr std::array BlockDefs = {
   BlockDef { .id = BlockId::Air, .breakTicks = 0, .hasBlockEntity = false, .openable = false, .OnBroken = nullptr },
   BlockDef { .id = BlockId::Dirt, .breakTicks = 10 },
   BlockDef { .id = BlockId::Stone, .breakTicks = 60 },
   BlockDef { .id = BlockId::Grass, .breakTicks = 12 },
   BlockDef { .id = BlockId::Bedrock, .breakTicks = 0xFFFFFFFFu },
   BlockDef { .id = BlockId::Furnace, .breakTicks = 80, .hasBlockEntity = true, .openable = true },
};
constexpr auto _blockDefsValidation = []() // compile-time validation of BlockDefs
{
   static_assert( BlockDefs.size() == static_cast< size_t >( BlockId::Count ), "BlockDefs size does not match BlockId::Count" );
   if constexpr( std::any_of( BlockDefs.begin(),
                              BlockDefs.end(),
                              []( const BlockDef& def ) { return def.id != static_cast< BlockId >( &def - BlockDefs.data() ); } ) )
      throw "BlockDef ID mismatch";

   return true;
}();


/*static*/ const BlockDef& BlockDefRegistry::Get( BlockId id ) noexcept
{
   return BlockDefs.at( static_cast< size_t >( id ) );
}

} // namespace World
