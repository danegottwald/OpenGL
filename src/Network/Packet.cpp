
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

/* static */ std::expected< Packet, Packet::DeserializationError > Packet::Deserialize( const std::vector< uint8_t >& data, size_t dataOffset )
{
   if( dataOffset >= data.size() )
      return std::unexpected( DeserializationError::DataOffsetOutOfBounds );

   if( data.size() < Packet::HeaderSize() )
      return std::unexpected( DeserializationError::InsufficientData );

   Packet packet;
   size_t offset = dataOffset;
   DeserializeFixedFields( packet, data, offset );

   if( packet.m_bufferLength == 0 )
      return packet; // No buffer data, return early

   if( packet.m_bufferLength > PACKET_BUFFER_SIZE )
      return std::unexpected( DeserializationError::BufferLengthExceedsMaxSize );

   if( data.size() - offset < packet.m_bufferLength )
      return std::unexpected( DeserializationError::InsufficientData ); // not enough remaining data for buffer

   std::memcpy( packet.m_buffer.data(), data.data() + offset, packet.m_bufferLength ); // deserialize m_buffer
   return packet;
}

// maybe the first member of our Packet should be length, then bufferdata is
//       data.data() + Packet::HeaderSize()
