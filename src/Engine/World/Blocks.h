#pragma once

// ------------------------------------------------------------
// Block Identifiers
// ------------------------------------------------------------
enum class BlockId : uint16_t
{
   Air     = 0,
   Dirt    = 1,
   Stone   = 2,
   Grass   = 3,
   Bedrock = 4,
   Furnace = 5,
   Count // Keep as last, new entries should be inserted before this
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
   Down  = 5
};


// ------------------------------------------------------------
// Block Properties - default metadata per block type
// ------------------------------------------------------------
struct BlockProperties
{
   BlockId          id { BlockId::Air };
   BlockOrientation orientation { BlockOrientation::North };
   // add more fields as needed
};


// ------------------------------------------------------------
// Block State - packed per-block instance data
// ------------------------------------------------------------
class BlockState
{
public:
   constexpr BlockState() = default;
   constexpr explicit BlockState( BlockId id ) :
      BlockState( BlockProperties { .id = id } )
   {}
   constexpr explicit BlockState( const BlockProperties& props )
   {
      uint16_t bits = 0;
      bits          = IdField::Insert( bits, static_cast< uint16_t >( props.id ) );
      bits          = OrientationField::Insert( bits, static_cast< uint16_t >( props.orientation ) );
      data          = bits;
   }

   constexpr bool             operator==( const BlockState& other ) const = default;
   constexpr BlockProperties  GetProperties() const { return BlockProperties { .id = GetId(), .orientation = GetOrientation() }; }
   constexpr BlockId          GetId() const { return static_cast< BlockId >( IdField::Extract( data ) ); }
   constexpr BlockOrientation GetOrientation() const { return static_cast< BlockOrientation >( OrientationField::Extract( data ) ); }

private:
   // Bit helper
   template< uint16_t StartBit, uint16_t BitCount >
   struct Field
   {
      static constexpr uint16_t mask = ( ( 1u << BitCount ) - 1 ) << StartBit;
      static constexpr uint16_t Extract( uint16_t v ) { return ( v & mask ) >> StartBit; }
      static constexpr uint16_t Insert( uint16_t v, uint16_t f ) { return ( v & ~mask ) | ( ( f << StartBit ) & mask ); }
   };

   // Layout of BlockState
   using IdField          = Field< 0, 12 >; // 12 bits for BlockId
   using OrientationField = Field< 12, 3 >; // 3 bits for orientation

   // Stored data
   uint32_t data { 0 };
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
constexpr auto _blockDataValidation = []() // compile-time validation of BlockData
{
   static_assert( std::size( BlockData ) == static_cast< size_t >( BlockId::Count ), "BlockData size mismatch" );
   if constexpr( std::any_of( BlockData.begin(),
                              BlockData.end(),
                              []( const BlockInfo& info ) { return info.id != static_cast< BlockId >( &info - BlockData.data() ); } ) )
      throw "BlockData ID mismatch";

   return 0;
}();


constexpr const BlockInfo& GetBlockInfo( BlockId id ) noexcept
{
   return BlockData[ static_cast< size_t >( id ) ];
}


constexpr const BlockInfo& GetBlockInfo( BlockState state ) noexcept
{
   return GetBlockInfo( state.GetId() );
}


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
