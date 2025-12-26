#pragma once

// ------------------------------------------------------------
// Block Identifiers
// ------------------------------------------------------------
enum class BlockId : uint16_t
{
   Air,
   Dirt,
   Stone,
   Grass,
   Bedrock,
   Count // Keep as last, new entries should be inserted before this
};


// ------------------------------------------------------------
// Block Flags - general properties of blocks
// ------------------------------------------------------------
enum class BlockFlag : uint32_t
{
   None   = 0,
   Solid  = 1 << 0, // Blocks that are solid (collidable)
   Opaque = 1 << 1, // Blocks that are opaque (not see-through)
};


constexpr BlockFlag operator|( BlockFlag a, BlockFlag b )
{
   using T = std::underlying_type_t< BlockFlag >;
   return static_cast< BlockFlag >( static_cast< T >( a ) | static_cast< T >( b ) );
}


// ------------------------------------------------------------
// Block Info - compile-time retrievable block metadata
// ------------------------------------------------------------
struct BlockInfo
{
   BlockId          id;
   std::string_view json;
   BlockFlag        flags;
};


// ------------------------------------------------------------
// BlockData - definition of all block types and their properties
// ------------------------------------------------------------
inline constexpr std::array BlockData = {
   BlockInfo { BlockId::Air,     "",                           BlockFlag::None                      },
   BlockInfo { BlockId::Dirt,    "assets/models/dirt.json",    BlockFlag::Solid | BlockFlag::Opaque },
   BlockInfo { BlockId::Stone,   "assets/models/stone.json",   BlockFlag::Solid | BlockFlag::Opaque },
   BlockInfo { BlockId::Grass,   "assets/models/grass.json",   BlockFlag::Solid | BlockFlag::Opaque },
   BlockInfo { BlockId::Bedrock, "assets/models/bedrock.json", BlockFlag::Solid | BlockFlag::Opaque },
};


constexpr const BlockInfo& GetBlockInfo( BlockId id ) noexcept
{
   return BlockData[ static_cast< size_t >( id ) ];
}


// compile-time eval
constexpr auto _blockDataValidation = []()
{
   static_assert( std::size( BlockData ) == static_cast< size_t >( BlockId::Count ), "BlockData size mismatch" );
   if constexpr( std::any_of( BlockData.begin(),
                              BlockData.end(),
                              []( const BlockInfo& info ) { return info.id != static_cast< BlockId >( &info - BlockData.data() ); } ) )
      throw "BlockData ID mismatch";

   return 0;
}();


// ------------------------------------------------------------
// Utility functions
// ------------------------------------------------------------
constexpr bool FHasFlag( BlockFlag v, BlockFlag f ) noexcept
{
   return ( static_cast< uint8_t >( v ) & static_cast< uint8_t >( f ) ) != 0;
}


constexpr bool FSolid( BlockId id ) noexcept
{
   return FHasFlag( GetBlockInfo( id ).flags, BlockFlag::Solid );
}
