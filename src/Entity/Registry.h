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

   FORCE_INLINE      operator Entity() const noexcept { return m_entity; }
   FORCE_INLINE bool operator==( const EntityHandle& other ) const noexcept { return m_entity == other.m_entity; }
   FORCE_INLINE bool operator!=( const EntityHandle& other ) const noexcept { return m_entity != other.m_entity; }
   FORCE_INLINE bool operator==( Entity other ) const noexcept { return m_entity == other; }
   FORCE_INLINE bool operator!=( Entity other ) const noexcept { return m_entity != other; }
   FORCE_INLINE bool operator<( const EntityHandle& other ) const noexcept { return m_entity < other.m_entity; }

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
   struct TypeIndexHelper { FORCE_INLINE static size_t Get() noexcept { static const size_t s_index = s_nextTypeIndex++; return s_index; } };
   template< typename TType >
   FORCE_INLINE static size_t GetTypeIndex() noexcept { return TypeIndexHelper< std::remove_cv_t< std::remove_pointer_t< std::remove_reference_t< TType > > > >::Get(); }
   // clang-format on

   // Storage pool for a specifc type
   struct StoragePool
   {
      std::unique_ptr< void, void ( * )( void* ) > pStorage { nullptr, nullptr }; // owns Storage<T>
      void ( *removeFn )( void*, Entity ) noexcept { nullptr };                   // removes entity from storage
   };

   template< typename TType >
   struct Storage
   {
      using DenseIndexType                 = uint32_t;
      static constexpr DenseIndexType npos = ( std::numeric_limits< DenseIndexType >::max )();

      std::vector< TType >          m_types;
      std::vector< Entity >         m_entities;      // dense entity list (parallel to m_types)
      std::vector< DenseIndexType > m_entityToIndex; // sparse mapping entity -> dense index

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
         return index != npos ? &m_types[ index ] : nullptr;
      }

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
   Registry()  = default;
   ~Registry() = default;

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
   }

   [[nodiscard]] FORCE_INLINE bool   FValid( Entity entity ) const noexcept { return entity < m_entityAlive.size() && m_entityAlive[ entity ]; }
   [[nodiscard]] FORCE_INLINE size_t GetEntityCount() const noexcept { return m_nextEntity - m_recycled.size(); }
   [[nodiscard]] FORCE_INLINE size_t GetComponentCount( Entity entity ) const noexcept { return FValid( entity ) ? m_entityTypes[ entity ].size() : 0; }

   template< typename TType, typename... TArgs >
   FORCE_INLINE TType& Add( Entity entity, TArgs&&... args )
   {
      assert( FValid( entity ) && "Add on invalid entity" );
      const size_t typeIndex = GetTypeIndex< TType >();
      EnsureStoragePool< TType >( typeIndex );

      auto& rec      = m_storagePools[ typeIndex ];
      auto* pStorage = static_cast< Storage< TType >* >( rec.pStorage.get() );
      auto& sparse   = pStorage->m_entityToIndex;
      if( entity < sparse.size() )
      {
         if( auto dense = sparse[ entity ]; dense != Storage< TType >::npos )
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

      auto removeOne = [ & ]< typename T >() -> void
      {
         const size_t compIndex = GetTypeIndex< T >();
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
      bool fValid    = FValid( entity );
      auto getOnePtr = [ & ]< typename T >() -> T*
      {
         if( !fValid )
            return nullptr;

         const size_t typeIndex = GetTypeIndex< T >();
         if( typeIndex >= m_storagePools.size() )
            return nullptr;

         auto& rec = m_storagePools[ typeIndex ];
         if( !rec.pStorage )
            return nullptr;

         auto* pStorage = static_cast< Storage< std::remove_cv_t< std::remove_pointer_t< std::remove_reference_t< T > > > >* >( rec.pStorage.get() );
         return pStorage ? pStorage->Get( entity ) : nullptr;
      };

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
         using CType = std::remove_cv_t< std::remove_pointer_t< std::remove_reference_t< T > > >;
         return *static_cast< Storage< CType >* >( m_storagePools[ GetTypeIndex< T >() ].pStorage.get() )->Get( entity );
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

      auto hasOne = [ & ]< typename T >() -> bool
      {
         size_t compIndex = GetTypeIndex< T >();
         if( compIndex >= m_storagePools.size() )
            return false;

         auto& rec = m_storagePools[ compIndex ];
         if( !rec.pStorage )
            return false;

         auto* pStorage = static_cast< Storage< T >* >( rec.pStorage.get() );
         return pStorage && entity < pStorage->m_entityToIndex.size() && pStorage->m_entityToIndex[ entity ] != Storage< T >::npos;
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
   template< typename TType >
   FORCE_INLINE void EnsureStoragePool( size_t index )
   {
      if( index >= m_storagePools.size() )
         m_storagePools.resize( index + 1 );

      auto& rec = m_storagePools[ index ];
      if( !rec.pStorage )
      {
         rec.pStorage = { new Storage< TType >(), []( void* pStorage ) { delete static_cast< Storage< TType >* >( pStorage ); } };
         rec.removeFn = []( void* pStorage, Entity e ) noexcept { static_cast< Storage< TType >* >( pStorage )->Remove( e ); };
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
   using StoragePtr = typename Registry::Storage< std::remove_cv_t< std::remove_pointer_t< std::remove_reference_t< T > > > >*;

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
      std::vector< Entity >*                                       pEntities { nullptr }; // dense entity list of smallest pool
      std::tuple< StoragePtr< TType >, StoragePtr< TOthers >... >& storages;

      FORCE_INLINE void Skip() noexcept
      {
         if constexpr( sizeof...( TOthers ) == 0 )
            return; // No skipping needed: single type iteration

         auto has = []< typename T >( Entity e, auto pStorage ) -> bool
         { return pStorage && e < pStorage->m_entityToIndex.size() && pStorage->m_entityToIndex[ e ] != Registry::Storage< T >::npos; };

         while( pEntities && index < pEntities->size() )
         {
            Entity e = ( *pEntities )[ index ];
            if( ( has.template operator()< TType >( e, std::get< StoragePtr< TType > >( storages ) ) && ... &&
                  has.template operator()< TOthers >( e, std::get< StoragePtr< TOthers > >( storages ) ) ) )
               break; // entity has all required components

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
         Entity entity = ( *pEntities )[ index ];
         if constexpr( sizeof...( TOthers ) == 0 ) // fast path for single type iteration
         {
            if constexpr( TViewType == ViewType::Entity )
               return entity;
            else if constexpr( TViewType == ViewType::Components )
               return std::tuple< TType& >( std::get< StoragePtr< TType > >( storages )->m_types[ index ] );
            else if constexpr( TViewType == ViewType::EntityAndComponents )
               return std::tuple< Entity, TType& >( entity, std::get< StoragePtr< TType > >( storages )->m_types[ index ] );
         }

         // General path for multiple types
         auto getRef = [ & ]( auto* pStorage ) -> decltype( auto ) { return pStorage->m_types[ pStorage->m_entityToIndex[ entity ] ]; };
         if constexpr( TViewType == ViewType::Entity )
            return entity;
         else if constexpr( TViewType == ViewType::Components )
            return std::tuple< TType&, TOthers&... >( getRef( std::get< StoragePtr< TType > >( storages ) ),
                                                      getRef( std::get< StoragePtr< TOthers > >( storages ) )... );
         else if constexpr( TViewType == ViewType::EntityAndComponents )
            return std::tuple< Entity, TType&, TOthers&... >( entity,
                                                              getRef( std::get< StoragePtr< TType > >( storages ) ),
                                                              getRef( std::get< StoragePtr< TOthers > >( storages ) )... );
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

         const size_t    index = Registry::GetTypeIndex< T >();
         StoragePtr< T > pPool = index < m_registry.m_storagePools.size() ? static_cast< StoragePtr< T > >( m_registry.m_storagePools[ index ].pStorage.get() )
                                                                          : nullptr;
         if( !pPool || pPool->m_entities.empty() )
         {
            minSize             = 0;
            m_pSmallestEntities = nullptr; // empty -> no iteration
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
   std::vector< Entity >*                                      m_pSmallestEntities { nullptr }; // points to entity list of smallest pool
   std::tuple< StoragePtr< TType >, StoragePtr< TOthers >... > m_typeStorages {};               // cached storage pointers
};


} // namespace Entity

// hash specialization
template<>
struct std::hash< Entity::EntityHandle >
{
   FORCE_INLINE size_t operator()( const Entity::EntityHandle& handle ) const noexcept { return std::hash< Entity::Entity > {}( handle.Get() ); }
};
