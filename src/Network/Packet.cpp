
#include "Packet.h"

// ===========================================================================
// Packet
// ===========================================================================
static void SerializeFixedFields( const Packet& packet, std::vector< uint8_t >& outData )
{
   size_t offset       = 0;
   auto   processField = [ &packet, &outData, &offset ]( auto pField )
   {
      const size_t field_size = sizeof( std::remove_reference_t< decltype( packet.*pField ) > );
      std::memcpy( outData.data() + offset, &( packet.*pField ), field_size );
      offset += field_size;
   };
   std::apply( [ & ]( auto... pField ) { ( processField( pField ), ... ); }, Packet::FixedFields );
}

static void DeserializeFixedFields( Packet& packet, const std::vector< uint8_t >& inData, size_t& offset )
{
   auto processField = [ &packet, &inData, &offset ]( auto pField )
   {
      const size_t field_size = sizeof( std::remove_reference_t< decltype( packet.*pField ) > );
      std::memcpy( static_cast< void* >( &( packet.*pField ) ), inData.data() + offset, field_size );
      offset += field_size;
   };
   std::apply( [ & ]( auto... pField ) { ( processField( pField ), ... ); }, Packet::FixedFields );
}

/* static */ std::vector< uint8_t > Packet::Serialize( const Packet& packet )
{
   std::vector< uint8_t > data( packet.Size() );
   SerializeFixedFields( packet, data );

   if( packet.m_bufferLength > 0 ) // serialize m_buffer
      std::memcpy( data.data() + Packet::HeaderSize(), packet.m_buffer.data(), packet.m_bufferLength );

   return data;
}

enum class DeserializeResult
{
   Success                    = 0,
   InsufficientData           = 1,
   DataOffsetOutOfBounds      = 2,
   BufferLengthExceedsMaxSize = 3,
};

struct DeserializationResult
{
   DeserializeResult       m_result;
   std::optional< Packet > m_optPacket; // only valid if m_result == DeserializeResult::Success
};

/* static */ DeserializationResult Deserialize( const std::vector< uint8_t >& data, size_t dataOffset )
{
   if( dataOffset >= data.size() )
      return { DeserializeResult::DataOffsetOutOfBounds, std::nullopt };

   if( data.size() < Packet::HeaderSize() )
      return { DeserializeResult::InsufficientData, std::nullopt };

   Packet packet;
   size_t offset = dataOffset;
   DeserializeFixedFields( packet, data, offset );

   if( packet.m_bufferLength == 0 )
      return { DeserializeResult::Success, std::move( packet ) }; // No buffer data, return early

   if( packet.m_bufferLength > PACKET_BUFFER_SIZE )
      return { DeserializeResult::BufferLengthExceedsMaxSize, std::nullopt };

   if( data.size() - offset < packet.m_bufferLength )
      return { DeserializeResult::InsufficientData, std::nullopt }; // not enough remaining data for buffer

   std::memcpy( packet.m_buffer.data(), data.data() + offset, packet.m_bufferLength ); // deserialize m_buffer
   return { DeserializeResult::Success, std::move( packet ) };
}

/* static */ Packet Packet::Deserialize( const std::vector< uint8_t >& data, size_t dataOffset )
{
   if( dataOffset >= data.size() )
      throw std::runtime_error( "Data offset is out of bounds." );

   if( data.size() < HeaderSize() )
      throw std::runtime_error( "Data size is insufficient for header deserialization." );

   Packet packet;
   size_t offset = dataOffset;
   DeserializeFixedFields( packet, data, offset );

   if( packet.m_bufferLength == 0 )
      return packet; // No buffer data, return early

   if( packet.m_bufferLength > PACKET_BUFFER_SIZE )
      throw std::runtime_error( "Buffer length exceeds the maximum allowed size." );

   if( data.size() - offset < packet.m_bufferLength )
      throw std::runtime_error( "Data size is insufficient for buffer deserialization." );

   std::memcpy( packet.m_buffer.data(), data.data() + offset, packet.m_bufferLength ); // deserialize m_buffer
   return packet;
}

// maybe the first member of our Packet should be length, then bufferdata is
//       data.data() + Packet::HeaderSize()
