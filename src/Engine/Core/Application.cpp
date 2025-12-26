#include "Application.h"

// Local dependencies
#include "UI.h"
#include "Time.h"
#include "Window.h"

// Project dependencies
#include <Engine/Events/ApplicationEvent.h>
#include <Engine/Events/KeyEvent.h>
#include <Engine/Events/MouseEvent.h>
#include <Engine/Input/Input.h>
#include <Engine/Network/Network.h>

#include <Engine/World/Level.h>
#include <Engine/World/ChunkRenderer.h>
#include <Engine/World/RenderSystem.h>

#include <Engine/Renderer/Shader.h>
#include <Engine/Renderer/Texture.h>
#include <Engine/World/Raycast.h>

#include <Engine/Events/NetworkEvent.h>


//#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// Component includes
#include <Engine/ECS/Components.h>
#include <Engine/ECS/Registry.h>

// ================ SYSTEMS ================
constexpr float GRAVITY           = -32.0f;
constexpr float TERMINAL_VELOCITY = -48.0f;
constexpr float JUMP_VELOCITY     = 9.0f;
constexpr float PLAYER_HEIGHT     = 1.8f;
constexpr float PLAYER_EYE_HEIGHT = 1.62f;
constexpr float GROUND_MAXSPEED   = 4.3f;
constexpr float AIR_MAXSPEED      = 10.0f;
constexpr float AIR_CONTROL       = 0.3f;
constexpr float SPRINT_MODIFIER   = 1.3f;

static void MouseLookSystem( Entity::Registry& registry, Window& window )
{
   //PROFILE_SCOPE( "MouseLookSystem" );
   GLFWwindow* nativeWindow   = window.GetNativeWindow();
   const bool  fMouseCaptured = glfwGetInputMode( nativeWindow, GLFW_CURSOR ) == GLFW_CURSOR_DISABLED;

   glm::vec2 mouseDelta { 0.0f };
   if( fMouseCaptured )
   {
      static glm::vec2 lastMousePos = window.GetMousePosition();
      mouseDelta                    = window.GetMousePosition() - lastMousePos;

      const WindowData& windowData = window.GetWindowData();
      lastMousePos                 = { windowData.Width * 0.5f, windowData.Height * 0.5f };
      glfwSetCursorPos( nativeWindow, lastMousePos.x, lastMousePos.y );
   }

   // Write intent only (no transform mutation)
   for( auto [ look ] : registry.CView< CLookInput >() )
   {
      look.yawDelta   = 0.0f;
      look.pitchDelta = 0.0f;
   }

   // Only apply mouse delta if captured
   if( fMouseCaptured )
   {
      for( auto [ cam, look ] : registry.CView< CCamera, CLookInput >() )
      {
         look.pitchDelta += mouseDelta.y * cam.sensitivity;
         look.yawDelta += mouseDelta.x * cam.sensitivity;
      }
   }
}

static void CameraRigSystem( Entity::Registry& registry )
{
   //PROFILE_SCOPE( "CameraRigSystem" );
   for( auto [ camEntity, camTran, rig ] : registry.ECView< CTransform, CCameraRig >() )
   {
      CTransform* pTargetTran = registry.TryGet< CTransform >( rig.targetEntity );
      if( !pTargetTran )
         continue;

      // Apply look input to camera
      if( CCamera* cam = registry.TryGet< CCamera >( camEntity ) )
      {
         if( CLookInput* look = registry.TryGet< CLookInput >( camEntity ) )
         {
            camTran.rotation.x = glm::clamp( camTran.rotation.x + look->pitchDelta, -90.0f, 90.0f );
            camTran.rotation.y = glm::mod( camTran.rotation.y + look->yawDelta + 360.0f, 360.0f );
         }
      }

      // Apply camera yaw/pitch to target
      if( rig.followYaw )
         pTargetTran->rotation.y = camTran.rotation.y;
      if( rig.followPitch )
         pTargetTran->rotation.x = camTran.rotation.x;

      // Position the camera (with offset if any)
      camTran.position = pTargetTran->position + rig.offset;
   }
}

static void PlayerInputSystem( Entity::Registry& registry, float delta )
{
   //PROFILE_SCOPE( "PlayerInputSystem" );
   for( auto [ tran, vel, input, tag ] : registry.CView< CTransform, CVelocity, CInput, CPlayerTag >() )
   {
      // Apply Player Movement Inputs
      glm::vec3 wishdir( 0.0f );
      if( Input::FKeyPressed( Input::W ) )
      {
         wishdir.x -= glm::cos( glm::radians( tran.rotation.y + 90 ) );
         wishdir.z -= glm::sin( glm::radians( tran.rotation.y + 90 ) );
      }
      if( Input::FKeyPressed( Input::S ) )
      {
         wishdir.x += glm::cos( glm::radians( tran.rotation.y + 90 ) );
         wishdir.z += glm::sin( glm::radians( tran.rotation.y + 90 ) );
      }
      if( Input::FKeyPressed( Input::A ) )
      {
         float y = glm::radians( tran.rotation.y );
         wishdir += glm::vec3( -glm::cos( y ), 0, -glm::sin( y ) );
      }
      if( Input::FKeyPressed( Input::D ) )
      {
         float y = glm::radians( tran.rotation.y );
         wishdir -= glm::vec3( -glm::cos( y ), 0, -glm::sin( y ) );
      }
      if( glm::length( wishdir ) > 0.0f )
         wishdir = glm::normalize( wishdir );

      float maxSpeed = GROUND_MAXSPEED;
      if( Input::FKeyPressed( Input::LeftShift ) )
         maxSpeed *= SPRINT_MODIFIER;

      vel.velocity.x = wishdir.x * maxSpeed;
      vel.velocity.z = wishdir.z * maxSpeed;

      // this needs to be per-component, not static which affects all
      static bool wasJumping = false;
      if( Input::FKeyPressed( Input::Space ) )
      {
         if( !wasJumping )
         {
            vel.velocity.y = JUMP_VELOCITY;
            wasJumping     = true;
         }
      }
      else
         wasJumping = false;
   }
}

static void PlayerPhysicsSystem( Entity::Registry& registry, Level& level, float delta )
{
   //PROFILE_SCOPE( "PlayerPhysicsSystem" );

   // World vs Entity collision
   {
      auto getVoxelBounds = [ & ]( const glm::vec3& pos, const glm::vec3& bbMin, const glm::vec3& bbMax ) -> std::tuple< glm::ivec3, glm::ivec3 >
      {
         constexpr float EPS = 1e-4f; // for stable voxel range calc
         return {
            glm::ivec3 { static_cast< int >( std::floor( pos.x + bbMin.x + EPS ) ), // min x
                         static_cast< int >( std::floor( pos.y + bbMin.y + EPS ) ), // min y
                         static_cast< int >( std::floor( pos.z + bbMin.z + EPS ) ) }, // min z
            glm::ivec3 { static_cast< int >( std::floor( pos.x + bbMax.x - EPS ) ), // max x
                         static_cast< int >( std::floor( pos.y + bbMax.y - EPS ) ), // max y
                         static_cast< int >( std::floor( pos.z + bbMax.z - EPS ) ) }  // max z
         };
      };

      auto computeGrounded = [ & ]( const glm::vec3& pos, const glm::vec3& bbMin, const glm::vec3& bbMax ) -> bool
      {
         constexpr float GROUND_PROBE = 0.05f; // how far below to probe for ground

         glm::vec3 probePos = pos;
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

         // Scan overlapped voxels, tracking closest hit plane along `axis`
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

         constexpr float SKIN = 0.001f; // small separation to prevent re-penetration
         if( positive )
         {
            if( hit == INT_MAX )
               return;

            // clamp so our max face sits just before `hit` voxel's min face
            pos[ axis ] = static_cast< float >( hit ) - phys.bbMax[ axis ] - SKIN;
         }
         else
         {
            if( hit == INT_MIN )
               return;

            // clamp so our min face sits just after (`hit + 1`) voxel's max face
            pos[ axis ] = static_cast< float >( hit + 1 ) - phys.bbMin[ axis ] + SKIN;
         }

         // Apply restitution on collision for this axis.
         // If bounciness is 0, this matches the old behavior (velocity becomes 0).
         const float restitution = std::clamp( phys.bounciness, 0.0f, 1.0f );
         vel[ axis ]             = -vel[ axis ] * restitution;

         // Avoid tiny jitter bounces
         if( std::abs( vel[ axis ] ) < 0.01f )
            vel[ axis ] = 0.0f;

         // Consider on-ground only if we hit while moving downward and we're not bouncing significantly.
         if( axis == Axis::Y && !positive && restitution < 0.5f )
            phys.fOnGround = true;
      };

      for( auto [ tran, vel, phys ] : registry.CView< CTransform, CVelocity, CPhysics >() )
      {
         // Determine if entity is on the ground and only apply gravity if not.
         phys.fOnGround = computeGrounded( tran.position, phys.bbMin, phys.bbMax );
         if( !phys.fOnGround )
            vel.velocity.y = ( std::max )( vel.velocity.y + ( GRAVITY * delta ), TERMINAL_VELOCITY );

         // Per-axis movement + collision
         glm::vec3 pos  = tran.position;
         glm::vec3 step = vel.velocity * delta;
         moveAndCollideAxis( pos, vel.velocity, phys, step.y, Axis::Y );
         moveAndCollideAxis( pos, vel.velocity, phys, step.x, Axis::X );
         moveAndCollideAxis( pos, vel.velocity, phys, step.z, Axis::Z );
         tran.position = pos;
      }
   }

   // ---- Optional: entity-vs-entity stays after world, but note it can fight world resolution ----
   //for( auto [ tranA, velA, physA ] : registry.CView< CTransform, CVelocity, CPhysics >() )
   //{
   //   for( auto [ tranB, velB, physB ] : registry.CView< CTransform, CVelocity, CPhysics >() )
   //   {
   //      if( &tranA == &tranB )
   //         continue;
   //
   //      // AABB vs AABB collision
   //      glm::vec3 minA     = tranA.position - physA.halfExtents;
   //      glm::vec3 maxA     = tranA.position + physA.halfExtents;
   //      glm::vec3 minB     = tranB.position - physB.halfExtents;
   //      glm::vec3 maxB     = tranB.position + physB.halfExtents;
   //      bool      overlapX = ( minA.x <= maxB.x ) && ( maxA.x >= minB.x );
   //      bool      overlapY = ( minA.y <= maxB.y ) && ( maxA.y >= minB.y );
   //      bool      overlapZ = ( minA.z <= maxB.z ) && ( maxA.z >= minB.z );
   //      if( overlapX && overlapY && overlapZ )
   //      {
   //         // Simple resolution: push A back along its velocity vector
   //         glm::vec3 normVelA = glm::normalize( velA.velocity );
   //         tranA.position -= normVelA * 0.1f; // arbitrary small pushback
   //         // Zero A's velocity
   //         velA.velocity = glm::vec3( 0.0f );
   //      }
   //   }
   //}
}


static void CameraViewSystem( Entity::Registry& registry )
{
   //PROFILE_SCOPE( "CameraViewSystem" );
   for( auto [ tran, cam ] : registry.CView< CTransform, CCamera >() )
   {
      cam.view = glm::mat4( 1.0f );
      cam.view = glm::rotate( cam.view, glm::radians( tran.rotation.x ), { 1, 0, 0 } );
      cam.view = glm::rotate( cam.view, glm::radians( tran.rotation.y ), { 0, 1, 0 } );
      cam.view = glm::rotate( cam.view, glm::radians( tran.rotation.z ), { 0, 0, 1 } );
      cam.view = glm::translate( cam.view, -tran.position );

      cam.viewProjection = cam.projection * cam.view;
   }
}


struct CTick
{
   uint32_t currentTick = 0;
   uint32_t maxTicks    = 0;
};


static void TickingSystem( Entity::Registry& registry, float /*delta*/ )
{
   //PROFILE_SCOPE( "TickingSystem" );
   std::unordered_set< Entity::Entity > entitiesToDestory;
   for( auto [ e, tick ] : registry.ECView< CTick >() )
   {
      if( tick.currentTick++ >= tick.maxTicks )
         entitiesToDestory.insert( e );
   }

   for( Entity::Entity e : entitiesToDestory )
      registry.Destroy( e );
}


static float RandRange( float min, float max )
{
   thread_local static std::mt19937                            rng( std::random_device {}() );
   thread_local static std::uniform_real_distribution< float > dist( 0.0f, 1.0f );
   return min + ( dist( rng ) * ( max - min ) );
}


static glm::mat4 CreateProjectionMatrix( uint16_t screenWidth, uint16_t screenHeight, float fov, float nearPlane, float farPlane )
{
   return glm::perspective( glm::radians( fov ), static_cast< float >( screenWidth ) / static_cast< float >( screenHeight ), nearPlane, farPlane );
}


void Application::Run()
{
   Window&     window     = Window::Get();
   WindowData& windowData = window.GetWindowData();
   window.Init();

   Level                level( "default" );
   Time::FixedTimeStep  timestep( 20 /*tickrate*/ );
   Engine::RenderSystem renderSystem( level );

   // ECS Registry
   Entity::Registry registry;

   Entity::Entity player = registry.Create();
   registry.Add< CTransform >( player, 0.0f, 128.0f, 0.0f );
   registry.Add< CVelocity >( player, 0.0f, 0.0f, 0.0f );
   registry.Add< CInput >( player );
   registry.Add< CPlayerTag >( player );
   registry.Add< CPhysics >( player, CPhysics { .bbMin = glm::vec3( -0.3f, 0.0f, -0.3f ), .bbMax = glm::vec3( 0.3f, PLAYER_HEIGHT, 0.3f ) } );

   Entity::Entity camera = registry.Create();
   registry.Add< CTransform >( camera, 0.0f, 128.0f + PLAYER_EYE_HEIGHT, 0.0f );
   registry.Add< CLookInput >( camera );
   if( CCamera* pCamera = &registry.Add< CCamera >( camera ) )
      pCamera->projection = CreateProjectionMatrix( windowData.Width, windowData.Height, pCamera->fov, 0.1f, 1000.0f );
   registry.Add< CCameraRig >( camera,
                               CCameraRig {
                                  .targetEntity = player,
                                  .offset       = glm::vec3( 0.0f, PLAYER_EYE_HEIGHT, 0.0f ),
                                  .followYaw    = true,
                                  .followPitch  = false,
                               } );
   //registry.Add< CParent >( camera, player, glm::vec3( 0.0f, PLAYER_EYE_HEIGHT, 0.0f ) );

   std::shared_ptr< Entity::EntityHandle > psBall = registry.CreateWithHandle();
   registry.Add< CTransform >( psBall->Get(), 5.0f, 128.0f, 5.0f );
   registry.Add< CMesh >( psBall->Get(), std::make_shared< SphereMesh >() );
   registry.Add< CVelocity >( psBall->Get(), glm::vec3( 0.0f, 0.0f, 0.0f ) );
   registry.Add< CPhysics >( psBall->Get(), CPhysics { .bbMin = glm::vec3( -0.5f ), .bbMax = glm::vec3( 0.5f ) } );

   std::shared_ptr< Entity::EntityHandle > psCube = registry.CreateWithHandle();
   registry.Add< CTransform >( psCube->Get(), 6.0f, 128.0f, 8.0f );
   registry.Add< CMesh >( psCube->Get(), std::make_shared< CubeMesh >() );
   registry.Add< CVelocity >( psCube->Get(), glm::vec3( 0.0f, 0.0f, 0.0f ) );
   registry.Add< CPhysics >( psCube->Get(), CPhysics { .bbMin = glm::vec3( -0.5f ), .bbMax = glm::vec3( 0.5f ) } );

   Events::EventSubscriber eventSubscriber;

   std::unordered_map< uint64_t, Entity::Entity > connectedPlayers;
   eventSubscriber.Subscribe< Events::NetworkClientConnectEvent >( [ & ]( const auto& e ) noexcept
   {
      Entity::Entity newPlayer = registry.Create();
      registry.Add< CTransform >( newPlayer, 0.0f, 128.0f, 0.0f );

      std::shared_ptr< CapsuleMesh > psClientMesh = std::make_shared< CapsuleMesh >();
      psClientMesh->SetColor( glm::vec3( 100.0f / 255.0f, 147.0f / 255.0f, 237.0f / 255.0f ) ); // cornflower blue
      registry.Add< CMesh >( newPlayer, psClientMesh );
      connectedPlayers.insert( { e.GetClientID(), newPlayer } );
   } );

   eventSubscriber.Subscribe< Events::NetworkPositionUpdateEvent >( [ & ]( const auto& e ) noexcept
   {
      auto it = connectedPlayers.find( e.GetClientID() );
      if( it == connectedPlayers.end() )
      {
         std::println( std::cerr, "Client with ID {} not found", e.GetClientID() );
         return; // Client not found
      }

      CTransform* pClientTransform = registry.TryGet< CTransform >( it->second );
      if( !pClientTransform )
      {
         std::println( std::cerr, "Transform component not found for client ID {}", e.GetClientID() );
         return; // Transform component not found
      }

      glm::vec3 meshOffset( 0.0f, 1.0f, 0.0f );                  // Offset for the mesh position
      pClientTransform->position = e.GetPosition() + meshOffset; // position is at feet level, so adjust for height
   } );

   eventSubscriber.Subscribe< Events::WindowResizeEvent >( [ & ]( const auto& e ) noexcept
   {
      if( e.GetHeight() == 0 )
         return;

      if( CCamera* pCamera = registry.TryGet< CCamera >( camera ) )
         pCamera->projection = CreateProjectionMatrix( e.GetWidth(), e.GetHeight(), pCamera->fov, 0.1f, 1000.0f );
   } );

   eventSubscriber.Subscribe< Events::MouseScrolledEvent >( [ & ]( const auto& e ) noexcept
   {
      if( CCamera* pCamera = registry.TryGet< CCamera >( camera ) )
      {
         pCamera->fov        = std::clamp( pCamera->fov - ( e.GetYOffset() * 5 ), 10.0f, 90.0f );
         pCamera->projection = CreateProjectionMatrix( windowData.Width, windowData.Height, pCamera->fov, 0.1f, 1000.0f );
      }
   } );

   eventSubscriber.Subscribe< Events::KeyPressedEvent >( [ & ]( const auto& e ) noexcept
   {
      switch( e.GetKeyCode() )
      {
         case Input::KeyCode::R: registry.Get< CTransform >( player ).position.y += 64; break;
      }
   } );

   eventSubscriber.Subscribe< Events::MouseButtonPressedEvent >( [ & ]( const auto& e ) noexcept
   {
      //if( e.GetMouseButton() != Input::ButtonLeft && e.GetMouseButton() != Input::ButtonRight && e.GetMouseButton() != Input::ButtonMiddle )
      //   return;

      CTransform* pCamTran = registry.TryGet< CTransform >( camera );
      if( !pCamTran )
         return;

      const float pitch = glm::radians( pCamTran->rotation.x );
      const float yaw   = glm::radians( pCamTran->rotation.y );

      glm::vec3 direction { glm::cos( pitch ) * glm::sin( yaw ), -glm::sin( pitch ), -glm::cos( pitch ) * glm::cos( yaw ) };
      direction = glm::normalize( direction );

      const glm::vec3 rayOrigin   = pCamTran->position;
      constexpr float maxDistance = 64.0f;
      //if( e.GetMouseButton() == Input::ButtonLeft )
      //{
      //   constexpr int   NUM_PROJECTILES = 500;
      //   constexpr float CONE_ANGLE_DEG  = 8.0f;
      //   constexpr float SPEED           = 50.0f;
      //
      //   glm::vec3 right = glm::normalize( ( glm::abs( direction.y ) < 0.99f ) ? glm::cross( direction, glm::vec3( 0, 1, 0 ) )
      //                                                                         : glm::cross( direction, glm::vec3( 1, 0, 0 ) ) );
      //   glm::vec3 up    = glm::cross( right, direction );
      //
      //   float     cosMax   = glm::cos( glm::radians( CONE_ANGLE_DEG ) );
      //   glm::vec3 spawnPos = rayOrigin + direction;
      //   for( int i = 0; i < NUM_PROJECTILES; ++i )
      //   {
      //      // Uniform random direction in cone
      //      float u        = RandRange( 0.0f, 1.0f );
      //      float v        = RandRange( 0.0f, 1.0f );
      //      float cosTheta = glm::mix( cosMax, 1.0f, u );
      //      float sinTheta = std::sqrt( 1.0f - cosTheta * cosTheta );
      //      float phi      = glm::two_pi< float >() * v;
      //
      //      glm::vec3 spreadDir = direction * cosTheta + right * ( std::cos( phi ) * sinTheta ) + up * ( std::sin( phi ) * sinTheta );
      //
      //      Entity::Entity projectile = registry.Create();
      //      registry.Add< CTransform >( projectile, spawnPos );
      //      registry.Add< CMesh >( projectile, std::make_shared< SphereMesh >( 0.1f ) );
      //      registry.Add< CVelocity >( projectile, spreadDir * SPEED );
      //      registry.Add< CPhysics >( projectile, CPhysics { .bbMin = glm::vec3( -0.05f ), .bbMax = glm::vec3( 0.05f ), .bounciness = 0.8f } );
      //      registry.Add< CTick >( projectile, 0, 200 );
      //   }
      //
      //   return;
      //}

      if( std::optional< RaycastResult > result = TryRaycast( level, Ray { rayOrigin, direction, maxDistance } ) )
      {
         std::cout << "Raycast hit at position: " << result->point.x << ", " << result->point.y << ", " << result->point.z << "\n";
         switch( e.GetMouseButton() )
         {
            case Input::ButtonLeft:
            {
               const glm::ivec3 pos = result->block;
               level.SetBlock( pos, BlockId::Air /*id*/ );
               Events::Dispatch< Events::NetworkRequestBlockUpdateEvent >( pos, static_cast< uint16_t >( BlockId::Air ), 1 /*break*/ );
               break;
            }
            case Input::ButtonRight:
            {
               const glm::ivec3 pos = result->block + result->faceNormal;
               level.SetBlock( pos, BlockId::Stone /*id*/ );
               Events::Dispatch< Events::NetworkRequestBlockUpdateEvent >( pos, static_cast< uint16_t >( BlockId::Stone ), 0 /*place*/ );
               break;
            }
            case Input::ButtonMiddle: level.Explode( result->block, 36 /*radius*/ ); break;
         }
      }
      else
         std::cout << "Raycast did not hit any block within " << maxDistance << " units.\n";
   } );

   eventSubscriber.Subscribe< Events::NetworkBlockUpdateEvent >( [ & ]( const auto& e ) noexcept
   { level.SetBlock( e.GetBlockPos(), static_cast< BlockId >( e.GetBlockId() ) ); } );

   // Initialize things
   TextureAtlasManager::Get().CompileBlockAtlas();

   // Debug UIs and such
   std::shared_ptr< UI::IDrawable > pDebugUI   = CreateDebugUI( registry, player, camera, timestep );
   std::shared_ptr< UI::IDrawable > pNetworkUI = CreateNetworkUI();

   while( window.IsOpen() )
   {
      const float delta = std::clamp( timestep.Step(), 0.0f, 0.05f /*50ms*/ ); // time since last frame, max 50ms

      CTransform* pPlayerTran = registry.TryGet< CTransform >( player );
      CTransform* pCameraTran = registry.TryGet< CTransform >( camera );
      CCamera*    pCameraComp = registry.TryGet< CCamera >( camera );

      {
         // Process queue events at the beginning of each frame
         Events::ProcessQueuedEvents();

         //level.Update( delta );
         if( pPlayerTran )
            renderSystem.Update( pPlayerTran->position, 8 /*viewRadius*/ );

         MouseLookSystem( registry, window );
         CameraRigSystem( registry );

         PlayerInputSystem( registry, delta );
         PlayerPhysicsSystem( registry, level, delta );

         if( timestep.FShouldTick() )
         {
            TickingSystem( registry, delta );

            if( INetwork* pNetwork = INetwork::Get(); pNetwork && pPlayerTran )
               pNetwork->Poll( pPlayerTran->position );
         }

         if( window.FMinimized() || !( pCameraComp && pCameraTran ) )
            continue;

         CameraViewSystem( registry );

         auto                               optResult = TryRaycast( level, CreateRay( pCameraTran->position, pCameraTran->rotation, 64.0f /*maxDistance*/ ) );
         Engine::RenderSystem::FrameContext ctx { .registry          = registry,
                                                  .view              = pCameraComp->view,
                                                  .projection        = pCameraComp->projection,
                                                  .viewProjection    = pCameraComp->viewProjection,
                                                  .optHighlightBlock = optResult ? std::optional< glm::ivec3 >( optResult->block ) : std::nullopt };
         renderSystem.Run( ctx );

         // Draw UIs
         UI::UIBuffer::Register( pDebugUI );
         UI::UIBuffer::Register( pNetworkUI );
         UI::UIBuffer::Draw(); // probably become part of RenderSystem later
      }

      window.OnUpdate();
   }
}
