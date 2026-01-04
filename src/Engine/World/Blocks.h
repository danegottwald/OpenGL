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
   Furnace, // Added for testing orientation
   Count    // Keep as last, new entries should be inserted before this
};


// ------------------------------------------------------------
// Block Orientation
// ------------------------------------------------------------
enum class BlockOrientation : uint8_t
{
   North = 0,
   East  = 1,
   South = 2,
   West  = 3,
   Up    = 4,
   Down  = 5,
   None  = 6
};

// ------------------------------------------------------------
// Block State - Compact 16-bit storage for ID + Metadata
// ------------------------------------------------------------
struct BlockState
{
   uint16_t                  data        = 0;
   static constexpr uint16_t ID_MASK     = 0x0FFF;
   static constexpr uint16_t ORIENT_MASK = 0xF000;

   constexpr BlockState() = default;
   constexpr BlockState( BlockId id, BlockOrientation o = BlockOrientation::North ) :
      data( ( static_cast< uint16_t >( id ) & ID_MASK ) | ( static_cast< uint16_t >( o ) << 12 ) )
   {}

   constexpr bool             operator==( const BlockState& other ) const = default;
   constexpr BlockId          GetId() const { return static_cast< BlockId >( data & ID_MASK ); }
   constexpr BlockOrientation GetOrientation() const { return static_cast< BlockOrientation >( ( data >> 12 ) & 0xF ); }
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
   BlockInfo { BlockId::Furnace, "assets/models/furnace.json", BlockFlag::Solid | BlockFlag::Opaque },
};


constexpr const BlockInfo& GetBlockInfo( BlockId id ) noexcept
{
   return BlockData[ static_cast< size_t >( id ) ];
}

constexpr const BlockInfo& GetBlockInfo( BlockState state ) noexcept
{
   return GetBlockInfo( state.GetId() );
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
   using T = std::underlying_type_t< BlockFlag >;
   return ( static_cast< T >( v ) & static_cast< T >( f ) ) != 0;
}


constexpr bool FSolid( BlockId id ) noexcept
{
   return FHasFlag( GetBlockInfo( id ).flags, BlockFlag::Solid );
}

constexpr bool FSolid( BlockState state ) noexcept
{
   return FSolid( state.GetId() );
}
