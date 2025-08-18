#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <limits>
#include <cassert>
#include <bit>
#include <tuple>
#include <utility>
#include <algorithm>

namespace Entity
{

// Hot-path inlining
#if defined( _MSC_VER )
#define ECS_FORCE_INLINE __forceinline
#else
#define ECS_FORCE_INLINE inline __attribute__( ( always_inline ) )
#endif

using Entity = uint64_t;
class Registry; // fwd

// =============================================================
// EntityHandle
// =============================================================
/**
 * @class EntityHandle
 * @brief Lightweight non-owning wrapper over an Entity ID with a binding to a Registry.
 */
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

// =============================================================
// ViewType
// =============================================================
enum class ViewType
{
   Entity,
   Components,
   EntityAndComponents
};

// =============================================================
// Registry (single 64-bit mask variant)
// =============================================================
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
      return s_index;
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
      std::vector< DenseIndexType > m_entityToIndex;

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

         DenseIndexType index = m_entityToIndex[ entity ];
         return index == npos ? nullptr : &m_components[ index ];
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
            m_components[ index ]          = std::move( m_components[ backIdx ] );
            Entity movedEntity             = m_entities[ backIdx ];
            m_entities[ index ]            = movedEntity;
            m_entityToIndex[ movedEntity ] = index;
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
         m_entityMasks.resize( static_cast< size_t >( entity ) + 1, 0 );
      }

      m_entityAlive[ entity ] = 1;
      return entity;
   }

   [[nodiscard]] std::shared_ptr< EntityHandle > CreateWithHandle() noexcept { return std::shared_ptr< EntityHandle >( new EntityHandle( Create(), *this ) ); }

   void Destroy( Entity entity ) noexcept
   {
      if( !FValid( entity ) )
         return;

      uint64_t mask = m_entityMasks[ entity ];
      if( mask )
      {
         std::unique_ptr< Registry::Pool >* pPools    = m_componentPools.data();
         const size_t                       poolCount = m_componentPools.size();
         while( mask )
         {
            size_t bit       = std::countr_zero( mask );
            size_t compIndex = bit;
            if( compIndex < poolCount )
            {
               Pool* pPool = pPools[ compIndex ].get();
               if( pPool )
                  pPool->Remove( entity );
            }
            mask &= mask - 1;
         }
      }

      m_entityMasks[ entity ] = 0;
      m_entityAlive[ entity ] = 0;
      m_recycled.push_back( entity );
   }

   [[nodiscard]] ECS_FORCE_INLINE bool   FValid( Entity entity ) const noexcept { return entity < m_entityAlive.size() && m_entityAlive[ entity ]; }
   [[nodiscard]] ECS_FORCE_INLINE size_t GetEntityCount() const noexcept { return m_nextEntity - m_recycled.size() - 1; }

   template< typename TComponent, typename... TArgs >
   ECS_FORCE_INLINE TComponent& AddComponent( Entity entity, TArgs&&... args )
   {
      assert( FValid( entity ) && "AddComponent on invalid entity" );
      size_t compIndex = GetComponentTypeIndex< TComponent >();
      assert( compIndex < 64 && "Exceeded 64 component types limit in single-mask optimized Registry" );
      EnsureComponentPool< TComponent >( compIndex );
      SetMaskBit( entity, compIndex );

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

      ClearMaskBit( entity, compIndex );
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
      return FValid( entity ) && FTestMaskBit( entity, GetComponentTypeIndex< TComponent >() );
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

   ECS_FORCE_INLINE void SetMaskBit( Entity entity, size_t compIndex ) { m_entityMasks[ entity ] |= ( 1ull << compIndex ); }
   ECS_FORCE_INLINE void ClearMaskBit( Entity entity, size_t compIndex )
   {
      if( entity >= m_entityMasks.size() )
         return;

      m_entityMasks[ entity ] &= ~( 1ull << compIndex );
   }
   ECS_FORCE_INLINE bool FTestMaskBit( Entity entity, size_t compIndex ) const noexcept
   {
      if( entity >= m_entityMasks.size() )
         return false;

      return ( ( m_entityMasks[ entity ] >> compIndex ) & 1ull ) != 0;
   }

   Entity                                 m_nextEntity { 1 };
   std::vector< Entity >                  m_recycled;
   std::vector< uint8_t >                 m_entityAlive;
   std::vector< uint64_t >                m_entityMasks; // single mask per entity
   std::vector< std::unique_ptr< Pool > > m_componentPools;
};

// =============================================================
// View
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

      BuildRequirementMask();
      SelectSmallestPool();
      CacheStorages();
   }

   struct Iterator
   {
      Registry*                                                            registry { nullptr };
      size_t                                                               index { 0 };
      std::vector< Entity >*                                               entities { nullptr };
      uint64_t                                                             requiredMask { 0 };
      std::tuple< typename Registry::ComponentStorage< TComponents >*... > storages;

      ECS_FORCE_INLINE bool HasAll( Entity e ) const noexcept { return ( registry->m_entityMasks[ e ] & requiredMask ) == requiredMask; }
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
         return std::tuple< TComponents&... >( *GetComponentRef< TComponents >( e, std::get< I >( storages ) )... );
      }

      template< typename TComp >
      ECS_FORCE_INLINE static TComp* GetComponentRef( Entity e, typename Registry::ComponentStorage< TComp >* storage ) noexcept
      {
         return &storage->m_components[ storage->m_entityToIndex[ e ] ];
      }
   };

   ECS_FORCE_INLINE Iterator begin() noexcept
   {
      Iterator it { &m_registry, 0, m_smallestEntities, m_requiredMask };
      it.storages = m_componentStorages;
      it.Skip();
      return it;
   }
   ECS_FORCE_INLINE Iterator end() noexcept
   {
      Iterator it { &m_registry, m_smallestEntities ? m_smallestEntities->size() : 0, m_smallestEntities, m_requiredMask };
      it.storages = m_componentStorages;
      return it;
   }

private:
   void BuildRequirementMask() noexcept
   {
      if constexpr( sizeof...( TComponents ) > 0 )
         ( AddRequirementFor< TComponents >(), ... );
   }

   template< typename TComponent >
   ECS_FORCE_INLINE void AddRequirementFor() noexcept
   {
      m_requiredMask |= ( 1ull << Registry::GetComponentTypeIndex< TComponent >() );
   }

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

   Registry&                                                            m_registry;
   std::vector< Entity >*                                               m_smallestEntities { nullptr };
   uint64_t                                                             m_requiredMask { 0 };
   std::tuple< typename Registry::ComponentStorage< TComponents >*... > m_componentStorages {};
};

// === Specialization: Entity-only view with no component filter (iterates alive entities) ===
template<>
class View< ViewType::Entity >
{
public:
   explicit View( Registry& registry ) noexcept :
      m_registry( registry )
   {
      const size_t count = m_registry.m_entityAlive.size();
      m_entities.reserve( count );
      for( size_t i = 0; i < count; ++i )
      {
         if( m_registry.m_entityAlive[ i ] )
            m_entities.push_back( static_cast< Entity >( i ) );
      }
   }

   struct Iterator
   {
      const std::vector< Entity >* list { nullptr };
      size_t                       index { 0 };
      ECS_FORCE_INLINE Iterator&   operator++() noexcept
      {
         ++index;
         return *this;
      }
      ECS_FORCE_INLINE bool   operator!=( const Iterator& rhs ) const noexcept { return index != rhs.index; }
      ECS_FORCE_INLINE Entity operator*() const noexcept { return ( *list )[ index ]; }
   };

   ECS_FORCE_INLINE Iterator begin() noexcept { return Iterator { &m_entities, 0 }; }
   ECS_FORCE_INLINE Iterator end() noexcept { return Iterator { &m_entities, m_entities.size() }; }

private:
   Registry&             m_registry;
   std::vector< Entity > m_entities;
};

} // namespace Entity

template<>
struct std::hash< Entity::EntityHandle >
{
   size_t operator()( const Entity::EntityHandle& handle ) const noexcept { return std::hash< Entity::Entity > {}( handle.Get() ); }
};
