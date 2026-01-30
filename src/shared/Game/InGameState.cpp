#include "InGameState.h"

// Project dependencies
#include <Engine/Platform/Window.h>
#include <Engine/ECS/Components.h>
#include <Engine/ECS/Systems/BlockInteractionPipeline.h>
#include <Engine/ECS/Systems/FurnaceSystem.h>
#include <Engine/Events/ApplicationEvent.h>
#include <Engine/Events/KeyEvent.h>
#include <Engine/Events/MouseEvent.h>
#include <Engine/Events/NetworkEvent.h>
#include <Engine/Input/Input.h>
#include <Engine/Network/Network.h>
#include <Engine/Renderer/Texture.h>
#include <Engine/World/Raycast.h>
#include <Engine/Physics/EntityCollisionSystem.h>

#include <Client/Network/NetworkUI.h>

namespace
{
constexpr float GRAVITY           = -32.0f;
constexpr float TERMINAL_VELOCITY = -48.0f;
constexpr float JUMP_VELOCITY     = 9.0f;
constexpr float PLAYER_HEIGHT     = 1.8f;
constexpr float PLAYER_EYE_HEIGHT = 1.62f;
constexpr float GROUND_MAXSPEED   = 4.3f;
constexpr float SPRINT_MODIFIER   = 1.3f;

static void MouseLookSystem( Entity::Registry& registry, Window& window )
{
   for( auto [ look ] : registry.CView< CLookInput >() )
   {
      look.yawDelta   = 0.0f;
      look.pitchDelta = 0.0f;
   }

   const WindowState& state = window.GetWindowState();
   if( state.fMouseCaptured && state.mouseDelta != glm::vec2( 0.0f ) )
   {
      for( auto [ cam, look ] : registry.CView< CCamera, CLookInput >() )
      {
         look.pitchDelta = state.mouseDelta.y * cam.sensitivity;
         look.yawDelta   = state.mouseDelta.x * cam.sensitivity;
      }
   }
}

static void CameraRigSystem( Entity::Registry& registry, float alpha )
{
   for( auto [ camEntity, camTran, rig ] : registry.ECView< CTransform, CCameraRig >() )
   {
      CTransform* pTargetTran = registry.TryGet< CTransform >( rig.targetEntity );
      if( !pTargetTran )
         continue;

      if( auto [ pCam, pLook ] = registry.TryGet< CCamera, CLookInput >( camEntity ); pCam && pLook )
      {
         camTran.rotation.x = glm::clamp( camTran.rotation.x + pLook->pitchDelta, -90.0f, 90.0f );
         camTran.rotation.y = glm::mod( camTran.rotation.y + pLook->yawDelta + 360.0f, 360.0f );
      }

      if( rig.followYaw )
         pTargetTran->rotation.y = camTran.rotation.y;
      if( rig.followPitch )
         pTargetTran->rotation.x = camTran.rotation.x;

      // Interpolate target position for smooth camera follow
      const glm::vec3 interpolatedTargetPos = glm::mix( pTargetTran->prevPosition, pTargetTran->position, alpha );
      camTran.position                      = interpolatedTargetPos + rig.offset;

      camTran.RecordPrev(); // Record prev after updating position/rotation
   }
}

static void LocalInputPollSystem( Entity::Registry& registry )
{
   for( auto [ input, tag ] : registry.CView< CInput, CLocalPlayerTag >() )
   {
      input.movement = { 0.0f, 0.0f };
      if( Input::FKeyPressed( Input::W ) )
         input.movement.y += 1.0f;
      if( Input::FKeyPressed( Input::S ) )
         input.movement.y -= 1.0f;
      if( Input::FKeyPressed( Input::D ) )
         input.movement.x += 1.0f;
      if( Input::FKeyPressed( Input::A ) )
         input.movement.x -= 1.0f;

      input.fJumpRequest   = Input::FKeyPressed( Input::Space );
      input.fSprintRequest = Input::FKeyPressed( Input::LeftShift );
   }
}

static void PlayerMovementSystem( Entity::Registry& registry )
{
   for( auto [ e, tran, vel, input ] : registry.ECView< CTransform, CVelocity, CInput >() )
   {
      const float     yawRad = glm::radians( tran.rotation.y + 90.0f );
      const glm::vec3 forward { -glm::cos( yawRad ), 0.0f, -glm::sin( yawRad ) };
      const glm::vec3 right { glm::sin( yawRad ), 0.0f, -glm::cos( yawRad ) };
      glm::vec3       wishDir = forward * input.movement.y + right * input.movement.x;
      if( glm::length( wishDir ) > 0.0f )
         wishDir = glm::normalize( wishDir );

      float maxSpeed = GROUND_MAXSPEED;
      if( input.fSprintRequest )
         maxSpeed *= SPRINT_MODIFIER;

      bool fOnGround = true;
      if( const CPhysics* pPhys = registry.TryGet< CPhysics >( e ) )
         fOnGround = pPhys->fOnGround;

      if( fOnGround )
      {
         vel.velocity.x = wishDir.x * maxSpeed;
         vel.velocity.z = wishDir.z * maxSpeed;
      }
      else
      {
         // Apply air control (acceleration + drag)
         constexpr float AIR_ACCEL = 0.5f;
         constexpr float AIR_DRAG  = 0.91f;
         vel.velocity.x            = ( vel.velocity.x + wishDir.x * AIR_ACCEL ) * AIR_DRAG;
         vel.velocity.z            = ( vel.velocity.z + wishDir.z * AIR_ACCEL ) * AIR_DRAG;
      }

      { // Jumping
         if( input.jumpCooldown > 0 )
            input.jumpCooldown--;

         if( fOnGround )
         {
            input.jumpCooldown /= 1.35f;
            if( input.fJumpRequest && input.jumpCooldown == 0 )
            {
               vel.velocity.y     = JUMP_VELOCITY;
               input.jumpCooldown = 10; // ticks
            }
         }

         input.fWasJumpDown = input.fJumpRequest;
      }
   }
}

static void PhysicsSystem( Entity::Registry& registry, Level& level, float tickInterval )
{
   auto getVoxelBounds = [ & ]( const glm::vec3& pos, const glm::vec3& bbMin, const glm::vec3& bbMax ) -> std::tuple< glm::ivec3, glm::ivec3 >
   {
      constexpr float EPS = 1e-4f;
      return {
         glm::ivec3 { ( int )std::floor( pos.x + bbMin.x + EPS ), ( int )std::floor( pos.y + bbMin.y + EPS ), ( int )std::floor( pos.z + bbMin.z + EPS ) },
         glm::ivec3 { ( int )std::floor( pos.x + bbMax.x - EPS ), ( int )std::floor( pos.y + bbMax.y - EPS ), ( int )std::floor( pos.z + bbMax.z - EPS ) }
      };
   };

   auto computeGrounded = [ & ]( const glm::vec3& pos, const glm::vec3& bbMin, const glm::vec3& bbMax ) -> bool
   {
      constexpr float GROUND_PROBE = 0.05f;
      glm::vec3       probePos     = pos;
      probePos.y -= GROUND_PROBE;

      auto [ min, max ] = getVoxelBounds( probePos, bbMin, bbMax );
      for( int y = min.y; y <= max.y; ++y )
         for( int x = min.x; x <= max.x; ++x )
            for( int z = min.z; z <= max.z; ++z )
               if( FSolid( level.GetBlock( WorldBlockPos { x, y, z } ) ) )
                  return true;

      return false;
   };

   enum Axis
   {
      X = 0,
      Y = 1,
      Z = 2
   };

   auto moveAndCollideAxis = [ & ]( glm::vec3& pos, glm::vec3& vel, CPhysics& phys, float d, Axis axis )
   {
      if( d == 0.0f )
         return;

      pos[ axis ] += d;
      auto [ min, max ]   = getVoxelBounds( pos, phys.bbMin, phys.bbMax );
      const bool positive = d > 0.0f;
      int        hit      = positive ? INT_MAX : INT_MIN;

      for( int y = min.y; y <= max.y; ++y )
      {
         for( int x = min.x; x <= max.x; ++x )
         {
            for( int z = min.z; z <= max.z; ++z )
            {
               if( !FSolid( level.GetBlock( WorldBlockPos { x, y, z } ) ) )
                  continue;

               switch( axis )
               {
                  case Axis::X: hit = positive ? ( std::min )( hit, x ) : ( std::max )( hit, x ); break;
                  case Axis::Y: hit = positive ? ( std::min )( hit, y ) : ( std::max )( hit, y ); break;
                  case Axis::Z: hit = positive ? ( std::min )( hit, z ) : ( std::max )( hit, z ); break;
               }
            }
         }
      }

      constexpr float SKIN = 0.001f;
      if( positive )
      {
         if( hit == INT_MAX )
            return;
         pos[ axis ] = ( float )hit - phys.bbMax[ axis ] - SKIN;
      }
      else
      {
         if( hit == INT_MIN )
            return;
         pos[ axis ] = ( float )( hit + 1 ) - phys.bbMin[ axis ] + SKIN;
      }

      const float restitution = std::clamp( phys.bounciness, 0.0f, 1.0f );
      vel[ axis ]             = -vel[ axis ] * restitution;

      if( std::abs( vel[ axis ] ) < 0.01f )
         vel[ axis ] = 0.0f;

      if( axis == Axis::Y && !positive && restitution < 0.5f )
         phys.fOnGround = true;
   };

   for( auto [ entity, tran, vel, phys ] : registry.ECView< CTransform, CVelocity, CPhysics >() )
   {
      tran.RecordPrev(); // Record previous transform state for interpolation

      phys.fOnGround = computeGrounded( tran.position, phys.bbMin, phys.bbMax );
      if( !phys.fOnGround )
         vel.velocity.y = ( std::max )( vel.velocity.y + ( GRAVITY * tickInterval ), TERMINAL_VELOCITY );

      // Apply ground friction if we are on ground and aren't player-controlled
      if( phys.fOnGround && !registry.TryGet< CInput >( entity ) )
      {
         constexpr float GROUND_FRICTION = 10.0f;
         const float     friction        = std::pow( 0.5f, tickInterval * GROUND_FRICTION );
         vel.velocity.x *= friction;
         vel.velocity.z *= friction;
      }

      const glm::vec3 step = vel.velocity * tickInterval;
      moveAndCollideAxis( tran.position, vel.velocity, phys, step.y, Axis::Y );
      moveAndCollideAxis( tran.position, vel.velocity, phys, step.x, Axis::X );
      moveAndCollideAxis( tran.position, vel.velocity, phys, step.z, Axis::Z );
   }
}

static void CameraViewSystem( Entity::Registry& registry )
{
   for( auto [ tran, cam ] : registry.CView< CTransform, CCamera >() )
   {
      cam.view           = glm::mat4( 1.0f );
      cam.view           = glm::rotate( cam.view, glm::radians( tran.rotation.x ), { 1, 0, 0 } );
      cam.view           = glm::rotate( cam.view, glm::radians( tran.rotation.y ), { 0, 1, 0 } );
      cam.view           = glm::rotate( cam.view, glm::radians( tran.rotation.z ), { 0, 0, 1 } );
      cam.view           = glm::translate( cam.view, -tran.position );
      cam.projection     = glm::perspective( glm::radians( cam.fov ), cam.aspectRatio, cam.nearPlane, cam.farPlane );
      cam.viewProjection = cam.projection * cam.view;
   }
}

struct CTick
{
   uint32_t currentTick = 0;
   uint32_t maxTicks    = 0;
};

static void TickingSystem( Entity::Registry& registry )
{
   std::unordered_set< Entity::Entity > entitiesToDestory;
   for( auto [ e, tick ] : registry.ECView< CTick >() )
   {
      if( tick.currentTick++ >= tick.maxTicks )
         entitiesToDestory.insert( e );
   }

   for( Entity::Entity e : entitiesToDestory )
      registry.Destroy( e );
}

static void ProjectileDamageSystem( Entity::Registry& registry, Engine::Physics::CollisionEventQueue& collisions )
{
   // Only react on Enter to avoid damaging every frame while overlapping.
   for( const Engine::Physics::CollisionEvent& ev : collisions.Events() )
   {
      if( ev.phase != Engine::Physics::CollisionPhase::Enter )
         continue;

      auto tryApply = [ & ]( Entity::Entity projEnt, Entity::Entity targetEnt )
      {
         const CProjectile* pProj = registry.TryGet< CProjectile >( projEnt );
         if( !pProj )
            return;

         if( pProj->owner != Entity::NullEntity && pProj->owner == targetEnt )
            return;

         CHealth* hp = registry.TryGet< CHealth >( targetEnt );
         if( !hp )
            return;

         hp->hp = ( std::max )( 0, hp->hp - pProj->damage );

         if( pProj->fDestroyOnHit )
            registry.Destroy( projEnt );
      };

      // projectile -> health in either order
      tryApply( ev.a, ev.b );
      tryApply( ev.b, ev.a );
   }
}

static void ItemPickupSystem( Entity::Registry& registry, Engine::Physics::CollisionEventQueue& collisions )
{
   for( const Engine::Physics::CollisionEvent& ev : collisions.Events() )
   {
      if( ev.phase != Engine::Physics::CollisionPhase::Enter )
         continue;

      auto tryPickup = [ & ]( Entity::Entity collector, Entity::Entity item )
      {
         if( !registry.FHas< CLocalPlayerTag >( collector ) || !registry.FHas< CItemDrop >( item ) )
            return;

         // TODO: Add to inventory
         registry.Destroy( item );
      };

      tryPickup( ev.a, ev.b );
      tryPickup( ev.b, ev.a );
   }
}

static void ItemDropSystem( Entity::Registry& registry, float tickInterval )
{
   constexpr float DROP_LIFETIME_SECONDS = 300.0f;
   const uint64_t  DROP_LIFETIME_TICKS   = static_cast< uint64_t >( DROP_LIFETIME_SECONDS / tickInterval );

   std::vector< Entity::Entity > toDestroy;
   for( auto [ e, drop ] : registry.ECView< CItemDrop >() )
   {
      // Initialize drop lifetime on first update
      if( drop.ticksRemaining == 0 )
      {
         drop.maxTicks       = DROP_LIFETIME_TICKS;
         drop.ticksRemaining = drop.maxTicks;
      }

      if( --drop.ticksRemaining == 0 )
         toDestroy.push_back( e );
   }

   for( Entity::Entity e : toDestroy )
      registry.Destroy( e );
}

// Per-world collision queue (keeps previous frame pair state)
static Engine::Physics::CollisionEventQueue g_collisionEvents;
} // namespace

namespace Game
{

void InGameState::OnEnter()
{
   Entity::Registry& registry = GameCtx().RegistryRef();

   m_pLevel        = std::make_unique< Level >( "default" );
   m_pRenderSystem = std::make_unique< Engine::RenderSystem >( *m_pLevel );

   m_blockRes = std::make_unique< Engine::ECS::BlockInteractionResource >();

   // Register block interaction systems in deterministic phase order.
   m_scheduler.Add( std::make_unique< Engine::ECS::BlockIntentSystem >( *m_blockRes, *m_pLevel ) );
   m_scheduler.Add( std::make_unique< Engine::ECS::BlockHitSystem >( *m_blockRes, *m_pLevel ) );
   m_scheduler.Add( std::make_unique< Engine::ECS::BlockBreakSystem >( *m_blockRes, *m_pLevel ) );
   m_scheduler.Add( std::make_unique< Engine::ECS::BlockUseSystem >( *m_blockRes, *m_pLevel ) );
   m_scheduler.Add( std::make_unique< Engine::ECS::FurnaceSystem >() );
   m_scheduler.Add( std::make_unique< Engine::ECS::BlockEntityInteractSystem >( *m_blockRes ) );

   m_player = registry.Create();
   registry.Add< CLocalPlayerTag >( m_player, CLocalPlayerTag { .cameraEntity = Entity::NullEntity } );
   registry.Add< CTransform >( m_player, 0.5f, static_cast< float >( m_pLevel->GetSurfaceY( 0, 0 ) ) + 1, 0.5f );
   registry.Add< CVelocity >( m_player, 0.0f, 0.0f, 0.0f );
   registry.Add< CInput >( m_player );
   registry.Add< CBlockInteractor >( m_player, CBlockInteractor { .reach = 6.0f } );
   registry.Add< CPhysics >( m_player, CPhysics { .bbMin = glm::vec3( -0.3f, 0.0f, -0.3f ), .bbMax = glm::vec3( 0.3f, PLAYER_HEIGHT, 0.3f ) } );
   registry.Add< CHealth >( m_player, CHealth { .hp = 100, .maxHp = 100 } );

   const auto winSize = GameCtx().WindowRef().GetWindowState().size;

   m_camera = registry.Create();
   registry.Add< CTransform >( m_camera, 0.0f, 128.0f + PLAYER_EYE_HEIGHT, 0.0f );
   registry.Add< CLookInput >( m_camera );
   registry.Add< CCamera >( m_camera, CCamera { .aspectRatio = winSize.x / static_cast< float >( winSize.y ) } );
   registry.Add< CCameraRig >( m_camera,
                               CCameraRig {
                                  .targetEntity = m_player,
                                  .offset       = glm::vec3( 0.0f, PLAYER_EYE_HEIGHT, 0.0f ),
                                  .followYaw    = true,
                                  .followPitch  = false,
                               } );

   registry.Get< CLocalPlayerTag >( m_player ).cameraEntity = m_camera;

   auto psBall = registry.CreateWithHandle();
   registry.Add< CTransform >( psBall->Get(), 5.0f, 128.0f, 5.0f );
   registry.Add< CMesh >( psBall->Get(), std::make_shared< SphereMesh >() );
   registry.Add< CVelocity >( psBall->Get(), glm::vec3( 0.0f, 0.0f, 0.0f ) );
   registry.Add< CPhysics >( psBall->Get(), CPhysics { .bbMin = glm::vec3( -0.5f ), .bbMax = glm::vec3( 0.5f ) } );

   auto psCube = registry.CreateWithHandle();
   registry.Add< CTransform >( psCube->Get(), 6.0f, 128.0f, 8.0f );
   registry.Add< CMesh >( psCube->Get(), std::make_shared< CubeMesh >() );
   registry.Add< CVelocity >( psCube->Get(), glm::vec3( 0.0f, 0.0f, 0.0f ) );
   registry.Add< CPhysics >( psCube->Get(), CPhysics { .bbMin = glm::vec3( -0.5f ), .bbMax = glm::vec3( 0.5f ) } );

   m_connectedPlayers.clear();

   // Keep state logic independent of Application singleton by capturing the registry/camera/player handles.
   m_events.Subscribe< Events::NetworkClientConnectEvent >( [ this, &registry ]( const Events::NetworkClientConnectEvent& e ) noexcept
   {
      Entity::Entity newPlayer = registry.Create();
      registry.Add< CTransform >( newPlayer, 0.0f, 128.0f, 0.0f );

      auto psClientMesh = std::make_shared< CapsuleMesh >();
      psClientMesh->SetColor( glm::vec3( 100.0f / 255.0f, 147.0f / 255.0f, 237.0f / 255.0f ) );
      registry.Add< CMesh >( newPlayer, psClientMesh );
      m_connectedPlayers.insert( { e.GetClientID(), newPlayer } );
   } );

   m_events.Subscribe< Events::NetworkPositionUpdateEvent >( [ this, &registry ]( const Events::NetworkPositionUpdateEvent& e ) noexcept
   {
      auto it = m_connectedPlayers.find( e.GetClientID() );
      if( it == m_connectedPlayers.end() )
         return;

      CTransform* pClientTransform = registry.TryGet< CTransform >( it->second );
      if( !pClientTransform )
         return;

      glm::vec3 meshOverride( 0.0f, 1.0f, 0.0f );
      pClientTransform->position = e.GetPosition() + meshOverride;
   } );

   m_events.Subscribe< Events::WindowResizeEvent >( [ this, &registry ]( const Events::WindowResizeEvent& e ) noexcept
   {
      if( e.GetHeight() == 0 )
         return;

      if( CCamera* pCamera = registry.TryGet< CCamera >( m_camera ) )
         pCamera->aspectRatio = static_cast< float >( e.GetWidth() ) / static_cast< float >( e.GetHeight() );
   } );

   m_events.Subscribe< Events::MouseScrolledEvent >( [ this, &registry ]( const Events::MouseScrolledEvent& e ) noexcept
   {
      if( CCamera* pCamera = registry.TryGet< CCamera >( m_camera ) )
         pCamera->fov = std::clamp( pCamera->fov - ( e.GetYOffset() * 5 ), 10.0f, 90.0f );
   } );

   m_events.Subscribe< Events::KeyPressedEvent >( [ this, &registry ]( const Events::KeyPressedEvent& e ) noexcept
   {
      if( e.GetKeyCode() == Input::KeyCode::R )
         registry.Get< CTransform >( m_player ).position.y += 64;
   } );

   m_events.Subscribe< Events::MouseButtonPressedEvent >( [ this, &registry ]( const Events::MouseButtonPressedEvent& e ) noexcept
   {
      if( e.GetMouseButton() == Input::ButtonMiddle && m_pLevel )
      {
         const CTransform* pCamTran = registry.TryGet< CTransform >( m_camera );
         if( !pCamTran )
            return;

         if( auto hit = TryRaycast( *m_pLevel, CreateRay( pCamTran->position, pCamTran->rotation, 64.0f ) ) )
            m_pLevel->Explode( WorldBlockPos { hit->block }, 36 );
      }
   } );

   m_events.Subscribe< Events::NetworkBlockUpdateEvent >( [ this ]( const Events::NetworkBlockUpdateEvent& e ) noexcept
   {
      if( m_pLevel )
         m_pLevel->SetBlock( WorldBlockPos { e.GetBlockPos() }, BlockState( static_cast< BlockId >( e.GetBlockId() ) ) );
   } );

   TextureAtlasManager::Get().CompileBlockAtlas();

   m_pDebugUI   = CreateDebugUI( registry, m_player, m_camera, GameCtx().TimeRef() );
   m_pNetworkUI = CreateNetworkUI();
}

void InGameState::OnExit()
{
   m_pNetworkUI.reset();
   m_pDebugUI.reset();

   m_blockRes.reset();

   // release large systems
   m_pRenderSystem.reset();
   m_pLevel.reset();

   // TODO: registry cleanup strategy (per-world registry vs global)
   m_connectedPlayers.clear();
}

void InGameState::Update( float dt )
{
   if( !m_pLevel || !m_pRenderSystem )
      return;

   Entity::Registry& registry = GameCtx().RegistryRef();
   const float       alpha    = GameCtx().TimeRef().GetTickFraction();

   LocalInputPollSystem( registry );
   MouseLookSystem( registry, GameCtx().WindowRef() );
   CameraRigSystem( registry, alpha );

   Engine::ECS::TickContext tctx { .game = GameCtx(), .dt = dt };
   m_scheduler.TickPhase( Engine::ECS::SystemPhase::Presentation, tctx );

   // Maybe can be be within fixed update?
   if( CTransform* pPlayerTran = registry.TryGet< CTransform >( m_player ) )
   {
      const glm::vec3 interpolatedPos = glm::mix( pPlayerTran->prevPosition, pPlayerTran->position, alpha );
      m_pRenderSystem->Update( interpolatedPos, 8 );
   }
}

void InGameState::FixedUpdate( float tickInterval )
{
   Entity::Registry& registry = GameCtx().RegistryRef();

   // Store previous state for interpolation
   for( auto [ tran ] : registry.CView< CTransform >() )
      tran.RecordPrev();

   TickingSystem( registry );

   Engine::ECS::FixedTickContext fctx { .game = GameCtx(), .tickInterval = tickInterval };
   m_scheduler.FixedTickPhase( Engine::ECS::SystemPhase::Intent, fctx );
   m_scheduler.FixedTickPhase( Engine::ECS::SystemPhase::Simulation, fctx );
   m_scheduler.FixedTickPhase( Engine::ECS::SystemPhase::LateSimulation, fctx );

   PlayerMovementSystem( registry );
   ItemDropSystem( registry, tickInterval );
   PhysicsSystem( registry, *m_pLevel, tickInterval );

   // Collect generic collisions and let gameplay consume them.
   Engine::Physics::CollectEntityAABBCollisions( registry, g_collisionEvents );
   ProjectileDamageSystem( registry, g_collisionEvents );
   ItemPickupSystem( registry, g_collisionEvents );

   if( INetwork* pNetwork = INetwork::Get() )
   {
      if( CTransform* pPlayerTran = registry.TryGet< CTransform >( m_player ) )
         pNetwork->Poll( pPlayerTran->position );
   }
}

void InGameState::Render()
{
   if( !m_pLevel || !m_pRenderSystem )
      return;

   if( GameCtx().WindowRef().FMinimized() )
      return;

   Entity::Registry& registry    = GameCtx().RegistryRef();
   const CTransform* pCameraTran = registry.TryGet< CTransform >( m_camera );
   const CCamera*    pCameraComp = registry.TryGet< CCamera >( m_camera );
   if( !pCameraTran || !pCameraComp )
      return;

   CameraViewSystem( registry );

   std::optional< glm::ivec3 > optHighlightBlock;
   if( CBlockInteractor* pInteractor = registry.TryGet< CBlockInteractor >( m_player ) )
   {
      if( auto optResult = TryRaycast( *m_pLevel, CreateRay( pCameraTran->position, pCameraTran->rotation, pInteractor->reach ) ) )
         optHighlightBlock = optResult->block;
   }

   Engine::RenderSystem::FrameContext rctx { .registry          = registry,
                                             .time              = GameCtx().TimeRef(),
                                             .view              = pCameraComp->view,
                                             .projection        = pCameraComp->projection,
                                             .viewProjection    = pCameraComp->viewProjection,
                                             .viewPos           = pCameraTran->position,
                                             .optHighlightBlock = optHighlightBlock };
   m_pRenderSystem->Run( rctx );
}

void InGameState::DrawUI( UI::UIContext& ui )
{
   if( m_pDebugUI )
      ui.Register( m_pDebugUI );

   if( m_pNetworkUI )
      ui.Register( m_pNetworkUI );
}

} // namespace Game
