#include "InGameState.h"

// Project dependencies
#include <Engine/Core/Window.h>
#include <Engine/ECS/Components.h>
#include <Engine/Events/ApplicationEvent.h>
#include <Engine/Events/KeyEvent.h>
#include <Engine/Events/MouseEvent.h>
#include <Engine/Events/NetworkEvent.h>
#include <Engine/Input/Input.h>
#include <Engine/Network/Network.h>
#include <Engine/Renderer/Texture.h>
#include <Engine/World/Raycast.h>
#include <Engine/Physics/EntityCollisionSystem.h>

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
      const glm::vec3 forward = { -glm::cos( glm::radians( tran.rotation.y + 90 ) ), 0, -glm::sin( glm::radians( tran.rotation.y + 90 ) ) };
      const glm::vec3 right   = { glm::sin( glm::radians( tran.rotation.y + 90 ) ), 0, -glm::cos( glm::radians( tran.rotation.y + 90 ) ) };
      glm::vec3       wishdir = ( forward * input.movement.y ) + ( right * input.movement.x );
      if( glm::length( wishdir ) > 0.0f )
         wishdir = glm::normalize( wishdir );

      float maxSpeed = GROUND_MAXSPEED;
      if( input.fSprintRequest )
         maxSpeed *= SPRINT_MODIFIER;

      bool fOnGround = true;
      if( const CPhysics* pPhys = registry.TryGet< CPhysics >( e ) )
         fOnGround = pPhys->fOnGround;

      if( fOnGround )
      {
         vel.velocity.x = wishdir.x * maxSpeed;
         vel.velocity.z = wishdir.z * maxSpeed;
      }
      else
      {
         // Apply air control (acceleration + drag)
         constexpr float AIR_ACCEL = 0.5f;
         constexpr float AIR_DRAG  = 0.91f;

         vel.velocity.x += wishdir.x * AIR_ACCEL;
         vel.velocity.z += wishdir.z * AIR_ACCEL;
         vel.velocity.x *= AIR_DRAG;
         vel.velocity.z *= AIR_DRAG;
      }

      // Cooldown management
      if( input.jumpCooldown > 0 )
         input.jumpCooldown--;

      if( input.fJumpRequest && fOnGround && input.jumpCooldown == 0 )
      {
         vel.velocity.y     = JUMP_VELOCITY;
         input.jumpCooldown = 10; // 10 tick cooldown
      }

      input.fWasJumpDown = input.fJumpRequest;
   }
}

static void PhysicsSystem( Entity::Registry& registry, Level& level, float delta )
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
               if( FSolid( level.GetBlock( x, y, z ) ) )
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
               if( !FSolid( level.GetBlock( x, y, z ) ) )
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

   for( auto [ tran, vel, phys ] : registry.CView< CTransform, CVelocity, CPhysics >() )
   {
      phys.fOnGround = computeGrounded( tran.position, phys.bbMin, phys.bbMax );
      if( !phys.fOnGround )
         vel.velocity.y = ( std::max )( vel.velocity.y + ( GRAVITY * delta ), TERMINAL_VELOCITY );

      const glm::vec3 step = vel.velocity * delta;
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

// Per-world collision queue (keeps previous frame pair state)
static Engine::Physics::CollisionEventQueue g_collisionEvents;
} // namespace

namespace Game
{

void InGameState::OnEnter( Engine::GameContext& ctx )
{
   Entity::Registry& registry = ctx.RegistryRef();

   m_pLevel        = std::make_unique< Level >( "default" );
   m_pRenderSystem = std::make_unique< Engine::RenderSystem >( *m_pLevel );

   m_player = registry.Create();
   registry.Add< CLocalPlayerTag >( m_player );
   registry.Add< CTransform >( m_player, 0.5f, static_cast< float >( m_pLevel->GetSurfaceY( 0, 0 ) ) + 1, 0.5f );
   registry.Add< CVelocity >( m_player, 0.0f, 0.0f, 0.0f );
   registry.Add< CInput >( m_player );
   registry.Add< CPhysics >( m_player, CPhysics { .bbMin = glm::vec3( -0.3f, 0.0f, -0.3f ), .bbMax = glm::vec3( 0.3f, PLAYER_HEIGHT, 0.3f ) } );
   registry.Add< CHealth >( m_player, CHealth { .hp = 100, .maxHp = 100 } );

   const auto winSize = ctx.WindowRef().GetWindowState().size;

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
      if( !m_pLevel )
         return;

      CTransform* pCamTran = registry.TryGet< CTransform >( m_camera );
      if( !pCamTran )
         return;

      const float pitch = glm::radians( pCamTran->rotation.x );
      const float yaw   = glm::radians( pCamTran->rotation.y );

      glm::vec3 direction { glm::cos( pitch ) * glm::sin( yaw ), -glm::sin( pitch ), -glm::cos( pitch ) * glm::cos( yaw ) };
      direction = glm::normalize( direction );

      const glm::vec3 rayOrigin = pCamTran->position;

      Entity::Entity projectile = registry.Create();
      registry.Add< CTransform >( projectile, rayOrigin + direction );
      registry.Add< CMesh >( projectile, std::make_shared< SphereMesh >( 0.1f ) );
      registry.Add< CVelocity >( projectile, direction * 120.0f );
      registry.Add< CPhysics >( projectile, CPhysics { .bbMin = glm::vec3( -0.05f ), .bbMax = glm::vec3( 0.05f ), .bounciness = 0.01f } );
      registry.Add< CProjectile >( projectile, CProjectile { .damage = 10, .owner = m_player, .fDestroyOnHit = true } );
      registry.Add< CTick >( projectile, 0, 200 );
      return;

      if( std::optional< RaycastResult > result = TryRaycast( *m_pLevel, Ray { rayOrigin, direction, 64.0f } ) )
      {
         switch( e.GetMouseButton() )
         {
            case Input::ButtonLeft:
            {
               const glm::ivec3 pos = result->block;
               m_pLevel->SetBlock( pos, BlockId::Air );
               Events::Dispatch< Events::NetworkRequestBlockUpdateEvent >( pos, static_cast< uint16_t >( BlockId::Air ), 1 );
               break;
            }
            case Input::ButtonRight:
            {
               const glm::ivec3 pos = result->block + result->faceNormal;
               m_pLevel->SetBlock( pos, BlockId::Stone );
               Events::Dispatch< Events::NetworkRequestBlockUpdateEvent >( pos, static_cast< uint16_t >( BlockId::Stone ), 0 );
               break;
            }
            case Input::ButtonMiddle: m_pLevel->Explode( result->block, 36 ); break;
         }
      }
   } );

   m_events.Subscribe< Events::NetworkBlockUpdateEvent >( [ this ]( const Events::NetworkBlockUpdateEvent& e ) noexcept
   {
      if( m_pLevel )
         m_pLevel->SetBlock( e.GetBlockPos(), static_cast< BlockId >( e.GetBlockId() ) );
   } );

   TextureAtlasManager::Get().CompileBlockAtlas();

   m_pDebugUI   = CreateDebugUI( registry, m_player, m_camera, ctx.TimeRef() );
   m_pNetworkUI = CreateNetworkUI();
}

void InGameState::OnExit( Engine::GameContext& )
{
   m_pNetworkUI.reset();
   m_pDebugUI.reset();

   // release large systems
   m_pRenderSystem.reset();
   m_pLevel.reset();

   // TODO: registry cleanup strategy (per-world registry vs global)
   m_connectedPlayers.clear();
}

void InGameState::Update( Engine::GameContext& ctx, float dt )
{
   if( !m_pLevel || !m_pRenderSystem )
      return;

   Entity::Registry& registry = ctx.RegistryRef();
   const float       alpha    = ctx.TimeRef().GetAlpha();

   if( CTransform* pPlayerTran = registry.TryGet< CTransform >( m_player ) )
   {
      const glm::vec3 interpolatedPos = glm::mix( pPlayerTran->prevPosition, pPlayerTran->position, alpha );
      m_pRenderSystem->Update( interpolatedPos, 8 );
   }

   LocalInputPollSystem( registry );
   MouseLookSystem( registry, ctx.WindowRef() );
   CameraRigSystem( registry, alpha );
}

void InGameState::FixedUpdate( Engine::GameContext& ctx, float fixedDt )
{
   Entity::Registry& registry = ctx.RegistryRef();

   // Store previous state for interpolation
   for( auto [ tran ] : registry.CView< CTransform >() )
      tran.RecordPrev();

   TickingSystem( registry );
   PlayerMovementSystem( registry );
   PhysicsSystem( registry, *m_pLevel, fixedDt );

   // Collect generic collisions and let gameplay consume them.
   Engine::Physics::CollectEntityAABBCollisions( registry, g_collisionEvents );
   ProjectileDamageSystem( registry, g_collisionEvents );

   if( INetwork* pNetwork = INetwork::Get() )
   {
      if( CTransform* pPlayerTran = registry.TryGet< CTransform >( m_player ) )
         pNetwork->Poll( pPlayerTran->position );
   }
}

void InGameState::Render( Engine::GameContext& ctx )
{
   if( !m_pLevel || !m_pRenderSystem )
      return;

   if( ctx.WindowRef().FMinimized() )
      return;

   Entity::Registry& registry = ctx.RegistryRef();

   const CTransform* pCameraTran = registry.TryGet< CTransform >( m_camera );
   const CCamera*    pCameraComp = registry.TryGet< CCamera >( m_camera );
   if( !pCameraTran || !pCameraComp )
      return;

   CameraViewSystem( registry );

   auto                               optResult = TryRaycast( *m_pLevel, CreateRay( pCameraTran->position, pCameraTran->rotation, 64.0f ) );
   Engine::RenderSystem::FrameContext rctx { .registry          = registry,
                                             .view              = pCameraComp->view,
                                             .projection        = pCameraComp->projection,
                                             .viewProjection    = pCameraComp->viewProjection,
                                             .viewPos           = pCameraTran->position,
                                             .optHighlightBlock = optResult ? std::optional< glm::ivec3 >( optResult->block ) : std::nullopt,
                                             .alpha             = ctx.TimeRef().GetAlpha() /*interpolation factor*/ };
   m_pRenderSystem->Run( rctx );
}

void InGameState::DrawUI( Engine::GameContext& ctx )
{
   if( m_pDebugUI )
      ctx.WindowRef().GetUIC().Register( m_pDebugUI );

   if( m_pNetworkUI )
      ctx.WindowRef().GetUIC().Register( m_pNetworkUI );
}

} // namespace Game
