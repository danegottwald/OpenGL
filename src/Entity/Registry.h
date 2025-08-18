#pragma once

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
 *
 * The optimized registry no longer automatically unbinds external handles on destruction for raw speed.
 * A handle simply exposes the numeric entity value and comparison operators. If the underlying entity
 * is destroyed, the handle becomes stale (user code should re‑validate via Registry::FValid before use).
 */
class EntityHandle
{
private:
   friend class Registry;
   EntityHandle( Entity entity, Registry& registry ) :
      m_entity( entity ),
      m_pRegistry( &registry )
   {}
   /// @brief Internal: clear registry pointer so subsequent use can detect invalidation.
   void Unbind() noexcept { m_pRegistry = nullptr; }

public:
   ~EntityHandle();
   EntityHandle( const EntityHandle& )                = delete; ///< Copying disabled.
   EntityHandle( EntityHandle&& ) noexcept            = delete; ///< Moving disabled (keep semantics simple).
   EntityHandle& operator=( EntityHandle&& ) noexcept = delete; ///< Move assignment disabled.
   EntityHandle& operator=( const EntityHandle& )     = delete; ///< Copy assignment disabled.

   /** @return The raw Entity ID. */
   [[nodiscard]] Entity Get() const noexcept { return m_entity; }
   /// @brief Implicit conversion to raw entity id.
   explicit operator Entity() const noexcept { return m_entity; }

   // Comparisons
   bool operator==( const EntityHandle& other ) const noexcept { return m_entity == other.m_entity; }
   bool operator!=( const EntityHandle& other ) const noexcept { return m_entity != other.m_entity; }
   bool operator==( Entity other ) const noexcept { return m_entity == other; }
   bool operator!=( Entity other ) const noexcept { return m_entity != other; }
   bool operator<( const EntityHandle& other ) const noexcept { return m_entity < other.m_entity; }

private:
   const Entity m_entity { 0 };          ///< Stored entity id.
   Registry*    m_pRegistry { nullptr }; ///< Pointer to owning registry (may be null after Unbind()).
};

// =============================================================
// ViewType
// =============================================================
/**
 * @enum ViewType
 * @brief Enumerates the output tuple shape for a view iteration.
 * - Entity: yields only the Entity id
 * - Components: yields references to the requested components (tuple)
 * - EntityAndComponents: yields (Entity, components...)
 */
enum class ViewType
{
   Entity,
   Components,
   EntityAndComponents
};

// =============================================================
// Registry
// =============================================================
/**
 * @class Registry
 * @brief High‑performance ECS registry focused on fast iteration & cache locality.
 *
 * Key differences vs the earlier version:
 *  - Component type dispatch uses a compact, incremental type index instead of unordered_map / void* keys.
 *  - Per entity component membership is encoded as a variable-length bit mask (vector<uint64_t>) enabling
 *    O(1) membership tests with simple bit operations.
 *  - Each component type uses a packed (SoA-like) storage: dense component array + parallel entity array + sparse index.
 *  - Add / remove are O(1) swap-and-pop operations; iteration chooses the smallest component pool to minimize scans.
 *  - No per-entity unordered_set of component types, eliminating hashing & dynamic allocations in hot paths.
 *
 * Design objectives: minimal branching, predictable memory access, zero virtual calls in inner loops (except removal),
 * and avoiding iterator adaptor overhead. The trade-off is that some safety features (like automatic handle unbinding)
 * are reduced for raw speed.
 *
 * Usage notes:
 *  - Always check FValid(entity) if using stored entity ids outside the frame they were created.
 *  - Adding an already-present component overwrites it in-place (reconstruct assignment semantics).
 *  - Removing a component invalidates references to the moved-from last element (swap/pop semantics).
 */
class Registry
{
private:
   template< ViewType TViewType, typename... TComponents >
   friend class View;

   // ------------------------------------------------------------------
   // Component type indexing
   // ------------------------------------------------------------------
   static inline size_t s_nextComponentTypeIndex = 0; ///< Monotonic counter assigning indices to component types.

   /**
    * @brief Returns the stable unique index for the component type TComponent.
    * @tparam TComponent Component type.
    */
   template< typename TComponent >
   ECS_FORCE_INLINE static size_t GetComponentTypeIndex() noexcept
   {
      static const size_t s_index = s_nextComponentTypeIndex++;
      return s_index;
   }

   // Bit mask per entity (vector of 64-bit blocks); grows only when new component indices exceed capacity.
   using ComponentMask = std::vector< uint64_t >;

   // Base pool for type erasure during destruction.
   struct Pool
   {
      virtual ~Pool()                        = default;
      virtual void Remove( Entity ) noexcept = 0; ///< Remove component for entity if present.
      virtual bool Empty() const noexcept    = 0; ///< @return true if storage contains zero components.
   };

   /**
    * @brief Dense component storage with O(1) add/remove and sparse lookup.
    * Layout:
    *  - m_components: tightly packed component values.
    *  - m_entities: parallel array mapping dense index -> entity.
    *  - m_entityToIndex: sparse array mapping entity -> dense index (npos if absent).
    */
   template< typename TComponent >
   struct ComponentStorage final : Pool
   {
      using DenseIndexType                 = uint32_t; // more cache friendly than size_t for large sparse arrays
      static constexpr DenseIndexType npos = ( std::numeric_limits< DenseIndexType >::max )();

      std::vector< TComponent >     m_components;    ///< Dense component values.
      std::vector< Entity >         m_entities;      ///< Dense entity list aligned with m_components.
      std::vector< DenseIndexType > m_entityToIndex; ///< Sparse lookup: entity -> dense index or npos.

      /** Ensure sparse index vector can address the given entity id. */
      ECS_FORCE_INLINE void EnsureEntityCapacity( Entity entity )
      {
         if( entity >= m_entityToIndex.size() )
            m_entityToIndex.resize( static_cast< size_t >( entity ) + 1, npos );
      }

      /** Emplace pre-constructed component (move). */
      ECS_FORCE_INLINE TComponent& Emplace( Entity entity, TComponent&& value )
      {
         EnsureEntityCapacity( entity );
         DenseIndexType index = static_cast< DenseIndexType >( m_components.size() );
         m_components.emplace_back( std::move( value ) );
         m_entities.emplace_back( entity );
         m_entityToIndex[ entity ] = index;
         return m_components.back();
      }

      /** Construct component in-place with forwarded args. */
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

         DenseIndexType idx = m_entityToIndex[ entity ];
         return idx == npos ? nullptr : &m_components[ idx ];
      }

      /** @overload const */
      const TComponent* Get( Entity entity ) const noexcept { return Get( entity ); }

      /**
       * @brief Remove component for entity with swap-and-pop; O(1).
       * Preserves contiguity but changes order. Updates sparse indices accordingly.
       */
      ECS_FORCE_INLINE void Remove( Entity entity ) noexcept override
      {
         if( entity >= m_entityToIndex.size() )
            return;

         DenseIndexType idx = m_entityToIndex[ entity ];
         if( idx == npos )
            return;

         DenseIndexType backIdx = static_cast< DenseIndexType >( m_components.size() - 1 );
         if( idx != backIdx )
         {
            m_components[ idx ]            = std::move( m_components[ backIdx ] );
            Entity movedEntity             = m_entities[ backIdx ];
            m_entities[ idx ]              = movedEntity;
            m_entityToIndex[ movedEntity ] = idx;
         }

         m_components.pop_back();
         m_entities.pop_back();
         m_entityToIndex[ entity ] = npos;
      }

      ECS_FORCE_INLINE bool Empty() const noexcept override { return m_components.empty(); }
   };

public:
   Registry()  = default; ///< Default constructor.
   ~Registry() = default; ///< Trivial destructor (fast, no handle sweeping).

   Registry( const Registry& )            = delete; ///< Non-copyable.
   Registry& operator=( const Registry& ) = delete; ///< Non-assignable.

   // ---------------- Entity Management ----------------
   /**
    * @brief Create a new entity id (reusing recycled ids when available).
    * @return Newly created entity id.
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
         m_entityMasks.resize( static_cast< size_t >( entity ) + 1 );
      }

      m_entityAlive[ entity ] = 1;
      return entity;
   }

   /**
    * @brief Create an entity and wrap it in an EntityHandle.
    * @warning Handle invalidation is not automatic on destruction for performance.
    */
   [[nodiscard]] std::shared_ptr< EntityHandle > CreateWithHandle() noexcept { return std::shared_ptr< EntityHandle >( new EntityHandle( Create(), *this ) ); }

   /**
    * @brief Destroy an entity and all its components (O(#component types set)).
    * @param entity Entity to destroy (ignored if invalid).
    */
   void Destroy( Entity entity ) noexcept
   {
      if( !FValid( entity ) )
         return;

      ComponentMask&                     mask      = m_entityMasks[ entity ];
      std::unique_ptr< Registry::Pool >* pPools    = m_componentPools.data(); // pointer to first unique_ptr
      const size_t                       poolCount = m_componentPools.size();
      for( size_t blockIndex = 0; blockIndex < mask.size(); ++blockIndex )
      {
         uint64_t bits = mask[ blockIndex ];
         while( bits )
         {
            size_t compIndex = ( blockIndex * 64 ) + std::countr_zero( bits );
            if( compIndex < poolCount )
            {
               Pool* pPool = pPools[ compIndex ].get();
               if( pPool )
                  pPool->Remove( entity );
            }

            bits &= bits - 1; // clear lowest set bit
         }
         mask[ blockIndex ] = 0; // keep capacity, zero out (avoids reallocation on reuse)
      }

      // retain capacity instead of mask.clear() to reduce future reallocations (micro-optimization)
      m_entityAlive[ entity ] = 0;
      m_recycled.push_back( entity );
   }

   /** @return True if entity id is currently alive. */
   [[nodiscard]] ECS_FORCE_INLINE bool FValid( Entity entity ) const noexcept { return entity < m_entityAlive.size() && m_entityAlive[ entity ]; }

   /**
    * @brief Approximate count of live entities.
    * @note This subtracts recycled ids; may not match number of alive flags if fragmentation occurs.
    */
   [[nodiscard]] ECS_FORCE_INLINE size_t GetEntityCount() const noexcept { return m_nextEntity - m_recycled.size() - 1; }

   // ---------------- Component Management ----------------
   /**
    * @brief Add or replace a component on an entity.
    * @tparam TComponent Component type.
    * @param entity Target entity (must be valid).
    * @param args Constructor arguments.
    * @return Reference to (new or replaced) component.
    */
   template< typename TComponent, typename... TArgs >
   ECS_FORCE_INLINE TComponent& AddComponent( Entity entity, TArgs&&... args )
   {
      assert( FValid( entity ) && "AddComponent on invalid entity" );
      size_t compIndex = GetComponentTypeIndex< TComponent >();
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
    * @brief Remove component TComponent from entity if present.
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

   /** @return Pointer to component TComponent for entity or nullptr. */
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

   /** @overload const */
   template< typename TComponent >
   [[nodiscard]] const TComponent* GetComponent( Entity entity ) const noexcept
   {
      return const_cast< Registry* >( this )->GetComponent< TComponent >( entity );
   }

   /** @return Pointer to components TComponents... for entity as a tuple. Missing components yield nullptr. */
   template< typename... TComponents >
   [[nodiscard]] ECS_FORCE_INLINE std::tuple< TComponents*... > GetComponents( Entity entity ) noexcept
   {
      return std::tuple< TComponents*... > { GetComponent< TComponents >( entity )... };
   }

   /** @overload const */
   template< typename... TComponents >
   [[nodiscard]] ECS_FORCE_INLINE std::tuple< const TComponents*... > GetComponents( Entity entity ) const noexcept
   {
      return std::tuple< const TComponents*... > { GetComponent< TComponents >( entity )... };
   }

   /** @return True if entity currently owns component TComponent. */
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

   // ---------------- Views ----------------
   /** @brief Entity-only view for entities possessing all listed components. */
   template< typename... TComponents >
   [[nodiscard]] ECS_FORCE_INLINE auto EView() noexcept
   {
      return View< ViewType::Entity, TComponents... >( *this );
   }
   /** @brief Component tuple view (returns references only). */
   template< typename... TComponents >
   [[nodiscard]] ECS_FORCE_INLINE auto CView() noexcept
   {
      return View< ViewType::Components, TComponents... >( *this );
   }
   /** @brief (Entity, Components...) view. */
   template< typename... TComponents >
   [[nodiscard]] ECS_FORCE_INLINE auto ECView() noexcept
   {
      return View< ViewType::EntityAndComponents, TComponents... >( *this );
   }

private:
   // ----- Internal helpers -----
   template< typename TComponent >
   ECS_FORCE_INLINE void EnsureComponentPool( size_t index )
   {
      if( index >= m_componentPools.size() )
         m_componentPools.resize( index + 1 );

      if( !m_componentPools[ index ] )
         m_componentPools[ index ] = std::make_unique< ComponentStorage< TComponent > >();
   }

   ECS_FORCE_INLINE void SetMaskBit( Entity entity, size_t compIndex )
   {
      ComponentMask& mask  = m_entityMasks[ entity ];
      size_t         block = compIndex / 64;
      if( block >= mask.size() )
         mask.resize( block + 1, 0 );

      mask[ block ] |= ( 1ull << ( compIndex & 63 ) );
   }

   ECS_FORCE_INLINE void ClearMaskBit( Entity entity, size_t compIndex )
   {
      if( entity >= m_entityMasks.size() )
         return;

      ComponentMask& mask  = m_entityMasks[ entity ];
      size_t         block = compIndex / 64;
      if( block < mask.size() )
         mask[ block ] &= ~( 1ull << ( compIndex & 63 ) );
   }

   ECS_FORCE_INLINE bool FTestMaskBit( Entity entity, size_t compIndex ) const noexcept
   {
      if( entity >= m_entityMasks.size() )
         return false;

      const ComponentMask& mask  = m_entityMasks[ entity ];
      size_t               block = compIndex / 64;
      return block < mask.size() && ( mask[ block ] >> ( compIndex & 63 ) ) & 1ull;
   }

   // ----- Data members -----
   Entity                                 m_nextEntity { 1 };
   std::vector< Entity >                  m_recycled;
   std::vector< uint8_t >                 m_entityAlive;
   std::vector< ComponentMask >           m_entityMasks;
   std::vector< std::unique_ptr< Pool > > m_componentPools;
};

// =============================================================
// View
// =============================================================
/**
 * @class View
 * @brief Lightweight iterable for entities holding a set of components.
 *
 * Chooses the smallest component pool among the requested types as the driving range to minimize iteration cost.
 * Each ++ skips entities missing any required component via fast bit tests (FHasComponents chain inlined / constexpr).
 *
 * Optimized view: precomputes component indices, aggregated bit requirements per 64-bit block,
 * and direct pointers to component storages so iteration avoids repeated virtual/lookups.
 *
 * @tparam TViewType Output tuple shape (see ViewType).
 * @tparam TComponents Component types required for inclusion.
 */
template< ViewType TViewType, typename... TComponents >
class View
{
   struct BitRequirement
   {
      size_t   block; ///< 64-bit block index
      uint64_t mask;  ///< aggregated bits required in this block
   };

public:
   explicit View( Registry& registry ) noexcept :
      m_registry( registry )
   {
      BuildRequirements();
      SelectSmallestPool();
      CacheStorages();

      m_requirementsPtr  = m_requirements.empty() ? nullptr : m_requirements.data();
      m_requirementCount = m_requirements.size();
   }

   /** @brief Forward iterator over the selected entity set. */
   struct Iterator
   {
      Registry*              registry { nullptr };
      size_t                 index { 0 };
      std::vector< Entity >* entities { nullptr };

      const BitRequirement*                                                reqPtr { nullptr }; ///< raw pointer to requirements array
      size_t                                                               reqCount { 0 };     ///< number of requirement entries
      std::tuple< typename Registry::ComponentStorage< TComponents >*... > storages;           // filled in view

      ECS_FORCE_INLINE bool HasAll( Entity e ) const noexcept
      {
         if( reqCount == 0 )
            return true;

         const auto& maskVec = registry->m_entityMasks[ e ];
         if( reqCount == 1 ) // single-block fast path
         {
            const auto& r = reqPtr[ 0 ];
            return r.block < maskVec.size() && ( maskVec[ r.block ] & r.mask ) == r.mask;
         }

         for( size_t i = 0; i < reqCount; ++i )
         {
            const auto& r = reqPtr[ i ];
            if( r.block >= maskVec.size() )
               return false;

            if( ( maskVec[ r.block ] & r.mask ) != r.mask )
               return false;
         }

         return true;
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
            return std::tuple_cat( std::tuple< Entity > { entity }, DerefComponents( entity, std::index_sequence_for< TComponents... > {} ) );
      }

      template< std::size_t... I >
      ECS_FORCE_INLINE auto DerefComponents( Entity e, std::index_sequence< I... > ) const noexcept
      {
         return std::tuple< TComponents&... > { *GetComponentRef< TComponents >( e, std::get< I >( storages ) )... };
      }

      template< typename TComp >
      ECS_FORCE_INLINE static TComp* GetComponentRef( Entity e, typename Registry::ComponentStorage< TComp >* storage ) noexcept
      {
         auto idx = storage->m_entityToIndex[ e ];
         return &storage->m_components[ idx ];
      }
   };

   ECS_FORCE_INLINE Iterator begin() noexcept
   {
      Iterator it { &m_registry, 0, m_smallestEntities, m_requirementsPtr, m_requirementCount };
      it.storages = m_componentStorages; // copy pre-cached storages
      it.Skip();
      return it;
   }
   ECS_FORCE_INLINE Iterator end() noexcept
   {
      Iterator it { &m_registry, m_smallestEntities ? m_smallestEntities->size() : 0, m_smallestEntities, m_requirementsPtr, m_requirementCount };
      it.storages = m_componentStorages;
      return it;
   }

private:
   void BuildRequirements() noexcept
   {
      m_requirements.reserve( sizeof...( TComponents ) );
      ( AddRequirementFor< TComponents >(), ... );
      if constexpr( sizeof...( TComponents ) > 1 )
      {
         std::sort( m_requirements.begin(), m_requirements.end(), []( const BitRequirement& a, const BitRequirement& b ) { return a.block < b.block; } );
         std::vector< BitRequirement > compact;
         compact.reserve( m_requirements.size() );
         for( const auto& r : m_requirements )
         {
            if( compact.empty() || compact.back().block != r.block )
               compact.push_back( r );
            else
               compact.back().mask |= r.mask;
         }

         m_requirements.swap( compact );
      }
   }

   template< typename TComponent >
   ECS_FORCE_INLINE void AddRequirementFor() noexcept
   {
      size_t   compIndex = Registry::GetComponentTypeIndex< TComponent >();
      size_t   block     = compIndex / 64;
      uint64_t bit       = 1ull << ( compIndex & 63 );
      m_requirements.push_back( { block, bit } );
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
      size_t minSize = ( std::numeric_limits< size_t >::max )();
      ( SelectIfSmaller< TComponents >( minSize ), ... );
   }

   template< typename TComponent >
   ECS_FORCE_INLINE void CacheStorage() noexcept
   {
      size_t index = Registry::GetComponentTypeIndex< TComponent >();
      if( index < m_registry.m_componentPools.size() )
      {
         auto* pPool = m_registry.m_componentPools[ index ].get();
         if( pPool )
            std::get< Registry::ComponentStorage< TComponent >* >( m_componentStorages ) = static_cast< Registry::ComponentStorage< TComponent >* >( pPool );
      }
   }

   ECS_FORCE_INLINE void CacheStorages() noexcept { ( CacheStorage< TComponents >(), ... ); }

   Registry&                                                            m_registry;
   std::vector< Entity >*                                               m_smallestEntities { nullptr };
   const BitRequirement*                                                m_requirementsPtr { nullptr }; ///< raw pointer for iteration
   size_t                                                               m_requirementCount { 0 };      ///< count of requirement entries
   std::vector< BitRequirement >                                        m_requirements;                ///< owned requirements
   std::tuple< typename Registry::ComponentStorage< TComponents >*... > m_componentStorages {};        ///< cached storages
};

} // namespace Entity

/** @brief std::hash specialization for EntityHandle enabling use in unordered containers. */
template<>
struct std::hash< Entity::EntityHandle >
{
   size_t operator()( const Entity::EntityHandle& handle ) const noexcept { return std::hash< Entity::Entity > {}( handle.Get() ); }
};
