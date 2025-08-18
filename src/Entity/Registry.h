#pragma once

namespace Entity
{

#if defined( _MSC_VER )
#define FORCE_INLINE __forceinline
#else
#define FORCE_INLINE inline __attribute__( ( always_inline ) )
#endif

using Entity = uint64_t;

class EntityHandle
{
private:
   friend class Registry;
   EntityHandle( Entity entity, Registry& registry ) :
      m_entity( entity ),
      m_registry( registry )
   {}
   ~EntityHandle() = default;

public:
   EntityHandle( const EntityHandle& )                = delete;
   EntityHandle( EntityHandle&& ) noexcept            = delete;
   EntityHandle& operator=( EntityHandle&& ) noexcept = delete;
   EntityHandle& operator=( const EntityHandle& )     = delete;

   [[nodiscard]] FORCE_INLINE Entity Get() const noexcept { return m_entity; }

   FORCE_INLINE explicit operator Entity() const noexcept { return m_entity; }
   FORCE_INLINE bool     operator==( const EntityHandle& other ) const noexcept { return m_entity == other.m_entity; }
   FORCE_INLINE bool     operator!=( const EntityHandle& other ) const noexcept { return m_entity != other.m_entity; }
   FORCE_INLINE bool     operator==( Entity other ) const noexcept { return m_entity == other; }
   FORCE_INLINE bool     operator!=( Entity other ) const noexcept { return m_entity != other; }
   FORCE_INLINE bool     operator<( const EntityHandle& other ) const noexcept { return m_entity < other.m_entity; }

private:
   const Entity m_entity { 0 };
   Registry&    m_registry;
};

enum class ViewType
{
   Entity,
   Components,
   EntityAndComponents
};

// =============================================
// Registry
// =============================================
class Registry
{
private:
   template< ViewType TViewType, typename TType, typename... TOthers >
   friend class View;

   // clang-format off
   static inline size_t s_nextTypeIndex = 0;
   template< typename TType >
   struct TypeIndexHelper { FORCE_INLINE static size_t Get() { static const size_t s_index = s_nextTypeIndex++; return s_index; } };
   template< typename TType >
   FORCE_INLINE static size_t GetTypeIndex() noexcept { return TypeIndexHelper< std::remove_pointer_t< std::remove_reference_t< std::remove_cv_t< TType > > > >::Get(); }
   // clang-format on

   // Type-erased pool record (replaces abstract Pool base)
   struct StoragePool
   {
      void* pStorage { nullptr };                               // points to Storage<T>
      void ( *removeFn )( void*, Entity ) noexcept { nullptr }; // removes entity from storage
      void ( *destroyFn )( void* ) noexcept { nullptr };        // destroys storage correctly
   };

   template< typename TType >
   struct Storage
   {
      using DenseIndexType                 = uint32_t;
      static constexpr DenseIndexType npos = ( std::numeric_limits< DenseIndexType >::max )();

      std::vector< TType >          m_types;
      std::vector< Entity >         m_entities;
      std::vector< DenseIndexType > m_entityToIndex; // sparse

      template< typename... Args >
      FORCE_INLINE TType& EmplaceConstruct( Entity entity, Args&&... args )
      {
         if( entity >= m_entityToIndex.size() )
            m_entityToIndex.resize( static_cast< size_t >( entity ) + 1, npos );

         DenseIndexType index = static_cast< DenseIndexType >( m_types.size() );
         m_types.emplace_back( std::forward< Args >( args )... );
         m_entities.emplace_back( entity );
         m_entityToIndex[ entity ] = index;
         return m_types.back();
      }

      FORCE_INLINE TType* Get( Entity entity ) noexcept
      {
         if( entity >= m_entityToIndex.size() )
            return nullptr;

         DenseIndexType index = m_entityToIndex[ entity ];
         return index == npos ? nullptr : &m_types[ index ];
      }
      FORCE_INLINE const TType* Get( Entity entity ) const noexcept { return const_cast< Storage* >( this )->Get( entity ); }

      FORCE_INLINE void Remove( Entity entity ) noexcept
      {
         if( entity < m_entityToIndex.size() )
         {
            DenseIndexType index = m_entityToIndex[ entity ];
            if( index != npos )
            {
               DenseIndexType backIndex = static_cast< DenseIndexType >( m_types.size() - 1 );
               if( index != backIndex )
               {
                  m_types[ index ]         = std::move( m_types[ backIndex ] );
                  Entity moved             = m_entities[ backIndex ];
                  m_entities[ index ]      = moved;
                  m_entityToIndex[ moved ] = index;
               }

               m_types.pop_back();
               m_entities.pop_back();
               m_entityToIndex[ entity ] = npos;
            }
         }
      }
   };

public:
   Registry() = default;
   ~Registry()
   {
      // destroy all storages
      for( auto& rec : m_storagePools )
      {
         if( rec.destroyFn && rec.pStorage )
            rec.destroyFn( rec.pStorage );
      }
   }

   Registry( const Registry& )            = delete;
   Registry& operator=( const Registry& ) = delete;

   // ----- Entity lifecycle -----
   [[nodiscard]] FORCE_INLINE Entity Create() noexcept
   {
      Entity entity;
      if( !m_recycled.empty() )
      {
         entity = m_recycled.back();
         m_recycled.pop_back();
      }
      else
         entity = m_nextEntity++;

      if( entity >= m_entityAlive.size() )
      {
         m_entityAlive.resize( static_cast< size_t >( entity ) + 1, 0 );
         m_entityTypes.resize( static_cast< size_t >( entity ) + 1 );
      }
      else
         m_entityTypes[ entity ].clear();

      m_entityAlive[ entity ] = 1;
      return entity;
   }

   [[nodiscard]] FORCE_INLINE std::shared_ptr< EntityHandle > CreateWithHandle() noexcept
   {
      return std::shared_ptr< EntityHandle >( new EntityHandle( Create(), *this ),
                                              []( EntityHandle* pPtr )
      {
         pPtr->m_registry.Destroy( pPtr->m_entity );
         delete pPtr;
      } );
   }

   void Destroy( Entity entity ) noexcept
   {
      if( !FValid( entity ) )
         return;

      auto& comps = m_entityTypes[ entity ];
      for( size_t compIndex : comps )
      {
         if( compIndex < m_storagePools.size() )
         {
            auto& rec = m_storagePools[ compIndex ];
            if( rec.removeFn )
               rec.removeFn( rec.pStorage, entity );
         }
      }
      comps.clear();

      m_entityAlive[ entity ] = 0;
      m_recycled.push_back( entity );
   }

   [[nodiscard]] FORCE_INLINE bool   FValid( Entity entity ) const noexcept { return entity < m_entityAlive.size() && m_entityAlive[ entity ]; }
   [[nodiscard]] FORCE_INLINE size_t GetEntityCount() const noexcept { return m_nextEntity - m_recycled.size(); }

   template< typename TType, typename... TArgs >
   FORCE_INLINE TType& Add( Entity entity, TArgs&&... args )
   {
      assert( FValid( entity ) && "Add on invalid entity" );
      size_t typeIndex = GetTypeIndex< TType >();
      EnsureStoragePool< TType >( typeIndex );

      auto& rec      = m_storagePools[ typeIndex ];
      auto* pStorage = static_cast< Storage< TType >* >( rec.pStorage );
      auto& sparse   = pStorage->m_entityToIndex;
      if( entity < sparse.size() )
      {
         auto dense = sparse[ entity ];
         if( dense != Storage< TType >::npos )
         {
            pStorage->m_types[ dense ] = TType( std::forward< TArgs >( args )... );
            return pStorage->m_types[ dense ];
         }
      }

      auto& owned = m_entityTypes[ entity ];
      if( std::find( owned.begin(), owned.end(), typeIndex ) == owned.end() )
         owned.push_back( typeIndex );

      return pStorage->EmplaceConstruct( entity, std::forward< TArgs >( args )... );
   }

   template< typename TType, typename... TOthers >
   FORCE_INLINE void Remove( Entity entity ) noexcept
   {
      if( !FValid( entity ) )
         return;

      auto removeOne = [ & ]< typename Type >()
      {
         size_t compIndex = GetTypeIndex< Type >();
         if( compIndex >= m_storagePools.size() )
            return;

         auto& rec = m_storagePools[ compIndex ];
         if( rec.removeFn )
            rec.removeFn( rec.pStorage, entity );

         auto& owned = m_entityTypes[ entity ];
         if( auto it = std::find( owned.begin(), owned.end(), compIndex ); it != owned.end() )
            owned.erase( it );
      };

      removeOne.template operator()< TType >();
      ( removeOne.template operator()< TOthers >(), ... ); // fold over additional types
   }

   template< typename TType, typename... TOthers >
   [[nodiscard]] FORCE_INLINE auto Get( Entity entity ) noexcept
   {
      auto getOne = [ & ]< typename Type >() -> Type*
      {
         if( !FValid( entity ) )
            return nullptr;

         size_t compIndex = GetTypeIndex< Type >();
         if( compIndex >= m_storagePools.size() )
            return nullptr;

         auto& rec = m_storagePools[ compIndex ];
         if( !rec.pStorage )
            return nullptr;

         auto* pStorage = static_cast< Storage< std::remove_const_t< Type > >* >( rec.pStorage );
         return pStorage ? pStorage->Get( entity ) : nullptr;
      };

      if constexpr( sizeof...( TOthers ) == 0 )
         return getOne.template operator()< TType >();
      else
         return std::tuple< TType*, TOthers*... > { getOne.template operator()< TType >(), getOne.template operator()< TOthers >()... };
   }

   template< typename TType, typename... TOthers >
   [[nodiscard]] FORCE_INLINE auto Get( Entity entity ) const noexcept
   {
      return const_cast< Registry* >( this )->Get< const TType, const TOthers... >( entity );
   }

   template< typename TType, typename... TOthers >
   [[nodiscard]] FORCE_INLINE bool FHas( Entity entity ) const noexcept
   {
      if( !FValid( entity ) )
         return false;

      auto hasOne = [ & ]< typename Type >() -> bool
      {
         size_t compIndex = GetTypeIndex< Type >();
         if( compIndex >= m_storagePools.size() )
            return false;

         auto& rec = m_storagePools[ compIndex ];
         if( !rec.pStorage )
            return false;

         auto* pStorage = static_cast< Storage< Type >* >( rec.pStorage );
         return pStorage && entity < pStorage->m_entityToIndex.size() && pStorage->m_entityToIndex[ entity ] != Storage< Type >::npos;
      };

      return hasOne.template operator()< TType >() && ( hasOne.template operator()< TOthers >() && ... );
   }

   template< typename TType, typename... TOthers >
   [[nodiscard]] FORCE_INLINE auto EView() noexcept
   {
      return View< ViewType::Entity, TType, TOthers... >( *this );
   }
   template< typename TType, typename... TOthers >
   [[nodiscard]] FORCE_INLINE auto EView() const noexcept
   {
      return const_cast< Registry* >( this )->EView< const TType, const TOthers... >();
   }
   template< typename TType, typename... TOthers >
   [[nodiscard]] FORCE_INLINE auto CView() noexcept
   {
      return View< ViewType::Components, TType, TOthers... >( *this );
   }
   template< typename TType, typename... TOthers >
   [[nodiscard]] FORCE_INLINE auto CView() const noexcept
   {
      return const_cast< Registry* >( this )->CView< const TType, const TOthers... >();
   }
   template< typename TType, typename... TOthers >
   [[nodiscard]] FORCE_INLINE auto ECView() noexcept
   {
      return View< ViewType::EntityAndComponents, TType, TOthers... >( *this );
   }
   template< typename TType, typename... TOthers >
   [[nodiscard]] FORCE_INLINE auto ECView() const noexcept
   {
      return const_cast< Registry* >( this )->ECView< const TType, const TOthers... >();
   }

private:
   template< typename TType >
   FORCE_INLINE void EnsureStoragePool( size_t index )
   {
      if( index >= m_storagePools.size() )
         m_storagePools.resize( index + 1 );

      auto& rec = m_storagePools[ index ];
      if( !rec.pStorage )
      {
         rec.pStorage  = new Storage< TType >();
         rec.removeFn  = []( void* p, Entity e ) noexcept { static_cast< Storage< TType >* >( p )->Remove( e ); };
         rec.destroyFn = []( void* p ) noexcept { delete static_cast< Storage< TType >* >( p ); };
      }
   }

   Entity                               m_nextEntity { 0 };
   std::vector< Entity >                m_recycled;
   std::vector< uint8_t >               m_entityAlive;
   std::vector< StoragePool >           m_storagePools; // type-erased pools
   std::vector< std::vector< size_t > > m_entityTypes;
};

// =============================================================
// View
// =============================================================
template< ViewType TViewType, typename TType, typename... TOthers >
class View
{
   template< typename T >
   using StoragePtr = typename Registry::Storage< std::remove_const_t< T > >*;

public:
   explicit View( Registry& registry ) noexcept :
      m_registry( registry )
   {
      SelectSmallestPool();
      CacheStorages();
   }

   struct Iterator
   {
      size_t                                                       index { 0 };
      std::vector< Entity >*                                       pEntities { nullptr };
      std::tuple< StoragePtr< TType >, StoragePtr< TOthers >... >& storages;

      template< typename TType >
      FORCE_INLINE static bool FHas( Entity e, StoragePtr< TType > pStorage ) noexcept
      {
         return pStorage && e < pStorage->m_entityToIndex.size() && pStorage->m_entityToIndex[ e ] != Registry::Storage< std::remove_const_t< TType > >::npos;
      }

      FORCE_INLINE bool FHasAll( Entity e ) const noexcept
      {
         return FHas< TType >( e, std::get< StoragePtr< TType > >( storages ) ) &&
                ( FHas< TOthers >( e, std::get< StoragePtr< TOthers > >( storages ) ) && ... );
      }

      FORCE_INLINE void Skip() noexcept
      {
         while( pEntities && index < pEntities->size() && !FHasAll( ( *pEntities )[ index ] ) )
            ++index;
      }

      FORCE_INLINE Iterator& operator++() noexcept
      {
         ++index;
         Skip();
         return *this;
      }

      FORCE_INLINE bool operator!=( const Iterator& rhs ) const noexcept { return index != rhs.index; }

      FORCE_INLINE auto operator*() const noexcept
      {
         Entity entity = ( *pEntities )[ index ];
         if constexpr( TViewType == ViewType::Entity )
            return entity;
         else if constexpr( TViewType == ViewType::Components )
            return std::tuple< TType&, TOthers&... >( GetRef< TType >( entity, std::get< StoragePtr< TType > >( storages ) ),
                                                      GetRef< TOthers >( entity, std::get< StoragePtr< TOthers > >( storages ) )... );
         else if constexpr( TViewType == ViewType::EntityAndComponents )
            return std::tuple< Entity, TType&, TOthers&... >( entity,
                                                              GetRef< TType >( entity, std::get< StoragePtr< TType > >( storages ) ),
                                                              GetRef< TOthers >( entity, std::get< StoragePtr< TOthers > >( storages ) )... );
      }

      template< typename TType >
      FORCE_INLINE static TType& GetRef( Entity e, StoragePtr< TType > pStorage ) noexcept
      {
         return pStorage->m_types[ pStorage->m_entityToIndex[ e ] ];
      }
   };

   FORCE_INLINE Iterator begin() noexcept
   {
      Iterator it { 0, m_pSmallestEntities, m_typeStorages };
      it.Skip();
      return it;
   }
   FORCE_INLINE Iterator end() noexcept
   {
      Iterator it { m_pSmallestEntities ? m_pSmallestEntities->size() : 0, m_pSmallestEntities, m_typeStorages };
      return it;
   }

private:
   template< typename TType >
   FORCE_INLINE std::vector< Entity >* GetEntitiesVectorFor() const noexcept
   {
      size_t index = Registry::GetTypeIndex< std::remove_const_t< TType > >();
      if( index >= m_registry.m_storagePools.size() )
         return nullptr;

      auto& rec = m_registry.m_storagePools[ index ];
      if( !rec.pStorage )
         return nullptr;

      auto* pStorage = static_cast< typename Registry::Storage< std::remove_const_t< TType > >* >( rec.pStorage );
      return &pStorage->m_entities;
   }

   template< typename TType >
   FORCE_INLINE void SelectIfSmaller( size_t& minSize ) noexcept
   {
      if( auto* pVec = GetEntitiesVectorFor< TType >(); pVec && pVec->size() < minSize )
      {
         minSize             = pVec->size();
         m_pSmallestEntities = pVec;
      }
   }

   void SelectSmallestPool() noexcept
   {
      size_t minSize = ( std::numeric_limits< size_t >::max )();
      SelectIfSmaller< TType >( minSize );
      ( SelectIfSmaller< TOthers >( minSize ), ... );
   }

   template< typename TType >
   FORCE_INLINE void CacheStorage() noexcept
   {
      size_t index = Registry::GetTypeIndex< std::remove_const_t< TType > >();
      if( index < m_registry.m_storagePools.size() )
      {
         auto& rec = m_registry.m_storagePools[ index ];
         if( rec.pStorage )
            std::get< StoragePtr< TType > >( m_typeStorages ) = static_cast< StoragePtr< TType > >( rec.pStorage );
      }
   }

   FORCE_INLINE void CacheStorages() noexcept
   {
      CacheStorage< TType >();
      ( CacheStorage< TOthers >(), ... );
   }

   Registry&                                                   m_registry;
   std::vector< Entity >*                                      m_pSmallestEntities { nullptr };
   std::tuple< StoragePtr< TType >, StoragePtr< TOthers >... > m_typeStorages {};
};


} // namespace Entity

// hash specialization
template<>
struct std::hash< Entity::EntityHandle >
{
   FORCE_INLINE size_t operator()( const Entity::EntityHandle& handle ) const noexcept { return std::hash< Entity::Entity > {}( handle.Get() ); }
};
