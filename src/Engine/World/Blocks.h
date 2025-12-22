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
   return static_cast< BlockFlag >( static_cast< uint8_t >( a ) | static_cast< uint8_t >( b ) );
}


// ------------------------------------------------------------
// Block Info - compile-time retrievable block metadata
// ------------------------------------------------------------
struct BlockInfo
{
   BlockId          id;
   std::string_view texture;
   BlockFlag        flags;
};


// ------------------------------------------------------------
// BlockData - definition of all block types and their properties
// ------------------------------------------------------------
constexpr BlockInfo BlockData[] = {
   { BlockId::Air,   "",                                 BlockFlag::None                      },
   { BlockId::Dirt,  "assets/textures/blocks/dirt.png",  BlockFlag::Solid | BlockFlag::Opaque },
   { BlockId::Stone, "assets/textures/blocks/stone.png", BlockFlag::Solid | BlockFlag::Opaque },
   { BlockId::Grass, "assets/textures/blocks/grass.png", BlockFlag::Solid | BlockFlag::Opaque },
};


constexpr const BlockInfo& GetBlockInfo( BlockId id )
{
   return BlockData[ static_cast< size_t >( id ) ];
}


// compile-time eval
constexpr auto _blockDataValidation = []()
{
   static_assert( sizeof( BlockData ) / sizeof( BlockInfo ) == static_cast< size_t >( BlockId::Count ), "BlockData size mismatch" );
   if constexpr( std::any_of( std::begin( BlockData ),
                              std::end( BlockData ),
                              []( const BlockInfo& info ) { return info.id != static_cast< BlockId >( &info - BlockData ); } ) )
      throw "BlockData ID mismatch";

   return 0;
}();


// ------------------------------------------------------------
// Utility functions
// ------------------------------------------------------------
constexpr bool FHasFlag( BlockFlag v, BlockFlag f )
{
   return ( static_cast< uint8_t >( v ) & static_cast< uint8_t >( f ) ) != 0;
}


constexpr bool FSolid( BlockId id )
{
   return FHasFlag( GetBlockInfo( id ).flags, BlockFlag::Solid );
}
