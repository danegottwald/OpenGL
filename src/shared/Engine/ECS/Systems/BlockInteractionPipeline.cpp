#include "BlockInteractionPipeline.h"

#include <Engine/ECS/Components.h>
#include <Engine/Input/Input.h>
#include <Engine/World/Raycast.h>

namespace Engine::ECS
{

static float GetBreakSpeedMultiplier( Entity::Registry& /*registry*/, Entity::Entity /*player*/ )
{
   // TODO: tools, enchantments, effects, underwater, etc.
   // Return a multiplier on ticks-per-tick (e.g. 1.0 = normal, 2.0 = twice as fast).
   return 1.0f;
}

static Entity::Entity EnsureBlockEntity( Entity::Registry& registry, BlockInteractionResource& res, WorldBlockPos pos, BlockId id )
{
   Entity::Entity existing = res.blockEntities.Find( pos );
   if( existing != Entity::NullEntity && registry.FValid( existing ) )
      return existing;

   Entity::Entity e = registry.Create();
   registry.Add< CBlockEntity >( e, CBlockEntity { .pos = pos, .blockId = id } );

   // Attach default components (data-driven mapping can replace this).
   if( id == BlockId::Furnace )
   {
      registry.Add< CFurnace >( e, CFurnace {} );
      registry.Add< CInventory >( e, CInventory( 3 ) ); // [0]=input, [1]=fuel, [2]=output
   }

   res.blockEntities.Bind( pos, e );
   return e;
}

static void DestroyBlockEntity( Entity::Registry& registry, BlockInteractionResource& res, WorldBlockPos pos )
{
   Entity::Entity e = res.blockEntities.Find( pos );
   if( e != Entity::NullEntity )
      registry.Destroy( e );

   res.blockEntities.Unbind( pos );
}

void BlockIntentSystem::FixedTick( FixedTickContext& ctx )
{
   m_res.ClearIntents();

   auto& registry = ctx.game.RegistryRef();
   for( auto [ player, interactor, tag ] : registry.ECView< CBlockInteractor, CLocalPlayerTag >() )
   {
      const CTransform* pCamTran = registry.TryGet< CTransform >( tag.cameraEntity );
      if( !pCamTran )
         continue;

      const bool leftDown  = Input::FMouseButtonPressed( Input::ButtonLeft );
      const bool rightDown = Input::FMouseButtonPressed( Input::ButtonRight );

      auto optHit = TryRaycast( m_level, CreateRay( pCamTran->position, pCamTran->rotation, interactor.reach ) );
      if( !optHit )
      {
         // When not targeting anything, clear mining target so progress resets.
         if( auto it = m_res.mining.find( player ); it != m_res.mining.end() )
         {
            it->second.fHasTarget       = false;
            it->second.accumulatedTicks = 0;
         }

         interactor.fWasLeftDown  = leftDown;
         interactor.fWasRightDown = rightDown;
         continue;
      }

      const WorldBlockPos target { optHit->block };
      const BlockState    state = m_level.GetBlock( target );
      const BlockPos      key { target.x, target.y, target.z };

      // Track target changes for Minecraft-like mining reset.
      PlayerMiningState& ms = m_res.mining[ player ];
      if( !ms.fHasTarget || ms.target != key )
      {
         ms.fHasTarget       = true;
         ms.target           = key;
         ms.accumulatedTicks = 0;
      }

      if( leftDown )
         m_res.hit.Push( BlockHitEvent { .player = player, .pos = target, .state = state, .faceNormal = optHit->faceNormal } );

      if( rightDown && !interactor.fWasRightDown )
         m_res.use.Push( BlockUseEvent { .player = player, .pos = target, .state = state, .faceNormal = optHit->faceNormal } );

      interactor.fWasLeftDown  = leftDown;
      interactor.fWasRightDown = rightDown;
   }
}

void BlockHitSystem::FixedTick( FixedTickContext& ctx )
{
   auto&          registry = ctx.game.RegistryRef();
   const uint64_t tick     = ctx.game.TimeRef().GetTickCount();
   for( const BlockHitEvent& ev : m_res.hit.Events() )
   {
      const BlockId id = ev.state.GetId();
      if( id == BlockId::Air )
         continue;

      const World::BlockDef& def = World::BlockDefRegistry::Get( id );
      if( def.breakTicks == 0 )
      {
         // Instant break blocks.
         m_res.brk.Push( BlockBreakEvent { .player = ev.player, .pos = ev.pos, .state = ev.state } );
         continue;
      }
      if( def.breakTicks == 0xFFFFFFFFu )
         continue; // unbreakable

      auto msIt = m_res.mining.find( ev.player );
      if( msIt == m_res.mining.end() || !msIt->second.fHasTarget )
         continue;

      const BlockPos key { ev.pos.x, ev.pos.y, ev.pos.z };
      if( msIt->second.target != key )
         continue;

      const float mult   = GetBreakSpeedMultiplier( registry, ev.player );
      const float scaled = std::max( 0.0f, mult );

      // Convert multiplier into integer tick progress.
      const uint32_t addTicks = std::max( 1u, static_cast< uint32_t >( std::floor( scaled ) ) );

      PlayerMiningState& ms = msIt->second;
      ms.accumulatedTicks += addTicks;
      ms.lastHitTick = tick;

      if( ms.accumulatedTicks >= def.breakTicks )
      {
         m_res.brk.Push( BlockBreakEvent { .player = ev.player, .pos = ev.pos, .state = ev.state } );
         ms.accumulatedTicks = 0;
      }
   }

   // Reset mining progress if not hit recently (Minecraft-style).
   constexpr uint64_t RESET_AFTER_TICKS = 7; // ~350ms @ 20tps
   for( auto& [ _, ms ] : m_res.mining )
   {
      if( ms.accumulatedTicks == 0 )
         continue;

      if( ( tick - ms.lastHitTick ) > RESET_AFTER_TICKS )
         ms.accumulatedTicks = 0;
   }
}

static void SpawnItemDrop( Entity::Registry& registry, const glm::ivec3& pos, BlockId id )
{
   auto randomFloatFn = [ & ]( float min, float max ) -> float
   {
      static std::mt19937                     rng( std::random_device {}() ); // persistent RNG
      std::uniform_real_distribution< float > dist( min, max );
      return dist( rng );
   };

   Entity::Entity drop = registry.Create();
   registry.Add< CTransform >( drop, glm::vec3( pos ) + 0.5f );
   registry.Add< CItemDrop >( drop, CItemDrop { .blockId = id } );
   registry.Add< CPhysics >( drop, CPhysics { .bbMin = glm::vec3( -0.125f ), .bbMax = glm::vec3( 0.125f ) } );
   registry.Add< CMesh >( drop, std::make_shared< BlockItemMesh >( id ) );

   // Randomized "shoot out" velocity
   float     angle  = randomFloatFn( 0.0f, 2.0f * glm::pi< float >() );
   float     speed  = randomFloatFn( 1.0f, 2.0f ); // horizontal speed
   float     upward = randomFloatFn( 2.0f, 5.0f ); // upward boost
   glm::vec3 velocity {
      std::cos( angle ) * speed, // x
      upward,                    // y
      std::sin( angle ) * speed  // z
   };
   registry.Add< CVelocity >( drop, velocity );
}

void BlockBreakSystem::FixedTick( FixedTickContext& ctx )
{
   auto& registry = ctx.game.RegistryRef();
   for( const BlockBreakEvent& ev : m_res.brk.Events() )
   {
      const BlockState current = m_level.GetBlock( ev.pos );
      const BlockId    id      = current.GetId();
      if( id == BlockId::Air )
         continue;

      const World::BlockDef& def = World::BlockDefRegistry::Get( id );
      if( def.OnBroken )
         def.OnBroken( m_level, ev.pos );

      // TODO: drops should be block-def-driven and tool-aware.
      SpawnItemDrop( registry, ev.pos.ToIVec3(), id );

      if( def.hasBlockEntity )
         DestroyBlockEntity( registry, m_res, ev.pos );

      m_level.SetBlock( ev.pos, BlockState( BlockId::Air ) );

      // Clear player mining progress if they were mining this block.
      const BlockPos key { ev.pos.x, ev.pos.y, ev.pos.z };
      for( auto& [ _, ms ] : m_res.mining )
      {
         if( ms.fHasTarget && ms.target == key )
            ms.accumulatedTicks = 0;
      }

      // Multiplayer hook (future): server broadcasts authoritative block change + drop spawns.
   }
}

void BlockUseSystem::FixedTick( FixedTickContext& ctx )
{
   auto& registry = ctx.game.RegistryRef();
   for( const BlockUseEvent& ev : m_res.use.Events() )
   {
      const BlockState current = m_level.GetBlock( ev.pos );
      const BlockId    id      = current.GetId();
      if( id == BlockId::Air )
         continue;

      const World::BlockDef& def = World::BlockDefRegistry::Get( id );
      if( def.hasBlockEntity )
      {
         Entity::Entity be = EnsureBlockEntity( registry, m_res, ev.pos, id );
         m_res.open.Push( OpenBlockEntityEvent { .player = ev.player, .blockEntity = be } );
         continue;
      }

      if( def.openable )
      {
         // Placeholder until BlockState supports richer properties.
         const auto      props    = current.GetProperties();
         BlockProperties newProps = props;
         newProps.orientation     = static_cast< BlockOrientation >( ( static_cast< int >( props.orientation ) + 1 ) % 6 );
         m_level.SetBlock( ev.pos, BlockState( newProps ) );
      }
   }
}

void BlockEntityInteractSystem::Tick( TickContext& ctx )
{
   auto& registry = ctx.game.RegistryRef();
   for( const OpenBlockEntityEvent& ev : m_res.open.Events() )
   {
      if( !registry.FValid( ev.blockEntity ) )
         continue;

      if( registry.FHas< CFurnace >( ev.blockEntity ) )
      {
         // TODO: open furnace UI
      }
      else if( registry.FHas< CInventory >( ev.blockEntity ) )
      {
         // TODO: open inventory UI
      }
   }
}

} // namespace Engine::ECS
