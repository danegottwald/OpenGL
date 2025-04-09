
#include "Packet.h"

// ===========================================================================
// Packet
// ===========================================================================
template< typename PacketType, typename Operation >
static void ProcessFixedFields( PacketType& packet, const std::vector< uint8_t >& data, size_t& offset, Operation operation )
{
   auto processField = [ &packet, &offset, &data, &operation ]( auto pField )
   {
      const size_t field_size = sizeof( std::remove_reference_t< decltype( packet.*pField ) > );
      operation( &( packet.*pField ), const_cast< void* >( static_cast< const void* >( data.data() + offset ) ), field_size );
      offset += field_size;
   };
   std::apply( [ & ]( auto... pField ) { ( processField( pField ), ... ); }, PacketType::FixedFields );
}

/* static */ std::vector< uint8_t > Packet::Serialize( const Packet& packet )
{
   std::vector< uint8_t > data( packet.Size() );
   size_t                 offset = 0;
   ProcessFixedFields( packet, data, offset, []( const void* src, void* dest, size_t size ) { std::memcpy( dest, src, size ); } );
   std::memcpy( data.data() + offset, packet.m_buffer.data(), packet.m_bufferLength ); // serialize m_buffer
   return data;
}

/* static */ Packet Packet::Deserialize( const std::vector< uint8_t >& data, size_t dataOffset )
{
   Packet packet;
   size_t offset = dataOffset;
   ProcessFixedFields( packet, data, offset, []( void* dest, const void* src, size_t size ) { std::memcpy( dest, src, size ); } );
   std::memcpy( packet.m_buffer.data(), data.data() + offset, packet.m_bufferLength ); // deserialize m_buffer
   return packet;
}
