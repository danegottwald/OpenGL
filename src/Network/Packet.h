#pragma once

#define PACKET_BUFFER_SIZE 1024
using PacketBuffer = std::array< uint8_t, PACKET_BUFFER_SIZE >;

enum class NetworkCode : uint8_t
{
   // Control Messages
   Invalid          = 0x00, // Invalid packet, probably dont need why would we make this
   Handshake        = 0x01,
   Heartbeat        = 0x02,
   ClientConnect    = 0x03,
   ClientDisconnect = 0x04,

   // Game Messages
   Chat           = 0x20,
   PositionUpdate = 0x21,
};

template< NetworkCode TCode >
struct NetworkCodeType;

// =========================================================================
// Packet
// =========================================================================
struct Packet
{
   // Packet Header (fixed fields)
   uint64_t    m_sourceID { 0 };
   uint64_t    m_destinationID { 0 }; // a destination id of 0 means broadcast to all clients
   NetworkCode m_code { NetworkCode::Invalid };
   uint16_t    m_bufferLength { 0 };

   //static inline constexpr auto FixedFields = std::make_tuple( &Packet::m_clientID, &Packet::m_code, &Packet::m_bufferLength );
   static inline constexpr auto FixedFields = std::make_tuple( &Packet::m_sourceID, &Packet::m_destinationID, &Packet::m_code, &Packet::m_bufferLength );

   // Buffer Data (variable fields)
   PacketBuffer m_buffer {};

   // Methods
   static constexpr size_t HeaderSize() noexcept;
   size_t                  Size() const noexcept { return HeaderSize() + m_bufferLength; }

   template< NetworkCode TCode > // Creates a packet with the specified code and data
   [[nodiscard]] static Packet Create( uint64_t srcID, uint64_t destID, const typename NetworkCodeType< TCode >::Type& data );

   template< NetworkCode TCode > // Parses the buffer and returns the parsed data
   [[nodiscard]] static typename NetworkCodeType< TCode >::Type ParseBuffer( const Packet& packet );

   [[nodiscard]] static std::vector< uint8_t > Serialize( const Packet& packet );
   [[nodiscard]] static Packet                 Deserialize( const std::vector< uint8_t >& data, size_t dataOffset = 0 );
};

inline constexpr size_t Packet::HeaderSize() noexcept
{
   size_t size = 0;
   std::apply( [ & ]( auto... pField ) { ( ( size += sizeof( std::remove_reference_t< decltype( std::declval< Packet >().*pField ) > ) ), ... ); },
               Packet::FixedFields );
   return size;
}

// =========================================================================
// Packet Specialization
//    All NetworkCode's should have a PacketMaker specialization defined
//    When adding a mapping that is not a primitive type, a specialization
//       for ParseBuffer is required
// =========================================================================
#define DEFINE_NETWORK_CODE_TYPE( code, type, fixedSize ) \
   template<> struct NetworkCodeType<code> { \
      using Type = type; \
      static constexpr size_t BufferSize = fixedSize; \
      template< typename Type = Type > \
      static auto ParseBuffer(const uint8_t* pBuffer, size_t bufferLength) { \
         if constexpr (std::is_fundamental_v<Type>) { Type value; std::memcpy( &value, pBuffer, bufferLength ); return value; } \
         else if constexpr( std::is_same_v< Type, glm::vec3 > ) { return Type( *reinterpret_cast< const glm::vec3* >( pBuffer ) ); } \
         else if constexpr( std::is_same_v< Type, std::string > ) { return Type( reinterpret_cast< const char* >( pBuffer ), bufferLength ); } \
         else if constexpr( std::is_same_v< Type, std::vector< type::value_type > > ) { return Type( pBuffer, pBuffer + bufferLength ); } \
         else if constexpr( std::is_same_v< Type, PacketBuffer > ) { PacketBuffer arrBuffer; std::memcpy( arrBuffer.data(), pBuffer, bufferLength ); return arrBuffer; } \
      } \
   };

// All NetworkCode's should have a PacketMaker specialization defined
//    When adding a mapping that is not a primitive type, add a specialization for ParseBuffer above
DEFINE_NETWORK_CODE_TYPE( NetworkCode::Handshake, uint64_t, sizeof( uint64_t ) )
DEFINE_NETWORK_CODE_TYPE( NetworkCode::Heartbeat, uint8_t, sizeof( uint8_t ) )
DEFINE_NETWORK_CODE_TYPE( NetworkCode::ClientConnect, uint64_t, sizeof( uint64_t ) )
DEFINE_NETWORK_CODE_TYPE( NetworkCode::ClientDisconnect, uint64_t, sizeof( uint64_t ) )
DEFINE_NETWORK_CODE_TYPE( NetworkCode::Chat, std::string, 0 )
DEFINE_NETWORK_CODE_TYPE( NetworkCode::PositionUpdate, glm::vec3, sizeof( glm::vec3 ) )

template< NetworkCode TCode >
inline Packet Packet::Create( uint64_t srcID, uint64_t destID, const typename NetworkCodeType< TCode >::Type& data )
{
   Packet packet;
   packet.m_destinationID = destID;
   packet.m_sourceID      = srcID; // maybe get rid of this, src should "always" be from the creator
   packet.m_code          = TCode;

   if constexpr( NetworkCodeType< TCode >::BufferSize > 0 ) // Fixed-size buffer
   {
      packet.m_bufferLength = NetworkCodeType< TCode >::BufferSize;
      std::memcpy( packet.m_buffer.data(), &data, packet.m_bufferLength );
   }
   else // Variable-size buffer
   {
      packet.m_bufferLength = data.size() * sizeof( typename NetworkCodeType< TCode >::Type::value_type );
      std::memcpy( packet.m_buffer.data(), data.data(), packet.m_bufferLength );
   }

   return packet;
}

template< NetworkCode TCode >
inline typename NetworkCodeType< TCode >::Type Packet::ParseBuffer( const Packet& packet )
{
   if( packet.m_code != TCode )
      throw std::runtime_error( "Attempted to parse packet with non-matching code" );

   return NetworkCodeType< TCode >::ParseBuffer( packet.m_buffer.data(), packet.m_bufferLength );
}
