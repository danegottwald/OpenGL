#pragma once

namespace Entity
{

using Entity = uint64_t;

class Registry
{
private:
   using EntityIndex      = uint32_t;
   using ComponentTypeKey = void*; // Unique identifier for component types, can be a typeid or similar

public:
   Registry()  = default;
   ~Registry() = default;

   [[nodiscard]] Entity Create() noexcept
   {
      Entity entity = !m_recycledEntities.empty() ? m_recycledEntities.front() : m_nextEntity++;
      if( !m_recycledEntities.empty() )
         m_recycledEntities.pop();

      m_entityComponentTypes[ entity ] = {}; // Initialize with an empty set of component types
      return entity;
   }

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
      m_recycledEntities.push( entity ); // Recycle the entity ID for future use
   }

   // FValid
   //   Checks if the entity is valid, i.e., it has not been destroyed.
   [[nodiscard]] bool FValid( Entity entity ) const noexcept { return m_entityComponentTypes.contains( entity ); }

   // AddComponent
   //   Adds a component to the entity, if the entity already has the component, it will throw an error.
   //   Enforces that each entity can only have one instance of a component type.
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

   // RemoveComponent
   //   Removes the component from the entity if the entity has the component; otherwise, it does nothing.
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

   // RemoveComponents
   //   Removes multiple components from the entity. If the entity does not have a component, it will do nothing for that component.
   template< typename... TComponents >
   void RemoveComponents( Entity entity )
   {
      ( RemoveComponent< TComponents >( entity ), ... );
   }

   // GetComponent
   //   Returns a pointer to the component of the entity. If the entity does not have the component, it returns nullptr.
   template< typename TComponent >
   [[nodiscard]] TComponent* GetComponent( Entity entity ) const noexcept
   {
      ComponentStorage< TComponent >* pStorage = GetStorage< TComponent >();
      if( !pStorage )
         return nullptr; // Storage does not exist for this component type

      auto it = pStorage->m_entityIndexMap.find( entity );
      return ( it != pStorage->m_entityIndexMap.end() ) ? &pStorage->m_components.at( it->second ) : nullptr;
   }

   // GetComponents
   //   Returns a tuple of pointers to the components of the entity. If the entity does not have a component, the pointer will be nullptr.
   template< typename... TComponents >
   [[nodiscard]] auto GetComponents( Entity entity ) const noexcept
   {
      return std::tuple< TComponents*... > { GetComponent< TComponents >( entity )... };
   }

   template< typename TComponent >
   [[nodiscard]] bool FHasComponent( Entity entity ) const noexcept
   {
      auto it = m_entityComponentTypes.find( entity );
      return it != m_entityComponentTypes.end() && ( it->second.find( GetTypeKey< TComponent >() ) != it->second.end() );
   }

   template< typename... TComponents >
   [[nodiscard]] bool FHasComponents( Entity entity ) const noexcept
   {
      return ( FHasComponent< TComponents >( entity ) && ... );
   }

   template< typename... TComponents >
   class ViewIterator
   {
   public:
      ViewIterator( Registry& registry ) noexcept :
         m_registry( registry )
      {
         size_t minSize     = ( std::numeric_limits< size_t >::max )();
         auto   checkPoolFn = [ this, &minSize ]< typename TComponent >() noexcept
         {
            if( auto* storage = m_registry.GetStorage< TComponent >(); storage && storage->m_entityAtIndex.size() < minSize )
            {
               minSize     = storage->m_entityAtIndex.size();
               m_pEntities = &storage->m_entityAtIndex;
            }
         };
         ( checkPoolFn.template operator()< TComponents >(), ... );
      }

      struct Iterator
      {
         Registry&                   registry;
         std::vector< EntityIndex >* entities;
         size_t                      index;

         Iterator( Registry& reg, std::vector< EntityIndex >* ents, size_t idx ) noexcept :
            registry( reg ),
            entities( ents ),
            index( idx )
         {
            SkipInvalid();
         }

         Iterator& operator++() noexcept
         {
            ++index;
            SkipInvalid();
            return *this;
         }
         bool operator!=( const Iterator& other ) const noexcept { return index != other.index; }
         auto operator*() const noexcept { return std::tuple< TComponents&... > { *( registry.GetComponent< TComponents >( ( *entities )[ index ] ) )... }; }

      private:
         void SkipInvalid() noexcept
         {
            while( entities && index < entities->size() && !registry.FHasComponents< TComponents... >( ( *entities )[ index ] ) )
               ++index;
         }
      };

      Iterator begin() noexcept { return Iterator( m_registry, m_pEntities, 0 ); }
      Iterator end() noexcept { return Iterator( m_registry, m_pEntities, m_pEntities ? m_pEntities->size() : 0 ); }

   private:
      Registry&                   m_registry;
      std::vector< EntityIndex >* m_pEntities = nullptr;
   };

   template< typename... TComponents >
   ViewIterator< TComponents... > View() noexcept
   {
      return ViewIterator< TComponents... >( *this );
   }

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
      std::vector< EntityIndex >           m_entityAtIndex;  // maps index -> entity
      std::unordered_map< Entity, size_t > m_entityIndexMap; // maps entity -> index

      void Remove( Entity entity ) noexcept override
      {
         auto it = m_entityIndexMap.find( entity );
         if( m_components.empty() || it == m_entityIndexMap.end() )
            return;

         const EntityIndex index = it->second;
         m_components[ index ]   = std::move( m_components.back() ); // swap-and-pop component
         m_components.pop_back();
         m_entityAtIndex[ index ] = std::move( m_entityAtIndex.back() ); // swap-and-pop entity
         m_entityAtIndex.pop_back();
         m_entityIndexMap.erase( entity ); // remove the entity from the index map
         if( !m_entityAtIndex.empty() && index < m_entityAtIndex.size() )
            m_entityIndexMap[ m_entityAtIndex[ index ] ] = index;
      }

      bool FEmpty() const noexcept override { return m_components.empty(); }
   };

   // Uses the unique type address as a key to identify the component type
   template< typename TComponent >
   ComponentTypeKey GetTypeKey() const noexcept
   {
      // This is a unique key for the GetTypeKey< TComponent > specialization (i.e. defines a unique key for each component type)
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

}
