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
class Registry; // fwd

class EntityHandle
{
private:
   friend class Registry;
   EntityHandle( Entity entity, Registry& registry ) :
      m_entity( entity ),
      m_pRegistry( &registry )
   {}
   void Unbind() noexcept { m_pRegistry = nullptr; }

public:
   ~EntityHandle();

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
   Registry*    m_pRegistry { nullptr };
};

enum class ViewType
{
   Entity,
   Components,
   EntityAndComponents
};

// =============================================
// Registry (mask-less unlimited components)
// =============================================
class Registry
{
private:
   template< ViewType TViewType, typename... TComponents >
   friend class View;

   static inline size_t s_nextComponentTypeIndex = 0;
   template< typename TComponent >
   ECS_FORCE_INLINE static size_t GetComponentTypeIndex() noexcept
   {
      static const size_t s_index = s_nextComponentTypeIndex++;
      return s_index; // unbounded
   }

   struct Pool
   {
      virtual ~Pool()                        = default;
      virtual void Remove( Entity ) noexcept = 0;
      virtual bool Empty() const noexcept    = 0;
   };

   template< typename TComponent >
   struct ComponentStorage final : Pool
   {
      using DenseIndexType                 = uint32_t;
      static constexpr DenseIndexType npos = ( std::numeric_limits< DenseIndexType >::max )();

      std::vector< TComponent >     m_components;
      std::vector< Entity >         m_entities;
      std::vector< DenseIndexType > m_entityToIndex; // sparse

      ECS_FORCE_INLINE void EnsureEntityCapacity( Entity entity )
      {
         if( entity >= m_entityToIndex.size() )
            m_entityToIndex.resize( static_cast< size_t >( entity ) + 1, npos );
      }

      ECS_FORCE_INLINE TComponent& Emplace( Entity entity, TComponent&& value )
      {
         EnsureEntityCapacity( entity );
         DenseIndexType index = static_cast< DenseIndexType >( m_components.size() );
         m_components.emplace_back( std::move( value ) );
         m_entities.emplace_back( entity );
         m_entityToIndex[ entity ] = index;
         return m_components.back();
      }

      template< typename... Args >
      ECS_FORCE_INLINE TComponent& EmplaceConstruct( Entity entity, Args&&... args )
      {
         EnsureEntityCapacity( entity );
         DenseIndexType index = static_cast< DenseIndexType >( m_components.size() );
         m_components.emplace_back( std::forward< Args >( args )... );
         m_entities.emplace_back( entity );
         m_entityToIndex[ entity ] = index;
         return m_components.back();
      }

      ECS_FORCE_INLINE TComponent* Get( Entity entity ) noexcept
      {
         if( entity >= m_entityToIndex.size() )
            return nullptr;

         DenseIndexType idx = m_entityToIndex[ entity ];
         return idx == npos ? nullptr : &m_components[ idx ];
      }
      ECS_FORCE_INLINE const TComponent* Get( Entity entity ) const noexcept { return Get( entity ); }

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

      ECS_FORCE_INLINE bool Empty() const noexcept override { return m_components.empty(); }
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

   [[nodiscard]] std::shared_ptr< EntityHandle > CreateWithHandle() noexcept { return std::shared_ptr< EntityHandle >( new EntityHandle( Create(), *this ) ); }

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
   template< typename TComponent, typename... TArgs >
   ECS_FORCE_INLINE TComponent& AddComponent( Entity entity, TArgs&&... args )
   {
      assert( FValid( entity ) && "AddComponent on invalid entity" );
      size_t compIndex = GetComponentTypeIndex< TComponent >();
      EnsureComponentPool< TComponent >( compIndex );

      auto* pStorage = static_cast< ComponentStorage< TComponent >* >( m_componentPools[ compIndex ].get() );
      auto& sparse   = pStorage->m_entityToIndex;
      if( entity < sparse.size() )
      {
         auto dense = sparse[ entity ];
         if( dense != ComponentStorage< TComponent >::npos )
         {
            pStorage->m_components[ dense ] = TComponent( std::forward< TArgs >( args )... );
            return pStorage->m_components[ dense ];
         }
      }

      auto& owned = m_entityComponentTypes[ entity ];
      if( std::find( owned.begin(), owned.end(), compIndex ) == owned.end() )
         owned.push_back( compIndex );

      return pStorage->EmplaceConstruct( entity, std::forward< TArgs >( args )... );
   }

   template< typename TComponent >
   ECS_FORCE_INLINE void RemoveComponent( Entity entity ) noexcept
   {
      if( !FValid( entity ) )
         return;

      size_t compIndex = GetComponentTypeIndex< TComponent >();
      if( compIndex >= m_componentPools.size() )
         return;

      if( auto* pPool = m_componentPools[ compIndex ].get() )
         pPool->Remove( entity );

      auto& owned = m_entityComponentTypes[ entity ];
      if( auto it = std::find( owned.begin(), owned.end(), compIndex ); it != owned.end() )
         owned.erase( it );
   }

   template< typename TComponent >
   [[nodiscard]] ECS_FORCE_INLINE TComponent* GetComponent( Entity entity ) noexcept
   {
      if( !FValid( entity ) )
         return nullptr;

      size_t compIndex = GetComponentTypeIndex< TComponent >();
      if( compIndex >= m_componentPools.size() )
         return nullptr;

      auto* storage = static_cast< ComponentStorage< TComponent >* >( m_componentPools[ compIndex ].get() );
      return storage ? storage->Get( entity ) : nullptr;
   }
   template< typename TComponent >
   [[nodiscard]] ECS_FORCE_INLINE const TComponent* GetComponent( Entity entity ) const noexcept
   {
      return const_cast< Registry* >( this )->GetComponent< TComponent >( entity );
   }

   template< typename... TComponents >
   [[nodiscard]] ECS_FORCE_INLINE std::tuple< TComponents*... > GetComponents( Entity entity ) noexcept
   {
      return std::tuple< TComponents*... >( GetComponent< TComponents >( entity )... );
   }
   template< typename... TComponents >
   [[nodiscard]] ECS_FORCE_INLINE std::tuple< const TComponents*... > GetComponents( Entity entity ) const noexcept
   {
      return std::tuple< const TComponents*... >( GetComponent< TComponents >( entity )... );
   }

   template< typename TComponent >
   [[nodiscard]] ECS_FORCE_INLINE bool FHasComponent( Entity entity ) const noexcept
   {
      if( !FValid( entity ) )
         return false;
      size_t compIndex = GetComponentTypeIndex< TComponent >();
      if( compIndex >= m_componentPools.size() )
         return false;
      auto* pool = m_componentPools[ compIndex ].get();
      if( !pool )
         return false;
      auto* storage = static_cast< ComponentStorage< TComponent >* >( pool );
      return entity < storage->m_entityToIndex.size() && storage->m_entityToIndex[ entity ] != ComponentStorage< TComponent >::npos;
   }
   template< typename... TComponents >
   [[nodiscard]] ECS_FORCE_INLINE bool FHasComponents( Entity entity ) const noexcept
   {
      return ( FHasComponent< TComponents >( entity ) && ... );
   }

   template< typename... TComponents >
   [[nodiscard]] ECS_FORCE_INLINE auto EView() noexcept
   {
      return View< ViewType::Entity, TComponents... >( *this );
   }
   template< typename... TComponents >
   [[nodiscard]] ECS_FORCE_INLINE auto CView() noexcept
   {
      return View< ViewType::Components, TComponents... >( *this );
   }
   template< typename... TComponents >
   [[nodiscard]] ECS_FORCE_INLINE auto ECView() noexcept
   {
      return View< ViewType::EntityAndComponents, TComponents... >( *this );
   }

private:
   template< typename TComponent >
   ECS_FORCE_INLINE void EnsureComponentPool( size_t index )
   {
      if( index >= m_componentPools.size() )
         m_componentPools.resize( index + 1 );

      if( !m_componentPools[ index ] )
         m_componentPools[ index ] = std::make_unique< ComponentStorage< TComponent > >();
   }

   // ----- Data members -----
   Entity                                 m_nextEntity { 1 };
   std::vector< Entity >                  m_recycled;
   std::vector< uint8_t >                 m_entityAlive;
   std::vector< std::unique_ptr< Pool > > m_componentPools;       ///< Indexed by component type index.
   std::vector< std::vector< size_t > >   m_entityComponentTypes; ///< Per-entity owned component indices.
};

// =============================================================
// View (mask-less variant)
// =============================================================

template< ViewType TViewType, typename... TComponents >
class View
{
public:
   explicit View( Registry& registry ) noexcept :
      m_registry( registry )
   {
      if constexpr( ( sizeof...( TComponents ) == 0 ) && ( TViewType != ViewType::Entity ) )
         static_assert( sizeof...( TComponents ) > 0 || TViewType == ViewType::Entity, "CView/ECView require at least one component type" );

      SelectSmallestPool();
      CacheStorages();
   }

   struct Iterator
   {
      Registry*                                                            registry { nullptr };
      size_t                                                               index { 0 };
      std::vector< Entity >*                                               entities { nullptr };
      std::tuple< typename Registry::ComponentStorage< TComponents >*... > storages;

      template< typename TComp >
      ECS_FORCE_INLINE static bool Has( Entity e, typename Registry::ComponentStorage< TComp >* pStorage ) noexcept
      {
         return pStorage && e < pStorage->m_entityToIndex.size() && pStorage->m_entityToIndex[ e ] != Registry::ComponentStorage< TComp >::npos;
      }

      ECS_FORCE_INLINE bool HasAll( Entity e ) const noexcept
      {
         return ( Has< TComponents >( e, std::get< Registry::ComponentStorage< TComponents >* >( storages ) ) && ... );
      }

      ECS_FORCE_INLINE void Skip() noexcept
      {
         while( entities && index < entities->size() && !HasAll( ( *entities )[ index ] ) )
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
            return DerefComponents( entity, std::index_sequence_for< TComponents... > {} );
         else if constexpr( TViewType == ViewType::EntityAndComponents )
            return std::tuple_cat( std::make_tuple( entity ), DerefComponents( entity, std::index_sequence_for< TComponents... > {} ) );
      }

      template< std::size_t... I >
      ECS_FORCE_INLINE auto DerefComponents( Entity e, std::index_sequence< I... > ) const noexcept
      {
         return std::tuple< TComponents&... >( *GetRef< TComponents >( e, std::get< I >( storages ) )... );
      }

      template< typename TComp >
      ECS_FORCE_INLINE static TComp* GetRef( Entity e, typename Registry::ComponentStorage< TComp >* storage ) noexcept
      {
         return &storage->m_components[ storage->m_entityToIndex[ e ] ];
      }
   };

   ECS_FORCE_INLINE Iterator begin() noexcept
   {
      Iterator it { &m_registry, 0, m_smallestEntities, m_componentStorages };
      it.Skip();
      return it;
   }
   ECS_FORCE_INLINE Iterator end() noexcept
   {
      Iterator it { &m_registry, m_smallestEntities ? m_smallestEntities->size() : 0, m_smallestEntities, m_componentStorages };
      return it;
   }

private:
   template< typename TComponent >
   ECS_FORCE_INLINE std::vector< Entity >* GetEntitiesVectorFor() const noexcept
   {
      size_t index = Registry::GetComponentTypeIndex< TComponent >();
      if( index >= m_registry.m_componentPools.size() )
         return nullptr;

      auto* pPool = m_registry.m_componentPools[ index ].get();
      return pPool ? &( static_cast< typename Registry::ComponentStorage< TComponent >* >( pPool )->m_entities ) : nullptr;
   }

   template< typename TComponent >
   ECS_FORCE_INLINE void SelectIfSmaller( size_t& minSize ) noexcept
   {
      if( auto* vec = GetEntitiesVectorFor< TComponent >(); vec && vec->size() < minSize )
      {
         minSize            = vec->size();
         m_smallestEntities = vec;
      }
   }

   void SelectSmallestPool() noexcept
   {
      if constexpr( sizeof...( TComponents ) > 0 )
      {
         size_t minSize = ( std::numeric_limits< size_t >::max )();
         ( SelectIfSmaller< TComponents >( minSize ), ... );
      }
   }

   template< typename TComponent >
   ECS_FORCE_INLINE void CacheStorage() noexcept
   {
      size_t index = Registry::GetComponentTypeIndex< TComponent >();
      if( index < m_registry.m_componentPools.size() )
      {
         if( auto* pPool = m_registry.m_componentPools[ index ].get() )
            std::get< Registry::ComponentStorage< TComponent >* >( m_componentStorages ) = static_cast< Registry::ComponentStorage< TComponent >* >( pPool );
      }
   }

   ECS_FORCE_INLINE void CacheStorages() noexcept
   {
      if constexpr( sizeof...( TComponents ) > 0 )
         ( CacheStorage< TComponents >(), ... );
   }

   Registry&                                                            m_registry;                     ///< Referenced registry.
   std::vector< Entity >*                                               m_smallestEntities { nullptr }; ///< Driving entity vector.
   std::tuple< typename Registry::ComponentStorage< TComponents >*... > m_componentStorages {};         ///< Cached storages.
};

} // namespace Entity

template<>
struct std::hash< Entity::EntityHandle >
{
   size_t operator()( const Entity::EntityHandle& handle ) const noexcept { return std::hash< Entity::Entity > {}( handle.Get() ); }
};
