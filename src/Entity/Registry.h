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

/**
 * @brief Strong typedef for an entity identifier.
 * @details Entity IDs are monotonically increasing unsigned 64-bit integers (0 reserved / invalid).
 */
using Entity = uint64_t;
class Registry; // fwd

// =============================================================
// EntityHandle
// =============================================================
/**
 * @class EntityHandle
 * @brief Lightweight non‑owning wrapper over an Entity ID bound to a Registry.
 *
 * @details Provides convenience access and comparison operators while deferring lifetime
 * management to the owning Registry. Destruction of the handle does NOT destroy the entity;
 * this is intentional for performance (no indirection / reference counting on every access).
 * A handle can become stale after the underlying entity is destroyed; always re‑validate
 * with Registry::FValid before use if the lifetime is uncertain.
 */
class EntityHandle
{
private:
   friend class Registry;
   /** @brief Internal constructor used by Registry::CreateWithHandle. */
   EntityHandle( Entity entity, Registry& registry ) :
      m_entity( entity ),
      m_pRegistry( &registry )
   {}
   /** @brief Manually unbinds from registry (not currently invoked automatically). */
   void Unbind() noexcept { m_pRegistry = nullptr; }

public:
   /** @brief Destructor (does not destroy the entity; see Registry for lifetime). */
   ~EntityHandle();

   EntityHandle( const EntityHandle& )                = delete; ///< Non-copyable for clarity.
   EntityHandle( EntityHandle&& ) noexcept            = delete; ///< Non-movable (keeps pointer stability assumptions simple).
   EntityHandle& operator=( EntityHandle&& ) noexcept = delete;
   EntityHandle& operator=( const EntityHandle& )     = delete;

   /** @return Raw entity identifier. */
   [[nodiscard]] Entity Get() const noexcept { return m_entity; }
   /** @brief Implicit conversion to raw Entity id. */
   explicit             operator Entity() const noexcept { return m_entity; }

   // ---- Comparisons ----
   bool operator==( const EntityHandle& other ) const noexcept { return m_entity == other.m_entity; }
   bool operator!=( const EntityHandle& other ) const noexcept { return m_entity != other.m_entity; }
   bool operator==( Entity other ) const noexcept { return m_entity == other; }
   bool operator!=( Entity other ) const noexcept { return m_entity != other; }
   bool operator<( const EntityHandle& other ) const noexcept { return m_entity < other.m_entity; }

private:
   const Entity m_entity { 0 };          ///< Stored entity id.
   Registry*    m_pRegistry { nullptr }; ///< Owning registry (nullable after Unbind).
};

// =============================================================
// ViewType
// =============================================================
/**
 * @enum ViewType
 * @brief Output tuple shape for a view iteration.
 * @var ViewType::Entity Yields only entity IDs.
 * @var ViewType::Components Yields references to component instances.
 * @var ViewType::EntityAndComponents Yields entity followed by component references.
 */
enum class ViewType
{
   Entity,
   Components,
   EntityAndComponents
};

// =============================================================
// Registry (single 64-bit mask variant)
// =============================================================
/**
 * @class Registry
 * @brief High‑performance entity‑component storage with packed component arrays and a single 64‑bit membership mask per entity.
 *
 * @details This variant sacrifices support for >64 distinct component types in exchange for:
 * - Extremely compact per-entity metadata (one u64 + alive flag)
 * - O(1) component presence checks via bit test
 * - Contiguous iteration over the smallest component pool (cache locality)
 * - Swap‑and‑pop removal semantics maintaining dense packing
 *
 * @note If future requirements exceed 64 component types, the design can be extended to a multi‑block mask.
 * @warning Entity IDs are reused after destruction; stale IDs must be re‑validated with FValid().
 */
class Registry
{
private:
   template< ViewType TViewType, typename... TComponents >
   friend class View;

   // ------------------------------------------------------------------
   // Component type indexing
   // ------------------------------------------------------------------
   static inline size_t s_nextComponentTypeIndex = 0; ///< Monotonic counter assigning indices (bit positions).

   /**
    * @brief Returns the stable unique bit index for component type.
    * @tparam TComponent Component type queried.
    * @return Unique index in [0,63].
    */
   template< typename TComponent >
   ECS_FORCE_INLINE static size_t GetComponentTypeIndex() noexcept
   {
      static const size_t s_index = s_nextComponentTypeIndex++;
      return s_index;
   }

   /**
    * @brief Abstract base for component storage (type erasure for destruction / removal loops).
    */
   struct Pool
   {
      virtual ~Pool()                        = default;
      /** @brief Remove component for entity if present (idempotent). */
      virtual void Remove( Entity ) noexcept = 0;
      /** @return True if storage currently holds zero components. */
      virtual bool Empty() const noexcept    = 0;
   };

   /**
    * @brief Packed dense storage for a single component type.
    * @tparam TComponent Component type.
    *
    * Dense layout: m_components[i] corresponds to entity m_entities[i]. Sparse index allows O(1) lookup via entity id.
    */
   template< typename TComponent >
   struct ComponentStorage final : Pool
   {
      using DenseIndexType                 = uint32_t; ///< Compact index type to reduce sparse vector footprint.
      static constexpr DenseIndexType npos = ( std::numeric_limits< DenseIndexType >::max )(); ///< Sentinel for "absent".

      std::vector< TComponent >     m_components;    ///< Dense component values.
      std::vector< Entity >         m_entities;      ///< Dense parallel entity list.
      std::vector< DenseIndexType > m_entityToIndex; ///< Sparse lookup: entity -> dense index or npos.

      /** @brief Ensure sparse mapping can address the provided entity id. */
      ECS_FORCE_INLINE void EnsureEntityCapacity( Entity entity )
      {
         if( entity >= m_entityToIndex.size() )
            m_entityToIndex.resize( static_cast< size_t >( entity ) + 1, npos );
      }

      /**
       * @brief Emplace an already constructed component (move).
       * @return Reference to new component.
       */
      ECS_FORCE_INLINE TComponent& Emplace( Entity entity, TComponent&& value )
      {
         EnsureEntityCapacity( entity );
         DenseIndexType index = static_cast< DenseIndexType >( m_components.size() );
         m_components.emplace_back( std::move( value ) );
         m_entities.emplace_back( entity );
         m_entityToIndex[ entity ] = index;
         return m_components.back();
      }

      /**
       * @brief Construct component in place forwarding arguments.
       * @tparam Args Constructor argument types.
       * @return Reference to new component.
       */
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

      /** @return Pointer to component for entity or nullptr if absent. */
      ECS_FORCE_INLINE TComponent* Get( Entity entity ) noexcept
      {
         if( entity >= m_entityToIndex.size() )
            return nullptr;

         DenseIndexType index = m_entityToIndex[ entity ];
         return index == npos ? nullptr : &m_components[ index ];
      }

      /** @overload */
      ECS_FORCE_INLINE const TComponent* Get( Entity entity ) const noexcept { return Get( entity ); }

      /**
       * @brief Remove component for entity (swap & pop keeps dense arrays compact).
       * @note Order is not preserved.
       */
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

      /** @return True if empty. */
      ECS_FORCE_INLINE bool Empty() const noexcept override { return m_components.empty(); }
   };

public:
   /** @brief Default constructor (no allocations). */
   Registry()  = default;
   /** @brief Destructor (does not automatically sweep external handles). */
   ~Registry() = default;

   Registry( const Registry& )            = delete; ///< Non-copyable.
   Registry& operator=( const Registry& ) = delete; ///< Non-assignable.

   /**
    * @brief Create a new entity. Reuses recycled IDs when available.
    * @return New entity ID (always > 0).
    */
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

   /**
    * @brief Create an entity and return a bound handle.
    * @warning The handle does NOT auto-invalidate when the entity is destroyed. Always call FValid before reuse.
    */
   [[nodiscard]] std::shared_ptr< EntityHandle > CreateWithHandle() noexcept { return std::shared_ptr< EntityHandle >( new EntityHandle( Create(), *this ) ); }

   /**
    * @brief Destroy entity and remove all its components.
    * @param entity Target entity (silently ignored if invalid).
    * @note Complexity proportional to number of set bits in its mask.
    */
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

   /** @return True if entity currently alive. */
   [[nodiscard]] ECS_FORCE_INLINE bool   FValid( Entity entity ) const noexcept { return entity < m_entityAlive.size() && m_entityAlive[ entity ]; }
   /** @return Approximate live entity count (recycled IDs subtracted). */
   [[nodiscard]] ECS_FORCE_INLINE size_t GetEntityCount() const noexcept { return m_nextEntity - m_recycled.size() - 1; }

   /**
    * @brief Add or replace a component on an entity.
    * @tparam TComponent Component type.
    * @tparam TArgs Constructor argument types.
    * @param entity Entity must be valid.
    * @param args Constructor arguments forwarded to TComponent.
    * @return Reference to the (new or replaced) component.
    */
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

   /**
    * @brief Remove a component if present.
    * @tparam TComponent Component type.
    * @param entity Target entity (ignored if invalid / component absent).
    */
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

   /**
    * @brief Get mutable component pointer.
    * @tparam TComponent Component type.
    * @param entity Target entity.
    * @return Pointer or nullptr.
    */
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

   /** @overload */
   template< typename TComponent >
   [[nodiscard]] ECS_FORCE_INLINE const TComponent* GetComponent( Entity entity ) const noexcept
   {
      return const_cast< Registry* >( this )->GetComponent< TComponent >( entity );
   }

   /**
    * @brief Get multiple component pointers as a tuple (nullptr for missing components).
    * @tparam TComponents Component types requested.
    * @param entity Target entity.
    */
   template< typename... TComponents >
   [[nodiscard]] ECS_FORCE_INLINE std::tuple< TComponents*... > GetComponents( Entity entity ) noexcept
   {
      return std::tuple< TComponents*... >( GetComponent< TComponents >( entity )... );
   }

   /** @overload */
   template< typename... TComponents >
   [[nodiscard]] ECS_FORCE_INLINE std::tuple< const TComponents*... > GetComponents( Entity entity ) const noexcept
   {
      return std::tuple< const TComponents*... >( GetComponent< TComponents >( entity )... );
   }

   /** @return True if entity currently has component TComponent. */
   template< typename TComponent >
   [[nodiscard]] ECS_FORCE_INLINE bool FHasComponent( Entity entity ) const noexcept
   {
      return FValid( entity ) && FTestMaskBit( entity, GetComponentTypeIndex< TComponent >() );
   }

   /** @return True if entity has all listed component types. */
   template< typename... TComponents >
   [[nodiscard]] ECS_FORCE_INLINE bool FHasComponents( Entity entity ) const noexcept
   {
      return ( FHasComponent< TComponents >( entity ) && ... );
   }

   /** @brief Entity-only view (filter by presence of TComponents...). */
   template< typename... TComponents >
   [[nodiscard]] ECS_FORCE_INLINE auto EView() noexcept
   {
      return View< ViewType::Entity, TComponents... >( *this );
   }
   /** @brief View yielding only component references (tuple). */
   template< typename... TComponents >
   [[nodiscard]] ECS_FORCE_INLINE auto CView() noexcept
   {
      return View< ViewType::Components, TComponents... >( *this );
   }
   /** @brief View yielding (Entity, Components...). */
   template< typename... TComponents >
   [[nodiscard]] ECS_FORCE_INLINE auto ECView() noexcept
   {
      return View< ViewType::EntityAndComponents, TComponents... >( *this );
   }

private:
   // ----- Internal helpers -----
   /** @brief Allocate component storage if missing. */
   template< typename TComponent >
   ECS_FORCE_INLINE void EnsureComponentPool( size_t index )
   {
      if( index >= m_componentPools.size() )
         m_componentPools.resize( index + 1 );

      if( !m_componentPools[ index ] )
         m_componentPools[ index ] = std::make_unique< ComponentStorage< TComponent > >();
   }

   /** @brief Set component bit for entity. */
   ECS_FORCE_INLINE void SetMaskBit( Entity entity, size_t compIndex ) { m_entityMasks[ entity ] |= ( 1ull << compIndex ); }
   /** @brief Clear component bit for entity if present. */
   ECS_FORCE_INLINE void ClearMaskBit( Entity entity, size_t compIndex )
   {
      if( entity >= m_entityMasks.size() )
         return;

      m_entityMasks[ entity ] &= ~( 1ull << compIndex );
   }
   /** @return True if component bit is set for entity. */
   ECS_FORCE_INLINE bool FTestMaskBit( Entity entity, size_t compIndex ) const noexcept
   {
      if( entity >= m_entityMasks.size() )
         return false;

      return ( ( m_entityMasks[ entity ] >> compIndex ) & 1ull ) != 0;
   }

   // ----- Data members -----
   Entity                                 m_nextEntity { 1 }; ///< Next entity id (incremented on creation).
   std::vector< Entity >                  m_recycled;         ///< Recycled entity id stack (LIFO reuse).
   std::vector< uint8_t >                 m_entityAlive;      ///< Alive flags (1 = alive).
   std::vector< uint64_t >                m_entityMasks;      ///< Single 64-bit component membership mask per entity.
   std::vector< std::unique_ptr< Pool > > m_componentPools;   ///< Heterogeneous component storages.
};

// =============================================================
// View
// =============================================================
/**
 * @class View
 * @brief Compile-time configured iterable over entities possessing a set of components.
 *
 * @tparam TViewType Output tuple form (entity only, components only, or entity+components).
 * @tparam TComponents Component types required.
 *
 * @details Picks the smallest component pool to drive iteration, performing membership bit tests
 * with a single 64-bit mask per entity. Component pointers are resolved through cached storage pointers.
 */
template< ViewType TViewType, typename... TComponents >
class View
{
public:
   /**
    * @brief Construct view over a registry (no ownership). Precomputes mask & caches storages.
    * @param registry Source registry.
    */
   explicit View( Registry& registry ) noexcept :
      m_registry( registry )
   {
      if constexpr( ( sizeof...( TComponents ) == 0 ) && ( TViewType != ViewType::Entity ) )
         static_assert( sizeof...( TComponents ) > 0 || TViewType == ViewType::Entity, "CView/ECView require at least one component type" );

      BuildRequirementMask();
      SelectSmallestPool();
      CacheStorages();
   }

   /**
    * @struct Iterator
    * @brief Forward iterator over the filtered entity set.
    * @note Provides value semantics; copying an iterator is inexpensive.
    */
   struct Iterator
   {
      Registry*                                                            registry { nullptr }; ///< Owning registry (non-owning ptr).
      size_t                                                               index { 0 };          ///< Current dense index inside driving pool.
      std::vector< Entity >*                                               entities { nullptr }; ///< Pointer to driving entity vector.
      uint64_t                                                             requiredMask { 0 };   ///< Aggregated component requirement bits.
      std::tuple< typename Registry::ComponentStorage< TComponents >*... > storages;             ///< Cached component storage pointers.

      /** @return True if entity e satisfies component mask. */
      ECS_FORCE_INLINE bool HasAll( Entity e ) const noexcept { return ( registry->m_entityMasks[ e ] & requiredMask ) == requiredMask; }
      /** @brief Skip forward until a valid entity is found or end reached. */
      ECS_FORCE_INLINE void Skip() noexcept
      {
         while( entities && index < entities->size() && !HasAll( ( *entities )[ index ] ) )
            ++index;
      }
      /** @brief Pre-increment. */
      ECS_FORCE_INLINE Iterator& operator++() noexcept
      {
         ++index;
         Skip();
         return *this;
      }
      /** @brief Inequality comparison. */
      ECS_FORCE_INLINE bool operator!=( const Iterator& rhs ) const noexcept { return index != rhs.index; }
      /** @brief Dereference: returns configured tuple shape. */
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

      /** @brief Build tuple of component references for entity e. */
      template< std::size_t... I >
      ECS_FORCE_INLINE auto DerefComponents( Entity e, std::index_sequence< I... > ) const noexcept
      {
         return std::tuple< TComponents&... >( *GetComponentRef< TComponents >( e, std::get< I >( storages ) )... );
      }

      /** @brief Direct component reference (unchecked). */
      template< typename TComp >
      ECS_FORCE_INLINE static TComp* GetComponentRef( Entity e, typename Registry::ComponentStorage< TComp >* storage ) noexcept
      {
         return &storage->m_components[ storage->m_entityToIndex[ e ] ];
      }
   };

   /** @return Iterator to first matching entity. */
   ECS_FORCE_INLINE Iterator begin() noexcept
   {
      Iterator it { &m_registry, 0, m_smallestEntities, m_requiredMask };
      it.storages = m_componentStorages;
      it.Skip();
      return it;
   }
   /** @return End iterator sentinel. */
   ECS_FORCE_INLINE Iterator end() noexcept
   {
      Iterator it { &m_registry, m_smallestEntities ? m_smallestEntities->size() : 0, m_smallestEntities, m_requiredMask };
      it.storages = m_componentStorages;
      return it;
   }

private:
   /** @brief Build aggregated requirement mask from component parameter pack. */
   void BuildRequirementMask() noexcept
   {
      if constexpr( sizeof...( TComponents ) > 0 )
         ( AddRequirementFor< TComponents >(), ... );
   }

   /** @brief Set bit for component type in requirement mask. */
   template< typename TComponent >
   ECS_FORCE_INLINE void AddRequirementFor() noexcept
   {
      m_requiredMask |= ( 1ull << Registry::GetComponentTypeIndex< TComponent >() );
   }

   /** @return Pointer to dense entity vector for component storage (or nullptr if missing). */
   template< typename TComponent >
   ECS_FORCE_INLINE std::vector< Entity >* GetEntitiesVectorFor() const noexcept
   {
      size_t index = Registry::GetComponentTypeIndex< TComponent >();
      if( index >= m_registry.m_componentPools.size() )
         return nullptr;

      auto* pPool = m_registry.m_componentPools[ index ].get();
      return pPool ? &( static_cast< typename Registry::ComponentStorage< TComponent >* >( pPool )->m_entities ) : nullptr;
   }

   /** @brief Update driving pool if candidate is smaller. */
   template< typename TComponent >
   ECS_FORCE_INLINE void SelectIfSmaller( size_t& minSize ) noexcept
   {
      if( auto* vec = GetEntitiesVectorFor< TComponent >(); vec && vec->size() < minSize )
      {
         minSize            = vec->size();
         m_smallestEntities = vec;
      }
   }

   /** @brief Choose smallest component pool as iteration driver. */
   void SelectSmallestPool() noexcept
   {
      if constexpr( sizeof...( TComponents ) > 0 )
      {
         size_t minSize = ( std::numeric_limits< size_t >::max )();
         ( SelectIfSmaller< TComponents >( minSize ), ... );
      }
   }

   /** @brief Cache raw storage pointers for direct, branch‑free access during iteration. */
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

   /** @brief Cache all component storages for the view. */
   ECS_FORCE_INLINE void CacheStorages() noexcept
   {
      if constexpr( sizeof...( TComponents ) > 0 )
         ( CacheStorage< TComponents >(), ... );
   }

   Registry&                                                            m_registry;          ///< Referenced registry.
   std::vector< Entity >*                                               m_smallestEntities { nullptr }; ///< Driving entity vector (smallest component pool).
   uint64_t                                                             m_requiredMask { 0 };          ///< Aggregated mask of required components.
   std::tuple< typename Registry::ComponentStorage< TComponents >*... > m_componentStorages {};         ///< Cached typed storage pointers.
};

// === Specialization: Entity-only view with no component filter (iterates alive entities) ===
/**
 * @brief Specialization for entity-only view without component filtering.
 * @details Captures a snapshot of alive entities at construction (no incremental update while iterating).
 */
template<>
class View< ViewType::Entity >
{
public:
   /** @brief Build entity list snapshot. */
   explicit View( Registry& registry ) noexcept :
      m_registry( registry )
   {
      // Use public API (FValid) and mask size instead of accessing private internals directly.
      const size_t count = m_registry.m_entityMasks.size();
      m_entities.reserve( count );
      for( size_t i = 0; i < count; ++i )
      {
         Entity e = static_cast< Entity >( i );
         if( m_registry.FValid( e ) )
            m_entities.push_back( e );
      }
   }

   /** @brief Forward iterator over entity snapshot. */
   struct Iterator
   {
      const std::vector< Entity >* list { nullptr }; ///< Ptr to entity list.
      size_t                       index { 0 };      ///< Current index.
      ECS_FORCE_INLINE Iterator&   operator++() noexcept
      {
         ++index; return *this;
      }
      ECS_FORCE_INLINE bool   operator!=( const Iterator& rhs ) const noexcept { return index != rhs.index; }
      ECS_FORCE_INLINE Entity operator*() const noexcept { return ( *list )[ index ]; }
   };

   /** @return begin iterator. */
   ECS_FORCE_INLINE Iterator begin() noexcept { return Iterator { &m_entities, 0 }; }
   /** @return end iterator. */
   ECS_FORCE_INLINE Iterator end() noexcept { return Iterator { &m_entities, m_entities.size() }; }

private:
   Registry&             m_registry; ///< Referenced registry.
   std::vector< Entity > m_entities; ///< Snapshot of alive entities.
};

} // namespace Entity

/**
 * @brief std::hash specialization enabling EntityHandle use in unordered containers.
 */
template<>
struct std::hash< Entity::EntityHandle >
{
   size_t operator()( const Entity::EntityHandle& handle ) const noexcept { return std::hash< Entity::Entity > {}( handle.Get() ); }
};
