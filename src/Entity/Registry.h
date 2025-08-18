#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <limits>
#include <cassert>
#include <tuple>
#include <utility>
#include <algorithm>

namespace Entity
{

#if defined( _MSC_VER )
#define ECS_FORCE_INLINE __forceinline
#else
#define ECS_FORCE_INLINE inline __attribute__( ( always_inline ) )
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

   [[nodiscard]] Entity Get() const noexcept { return m_entity; }
   explicit             operator Entity() const noexcept { return m_entity; }

   bool operator==( const EntityHandle& other ) const noexcept { return m_entity == other.m_entity; }
   bool operator!=( const EntityHandle& other ) const noexcept { return m_entity != other.m_entity; }
   bool operator==( Entity other ) const noexcept { return m_entity == other; }
   bool operator!=( Entity other ) const noexcept { return m_entity != other; }
   bool operator<( const EntityHandle& other ) const noexcept { return m_entity < other.m_entity; }

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

   static inline size_t s_nextComponentTypeIndex = 0;
   template< typename TType >
   ECS_FORCE_INLINE static size_t GetTypeIndex() noexcept
   {
      static const size_t s_index = s_nextComponentTypeIndex++;
      return s_index;
   }

   struct Pool
   {
      virtual ~Pool()                        = default;
      virtual void Remove( Entity ) noexcept = 0;
      virtual bool FEmpty() const noexcept   = 0;
   };

   template< typename TType >
   struct Storage final : Pool
   {
      using DenseIndexType                 = uint32_t;
      static constexpr DenseIndexType npos = ( std::numeric_limits< DenseIndexType >::max )();

      std::vector< TType >          m_components;
      std::vector< Entity >         m_entities;
      std::vector< DenseIndexType > m_entityToIndex; // sparse

      ECS_FORCE_INLINE void EnsureEntityCapacity( Entity entity )
      {
         if( entity >= m_entityToIndex.size() )
            m_entityToIndex.resize( static_cast< size_t >( entity ) + 1, npos );
      }

      ECS_FORCE_INLINE TType& Emplace( Entity entity, TType&& value )
      {
         EnsureEntityCapacity( entity );
         DenseIndexType index = static_cast< DenseIndexType >( m_components.size() );
         m_components.emplace_back( std::move( value ) );
         m_entities.emplace_back( entity );
         m_entityToIndex[ entity ] = index;
         return m_components.back();
      }

      template< typename... Args >
      ECS_FORCE_INLINE TType& EmplaceConstruct( Entity entity, Args&&... args )
      {
         EnsureEntityCapacity( entity );
         DenseIndexType index = static_cast< DenseIndexType >( m_components.size() );
         m_components.emplace_back( std::forward< Args >( args )... );
         m_entities.emplace_back( entity );
         m_entityToIndex[ entity ] = index;
         return m_components.back();
      }

      ECS_FORCE_INLINE TType* Get( Entity entity ) noexcept
      {
         if( entity >= m_entityToIndex.size() )
            return nullptr;

         DenseIndexType idx = m_entityToIndex[ entity ];
         return idx == npos ? nullptr : &m_components[ idx ];
      }
      ECS_FORCE_INLINE const TType* Get( Entity entity ) const noexcept { return Get( entity ); }

      ECS_FORCE_INLINE void Remove( Entity entity ) noexcept override
      {
         if( entity >= m_entityToIndex.size() )
            return;

         DenseIndexType index = m_entityToIndex[ entity ];
         if( index == npos )
            return;

         DenseIndexType backIdx = static_cast< DenseIndexType >( m_components.size() - 1 );
         if( index != backIdx )
         {
            m_components[ index ]    = std::move( m_components[ backIdx ] );
            Entity moved             = m_entities[ backIdx ];
            m_entities[ index ]      = moved;
            m_entityToIndex[ moved ] = index;
         }

         m_components.pop_back();
         m_entities.pop_back();
         m_entityToIndex[ entity ] = npos;
      }

      ECS_FORCE_INLINE bool FEmpty() const noexcept override { return m_components.empty(); }
   };

public:
   Registry()  = default;
   ~Registry() = default;

   Registry( const Registry& )            = delete;
   Registry& operator=( const Registry& ) = delete;

   // ----- Entity lifecycle -----
   [[nodiscard]] ECS_FORCE_INLINE Entity Create() noexcept
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

   [[nodiscard]] std::shared_ptr< EntityHandle > CreateWithHandle() noexcept
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

   [[nodiscard]] ECS_FORCE_INLINE bool   FValid( Entity entity ) const noexcept { return entity < m_entityAlive.size() && m_entityAlive[ entity ]; }
   [[nodiscard]] ECS_FORCE_INLINE size_t GetEntityCount() const noexcept { return m_nextEntity - m_recycled.size() - 1; }

   // ----- Component API -----
   template< typename TType, typename... TArgs >
   ECS_FORCE_INLINE TType& AddComponent( Entity entity, TArgs&&... args )
   {
      assert( FValid( entity ) && "AddComponent on invalid entity" );
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

   template< typename TType >
   ECS_FORCE_INLINE void RemoveComponent( Entity entity ) noexcept
   {
      if( !FValid( entity ) )
         return;

      size_t compIndex = GetTypeIndex< TType >();
      if( compIndex >= m_componentPools.size() )
         return;

      if( auto* pPool = m_componentPools[ compIndex ].get() )
         pPool->Remove( entity );

      auto& owned = m_entityComponentTypes[ entity ];
      if( auto it = std::find( owned.begin(), owned.end(), compIndex ); it != owned.end() )
         owned.erase( it );
   }

   template< typename TType >
   [[nodiscard]] ECS_FORCE_INLINE TType* Get( Entity entity ) noexcept
   {
      if( !FValid( entity ) )
         return nullptr;

      size_t compIndex = GetTypeIndex< TType >();
      if( compIndex >= m_componentPools.size() )
         return nullptr;

      auto* storage = static_cast< Storage< TType >* >( m_componentPools[ compIndex ].get() );
      return storage ? storage->Get( entity ) : nullptr;
   }
   template< typename TType >
   [[nodiscard]] ECS_FORCE_INLINE const TType* Get( Entity entity ) const noexcept
   {
      return const_cast< Registry* >( this )->Get< TType >( entity );
   }

   // --- Multi-component helpers (require at least one component type) ---
   template< typename TType, typename... TOthers >
   [[nodiscard]] ECS_FORCE_INLINE std::tuple< TType*, TOthers*... > GetComponents( Entity entity ) noexcept
   {
      return std::tuple< TType*, TOthers*... >( Get< TType >( entity ), Get< TOthers >( entity )... );
   }
   template< typename TType, typename... TOthers >
   [[nodiscard]] ECS_FORCE_INLINE std::tuple< const TType*, const TOthers*... > GetComponents( Entity entity ) const noexcept
   {
      return std::tuple< const TType*, const TOthers*... >( Get< TType >( entity ), Get< TOthers >( entity )... );
   }

   template< typename TType >
   [[nodiscard]] ECS_FORCE_INLINE bool FHas( Entity entity ) const noexcept
   {
      if( !FValid( entity ) )
         return false;

      size_t compIndex = GetTypeIndex< TType >();
      if( compIndex >= m_componentPools.size() )
         return false;

      auto* pool = m_componentPools[ compIndex ].get();
      if( !pool )
         return false;

      auto* storage = static_cast< Storage< TType >* >( pool );
      return entity < storage->m_entityToIndex.size() && storage->m_entityToIndex[ entity ] != Storage< TType >::npos;
   }
   template< typename TType, typename... TOthers >
   [[nodiscard]] ECS_FORCE_INLINE bool FHasComponents( Entity entity ) const noexcept
   {
      return FHas< TType >( entity ) && ( FHas< TOthers >( entity ) && ... );
   }

   template< typename TType, typename... TOthers >
   [[nodiscard]] ECS_FORCE_INLINE auto EView() noexcept
   {
      return View< ViewType::Entity, TType, TOthers... >( *this );
   }
   template< typename TType, typename... TOthers >
   [[nodiscard]] ECS_FORCE_INLINE auto CView() noexcept
   {
      return View< ViewType::Components, TType, TOthers... >( *this );
   }
   template< typename TType, typename... TOthers >
   [[nodiscard]] ECS_FORCE_INLINE auto ECView() noexcept
   {
      return View< ViewType::EntityAndComponents, TType, TOthers... >( *this );
   }

private:
   template< typename TType >
   ECS_FORCE_INLINE void EnsureComponentPool( size_t index )
   {
      if( index >= m_componentPools.size() )
         m_componentPools.resize( index + 1 );

      if( !m_componentPools[ index ] )
         m_componentPools[ index ] = std::make_unique< Storage< TType > >();
   }

   Entity                                 m_nextEntity { 1 };
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
public:
   explicit View( Registry& registry ) noexcept :
      m_registry( registry )
   {
      SelectSmallestPool();
      CacheStorages();
   }

   struct Iterator
   {
      Registry*                                                                                     registry { nullptr };
      size_t                                                                                        index { 0 };
      std::vector< Entity >*                                                                        entities { nullptr };
      std::tuple< typename Registry::Storage< TType >*, typename Registry::Storage< TOthers >*... > storages;

      template< typename TType >
      ECS_FORCE_INLINE static bool FHas( Entity e, typename Registry::Storage< TType >* pStorage ) noexcept
      {
         return pStorage && e < pStorage->m_entityToIndex.size() && pStorage->m_entityToIndex[ e ] != Registry::Storage< TType >::npos;
      }

      ECS_FORCE_INLINE bool FHasAll( Entity e ) const noexcept
      {
         return FHas< TType >( e, std::get< Registry::Storage< TType >* >( storages ) ) &&
                ( FHas< TOthers >( e, std::get< Registry::Storage< TOthers >* >( storages ) ) && ... );
      }

      ECS_FORCE_INLINE void Skip() noexcept
      {
         while( entities && index < entities->size() && !FHasAll( ( *entities )[ index ] ) )
            ++index;
      }
      ECS_FORCE_INLINE Iterator& operator++() noexcept
      {
         ++index;
         Skip();
         return *this;
      }
      ECS_FORCE_INLINE bool operator!=( const Iterator& rhs ) const noexcept { return index != rhs.index; }
      ECS_FORCE_INLINE auto operator*() const noexcept
      {
         Entity entity = ( *entities )[ index ];
         if constexpr( TViewType == ViewType::Entity )
            return entity;
         else if constexpr( TViewType == ViewType::Components )
            return std::tuple< TType&, TOthers&... >( *GetRef< TType >( entity, std::get< Registry::Storage< TType >* >( storages ) ),
                                                      *GetRef< TOthers >( entity, std::get< Registry::Storage< TOthers >* >( storages ) )... );
         else if constexpr( TViewType == ViewType::EntityAndComponents )
            return std::tuple_cat( std::make_tuple( entity ),
                                   std::tuple< TType&, TOthers&... >( *GetRef< TType >( entity, std::get< Registry::Storage< TType >* >( storages ) ),
                                                                      *GetRef< TOthers >( entity,
                                                                                          std::get< Registry::Storage< TOthers >* >( storages ) )... ) );
      }

      template< typename TType >
      ECS_FORCE_INLINE static TType* GetRef( Entity e, typename Registry::Storage< TType >* storage ) noexcept
      {
         return &storage->m_components[ storage->m_entityToIndex[ e ] ];
      }
   };

   ECS_FORCE_INLINE Iterator begin() noexcept
   {
      Iterator it { &m_registry, 0, m_smallestEntities };
      it.storages = m_componentStorages;
      it.Skip();
      return it;
   }
   ECS_FORCE_INLINE Iterator end() noexcept
   {
      Iterator it { &m_registry, m_smallestEntities ? m_smallestEntities->size() : 0, m_smallestEntities };
      it.storages = m_componentStorages;
      return it;
   }

private:
   template< typename TType >
   ECS_FORCE_INLINE std::vector< Entity >* GetEntitiesVectorFor() const noexcept
   {
      size_t index = Registry::GetTypeIndex< TType >();
      if( index >= m_registry.m_componentPools.size() )
         return nullptr;

      auto* pPool = m_registry.m_componentPools[ index ].get();
      return pPool ? &( static_cast< typename Registry::Storage< TType >* >( pPool )->m_entities ) : nullptr;
   }

   template< typename TType >
   ECS_FORCE_INLINE void SelectIfSmaller( size_t& minSize ) noexcept
   {
      if( auto* vec = GetEntitiesVectorFor< TType >(); vec && vec->size() < minSize )
      {
         minSize            = vec->size();
         m_smallestEntities = vec;
      }
   }

   void SelectSmallestPool() noexcept
   {
      size_t minSize = ( std::numeric_limits< size_t >::max )();
      SelectIfSmaller< TType >( minSize );
      ( SelectIfSmaller< TOthers >( minSize ), ... );
   }

   template< typename TType >
   ECS_FORCE_INLINE void CacheStorage() noexcept
   {
      size_t index = Registry::GetTypeIndex< TType >();
      if( index < m_registry.m_componentPools.size() )
      {
         if( auto* pPool = m_registry.m_componentPools[ index ].get() )
            std::get< typename Registry::Storage< TType >* >( m_componentStorages ) = static_cast< typename Registry::Storage< TType >* >( pPool );
      }
   }

   ECS_FORCE_INLINE void CacheStorages() noexcept
   {
      CacheStorage< TType >();
      ( CacheStorage< TOthers >(), ... );
   }

   Registry&                                                                                     m_registry;                     ///< Referenced registry.
   std::vector< Entity >*                                                                        m_smallestEntities { nullptr }; ///< Driving entity vector.
   std::tuple< typename Registry::Storage< TType >*, typename Registry::Storage< TOthers >*... > m_componentStorages {};         ///< Cached storages.
};

} // namespace Entity

// hash specialization
template<>
struct std::hash< Entity::EntityHandle >
{
   size_t operator()( const Entity::EntityHandle& handle ) const noexcept { return std::hash< Entity::Entity > {}( handle.Get() ); }
};
