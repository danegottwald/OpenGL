#pragma once

#include <Engine/World/ChunkRenderer.h>

#include <Engine/ECS/Registry.h>
#include <Engine/ECS/Components.h>

namespace Engine
{

// maybe can have a IFrameContext interface
//    holds common system data (registry, etc?)
//    pass that to systems' Run() methods

class RenderSystem
{
public:
   explicit RenderSystem( Level& level ) noexcept;
   ~RenderSystem() = default;

   void Update( const glm::vec3& playerPos, uint8_t viewRadius );

   struct FrameContext
   {
      Entity::Registry& registry;
      glm::mat4         view { 1.0f };
      glm::mat4         projection { 1.0f };
      glm::mat4         viewProjection { 1.0f };
      glm::vec3         viewPos { 0.0f };

      std::optional< glm::ivec3 > optHighlightBlock; // current aim at block
   };

   // Extensible entrypoint.
   void Run( const FrameContext& ctx );

   // Toggles for optional features (can become proper sub-renderers later)
   void EnableSkybox( bool fEnable ) noexcept { m_fSkyboxEnabled = fEnable; }
   void EnableReticle( bool fEnable ) noexcept { m_fReticleEnabled = fEnable; }
   void EnableBlockHighlight( bool fEnable ) noexcept { m_fHighlightEnabled = fEnable; }

private:
   NO_COPY_MOVE( RenderSystem )

   void DrawTerrain( const FrameContext& ctx );
   void DrawBlockHighlight( const FrameContext& ctx );
   void DrawSkybox( const FrameContext& ctx );
   void DrawReticle( const FrameContext& ctx );

   Level&        m_level;
   ChunkRenderer m_chunkRenderer;

   bool m_fSkyboxEnabled { true };
   bool m_fReticleEnabled { true };
   bool m_fHighlightEnabled { true };
};

} // namespace Engine
