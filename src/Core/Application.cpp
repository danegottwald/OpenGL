#include "Application.h"

// Local dependencies
#include "GUIManager.h"
#include "Timestep.h"
#include "Window.h"

// Project dependencies
#include <Events/ApplicationEvent.h>
#include <Events/KeyEvent.h>
#include <Events/MouseEvent.h>
#include <Input/Input.h>
#include <Network/Network.h>
#include <World.h>

#include <World/Level.h>
#include <Renderer/Shader.h>
#include <Renderer/Texture.h>

//#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// Component includes
#include <Entity/Components.h>
#include <Entity/Registry.h>

// ================ SYSTEMS ================
constexpr float GRAVITY           = -32.0f;
constexpr float TERMINAL_VELOCITY = -48.0f;
constexpr float JUMP_VELOCITY     = 9.0f;
constexpr float PLAYER_HEIGHT     = 1.8f;
constexpr float PLAYER_EYE_HEIGHT = 1.62f;
constexpr float GROUND_MAXSPEED   = 4.3f;
constexpr float AIR_MAXSPEED      = 10.0f;
constexpr float AIR_CONTROL       = 0.3f;
constexpr float SPRINT_MODIFIER   = 1.5f;

void PlayerInputSystem( Entity::Registry& registry, float delta )
{
   //PROFILE_SCOPE( "PlayerInputSystem" );
   for( auto [ tran, vel, input, tag ] : registry.CView< CTransform, CVelocity, CInput, CPlayerTag >() )
   {
      // Apply Player Movement Inputs
      glm::vec3 wishdir( 0.0f );
      if( Input::IsKeyPressed( Input::W ) )
      {
         wishdir.x -= glm::cos( glm::radians( tran.rotation.y + 90 ) );
         wishdir.z -= glm::sin( glm::radians( tran.rotation.y + 90 ) );
      }
      if( Input::IsKeyPressed( Input::S ) )
      {
         wishdir.x += glm::cos( glm::radians( tran.rotation.y + 90 ) );
         wishdir.z += glm::sin( glm::radians( tran.rotation.y + 90 ) );
      }
      if( Input::IsKeyPressed( Input::A ) )
      {
         float y = glm::radians( tran.rotation.y );
         wishdir += glm::vec3( -glm::cos( y ), 0, -glm::sin( y ) );
      }
      if( Input::IsKeyPressed( Input::D ) )
      {
         float y = glm::radians( tran.rotation.y );
         wishdir -= glm::vec3( -glm::cos( y ), 0, -glm::sin( y ) );
      }
      if( glm::length( wishdir ) > 0.0f )
         wishdir = glm::normalize( wishdir );

      float maxSpeed = GROUND_MAXSPEED;
      if( Input::IsKeyPressed( Input::LeftShift ) )
         maxSpeed *= SPRINT_MODIFIER;

      vel.velocity.x = wishdir.x * maxSpeed;
      vel.velocity.z = wishdir.z * maxSpeed;

      // this needs to be per-component, not static which affects all
      static bool wasJumping = false;
      if( Input::IsKeyPressed( Input::Space ) )
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

void PlayerPhysicsSystem( Entity::Registry& registry, Level& level, float delta )
{
   //PROFILE_SCOPE( "PlayerPhysicsSystem" );
   constexpr float SKIN = 0.001f; // small separation to prevent re-penetration
   constexpr float EPS  = 1e-4f;  // for stable voxel range calc

   auto isSolid = [ & ]( int x, int y, int z ) -> bool { return level.GetBlock( x, y, z ) != BLOCK_AIR; };

   // Small downward probe to keep a stable grounded state and prevent Y jitter when velocity.y ~ 0.
   // This avoids toggling fOnGround each frame due to SKIN/EPS rounding.
   constexpr float GROUND_PROBE = 0.05f;

   // Convert AABB at (pos, half) to voxel inclusive bounds, with epsilon to avoid "touching counts"
   auto getVoxelBounds = [ & ]( const glm::vec3& pos, const glm::vec3& half, int& minX, int& maxX, int& minY, int& maxY, int& minZ, int& maxZ )
   {
      const float minEdgeX = pos.x - half.x;
      const float maxEdgeX = pos.x + half.x;
      const float minEdgeY = pos.y - half.y;
      const float maxEdgeY = pos.y + half.y;
      const float minEdgeZ = pos.z - half.z;
      const float maxEdgeZ = pos.z + half.z;

      minX = ( int )std::floor( minEdgeX + EPS );
      maxX = ( int )std::floor( maxEdgeX - EPS );
      minY = ( int )std::floor( minEdgeY + EPS );
      maxY = ( int )std::floor( maxEdgeY - EPS );
      minZ = ( int )std::floor( minEdgeZ + EPS );
      maxZ = ( int )std::floor( maxEdgeZ - EPS );
   };

   // Check if we're standing on something without needing to move down this frame.
   auto computeGrounded = [ & ]( const glm::vec3& pos, const glm::vec3& half ) -> bool
   {
      // Probe a tiny distance below the feet.
      glm::vec3 probePos = pos;
      probePos.y -= GROUND_PROBE;

      int minX, maxX, minY, maxY, minZ, maxZ;
      getVoxelBounds( probePos, half, minX, maxX, minY, maxY, minZ, maxZ );

      for( int y = minY; y <= maxY; ++y )
      {
         for( int x = minX; x <= maxX; ++x )
            for( int z = minZ; z <= maxZ; ++z )
               if( isSolid( x, y, z ) )
                  return true;
      }
      return false;
   };

   // Resolve movement along X by clamping to nearest blocking voxel face
   auto moveAndCollideX = [ & ]( glm::vec3& pos, glm::vec3& vel, const glm::vec3& half, float dx )
   {
      if( dx == 0.0f )
         return;

      pos.x += dx;

      int minX, maxX, minY, maxY, minZ, maxZ;
      getVoxelBounds( pos, half, minX, maxX, minY, maxY, minZ, maxZ );

      if( dx > 0.0f )
      {
         // moving +X: find the nearest solid voxel at/inside our maxX plane
         int hitX = INT_MAX;
         for( int x = minX; x <= maxX; ++x )
         {
            for( int y = minY; y <= maxY; ++y )
               for( int z = minZ; z <= maxZ; ++z )
               {
                  if( isSolid( x, y, z ) )
                  {
                     hitX = (std::min)( hitX, x );
                     break;
                  }
               }
         }

         if( hitX != INT_MAX )
         {
            // clamp so our max face sits just left of hitX face
            pos.x = ( float )hitX - half.x - SKIN;
            vel.x = 0.0f;
         }
      }
      else
      {
         // moving -X: find the nearest solid voxel at/inside our minX plane (largest x)
         int hitX = INT_MIN;
         for( int x = maxX; x >= minX; --x )
         {
            for( int y = minY; y <= maxY; ++y )
               for( int z = minZ; z <= maxZ; ++z )
               {
                  if( isSolid( x, y, z ) )
                  {
                     hitX = (std::max)( hitX, x );
                     break;
                  }
               }
         }

         if( hitX != INT_MIN )
         {
            // clamp so our min face sits just right of (hitX + 1) face
            pos.x = ( float )( hitX + 1 ) + half.x + SKIN;
            vel.x = 0.0f;
         }
      }
   };

   // Resolve movement along Y
   auto moveAndCollideY = [ & ]( glm::vec3& pos, glm::vec3& vel, CPhysics& phys, float dy )
   {
      if( dy == 0.0f )
         return;

      pos.y += dy;

      int minX, maxX, minY, maxY, minZ, maxZ;
      getVoxelBounds( pos, phys.halfExtents, minX, maxX, minY, maxY, minZ, maxZ );

      if( dy > 0.0f )
      {
         int hitY = INT_MAX;
         for( int y = minY; y <= maxY; ++y )
         {
            for( int x = minX; x <= maxX; ++x )
               for( int z = minZ; z <= maxZ; ++z )
               {
                  if( isSolid( x, y, z ) )
                  {
                     hitY = (std::min)( hitY, y );
                     break;
                  }
               }
         }

         if( hitY != INT_MAX )
         {
            pos.y = ( float )hitY - phys.halfExtents.y - SKIN;
            vel.y = 0.0f;
            phys.fOnGround = true;
         }
      }
      else
      {
         int hitY = INT_MIN;
         for( int y = maxY; y >= minY; --y )
         {
            for( int x = minX; x <= maxX; ++x )
               for( int z = minZ; z <= maxZ; ++z )
               {
                  if( isSolid( x, y, z ) )
                  {
                     hitY = (std::max)( hitY, y );
                     break;
                  }
               }
         }

         if( hitY != INT_MIN )
         {
            pos.y = ( float )( hitY + 1 ) + phys.halfExtents.y + SKIN;
            vel.y = 0.0f;
            phys.fOnGround = true;
         }
      }
   };

   // Resolve movement along Z
   auto moveAndCollideZ = [ & ]( glm::vec3& pos, glm::vec3& vel, const glm::vec3& half, float dz )
   {
      if( dz == 0.0f )
         return;

      pos.z += dz;

      int minX, maxX, minY, maxY, minZ, maxZ;
      getVoxelBounds( pos, half, minX, maxX, minY, maxY, minZ, maxZ );

      if( dz > 0.0f )
      {
         int hitZ = INT_MAX;
         for( int z = minZ; z <= maxZ; ++z )
         {
            for( int x = minX; x <= maxX; ++x )
               for( int y = minY; y <= maxY; ++y )
               {
                  if( isSolid( x, y, z ) )
                  {
                     hitZ = (std::min)( hitZ, z );
                     break;
                  }
               }
         }

         if( hitZ != INT_MAX )
         {
            pos.z = ( float )hitZ - half.z - SKIN;
            vel.z = 0.0f;
         }
      }
      else
      {
         int hitZ = INT_MIN;
         for( int z = maxZ; z >= minZ; --z )
         {
            for( int x = minX; x <= maxX; ++x )
               for( int y = minY; y <= maxY; ++y )
               {
                  if( isSolid( x, y, z ) )
                  {
                     hitZ = (std::max)( hitZ, z );
                     break;
                  }
               }
         }

         if( hitZ != INT_MIN )
         {
            pos.z = ( float )( hitZ + 1 ) + half.z + SKIN;
            vel.z = 0.0f;
         }
      }
   };

   // ---- Single-pass integration + world collision (NO double movement) ----
   for( auto [ tran, vel, phys ] : registry.CView< CTransform, CVelocity, CPhysics >() )
   {
      // Preserve grounded state if we're already resting on something.
      // This prevents a 1-frame "airborne" when step.y == 0 and thus no Y collision test runs.
      phys.fOnGround = computeGrounded( tran.position, phys.halfExtents );

       // Gravity (keep your sign conventions)
       if( !phys.fOnGround)
         vel.velocity.y = ( std::max )( vel.velocity.y + ( GRAVITY * delta ), TERMINAL_VELOCITY );

       // Desired movement this frame
       const glm::vec3 step = vel.velocity * delta;

       glm::vec3 pos = tran.position;

       // Axis-separated move & clamp
       moveAndCollideY( pos, vel.velocity, phys, step.y );
       moveAndCollideX( pos, vel.velocity, phys.halfExtents, step.x );
       moveAndCollideZ( pos, vel.velocity, phys.halfExtents, step.z );

       tran.position = pos;
    }

    // ---- Optional: entity-vs-entity stays after world, but note it can fight world resolution ----
    // Keep your existing entity collision if you want, but it is simplistic and can cause odd pushes.
}


void CameraSystem( Entity::Registry& registry, Window& window )
{
   //PROFILE_SCOPE( "CameraSystem" );
   GLFWwindow* nativeWindow   = window.GetNativeWindow();
   bool        fMouseCaptured = glfwGetInputMode( nativeWindow, GLFW_CURSOR ) == GLFW_CURSOR_DISABLED;

   glm::vec2 mouseDelta { 0.0f };
   if( fMouseCaptured )
   {
      static glm::vec2 lastMousePos = window.GetMousePosition();
      mouseDelta                    = window.GetMousePosition() - lastMousePos;

      const WindowData& windowData = window.GetWindowData();
      lastMousePos                 = { windowData.Width * 0.5f, windowData.Height * 0.5f };
      glfwSetCursorPos( nativeWindow, lastMousePos.x, lastMousePos.y );
   }

   for( auto [ tran, cam, tag ] : registry.CView< CTransform, CCamera, CCameraTag >() )
   {
      if( fMouseCaptured )
      {
         tran.rotation.x = glm::clamp( tran.rotation.x + ( mouseDelta.y * cam.sensitivity ), -90.0f, 90.0f );
         tran.rotation.y = glm::mod( tran.rotation.y + ( mouseDelta.x * cam.sensitivity ) + 360.0f, 360.0f );
      }

      cam.view = glm::mat4( 1.0f );
      cam.view = glm::rotate( cam.view, glm::radians( tran.rotation.x ), { 1, 0, 0 } );
      cam.view = glm::rotate( cam.view, glm::radians( tran.rotation.y ), { 0, 1, 0 } );
      cam.view = glm::rotate( cam.view, glm::radians( tran.rotation.z ), { 0, 0, 1 } );
      cam.view = glm::translate( cam.view, -tran.position );

      cam.viewProjection = cam.projection * cam.view;
   }
}

// Simple helper to load a 2D image; replace with your own loader.
// Returns raw RGBA8 data + width/height, or false on failure.
struct ImageData
{
   int            width  = 0;
   int            height = 0;
   unsigned char* pixels = nullptr; // RGBA8
};

GLuint      blockTextureArrayId = 0;
static void InitBlockTextureArray()
{
   if( blockTextureArrayId != 0 )
      return; // already initialized

   // List your block textures here, order = layer index
   // layer 0 can be a dummy/air texture if you want
   struct TexDef
   {
      const char* path;
   };

   const std::vector< TexDef > texDefs = {
      { "res/textures/dirt.png" },  // layer 0 (optional)
      { "res/textures/dirt.png" },  // layer 1 (BLOCK_DIRT)
      { "res/textures/stone.png" }, // layer 2 (BLOCK_STONE)
   };

   std::vector< ImageData > images;
   images.reserve( texDefs.size() );

   int atlasWidth  = 0;
   int atlasHeight = 0;

   // Load all images
   for( const auto& def : texDefs )
   {
      ImageData img;
      int       bpp;
      img.pixels = stbi_load( def.path, &img.width, &img.height, &bpp, 4 );
      if( !img.pixels )
      {
         std::cerr << "Failed to load texture: " << def.path << " | Reason: " << stbi_failure_reason() << "\n";
         continue;
      }

      if( atlasWidth == 0 )
      {
         atlasWidth  = img.width;
         atlasHeight = img.height;
      }
      else
      {
         // All layers must have same size
         if( img.width != atlasWidth || img.height != atlasHeight )
         {
            std::cerr << "Block texture size mismatch: " << def.path << "\n";
            return;
         }
      }

      images.push_back( std::move( img ) );
   }

   // Create texture array
   glGenTextures( 1, &blockTextureArrayId );
   glBindTexture( GL_TEXTURE_2D_ARRAY, blockTextureArrayId );

   const GLsizei layers = static_cast< GLsizei >( images.size() );

   glTexImage3D( GL_TEXTURE_2D_ARRAY,
                 0,        // level
                 GL_RGBA8, // internal format
                 atlasWidth,
                 atlasHeight,
                 layers,           // depth = number of layers
                 0,                // border
                 GL_RGBA,          // data format
                 GL_UNSIGNED_BYTE, // data type
                 nullptr           // no data yet, we'll upload per-layer
   );

   // Upload each layer
   for( GLsizei layer = 0; layer < layers; ++layer )
   {
      const auto& img = images[ layer ];
      glTexSubImage3D( GL_TEXTURE_2D_ARRAY,
                       0, // level
                       0,
                       0,
                       layer, // x, y, zoffset
                       atlasWidth,
                       atlasHeight,
                       1, // depth = 1 layer
                       GL_RGBA,
                       GL_UNSIGNED_BYTE,
                       img.pixels );
   }

   // Set filtering and wrapping
   glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST );
   glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
   glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT );
   glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT );

   glGenerateMipmap( GL_TEXTURE_2D_ARRAY );

   glBindTexture( GL_TEXTURE_2D_ARRAY, 0 );
}


std::optional< glm::vec3 > g_highlightBlock;
void                       RenderSystem( Entity::Registry& registry, Level& level, const CCamera& camera, const glm::vec3& camPosition )
{
   //PROFILE_SCOPE( "RenderSystem" );
   // TODO - refactor this system, Shader should be within component data for entity

   // hack to get working
   static std::unique_ptr< Shader > pShader = std::make_unique< Shader >( Shader::FILE, "basic_vert.glsl", "basic_frag.glsl" );

   pShader->Bind();                                       // Bind the shader program
   pShader->SetUniform( "u_MVP", camera.viewProjection ); // Set Model-View-Projection matrix
   pShader->SetUniform( "u_viewPos", camPosition );       // Set camera position
   pShader->SetUniform( "u_lightPos", camPosition );      // Set light position at camera position

   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ); // clear color and depth buffers

   {
      glActiveTexture( GL_TEXTURE0 );
      glBindTexture( GL_TEXTURE_2D_ARRAY, blockTextureArrayId );
      pShader->SetUniform( "u_blockTextures", 0 );

      pShader->SetUniform( "u_color", glm::vec3( 1.0f, 1.0f, 1.0f ) );
      for( const auto& [ coord, pChunk ] : level.GetChunks() )
      {
         if( !pChunk )
            continue;

         const ChunkRender* pRender = level.GetChunkRender( coord );
         if( !pRender )
            continue;

         glm::vec3 chunkOffset( coord.x * CHUNK_SIZE_X, 0 /*y*/, coord.z * CHUNK_SIZE_Z );
         glm::mat4 model = glm::translate( glm::mat4( 1.0f ), chunkOffset );
         glm::mat4 mvp   = camera.viewProjection * model;
         pShader->SetUniform( "u_MVP", mvp );
         pShader->SetUniform( "u_Model", model );
         pRender->Render();
      }
   }

   // Render all entities with CMesh component
   pShader->SetUniform( "u_MVP", camera.viewProjection ); // Set Model-View-Projection matrix
   for( auto [ tran, mesh ] : registry.CView< CTransform, CMesh >() )
   {
      pShader->SetUniform( "u_color", mesh.mesh->GetColor() );

      mesh.mesh->SetPosition( tran.position ); // this shouldn't be here
      mesh.mesh->Render();
   }

   // Render CAABB bounds for debugging
   for( auto [ tran, phys ] : registry.CView< CTransform, CPhysics >() )
   {
      glm::mat4 model = glm::translate( glm::mat4( 1.0f ), tran.position );
      glm::mat4 mvp   = camera.viewProjection * model; // MVP = Projection * View * Model
      pShader->SetUniform( "u_MVP", mvp );

      // Build vertices relative to origin
      glm::vec3 min           = -phys.halfExtents;
      glm::vec3 max           = phys.halfExtents;
      glm::vec3 vertices[ 8 ] = {
         { min.x, min.y, min.z },
         { max.x, min.y, min.z },
         { max.x, max.y, min.z },
         { min.x, max.y, min.z },
         { min.x, min.y, max.z },
         { max.x, min.y, max.z },
         { max.x, max.y, max.z },
         { min.x, max.y, max.z }
      };

      constexpr unsigned int indices[ 24 ] = { 0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6, 6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7 };

      unsigned int vao, vbo, ebo;
      glGenVertexArrays( 1, &vao );
      glGenBuffers( 1, &vbo );
      glGenBuffers( 1, &ebo );

      glBindVertexArray( vao );
      glBindBuffer( GL_ARRAY_BUFFER, vbo );
      glBufferData( GL_ARRAY_BUFFER, sizeof( vertices ), vertices, GL_STATIC_DRAW );

      glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ebo );
      glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( indices ), indices, GL_STATIC_DRAW );

      glEnableVertexAttribArray( 0 );
      glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof( float ), ( void* )0 );

      glDrawElements( GL_LINES, 24, GL_UNSIGNED_INT, 0 );

      glDeleteBuffers( 1, &vbo );
      glDeleteBuffers( 1, &ebo );
      glDeleteVertexArrays( 1, &vao );
   }

   // Highlight block under crosshair
   {
      // Highlight current block
      if( g_highlightBlock.has_value() )
      {
         // Use the same shader; we draw a simple wireframe cube around the block
         glm::vec3 blockPos = *g_highlightBlock;

         // Model: translate to block center, scale to slightly larger than 1x1x1
         glm::mat4 model = glm::translate( glm::mat4( 1.0f ), blockPos + glm::vec3( 0.5f ) );
         model           = glm::scale( model, glm::vec3( 1.02f ) ); // tiny inflation

         glm::mat4 mvp = camera.viewProjection * model;
         pShader->SetUniform( "u_MVP", mvp );
         pShader->SetUniform( "u_color", glm::vec3( 1.0f, 1.0f, 1.0f ) ); // white

         // Unit cube wireframe in local space [-0.5,0.5]^3
         glm::vec3 min           = glm::vec3( -0.5f );
         glm::vec3 max           = glm::vec3( 0.5f );
         glm::vec3 vertices[ 8 ] = {
            { min.x, min.y, min.z },
            { max.x, min.y, min.z },
            { max.x, max.y, min.z },
            { min.x, max.y, min.z },
            { min.x, min.y, max.z },
            { max.x, min.y, max.z },
            { max.x, max.y, max.z },
            { min.x, max.y, max.z },
         };

         constexpr unsigned int indices[ 24 ] = {
            0, 1, 1, 2, 2, 3, 3, 0, // bottom
            4, 5, 5, 6, 6, 7, 7, 4, // top
            0, 4, 1, 5, 2, 6, 3, 7  // verticals
         };

         GLuint vao = 0, vbo = 0, ebo = 0;
         glGenVertexArrays( 1, &vao );
         glGenBuffers( 1, &vbo );
         glGenBuffers( 1, &ebo );

         glBindVertexArray( vao );
         glBindBuffer( GL_ARRAY_BUFFER, vbo );
         glBufferData( GL_ARRAY_BUFFER, sizeof( vertices ), vertices, GL_STATIC_DRAW );

         glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ebo );
         glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( indices ), indices, GL_STATIC_DRAW );

         glEnableVertexAttribArray( 0 );
         glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof( float ), ( void* )0 );

         glDrawElements( GL_LINES, 24, GL_UNSIGNED_INT, 0 );

         glDeleteBuffers( 1, &vbo );
         glDeleteBuffers( 1, &ebo );
         glDeleteVertexArrays( 1, &vao );
      }
   }

   pShader->Unbind();

   // Draw Skybox
   {
      constexpr std::string_view faceFiles[ 6 ] = { "res/textures/skybox/px.png", "res/textures/skybox/nx.png", "res/textures/skybox/py.png",
                                                    "res/textures/skybox/ny.png", "res/textures/skybox/pz.png", "res/textures/skybox/nz.png" };
      static SkyboxTexture       skyboxTexture( faceFiles );
      skyboxTexture.Draw( camera.view, camera.projection );
   }

   // Draw simple reticle
   {
      pShader->Bind();

      glDisable( GL_DEPTH_TEST );
      pShader->SetUniform( "u_MVP", glm::mat4( 1.0f ) );
      pShader->SetUniform( "u_color", glm::vec3( 1.0f, 1.0f, 1.0f ) );

      const float                r            = 0.01f; // reticle arm length in NDC
      std::array< glm::vec3, 4 > reticleVerts = {
         { { -r, 0.0f, 0.0f }, { r, 0.0f, 0.0f }, { 0.0f, -r, 0.0f }, { 0.0f, r, 0.0f } }
      };

      GLuint reticleVao = 0, reticleVbo = 0;
      glGenVertexArrays( 1, &reticleVao );
      glGenBuffers( 1, &reticleVbo );
      glBindVertexArray( reticleVao );
      glBindBuffer( GL_ARRAY_BUFFER, reticleVbo );
      glBufferData( GL_ARRAY_BUFFER, sizeof( reticleVerts ), reticleVerts.data(), GL_STATIC_DRAW );
      glEnableVertexAttribArray( 0 );
      glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof( float ), ( void* )0 );
      glDrawArrays( GL_LINES, 0, reticleVerts.size() );
      glDeleteBuffers( 1, &reticleVbo );
      glDeleteVertexArrays( 1, &reticleVao );
      glEnable( GL_DEPTH_TEST );

      pShader->Unbind();
   }
}

void TickingSystem( Entity::Registry& /*registry*/, float /*delta*/ )
{
   //PROFILE_SCOPE( "TickingSystem" );
}

struct Ray
{
   glm::vec3 origin;
   glm::vec3 direction;
};

struct RaycastResult
{
   glm::vec3  position;
   glm::ivec3 normal;
   float      distance;
};

// add debug rendering for Raycasts

std::optional< RaycastResult > DoRaycast( const Level& level, const Ray& ray, float maxDistance )
{
   PROFILE_SCOPE( "DoRaycast" );

   glm::vec3 dir = glm::normalize( ray.direction );
   if( glm::dot( dir, dir ) < 1e-12f )
      return std::nullopt;

   // Current voxel
   glm::ivec3 blockPos = glm::floor( ray.origin );

   // If we start inside a block, hit it immediately
   // if( FIsSolid( level.GetBlock( blockPos.x, blockPos.y, blockPos.z ) ) )
   if( level.GetBlock( blockPos.x, blockPos.y, blockPos.z ) != BLOCK_AIR )
      return RaycastResult { glm::vec3( blockPos ), glm::ivec3( 0 ) /*normal*/, 0.0f /*distance*/ };

   // DDA setup
   glm::ivec3 step( dir.x < 0.0f ? -1 : 1, dir.y < 0.0f ? -1 : 1, dir.z < 0.0f ? -1 : 1 );
   glm::vec3  invDir( dir.x != 0.0f ? 1.0f / dir.x : std::numeric_limits< float >::infinity(),
                     dir.y != 0.0f ? 1.0f / dir.y : std::numeric_limits< float >::infinity(),
                     dir.z != 0.0f ? 1.0f / dir.z : std::numeric_limits< float >::infinity() );

   // Distance from origin to first voxel boundary on each axis
   glm::vec3 tMax {};
   for( int i = 0; i < 3; ++i )
      tMax[ i ] = ( static_cast< float >( blockPos[ i ] ) + ( step[ i ] > 0 ? 1.0f : 0.0f ) - ray.origin[ i ] ) * invDir[ i ];

   float      dist      = 0.0f;
   glm::vec3  tDelta    = glm::abs( invDir );
   glm::ivec3 hitNormal = glm::ivec3( 0 );
   while( dist <= maxDistance )
   {
      // TODO: integrate world access and solidity check here.
      // if( FIsSolid( level.GetBlock( blockPos.x, blockPos.y, blockPos.z ) ) )
      if( level.GetBlock( blockPos.x, blockPos.y, blockPos.z ) != BLOCK_AIR )
         return RaycastResult { glm::vec3( blockPos ), hitNormal, dist };

      // Determine next axis to step
      int axis = 0;
      if( tMax.y < tMax.x )
         axis = 1;
      if( tMax.z < tMax[ axis ] )
         axis = 2;

      dist = tMax[ axis ];
      tMax[ axis ] += tDelta[ axis ];
      blockPos[ axis ] += step[ axis ];
      hitNormal         = glm::ivec3( 0 );
      hitNormal[ axis ] = -step[ axis ];
   }

   return std::nullopt;
}


void Application::Run()
{
   Entity::Registry registry;

   // Create player entity
   Entity::Entity player = registry.Create();
   registry.Add< CTransform >( player, 0.0f, 128.0f + ( PLAYER_HEIGHT / 2 ), 0.0f );
   registry.Add< CVelocity >( player, 0.0f, 0.0f, 0.0f );
   registry.Add< CInput >( player );
   registry.Add< CPlayerTag >( player );
   registry.Add< CPhysics >( player, glm::vec3( 0.3f, 0.9f, 0.3f ) ); // center at body

   // Create camera entity
   Entity::Entity camera = registry.Create();
   registry.Add< CTransform >( camera, 0.0f, 128.0f + PLAYER_EYE_HEIGHT, 0.0f );
   registry.Add< CCameraTag >( camera );

   if( CCamera* pCameraComp = &registry.Add< CCamera >( camera ) )
      pCameraComp->projection = glm::perspective( glm::radians( pCameraComp->fov ),
                                                  Window::Get().GetWindowData().Width / static_cast< float >( Window::Get().GetWindowData().Height ),
                                                  0.1f,
                                                  1000.0f );

   Events::EventSubscriber eventSubscriber;
   eventSubscriber.Subscribe< Events::WindowResizeEvent >( [ &registry, camera ]( const Events::WindowResizeEvent& e ) noexcept
   {
      if( e.GetHeight() == 0 )
         return;

      if( CCamera* pCameraComp = registry.TryGet< CCamera >( camera ) )
         pCameraComp->projection = glm::perspective( glm::radians( pCameraComp->fov ), e.GetWidth() / static_cast< float >( e.GetHeight() ), 0.1f, 1000.0f );
   } );

   eventSubscriber.Subscribe< Events::MouseScrolledEvent >( [ &registry, camera ]( const Events::MouseScrolledEvent& e ) noexcept
   {
      if( CCamera* pCameraComp = registry.TryGet< CCamera >( camera ) )
      {
         pCameraComp->fov        = std::clamp( pCameraComp->fov - ( e.GetYOffset() * 5 ), 10.0f, 90.0f );
         pCameraComp->projection = glm::perspective( glm::radians( pCameraComp->fov ),
                                                     Window::Get().GetWindowData().Width / static_cast< float >( Window::Get().GetWindowData().Height ),
                                                     0.1f,
                                                     1000.0f );
      }
   } );

   eventSubscriber.Subscribe< Events::KeyPressedEvent >( [ &registry, player ]( const Events::KeyPressedEvent& e ) noexcept
   {
      switch( e.GetKeyCode() )
      {
         case Input::KeyCode::R: registry.Get< CTransform >( player ).position.y = 128.0f; break;
      }
   } );

   Level level;
   eventSubscriber.Subscribe< Events::MouseButtonPressedEvent >( [ & ]( const Events::MouseButtonPressedEvent& e ) noexcept
   {
      if( e.GetMouseButton() != Input::ButtonLeft && e.GetMouseButton() != Input::ButtonRight )
         return;

      // Get camera transform
      CTransform* pCamTran = registry.TryGet< CTransform >( camera );
      CCamera*    pCamComp = registry.TryGet< CCamera >( camera );
      if( !pCamTran || !pCamComp )
         return;

      // Camera forward vector from yaw (y) and pitch (x). This is equivalent
      // to rotating the default forward (0,0,-1) by pitch then yaw.
      float pitch = glm::radians( pCamTran->rotation.x );
      float yaw   = glm::radians( pCamTran->rotation.y );

      glm::vec3 dir {};
      dir.x = glm::cos( pitch ) * glm::sin( yaw );
      dir.y = -glm::sin( pitch );
      dir.z = -glm::cos( pitch ) * glm::cos( yaw );
      dir   = glm::normalize( dir );

      glm::vec3 rayOrigin   = pCamTran->position;
      float     maxDistance = 64.0f;

      if( std::optional< RaycastResult > result = DoRaycast( level, Ray { rayOrigin, dir }, maxDistance ) )
      {
         std::cout << "Raycast hit at position: " << result->position.x << ", " << result->position.y << ", " << result->position.z << "\n";
         if( e.GetMouseButton() == Input::ButtonLeft )
         {
            level.SetBlock( static_cast< int >( result->position.x ),
                            static_cast< int >( result->position.y ),
                            static_cast< int >( result->position.z ),
                            BLOCK_AIR );
         }
         else
         {
            level.Explode( static_cast< int >( result->position.x ),
                           static_cast< int >( result->position.y ),
                           static_cast< int >( result->position.z ),
                           96 /*radius*/ );
         }
      }
      else
         std::cout << "Raycast did not hit any block within " << maxDistance << " units.\n";
   } );
   // -----------------------------------

   // Initialize things
   Window& window = Window::Get();
   window.Init();

   InitBlockTextureArray();

   GUIManager::Init();
   INetwork::RegisterGUI();

   Timestep timestep( 20 /*tickrate*/ );
   GUIManager::Attach( std::make_shared< DebugGUI >( registry, player, camera, timestep ) );

   std::shared_ptr< Entity::EntityHandle > psBall = registry.CreateWithHandle();
   registry.Add< CTransform >( psBall->Get(), 5.0f, 128.0f, 5.0f );
   registry.Add< CMesh >( psBall->Get(), std::make_shared< SphereMesh >() );
   registry.Add< CVelocity >( psBall->Get(), glm::vec3( 0.0f, 0.0f, 0.0f ) );
   registry.Add< CPhysics >( psBall->Get(), CPhysics { glm::vec3( 0.5f ) } );

   std::shared_ptr< Entity::EntityHandle > psCube = registry.CreateWithHandle();
   registry.Add< CTransform >( psCube->Get(), 6.0f, 128.0f, 8.0f );
   registry.Add< CMesh >( psCube->Get(), std::make_shared< CubeMesh >() );
   registry.Add< CVelocity >( psCube->Get(), glm::vec3( 0.0f, 0.0f, 0.0f ) );
   registry.Add< CPhysics >( psCube->Get(), CPhysics { glm::vec3( 1.0f ) } );

   while( window.IsOpen() )
   {
      // Clamp simulation dt to avoid huge steps when dragging/resizing window
      const float delta = std::clamp( timestep.Step(), 0.0f, 0.05f /*50ms*/ );

      CTransform* pPlayerTran = registry.TryGet< CTransform >( player );
      CTransform* pCameraTran = registry.TryGet< CTransform >( camera );
      CCamera*    pCameraComp = registry.TryGet< CCamera >( camera );

      {
         if( pPlayerTran )
            level.UpdateVisibleRegion( pPlayerTran->position, 8 /*viewRadius*/ );

         Events::ProcessQueuedEvents();
         // ProcessQueuedNetworkEvents(); - maybe do this here?

         PlayerInputSystem( registry, delta );
         PlayerPhysicsSystem( registry, level, delta );
         CameraSystem( registry, window );

         if( timestep.FTick() )
         {
            TickingSystem( registry, delta ); // TODO: implement this system (with CTick component)

            // remove this eventually
            if( INetwork* pNetwork = INetwork::Get(); pNetwork && pPlayerTran )
               pNetwork->Poll( pPlayerTran->position );
         }

         if( pPlayerTran && pCameraTran ) // hacky way to set camera position to follow player (update before render, should change)
         {
            pCameraTran->position = pPlayerTran->position + glm::vec3( 0.0f, PLAYER_EYE_HEIGHT - ( PLAYER_HEIGHT / 2 ), 0.0f );
            pPlayerTran->rotation = pCameraTran->rotation;
         }

         if( pCameraTran && pCameraComp )
         {
            // Recompute camera forward like in mouse handler
            float pitch = glm::radians( pCameraTran->rotation.x );
            float yaw   = glm::radians( pCameraTran->rotation.y );

            glm::vec3 dir {};
            dir.x = glm::cos( pitch ) * glm::sin( yaw );
            dir.y = -glm::sin( pitch );
            dir.z = -glm::cos( pitch ) * glm::cos( yaw );
            dir   = glm::normalize( dir );

            glm::vec3 rayOrigin   = pCameraTran->position;
            float     maxDistance = 64.0f;

            std::optional< glm::vec3 > highlightBlock;
            if( std::optional< RaycastResult > result = DoRaycast( level, Ray { rayOrigin, dir }, maxDistance ) )
               highlightBlock = result->position; // result->position is integer block coords

            g_highlightBlock = highlightBlock;
         }

         if( !window.FMinimized() && pCameraComp && pCameraTran )
         {
            RenderSystem( registry, level, *pCameraComp, pCameraTran->position );
            GUIManager::Draw();
         }
      }

      window.OnUpdate();
   }

   // Shutdown everything
   GUIManager::Shutdown();
}
