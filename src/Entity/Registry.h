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
   static inline size_t s_nextComponentTypeIndex = 0;
   template< typename TType >
   struct TypeIndexHelper { FORCE_INLINE static size_t Get() { static const size_t s_index = s_nextComponentTypeIndex++; return s_index; } };
   template< typename TType >
   FORCE_INLINE static size_t GetTypeIndex() noexcept { return TypeIndexHelper< std::remove_pointer_t< std::remove_reference_t< std::remove_cv_t< TType > > > >::Get(); }
   // clang-format on

   struct Pool
   {
      virtual ~Pool()                        = default;
      virtual void Remove( Entity ) noexcept = 0;
   };

   template< typename TType >
   struct Storage final : Pool
   {
      using DenseIndexType                 = uint32_t;
      static constexpr DenseIndexType npos = ( std::numeric_limits< DenseIndexType >::max )();

      std::vector< TType >          m_components;
      std::vector< Entity >         m_entities;
      std::vector< DenseIndexType > m_entityToIndex; // sparse

      template< typename... Args >
      FORCE_INLINE TType& EmplaceConstruct( Entity entity, Args&&... args )
      {
         if( entity >= m_entityToIndex.size() )
            m_entityToIndex.resize( static_cast< size_t >( entity ) + 1, npos );

         DenseIndexType index = static_cast< DenseIndexType >( m_components.size() );
         m_components.emplace_back( std::forward< Args >( args )... );
         m_entities.emplace_back( entity );
         m_entityToIndex[ entity ] = index;
         return m_components.back();
      }

      FORCE_INLINE TType* Get( Entity entity ) noexcept
      {
         if( entity >= m_entityToIndex.size() )
            return nullptr;

         DenseIndexType index = m_entityToIndex[ entity ];
         return index == npos ? nullptr : &m_components[ index ];
      }
      FORCE_INLINE const TType* Get( Entity entity ) const noexcept { return Get( entity ); }

      FORCE_INLINE void Remove( Entity entity ) noexcept override
      {
         if( entity < m_entityToIndex.size() )
         {
            DenseIndexType index = m_entityToIndex[ entity ];
            if( index != npos )
            {
               DenseIndexType backIndex = static_cast< DenseIndexType >( m_components.size() - 1 );
               if( index != backIndex )
               {
                  m_components[ index ]    = std::move( m_components[ backIndex ] );
                  Entity moved             = m_entities[ backIndex ];
                  m_entities[ index ]      = moved;
                  m_entityToIndex[ moved ] = index;
               }

               m_components.pop_back();
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
         m_entityComponentTypes.resize( static_cast< size_t >( entity ) + 1 );
      }
      else
         m_entityComponentTypes[ entity ].clear();

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

      auto& comps = m_entityComponentTypes[ entity ];
      for( size_t compIndex : comps )
      {
         if( compIndex < m_componentPools.size() )
         {
            if( auto* pPool = m_componentPools[ compIndex ].get() )
               pPool->Remove( entity );
         }
      }
      comps.clear();

      m_entityAlive[ entity ] = 0;
      m_recycled.push_back( entity );
   }

   [[nodiscard]] FORCE_INLINE bool   FValid( Entity entity ) const noexcept { return entity < m_entityAlive.size() && m_entityAlive[ entity ]; }
   [[nodiscard]] FORCE_INLINE size_t GetEntityCount() const noexcept { return m_nextEntity - m_recycled.size(); }

   // ----- Component API -----
   template< typename TType, typename... TArgs >
   FORCE_INLINE TType& Add( Entity entity, TArgs&&... args )
   {
      assert( FValid( entity ) && "Add on invalid entity" );
      size_t compIndex = GetTypeIndex< TType >();
      EnsureComponentPool< TType >( compIndex );

      auto* pStorage = static_cast< Storage< TType >* >( m_componentPools[ compIndex ].get() );
      auto& sparse   = pStorage->m_entityToIndex;
      if( entity < sparse.size() )
      {
         auto dense = sparse[ entity ];
         if( dense != Storage< TType >::npos )
         {
            pStorage->m_components[ dense ] = TType( std::forward< TArgs >( args )... );
            return pStorage->m_components[ dense ];
         }
      }

      auto& owned = m_entityComponentTypes[ entity ];
      if( std::find( owned.begin(), owned.end(), compIndex ) == owned.end() )
         owned.push_back( compIndex );

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
         if( compIndex >= m_componentPools.size() )
            return;

         if( auto* pPool = m_componentPools[ compIndex ].get() )
            pPool->Remove( entity );

         auto& owned = m_entityComponentTypes[ entity ];
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
         if( compIndex >= m_componentPools.size() )
            return nullptr;

         auto* pStorage = static_cast< Storage< std::remove_const_t< Type > >* >( m_componentPools[ compIndex ].get() );
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

      auto hasOne = []< typename Type >()
      {
         size_t compIndex = GetTypeIndex< Type >();
         if( compIndex >= m_componentPools.size() )
            return false;

         auto* pPool = m_componentPools[ compIndex ].get();
         if( !pPool )
            return false;

         auto* pStorage = static_cast< Storage< Type >* >( pPool );
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
   FORCE_INLINE void EnsureComponentPool( size_t index )
   {
      if( index >= m_componentPools.size() )
         m_componentPools.resize( index + 1 );

      if( !m_componentPools[ index ] )
         m_componentPools[ index ] = std::make_unique< Storage< TType > >();
   }

   Entity                                 m_nextEntity { 0 };
   std::vector< Entity >                  m_recycled;
   std::vector< uint8_t >                 m_entityAlive;
   std::vector< std::unique_ptr< Pool > > m_componentPools;
   std::vector< std::vector< size_t > >   m_entityComponentTypes;
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
         return pStorage->m_components[ pStorage->m_entityToIndex[ e ] ];
      }
   };

   FORCE_INLINE Iterator begin() noexcept
   {
      Iterator it { 0, m_pSmallestEntities, m_componentStorages };
      it.Skip();
      return it;
   }
   FORCE_INLINE Iterator end() noexcept
   {
      Iterator it { m_pSmallestEntities ? m_pSmallestEntities->size() : 0, m_pSmallestEntities, m_componentStorages };
      return it;
   }

private:
   template< typename TType >
   FORCE_INLINE std::vector< Entity >* GetEntitiesVectorFor() const noexcept
   {
      size_t index = Registry::GetTypeIndex< std::remove_const_t< TType > >();
      if( index >= m_registry.m_componentPools.size() )
         return nullptr;

      auto* pPool = m_registry.m_componentPools[ index ].get();
      return pPool ? &( static_cast< Registry::Storage< std::remove_const_t< TType > >* >( pPool )->m_entities ) : nullptr;
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
      if( index < m_registry.m_componentPools.size() )
      {
         if( auto* pPool = m_registry.m_componentPools[ index ].get() )
            std::get< StoragePtr< TType > >( m_componentStorages ) = static_cast< StoragePtr< TType > >( pPool );
      }
   }

   FORCE_INLINE void CacheStorages() noexcept
   {
      CacheStorage< TType >();
      ( CacheStorage< TOthers >(), ... );
   }

   Registry&                                                   m_registry;
   std::vector< Entity >*                                      m_pSmallestEntities { nullptr };
   std::tuple< StoragePtr< TType >, StoragePtr< TOthers >... > m_componentStorages {};
};


} // namespace Entity

// hash specialization
template<>
struct std::hash< Entity::EntityHandle >
{
   FORCE_INLINE size_t operator()( const Entity::EntityHandle& handle ) const noexcept { return std::hash< Entity::Entity > {}( handle.Get() ); }
};
