#pragma once

namespace Entity
{

#if defined( _MSC_VER )
#define FORCE_INLINE __forceinline
#else
#define FORCE_INLINE inline __attribute__( ( always_inline ) )
#endif

using Entity                = uint64_t;
constexpr Entity NullEntity = ( std::numeric_limits< Entity >::max )();


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
   [[nodiscard]] FORCE_INLINE Entity Get() const noexcept { return m_entity; }

   FORCE_INLINE      operator Entity() const noexcept { return m_entity; }
   FORCE_INLINE bool operator==( const EntityHandle& other ) const noexcept { return m_entity == other.m_entity; }
   FORCE_INLINE bool operator!=( const EntityHandle& other ) const noexcept { return m_entity != other.m_entity; }
   FORCE_INLINE bool operator==( Entity other ) const noexcept { return m_entity == other; }
   FORCE_INLINE bool operator!=( Entity other ) const noexcept { return m_entity != other; }
   FORCE_INLINE bool operator<( const EntityHandle& other ) const noexcept { return m_entity < other.m_entity; }

private:
   NO_COPY_MOVE( EntityHandle )

   const Entity m_entity { NullEntity };
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

   // Normalize any T to its underlying component type
   template< typename TType >
   using ComponentT = std::remove_cv_t< std::remove_pointer_t< std::remove_reference_t< TType > > >;

   // clang-format off
   static inline size_t s_nextTypeIndex = 0;
   template< typename TType >
   struct TypeIndexHelper { FORCE_INLINE static size_t Get() noexcept { static const size_t s_index = s_nextTypeIndex++; return s_index; } };
   template< typename TType >
   FORCE_INLINE static size_t GetTypeIndex() noexcept { return TypeIndexHelper< ComponentT< TType > >::Get(); }
   // clang-format on

   struct StoragePool
   {
      std::unique_ptr< void, void ( * )( void* ) > pStorage { nullptr, nullptr };
      void ( *removeFn )( void*, Entity ) noexcept { nullptr };
      void* ( *getPtrFn )( void*, uint32_t ) noexcept { nullptr };
   };

   template< typename TType >
   struct Storage
   {
      using DenseIndexType                      = uint32_t;
      static constexpr DenseIndexType npos      = ( std::numeric_limits< DenseIndexType >::max )();
      static constexpr DenseIndexType kPageSize = 1024;

      std::vector< std::unique_ptr< TType[] > > m_pages;
      std::vector< Entity >                     m_entities;
      std::vector< DenseIndexType >             m_entityToIndex;

      FORCE_INLINE TType* DensePtr( DenseIndexType index ) noexcept
      {
         const DenseIndexType page   = index / kPageSize;
         const DenseIndexType offset = index % kPageSize;
         return m_pages[ page ].get() + offset;
      }

      FORCE_INLINE const TType* DensePtr( DenseIndexType index ) const noexcept
      {
         const DenseIndexType page   = index / kPageSize;
         const DenseIndexType offset = index % kPageSize;
         return m_pages[ page ].get() + offset;
      }

      template< typename... Args >
      FORCE_INLINE TType& EmplaceConstruct( Entity entity, Args&&... args )
      {
         if( entity >= m_entityToIndex.size() )
            m_entityToIndex.resize( static_cast< size_t >( entity ) + 1, npos );

         const DenseIndexType index = static_cast< DenseIndexType >( m_entities.size() );
         while( m_pages.size() < ( index / kPageSize ) + 1 )
            m_pages.emplace_back( std::make_unique< TType[] >( kPageSize ) );

         TType* p = DensePtr( index );
         *p       = TType( std::forward< Args >( args )... );

         m_entities.emplace_back( entity );
         m_entityToIndex[ entity ] = index;
         return *p;
      }

      FORCE_INLINE TType* Get( Entity entity ) noexcept
      {
         if( entity >= m_entityToIndex.size() )
            return nullptr;

         const DenseIndexType index = m_entityToIndex[ entity ];
         return index != npos ? DensePtr( index ) : nullptr;
      }

      FORCE_INLINE const TType* Get( Entity entity ) const noexcept { return const_cast< Registry* >( this )->Get( entity ); }

      FORCE_INLINE void Remove( Entity entity ) noexcept
      {
         if( entity >= m_entityToIndex.size() )
            return;

         const DenseIndexType index = m_entityToIndex[ entity ];
         if( index == npos )
            return;

         const DenseIndexType backIndex = static_cast< DenseIndexType >( m_entities.size() - 1 );
         if( index != backIndex )
         {
            *DensePtr( index ) = std::move( *DensePtr( backIndex ) );

            const Entity moved       = m_entities[ backIndex ];
            m_entities[ index ]      = moved;
            m_entityToIndex[ moved ] = index;
         }

         m_entities.pop_back();
         m_entityToIndex[ entity ] = npos;
      }
   };

   template< typename TType >
   [[nodiscard]] FORCE_INLINE ComponentT< TType >* TryGetPtr( Entity entity ) noexcept
   {
      using CType = ComponentT< TType >;
      if( !FValid( entity ) )
         return nullptr;

      const size_t typeIndex = GetTypeIndex< CType >();
      if( typeIndex >= m_storagePools.size() )
         return nullptr;

      auto& rec = m_storagePools[ typeIndex ];
      if( !rec.pStorage )
         return nullptr;

      auto* pStorage = static_cast< Storage< CType >* >( rec.pStorage.get() );
      return pStorage ? pStorage->Get( entity ) : nullptr;
   }

   template< typename TType >
   [[nodiscard]] FORCE_INLINE const ComponentT< TType >* TryGetPtr( Entity entity ) const noexcept
   {
      return const_cast< Registry* >( this )->TryGetPtr< const TType >( entity );
   }

   template< typename TType >
   [[nodiscard]] FORCE_INLINE ComponentT< TType >* GetPtr( Entity entity ) noexcept
   {
      using CType = ComponentT< TType >;
      assert( FValid( entity ) && "GetPtr on invalid entity" );
      const size_t typeIndex = GetTypeIndex< CType >();
      auto&        rec       = m_storagePools[ typeIndex ];
      auto*        pStorage  = static_cast< Storage< CType >* >( rec.pStorage.get() );
      return pStorage->Get( entity );
   }

public:
   Registry()  = default;
   ~Registry() = default;

   // ----- Entity lifecycle -----
   [[nodiscard]] FORCE_INLINE Entity Create() noexcept
   {
      Entity entity { NullEntity };
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
      ++m_aliveCount;
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

   FORCE_INLINE void Destroy( Entity entity ) noexcept
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
               rec.removeFn( rec.pStorage.get(), entity );
         }
      }
      comps.clear();

      m_entityAlive[ entity ] = 0;
      m_recycled.push_back( entity );
      --m_aliveCount;
   }

   [[nodiscard]] FORCE_INLINE bool   FValid( Entity entity ) const noexcept { return entity < m_entityAlive.size() && m_entityAlive[ entity ]; }
   [[nodiscard]] FORCE_INLINE size_t GetEntityCount() const noexcept { return m_aliveCount; }
   [[nodiscard]] FORCE_INLINE size_t GetComponentCount( Entity entity ) const noexcept { return FValid( entity ) ? m_entityTypes[ entity ].size() : 0; }

   template< typename TType, typename... TArgs >
   FORCE_INLINE ComponentT< TType >& EmplaceOrGet( Entity entity, TArgs&&... args )
   {
      if( auto* p = TryGetPtr< TType >( entity ) )
         return *p;

      return Add< ComponentT< TType > >( entity, std::forward< TArgs >( args )... );
   }

   template< typename TType, typename... TArgs >
   FORCE_INLINE ComponentT< TType >& Add( Entity entity, TArgs&&... args )
   {
      using CType = ComponentT< TType >;
      assert( FValid( entity ) && "Add on invalid entity" );

      const size_t typeIndex = GetTypeIndex< CType >();
      EnsureStoragePool< CType >( typeIndex );

      auto& rec      = m_storagePools[ typeIndex ];
      auto* pStorage = static_cast< Storage< CType >* >( rec.pStorage.get() );
      auto& sparse   = pStorage->m_entityToIndex;
      if( entity < sparse.size() )
      {
         if( auto dense = sparse[ entity ]; dense != Storage< CType >::npos )
         {
            *pStorage->DensePtr( dense ) = CType( std::forward< TArgs >( args )... );
            return *pStorage->DensePtr( dense );
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

      auto removeOne = [ & ]< typename T >() noexcept -> void
      {
         using CType            = ComponentT< T >;
         const size_t compIndex = GetTypeIndex< CType >();
         if( compIndex < m_storagePools.size() )
         {
            auto& rec = m_storagePools[ compIndex ];
            if( rec.removeFn )
               rec.removeFn( rec.pStorage.get(), entity );

            auto& owned = m_entityTypes[ entity ];
            if( auto it = std::find( owned.begin(), owned.end(), compIndex ); it != owned.end() )
               owned.erase( it );
         }
      };
      removeOne.template operator()< TType >();
      ( removeOne.template operator()< TOthers >(), ... );
   }

   template< typename TType, typename... TOthers >
   [[nodiscard]] FORCE_INLINE auto TryGet( Entity entity ) noexcept
   {
      auto getOnePtr = [ & ]< typename T >() -> T* { return TryGetPtr< T >( entity ); };

      if constexpr( sizeof...( TOthers ) == 0 )
         return getOnePtr.template operator()< TType >();
      else
         return std::tuple< TType*, TOthers*... > { getOnePtr.template operator()< TType >(), getOnePtr.template operator()< TOthers >()... };
   }

   template< typename TType, typename... TOthers >
   [[nodiscard]] FORCE_INLINE auto TryGet( Entity entity ) const noexcept
   {
      return const_cast< Registry* >( this )->TryGet< const TType, const TOthers... >( entity );
   }

   template< typename TType, typename... TOthers >
   [[nodiscard]] FORCE_INLINE decltype( auto ) Get( Entity entity )
   {
      assert( FValid( entity ) && "Get on invalid entity" );
      auto getOneRef = [ & ]< typename T >() -> T&
      {
         auto* p = GetPtr< T >( entity );
         assert( p && "Get on missing component" );
         return *p;
      };

      if constexpr( sizeof...( TOthers ) == 0 )
         return getOneRef.template operator()< TType >();
      else
         return std::tuple< TType&, TOthers&... > { getOneRef.template operator()< TType >(), getOneRef.template operator()< TOthers >()... };
   }

   template< typename TType, typename... TOthers >
   [[nodiscard]] FORCE_INLINE decltype( auto ) Get( Entity entity ) const
   {
      return const_cast< Registry* >( this )->Get< const TType, const TOthers... >( entity );
   }

   template< typename TType, typename... TOthers >
   [[nodiscard]] FORCE_INLINE bool FHas( Entity entity ) const noexcept
   {
      if( !FValid( entity ) )
         return false;

      auto hasOne = [ & ]< typename T >() noexcept -> bool
      {
         using CType            = ComponentT< T >;
         const size_t compIndex = GetTypeIndex< CType >();
         if( compIndex >= m_storagePools.size() )
            return false;

         auto& rec = m_storagePools[ compIndex ];
         if( !rec.pStorage )
            return false;

         auto* pStorage = static_cast< const Storage< CType >* >( rec.pStorage.get() );
         return pStorage && entity < pStorage->m_entityToIndex.size() && pStorage->m_entityToIndex[ entity ] != Storage< CType >::npos;
      };

      return hasOne.template operator()< TType >() && ( hasOne.template operator()< TOthers >() && ... );
   }

   // clang-format off
   /**
    * @brief Creates a view for iterating over entities with the specified component types.
    * @tparam TType The first component type to include in the view.
    * @tparam TOthers Additional component types to include in the view.
    * @return A view object that allows iteration over entities with the specified components.
    */
   template<typename TType, typename... TOthers> [[nodiscard]] FORCE_INLINE auto EView() noexcept { return View< ViewType::Entity, TType, TOthers... >( *this ); }
   template<typename TType, typename... TOthers> [[nodiscard]] FORCE_INLINE auto EView() const noexcept { return const_cast< Registry* >( this )->EView< const TType, const TOthers... >(); }

   /**
    * @brief Creates a view for iterating over entities with the specified component types, returning only the components.
    * @tparam TType The first component type to include in the view.
    * @tparam TOthers Additional component types to include in the view.
    * @return A view object that allows iteration over components of entities with the specified components.
    */
   template<typename TType, typename... TOthers> [[nodiscard]] FORCE_INLINE auto CView() noexcept { return View< ViewType::Components, TType, TOthers... >( *this ); }
   template<typename TType, typename... TOthers> [[nodiscard]] FORCE_INLINE auto CView() const noexcept { return const_cast< Registry* >( this )->CView< const TType, const TOthers... >(); }

   /**
    * @brief Creates a view for iterating over entities with the specified component types, returning both the entity and components.
    * @tparam TType The first component type to include in the view.
    * @tparam TOthers Additional component types to include in the view.
    * @return A view object that allows iteration over entities and their components with the specified types.
    */
   template<typename TType, typename... TOthers> [[nodiscard]] FORCE_INLINE auto ECView() noexcept { return View< ViewType::EntityAndComponents, TType, TOthers... >( *this ); }
   template<typename TType, typename... TOthers> [[nodiscard]] FORCE_INLINE auto ECView() const noexcept { return const_cast< Registry* >( this )->ECView< const TType, const TOthers... >(); }
   // clang-format on

private:
   NO_COPY_MOVE( Registry )

   template< typename TType >
   FORCE_INLINE void EnsureStoragePool( size_t index )
   {
      using CType = ComponentT< TType >;

      if( index >= m_storagePools.size() )
         m_storagePools.resize( index + 1 );

      auto& rec = m_storagePools[ index ];
      if( !rec.pStorage )
      {
         rec.pStorage = { new Storage< CType >(), []( void* pStorage ) { delete static_cast< Storage< CType >* >( pStorage ); } };
         rec.removeFn = []( void* pStorage, Entity e ) noexcept { static_cast< Storage< CType >* >( pStorage )->Remove( e ); };
         rec.getPtrFn = []( void* pStorage, uint32_t denseIndex ) noexcept -> void*
         {
            using S = Storage< CType >;
            return static_cast< void* >( static_cast< S* >( pStorage )->DensePtr( denseIndex ) );
         };
      }
   }

   Entity                               m_nextEntity { 0 };
   size_t                               m_aliveCount { 0 };
   std::vector< Entity >                m_recycled;
   std::vector< uint8_t >               m_entityAlive;
   std::vector< StoragePool >           m_storagePools;
   std::vector< std::vector< size_t > > m_entityTypes;
};

// =============================================================
// View
// =============================================================
template< ViewType TViewType, typename TType, typename... TOthers >
class View
{
   template< typename T >
   using CType = Registry::ComponentT< T >;

   template< typename T >
   using StoragePtr = typename Registry::Storage< CType< T > >*;

public:
   explicit View( Registry& registry ) noexcept :
      m_registry( registry )
   {
      SelectSmallestPool();
      if( m_pSmallestEntities )
         CacheStorages();
   }

   struct Iterator
   {
      size_t                                                       index { 0 };
      std::vector< Entity >*                                       pEntities { nullptr };
      std::tuple< StoragePtr< TType >, StoragePtr< TOthers >... >& storages;

      FORCE_INLINE void Skip() noexcept
      {
         if constexpr( sizeof...( TOthers ) == 0 )
            return; // single component: smallest pool already guarantees membership

         auto has = []< typename T >( Entity e, StoragePtr< T > pStorage ) noexcept -> bool
         { return pStorage && e < pStorage->m_entityToIndex.size() && pStorage->m_entityToIndex[ e ] != Registry::Storage< CType< T > >::npos; };

         while( pEntities && index < pEntities->size() )
         {
            const Entity e = ( *pEntities )[ index ];
            if( ( has.template operator()< TType >( e, std::get< StoragePtr< TType > >( storages ) ) && ... &&
                  has.template operator()< TOthers >( e, std::get< StoragePtr< TOthers > >( storages ) ) ) )
               break;

            ++index;
         }
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
         const Entity entity = ( *pEntities )[ index ];
         auto         getRef = [ & ]( auto* pStorage ) -> decltype( auto )
         {
            using S                    = std::remove_pointer_t< decltype( pStorage ) >;
            using DenseIndexType       = typename S::DenseIndexType;
            const DenseIndexType dense = pStorage->m_entityToIndex[ entity ];
            return *pStorage->DensePtr( dense );
         };

         if constexpr( TViewType == ViewType::Entity )
            return entity;
         else if constexpr( TViewType == ViewType::Components )
         {
            if constexpr( sizeof...( TOthers ) == 0 )
               return std::tuple< TType& >( getRef( std::get< StoragePtr< TType > >( storages ) ) );
            else
               return std::tuple< TType&, TOthers&... >( getRef( std::get< StoragePtr< TType > >( storages ) ),
                                                         getRef( std::get< StoragePtr< TOthers > >( storages ) )... );
         }
         else if constexpr( TViewType == ViewType::EntityAndComponents )
         {
            if constexpr( sizeof...( TOthers ) == 0 )
               return std::tuple< Entity, TType& >( entity, getRef( std::get< StoragePtr< TType > >( storages ) ) );
            else
               return std::tuple< Entity, TType&, TOthers&... >( entity,
                                                                 getRef( std::get< StoragePtr< TType > >( storages ) ),
                                                                 getRef( std::get< StoragePtr< TOthers > >( storages ) )... );
         }
      }
   };

   FORCE_INLINE Iterator begin() noexcept
   {
      Iterator it { 0, m_pSmallestEntities, m_typeStorages };
      it.Skip();
      return it;
   }
   FORCE_INLINE Iterator end() noexcept { return Iterator { m_pSmallestEntities ? m_pSmallestEntities->size() : 0, m_pSmallestEntities, m_typeStorages }; }

private:
   FORCE_INLINE void SelectSmallestPool() noexcept
   {
      size_t minSize         = ( std::numeric_limits< size_t >::max )();
      auto   selectIfSmaller = [ & ]< typename T >() noexcept -> void
      {
         if( minSize == 0 )
            return;

         const size_t index = Registry::GetTypeIndex< T >();
         auto* pPool = index < m_registry.m_storagePools.size() ? static_cast< StoragePtr< T > >( m_registry.m_storagePools[ index ].pStorage.get() ) : nullptr;
         if( !pPool || pPool->m_entities.empty() )
         {
            minSize             = 0;
            m_pSmallestEntities = nullptr;
         }
         else if( pPool->m_entities.size() < minSize )
         {
            minSize             = pPool->m_entities.size();
            m_pSmallestEntities = &pPool->m_entities;
         }
      };
      selectIfSmaller.template operator()< TType >();
      ( selectIfSmaller.template operator()< TOthers >(), ... );
   }

   FORCE_INLINE void CacheStorages() noexcept
   {
      auto cacheStorage = [ & ]< typename T >() noexcept
      {
         const size_t index = Registry::GetTypeIndex< T >();
         if( index < m_registry.m_storagePools.size() )
         {
            auto& rec = m_registry.m_storagePools[ index ];
            if( rec.pStorage )
               std::get< StoragePtr< T > >( m_typeStorages ) = static_cast< StoragePtr< T > >( rec.pStorage.get() );
         }
      };
      cacheStorage.template operator()< TType >();
      ( cacheStorage.template operator()< TOthers >(), ... );
   }

   Registry&                                                   m_registry;
   std::vector< Entity >*                                      m_pSmallestEntities { nullptr };
   std::tuple< StoragePtr< TType >, StoragePtr< TOthers >... > m_typeStorages {};
};


} // namespace Entity

template<>
struct std::hash< Entity::EntityHandle >
{
   FORCE_INLINE size_t operator()( const Entity::EntityHandle& handle ) const noexcept { return std::hash< Entity::Entity > {}( handle.Get() ); }
};
