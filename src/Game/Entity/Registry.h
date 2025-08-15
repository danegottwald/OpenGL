#pragma once

namespace Entity
{

using Entity = uint64_t;

class Registry
{
private:
   using ComponentTypeKey = void*; // Unique identifier for component types, can be a typeid or similar

public:
   /*! @brief Default constructor. */
   Registry() = default;

   /*! @brief Default destructor. */
   ~Registry() = default;

   /*! @brief Default copy constructor and copy assignment operator, copying is not allowed. */
   Registry( const Registry& )            = delete;
   Registry& operator=( const Registry& ) = delete;

   /**
    * @brief Creates a new entity.
    * @return The ID of the newly created entity.
    */
   [[nodiscard]] Entity Create() noexcept
   {
      Entity entity = !m_recycledEntities.empty() ? m_recycledEntities.front() : m_nextEntity++;
      if( !m_recycledEntities.empty() )
         m_recycledEntities.pop();

      m_entityComponentTypes[ entity ] = {}; // Initialize with an empty set of component types
      return entity;
   }

   /**
    * @brief Destroys the entity and all its components.
    * @param entity The entity to destroy.
    */
   void Destroy( Entity entity )
   {
      auto compTypesIter = m_entityComponentTypes.find( entity );
      if( compTypesIter == m_entityComponentTypes.end() )
         return; // Entity does not exist, nothing to destroy

      for( ComponentTypeKey typeKey : compTypesIter->second )
      {
         // The pool MUST exist at this point; otherwise, why do we have a record of the typeKey?
         Pool* pPool = m_storagePools.at( typeKey ).get();
         pPool->Remove( entity );

         // Will invalidate pPool pointer, do not use it after this point
         if( pPool->FEmpty() )
            m_storagePools.erase( typeKey ); // pPool is no longer valid after this point
      }

      m_entityComponentTypes.erase( entity );
      m_recycledEntities.push( entity ); // Recycle the entity for future use
   }

   /**
    * @brief Checks if the entity is valid.
    * @param entity The entity to check.
    * @return True if the entity is valid, false otherwise.
    */
   [[nodiscard]] bool FValid( Entity entity ) const noexcept { return m_entityComponentTypes.contains( entity ); }

   /**
    * @brief Adds a component of the specified type to the entity. Ensures that the entity does not already have the component.
    * @tparam TComponent The type of the component to add.
    * @param entity The entity to which the component will be added.
    * @param args The arguments to construct the component.
    */
   template< typename TComponent, typename... TArgs >
   void AddComponent( Entity entity, TArgs&&... args )
   {
      auto it = m_entityComponentTypes.find( entity );
      if( it == m_entityComponentTypes.end() )
         throw std::runtime_error( std::string( "Entity does not exist: " ) + std::to_string( entity ) );

      ComponentTypeKey typeKey = GetTypeKey< TComponent >();
      if( it->second.contains( typeKey ) )
         throw std::runtime_error( std::string( "Entity already has component: " ) + typeid( TComponent ).name() );

      ComponentStorage< TComponent >& storage = EnsureStorage< TComponent >();
      storage.m_components.emplace_back( std::forward< TArgs >( args )... );
      storage.m_entityAtIndex.push_back( entity );
      storage.m_entityIndexMap[ entity ] = storage.m_components.size() - 1;
      it->second.insert( typeKey );
   }

   /**
    * @brief Removes a component from the entity.
    * @tparam TComponent The type of the component to remove.
    * @param entity The entity from which to remove the component.
    */
   template< typename TComponent >
   void RemoveComponent( Entity entity ) noexcept
   {
      ComponentStorage< TComponent >* pStorage = GetStorage< TComponent >();
      if( !pStorage )
         return; // Storage does not exist for this component type

      pStorage->Remove( entity );
      ComponentTypeKey typeKey = GetTypeKey< TComponent >();
      m_entityComponentTypes[ entity ].erase( typeKey );

      if( pStorage->FEmpty() )
         m_storagePools.erase( typeKey ); // pStorage is no longer valid after this point
   }

   /**
    * @brief Removes multiple components from the entity.
    * @tparam TComponents The types of the components to remove.
    * @param entity The entity from which to remove the components.
    */
   template< typename... TComponents >
   void RemoveComponents( Entity entity )
   {
      ( RemoveComponent< TComponents >( entity ), ... );
   }

   /**
    * @brief Gets a component of the specified type from the entity.
    * @tparam TComponent The type of the component.
    * @param entity The entity from which to get the component.
    * @return A pointer to the component if it exists, nullptr otherwise.
    */
   template< typename TComponent >
   [[nodiscard]] TComponent* GetComponent( Entity entity ) const noexcept
   {
      ComponentStorage< TComponent >* pStorage = GetStorage< TComponent >();
      if( !pStorage )
         return nullptr; // Storage does not exist for this component type

      auto it = pStorage->m_entityIndexMap.find( entity );
      return ( it != pStorage->m_entityIndexMap.end() ) ? &pStorage->m_components.at( it->second ) : nullptr;
   }

   /**
    * @brief Gets multiple components of the specified types from the entity.
    * @tparam TComponents The types of the components.
    * @param entity The entity from which to get the components.
    * @return A tuple of pointers to the components if they exist, nullptr otherwise.
    */
   template< typename... TComponents >
   [[nodiscard]] auto GetComponents( Entity entity ) const noexcept
   {
      return std::tuple< TComponents*... > { GetComponent< TComponents >( entity )... };
   }

   /**
    * @brief Checks if the entity has a component of the specified type.
    * @tparam TComponent The type of the component to check.
    * @param entity The entity to check.
    * @return True if the entity has the component, false otherwise.
    */
   template< typename TComponent >
   [[nodiscard]] bool FHasComponent( Entity entity ) const noexcept
   {
      auto it = m_entityComponentTypes.find( entity );
      return it != m_entityComponentTypes.end() && ( it->second.find( GetTypeKey< TComponent >() ) != it->second.end() );
   }

   /**
    * @brief Checks if the entity has all of the specified components.
    * @tparam TComponents The types of the components to check.
    * @param entity The entity to check.
    * @return True if the entity has all of the components, false otherwise.
    */
   template< typename... TComponents >
   [[nodiscard]] bool FHasComponents( Entity entity ) const noexcept
   {
      return ( FHasComponent< TComponents >( entity ) && ... );
   }

   // clang-format off
   enum class ViewTupleType { Entity, Components, EntityAndComponents };
   template< ViewTupleType TTupleType, typename... TComponents >
   class ViewIterator
   {
   public:
      ViewIterator( Registry& registry ) noexcept :
         m_registry( registry )
      {
         size_t minSize     = ( std::numeric_limits< size_t >::max )();
         auto   checkPoolFn = [ this, &minSize ]< typename TComponent >() noexcept
         {
            auto* storage = m_registry.GetStorage< TComponent >();
            if( !storage || storage->m_entityAtIndex.size() >= minSize )
               return; // Skip this component type if it does not have fewer entities than the current minimum

            minSize          = storage->m_entityAtIndex.size();
            m_pEntityAtIndex = &storage->m_entityAtIndex;
         };
         ( checkPoolFn.template operator()< TComponents >(), ... );
      }

      struct Iterator
      {
         Registry&              registry;
         std::vector< Entity >* entityAtIndex;
         size_t                 index;

         Iterator( Registry& reg, std::vector< Entity >* ents, size_t idx ) noexcept :
            registry( reg ),
            entityAtIndex( ents ),
            index( idx )
         { SkipInvalid(); }

         Iterator& operator++() noexcept { ++index; SkipInvalid(); return *this; }
         bool operator!=( const Iterator& other ) const noexcept { return index != other.index; }
         auto operator*() const noexcept
         {
            Entity entity = ( *entityAtIndex )[ index ];
            if constexpr( TTupleType == ViewTupleType::Entity )
               return entity;
            else if constexpr( TTupleType == ViewTupleType::Components )
               return std::tuple< TComponents&... > { *( registry.GetComponent< TComponents >( entity ) )... };
            else if constexpr( TTupleType == ViewTupleType::EntityAndComponents )
               return std::tuple< Entity, TComponents&... > { entity, *( registry.GetComponent< TComponents >( entity ) )... };
         }

      private:
         void SkipInvalid() noexcept
         {
            while( entityAtIndex && index < entityAtIndex->size() && !registry.FHasComponents< TComponents... >( ( *entityAtIndex )[ index ] ) )
               ++index;
         }
      };

      Iterator begin() noexcept { return Iterator( m_registry, m_pEntityAtIndex, 0 ); }
      Iterator end() noexcept { return Iterator( m_registry, m_pEntityAtIndex, m_pEntityAtIndex ? m_pEntityAtIndex->size() : 0 ); }

   private:
      Registry&              m_registry;
      std::vector< Entity >* m_pEntityAtIndex = nullptr;
   };

private:
   template< ViewTupleType TTupleType, typename... TComponents >
   auto ViewInternal() noexcept { return ViewIterator< TTupleType, TComponents... >( *this ); }

public:
   /**
    * @brief (Entity View) A view into the registry that iterates entities with the specified components, returning just the entity.
    * @tparam TComponents The types of the components to filter by.
    * @return An iterator over entities that have the specified components, returning just the entity.
    */
   template<typename... TComponents>
   auto EView() noexcept { return ViewInternal< ViewTupleType::Entity, TComponents... >(); }

   /**
    * @brief (Component View) A view into the registry that iterates entities with the specified components, returning just the components as a tuple.
    * @tparam TComponents The types of the components to filter by.
    * @return An iterator over entities that have the specified components, returning just the components as a tuple.
    */
   template<typename... TComponents>
   auto CView() noexcept { return ViewInternal< ViewTupleType::Components, TComponents... >(); }

   /**
    * @brief (Entity and Component View) A view into the registry that iterates entities with the specified components, returning both the entity and the components as a tuple.
    * @tparam TComponents The types of the components to filter by.
    * @return An iterator over entities that have the specified components, returning both the entity and the components as a tuple.
    */
   template<typename... TComponents>
   auto ECView() noexcept { return ViewInternal< ViewTupleType::EntityAndComponents, TComponents... >(); }
   // clang-format on

private:
   struct Pool
   {
      virtual ~Pool() noexcept               = default;
      virtual void Remove( Entity ) noexcept = 0;
      virtual bool FEmpty() const noexcept   = 0;
   };

   template< typename TComponent >
   struct ComponentStorage : public Pool
   {
      std::vector< TComponent >            m_components;     // dense array of components
      std::vector< Entity >                m_entityAtIndex;  // maps index -> entity ([index] = entity)
      std::unordered_map< Entity, size_t > m_entityIndexMap; // maps entity -> index ([entity] = index)

      void Remove( Entity entity ) noexcept override
      {
         auto it = m_entityIndexMap.find( entity );
         if( m_components.empty() || it == m_entityIndexMap.end() )
            return;

         size_t entityIndex          = it->second;
         m_components[ entityIndex ] = std::move( m_components.back() ); // swap-and-pop component
         m_components.pop_back();
         m_entityAtIndex[ entityIndex ] = std::move( m_entityAtIndex.back() ); // swap-and-pop entity
         m_entityAtIndex.pop_back();
         m_entityIndexMap.erase( entity ); // remove the entity from the index map
         if( !m_entityAtIndex.empty() && entityIndex < m_entityAtIndex.size() )
            m_entityIndexMap[ m_entityAtIndex[ entityIndex ] ] = entityIndex;
      }

      bool FEmpty() const noexcept override { return m_components.empty(); }
   };

   // Uses the unique type address as a key to identify the component type
   template< typename TComponent >
   ComponentTypeKey GetTypeKey() const noexcept
   {
      static int typeKey;
      return &typeKey;
   }

   template< typename TComponent >
   ComponentStorage< TComponent >& EnsureStorage()
   {
      ComponentTypeKey typeKey = GetTypeKey< TComponent >();
      if( auto it = m_storagePools.find( typeKey ); it != m_storagePools.end() )
         return static_cast< ComponentStorage< TComponent >& >( *it->second );

      return static_cast< ComponentStorage< TComponent >& >(
         *m_storagePools.emplace( typeKey, std::make_unique< ComponentStorage< TComponent > >() ).first->second );
   }

   template< typename TComponent >
   ComponentStorage< TComponent >* GetStorage() const noexcept
   {
      auto it = m_storagePools.find( GetTypeKey< TComponent >() );
      return ( it != m_storagePools.end() ) ? static_cast< ComponentStorage< TComponent >* >( it->second.get() ) : nullptr;
   }

   Entity               m_nextEntity { 1 };
   std::queue< Entity > m_recycledEntities;

   // Component and Entity <-> Component association mappings
   std::unordered_map< ComponentTypeKey, std::unique_ptr< Pool > >      m_storagePools;
   std::unordered_map< Entity, std::unordered_set< ComponentTypeKey > > m_entityComponentTypes;
};

} // namespace Entity
