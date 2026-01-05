#include "WorldSave.h"

namespace World
{

enum class SaveKind : uint16_t
{
   Meta,
   Player,
   Chunk,
   Entity,
   Count // Keep as last, new entries should be inserted before this
};

struct Directory
{
   SaveKind         kind;
   std::string_view directory;
   std::string_view filePattern;
};

static constexpr std::string_view SavesDirectory = "saves";
static constexpr std::array       Directories    = {
   Directory { .kind = SaveKind::Meta,   .directory = "",         .filePattern = "meta.bin"           },
   Directory { .kind = SaveKind::Player, .directory = "",         .filePattern = "player.dat"         },
   Directory { .kind = SaveKind::Chunk,  .directory = "chunks",   .filePattern = "chunk_{}_{}_{}.bin" },
   Directory { .kind = SaveKind::Entity, .directory = "entities", .filePattern = "entity_{}.ent"      },
};


// compile-time eval
constexpr auto _directoryValidation = []()
{
   static_assert( std::size( Directories ) == static_cast< size_t >( SaveKind::Count ), "Directories size mismatch" );
   if constexpr( std::any_of( Directories.begin(),
                              Directories.end(),
                              []( const Directory& dir ) { return dir.kind != static_cast< SaveKind >( &dir - Directories.data() ); } ) )
      throw "Directory kind mismatch";

   return 0;
}();


template< typename... Args >
static std::filesystem::path Path( const std::filesystem::path& worldDir, SaveKind kind, Args&&... args )
{
   const auto& dir = Directories[ static_cast< size_t >( kind ) ];
   return worldDir / dir.directory / std::vformat( std::string( dir.filePattern ), std::make_format_args( std::forward< Args >( args )... ) );
}


static void EnsureDirectories( const std::filesystem::path& worldDir )
{
   static bool s_directoriesEnsured = [ & ]()
   {
      std::filesystem::create_directories( worldDir );
      std::for_each( Directories.begin(),
                     Directories.end(),
                     [ & ]( const Directory& dir ) { std::filesystem::create_directories( worldDir / dir.directory ); } );
      return true;
   }();
}


static bool FWriteAllBytes( const std::filesystem::path& path, std::span< const std::byte > bytes )
{
   std::ofstream out( path, std::ios::binary | std::ios::trunc );
   if( !out )
      return false;

   out.write( reinterpret_cast< const char* >( bytes.data() ), static_cast< std::streamsize >( bytes.size() ) );
   return out.good();
}


static bool FReadAllBytes( const std::filesystem::path& path, std::vector< std::byte >& outBytes )
{
   std::ifstream in( path, std::ios::binary );
   if( !in )
      return false;

   in.seekg( 0, std::ios::end );
   const auto size = static_cast< size_t >( in.tellg() );
   in.seekg( 0, std::ios::beg );

   outBytes.resize( size );
   if( size )
      in.read( reinterpret_cast< char* >( outBytes.data() ), static_cast< std::streamsize >( size ) );

   return in.good();
}


template< typename T >
static T Deserialize( std::span< const std::byte > bytes )
{
   T t {};
   std::memcpy( &t, bytes.data(), sizeof( T ) );
   return t;
}


/*static*/ std::filesystem::path WorldSave::RootDir( const std::filesystem::path& worldName )
{
   if( worldName.has_parent_path() )
      return worldName;

   return std::filesystem::path( SavesDirectory ) / worldName;
}


// Meta
/*static*/ bool WorldSave::FSaveMeta( const std::filesystem::path& worldDir, const WorldMeta& meta )
{
   EnsureDirectories( worldDir );

   std::array< std::byte, sizeof( WorldMeta ) > bytes {};
   std::memcpy( bytes.data(), &meta, sizeof( WorldMeta ) );
   return FWriteAllBytes( Path( worldDir, SaveKind::Meta ), bytes );
}


/*static*/ std::optional< WorldMeta > WorldSave::LoadMeta( const std::filesystem::path& worldDir )
{
   std::vector< std::byte > bytes;
   if( !FReadAllBytes( Path( worldDir, SaveKind::Meta ), bytes ) || bytes.size() != sizeof( WorldMeta ) )
      return std::nullopt;

   return Deserialize< WorldMeta >( bytes );
}


// Player
/*static*/ bool WorldSave::FSavePlayer( const std::filesystem::path& worldDir, const PlayerSave& player )
{
   EnsureDirectories( worldDir );

   std::array< std::byte, sizeof( PlayerSave ) > bytes {};
   std::memcpy( bytes.data(), &player, sizeof( PlayerSave ) );
   return FWriteAllBytes( Path( worldDir, SaveKind::Player ), bytes );
}

/*static*/ std::optional< PlayerSave > WorldSave::LoadPlayer( const std::filesystem::path& worldDir )
{
   std::vector< std::byte > bytes;
   if( !FReadAllBytes( Path( worldDir, SaveKind::Player ), bytes ) || bytes.size() != sizeof( PlayerSave ) )
      return std::nullopt;

   return Deserialize< PlayerSave >( bytes );
}


// Entity
/*static*/ bool WorldSave::FSaveEntity( const std::filesystem::path& worldDir, const EntitySave& entity )
{
   EnsureDirectories( worldDir );

   std::array< std::byte, sizeof( EntitySave ) > bytes {};
   std::memcpy( bytes.data(), &entity, sizeof( EntitySave ) );
   return FWriteAllBytes( Path( worldDir, SaveKind::Entity, entity.id ), bytes );
}


/*static*/ bool WorldSave::FDeleteEntityFile( const std::filesystem::path& worldDir, uint64_t entityId )
{
   std::error_code ec;
   return std::filesystem::remove( Path( worldDir, SaveKind::Entity, entityId ), ec );
}


// Chunk
/*static*/ bool WorldSave::FSaveChunkBytes( const std::filesystem::path& worldDir, const ChunkPos3& cpos, std::span< const std::byte > bytes )
{
   EnsureDirectories( worldDir );
   return FWriteAllBytes( Path( worldDir, SaveKind::Chunk, cpos.x, cpos.y, cpos.z ), bytes );
}


/*static*/ bool WorldSave::FLoadChunkBytes( const std::filesystem::path& worldDir, const ChunkPos3& cpos, std::vector< std::byte >& outBytes )
{
   return FReadAllBytes( Path( worldDir, SaveKind::Chunk, cpos.x, cpos.y, cpos.z ), outBytes );
}


} // namespace World
