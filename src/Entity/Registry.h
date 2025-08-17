#pragma once

namespace Entity
{

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
   static size_t GetComponentTypeIndex() noexcept
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
      static constexpr size_t npos = ( std::numeric_limits< size_t >::max )();

      std::vector< TComponent > m_components;    ///< Dense component values.
      std::vector< Entity >     m_entities;      ///< Dense entity list aligned with m_components.
      std::vector< size_t >     m_entityToIndex; ///< Sparse lookup: entity -> dense index or npos.

      /** Ensure sparse index vector can address the given entity id. */
      void EnsureEntityCapacity( Entity entity )
      {
         if( entity >= m_entityToIndex.size() )
            m_entityToIndex.resize( static_cast< size_t >( entity ) + 1, npos );
      }

      /** Emplace pre-constructed component (move). */
      TComponent& Emplace( Entity entity, TComponent&& value )
      {
         EnsureEntityCapacity( entity );
         size_t index = m_components.size();
         m_components.emplace_back( std::move( value ) );
         m_entities.emplace_back( entity );
         m_entityToIndex[ entity ] = index;
         return m_components.back();
      }

      /** Construct component in-place with forwarded args. */
      template< typename... Args >
      TComponent& EmplaceConstruct( Entity entity, Args&&... args )
      {
         EnsureEntityCapacity( entity );
         size_t index = m_components.size();
         m_components.emplace_back( std::forward< Args >( args )... );
         m_entities.emplace_back( entity );
         m_entityToIndex[ entity ] = index;
         return m_components.back();
      }

      /** @return Pointer to component for entity or nullptr if absent. */
      TComponent* Get( Entity entity ) noexcept
      {
         if( entity >= m_entityToIndex.size() )
            return nullptr;

         size_t idx = m_entityToIndex[ entity ];
         return idx == npos ? nullptr : &m_components[ idx ];
      }

      /** @overload const */
      const TComponent* Get( Entity entity ) const noexcept { return Get( entity ); }

      /**
       * @brief Remove component for entity with swap-and-pop; O(1).
       * Preserves contiguity but changes order. Updates sparse indices accordingly.
       */
      void Remove( Entity entity ) noexcept override
      {
         if( entity >= m_entityToIndex.size() )
            return;

         size_t idx = m_entityToIndex[ entity ];
         if( idx == npos )
            return;

         size_t backIdx = m_components.size() - 1;
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

      bool Empty() const noexcept override { return m_components.empty(); }
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
   [[nodiscard]] Entity Create() noexcept
   {
      Entity entity = 0;
      if( !m_recycled.empty() )
      {
         entity = m_recycled.front();
         m_recycled.pop();
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

      ComponentMask& mask = m_entityMasks[ entity ];
      for( size_t blockIndex = 0; blockIndex < mask.size(); ++blockIndex )
      {
         uint64_t bits = mask[ blockIndex ];
         while( bits )
         {
            unsigned long bit;
#if defined( _MSC_VER )
            _BitScanForward64( &bit, bits );
#else
            bit = static_cast< unsigned long >( __builtin_ctzll( bits ) );
#endif

            size_t compIndex = blockIndex * 64 + bit;
            if( compIndex < m_componentPools.size() )
            {
               if( Pool* pPool = m_componentPools[ compIndex ].get() )
                  pPool->Remove( entity );
            }

            bits &= bits - 1; // clear lowest set bit
         }
      }

      mask.clear();
      m_entityAlive[ entity ] = 0;
      m_recycled.push( entity );
   }

   /** @return True if entity id is currently alive. */
   [[nodiscard]] bool FValid( Entity entity ) const noexcept { return entity < m_entityAlive.size() && m_entityAlive[ entity ]; }

   /**
    * @brief Approximate count of live entities.
    * @note This subtracts recycled ids; may not match number of alive flags if fragmentation occurs.
    */
   [[nodiscard]] size_t GetEntityCount() const noexcept { return m_nextEntity - m_recycled.size() - 1; }

   // ---------------- Component Management ----------------
   /**
    * @brief Add or replace a component on an entity.
    * @tparam TComponent Component type.
    * @param entity Target entity (must be valid).
    * @param args Constructor arguments.
    * @return Reference to (new or replaced) component.
    */
   template< typename TComponent, typename... TArgs >
   TComponent& AddComponent( Entity entity, TArgs&&... args )
   {
      assert( FValid( entity ) && "AddComponent on invalid entity" );
      size_t compIndex = GetComponentTypeIndex< TComponent >();
      EnsureComponentPool< TComponent >( compIndex );
      SetMaskBit( entity, compIndex );

      auto* pStorage = static_cast< ComponentStorage< TComponent >* >( m_componentPools[ compIndex ].get() );
      if( TComponent* existing = pStorage->Get( entity ) )
      {
         *existing = TComponent( std::forward< TArgs >( args )... );
         return *existing;
      }

      return pStorage->EmplaceConstruct( entity, std::forward< TArgs >( args )... );
   }

   /**
    * @brief Remove component TComponent from entity if present.
    */
   template< typename TComponent >
   void RemoveComponent( Entity entity ) noexcept
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
   [[nodiscard]] TComponent* GetComponent( Entity entity ) noexcept
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
   [[nodiscard]] std::tuple< TComponents*... > GetComponents( Entity entity ) noexcept
   {
      return std::tuple< TComponents*... > { GetComponent< TComponents >( entity )... };
   }

   /** @overload const */
   template< typename... TComponents >
   [[nodiscard]] std::tuple< const TComponents*... > GetComponents( Entity entity ) const noexcept
   {
      return std::tuple< const TComponents*... > { GetComponent< TComponents >( entity )... };
   }

   /** @return True if entity currently owns component TComponent. */
   template< typename TComponent >
   [[nodiscard]] bool FHasComponent( Entity entity ) const noexcept
   {
      return FValid( entity ) && FTestMaskBit( entity, GetComponentTypeIndex< TComponent >() );
   }

   /** @return True if entity has all listed component types. */
   template< typename... TComponents >
   [[nodiscard]] bool FHasComponents( Entity entity ) const noexcept
   {
      return ( FHasComponent< TComponents >( entity ) && ... );
   }

   // ---------------- Views ----------------
   /** @brief Entity-only view for entities possessing all listed components. */
   template< typename... TComponents >
   [[nodiscard]] auto EView() noexcept
   {
      return View< ViewType::Entity, TComponents... >( *this );
   }
   /** @brief Component tuple view (returns references only). */
   template< typename... TComponents >
   [[nodiscard]] auto CView() noexcept
   {
      return View< ViewType::Components, TComponents... >( *this );
   }
   /** @brief (Entity, Components...) view. */
   template< typename... TComponents >
   [[nodiscard]] auto ECView() noexcept
   {
      return View< ViewType::EntityAndComponents, TComponents... >( *this );
   }

private:
   // ----- Internal helpers -----
   template< typename TComponent >
   void EnsureComponentPool( size_t index )
   {
      if( index >= m_componentPools.size() )
         m_componentPools.resize( index + 1 );

      if( !m_componentPools[ index ] )
         m_componentPools[ index ] = std::make_unique< ComponentStorage< TComponent > >();
   }

   void SetMaskBit( Entity entity, size_t compIndex )
   {
      ComponentMask& mask  = m_entityMasks[ entity ];
      size_t         block = compIndex / 64;
      if( block >= mask.size() )
         mask.resize( block + 1, 0 );

      mask[ block ] |= ( 1ull << ( compIndex & 63 ) );
   }
   void ClearMaskBit( Entity entity, size_t compIndex )
   {
      if( entity >= m_entityMasks.size() )
         return;

      ComponentMask& mask  = m_entityMasks[ entity ];
      size_t         block = compIndex / 64;
      if( block < mask.size() )
         mask[ block ] &= ~( 1ull << ( compIndex & 63 ) );
   }

   bool FTestMaskBit( Entity entity, size_t compIndex ) const noexcept
   {
      if( entity >= m_entityMasks.size() )
         return false;

      const ComponentMask& mask  = m_entityMasks[ entity ];
      size_t               block = compIndex / 64;
      return block < mask.size() && ( mask[ block ] >> ( compIndex & 63 ) ) & 1ull;
   }

   // ----- Data members -----
   Entity                                 m_nextEntity { 1 }; ///< Next entity id to hand out.
   std::queue< Entity >                   m_recycled;         ///< Recycled ids for reuse.
   std::vector< uint8_t >                 m_entityAlive;      ///< Alive flags (0/1) per entity.
   std::vector< ComponentMask >           m_entityMasks;      ///< Per-entity membership bit masks.
   std::vector< std::unique_ptr< Pool > > m_componentPools;   ///< Indexed by component type index.
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
 * @tparam TViewType Output tuple shape (see ViewType).
 * @tparam TComponents Component types required for inclusion.
 */
template< ViewType TViewType, typename... TComponents >
class View
{
public:
   explicit View( Registry& registry ) noexcept :
      m_registry( registry )
   {
      SelectSmallestPool();
   }

   /** @brief Forward iterator over the selected entity set. */
   struct Iterator
   {
      Registry*              registry { nullptr }; ///< Owning registry.
      size_t                 index { 0 };          ///< Current dense index in smallest pool.
      std::vector< Entity >* entities { nullptr }; ///< Pointer to smallest entity vector.

      void Skip() noexcept
      {
         while( entities && index < entities->size() && !registry->template FHasComponents< TComponents... >( ( *entities )[ index ] ) )
            ++index;
      }
      Iterator& operator++() noexcept
      {
         ++index;
         Skip();
         return *this;
      }
      bool operator!=( const Iterator& rhs ) const noexcept { return index != rhs.index; }
      auto operator*() const noexcept
      {
         Entity entity = ( *entities )[ index ];
         if constexpr( TViewType == ViewType::Entity )
            return entity;
         else if constexpr( TViewType == ViewType::Components )
            return std::tuple< TComponents&... > { *( registry->template GetComponent< TComponents >( entity ) )... };
         else
            return std::tuple< Entity, TComponents&... > { entity, *( registry->template GetComponent< TComponents >( entity ) )... };
      }
   };

   Iterator begin() noexcept
   {
      Iterator it { &m_registry, 0, m_smallestEntities };
      it.Skip();
      return it;
   }
   Iterator end() noexcept { return Iterator { &m_registry, m_smallestEntities ? m_smallestEntities->size() : 0, m_smallestEntities }; }

private:
   template< typename TComponent >
   std::vector< Entity >* GetEntitiesVectorFor() const noexcept
   {
      size_t index = Registry::GetComponentTypeIndex< TComponent >();
      if( index >= m_registry.m_componentPools.size() )
         return nullptr;

      auto* pPool = m_registry.m_componentPools[ index ].get();
      return pPool ? &( static_cast< typename Registry::ComponentStorage< TComponent >* >( pPool )->m_entities ) : nullptr;
   }

   void SelectSmallestPool() noexcept
   {
      size_t minSize = ( std::numeric_limits< size_t >::max )();
      ( SelectIfSmaller< TComponents >( minSize ), ... );
   }

   template< typename TComponent >
   void SelectIfSmaller( size_t& minSize ) noexcept
   {
      if( auto* vec = GetEntitiesVectorFor< TComponent >(); vec && vec->size() < minSize )
      {
         minSize            = vec->size();
         m_smallestEntities = vec;
      }
   }

   Registry&              m_registry;                     ///< Referenced registry.
   std::vector< Entity >* m_smallestEntities { nullptr }; ///< Driving dense entity list.
};

} // namespace Entity

/** @brief std::hash specialization for EntityHandle enabling use in unordered containers. */
template<>
struct std::hash< Entity::EntityHandle >
{
   size_t operator()( const Entity::EntityHandle& handle ) const noexcept { return std::hash< Entity::Entity > {}( handle.Get() ); }
};
