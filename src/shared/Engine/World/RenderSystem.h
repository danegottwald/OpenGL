#pragma once

#include <Engine/World/ChunkRenderer.h>
#include <Engine/Core/Time.h>

#include <Engine/ECS/Registry.h>
#include <Engine/ECS/Components.h>

namespace Engine
{

struct RenderKey
{
   // [ Transparent (1) | Layer (8) | Shader (8) | Material (15) ] (implementation-defined by caller)
   uint32_t value {};
   bool     operator<( const RenderKey& other ) const { return value < other.value; }
};

class RenderQueues
{
public:
   struct IndexedDraw
   {
      RenderKey key;
      uint32_t  vertexArrayId {};
      uint32_t  textureId {};
      uint32_t  indexCount {};
      glm::mat4 model { 1.0f };
   };

   void Clear()
   {
      m_opaqueIndexed.clear();
      m_overlayIndexed.clear();
   }

   void SubmitOpaque( const IndexedDraw& item ) { m_opaqueIndexed.push_back( item ); }
   void SubmitOverlay( const IndexedDraw& item ) { m_overlayIndexed.push_back( item ); }

   void Sort()
   {
      auto byKey = []( const IndexedDraw& a, const IndexedDraw& b ) { return a.key < b.key; };
      std::sort( m_opaqueIndexed.begin(), m_opaqueIndexed.end(), byKey );
      std::sort( m_overlayIndexed.begin(), m_overlayIndexed.end(), byKey );
   }

   std::span< const IndexedDraw > GetOpaqueIndexed() const { return m_opaqueIndexed; }
   std::span< const IndexedDraw > GetOverlayIndexed() const { return m_overlayIndexed; }

private:
   std::vector< IndexedDraw > m_opaqueIndexed;
   std::vector< IndexedDraw > m_overlayIndexed;
};

class RenderSystem
{
public:
   explicit RenderSystem( Level& level ) noexcept;
   ~RenderSystem() = default;

   void Update( const glm::vec3& playerPos, uint8_t viewRadius );

   struct FrameContext
   {
      Entity::Registry&          registry;
      const Time::FixedTimeStep& time;

      glm::mat4 view { 1.0f };
      glm::mat4 projection { 1.0f };
      glm::mat4 viewProjection { 1.0f };
      glm::vec3 viewPos { 0.0f };

      std::optional< glm::ivec3 > optHighlightBlock;
   };

   void Run( const FrameContext& ctx );

   void EnableSkybox( bool fEnable ) noexcept { m_fSkyboxEnabled = fEnable; }
   void EnableReticle( bool fEnable ) noexcept { m_fReticleEnabled = fEnable; }
   void EnableBlockHighlight( bool fEnable ) noexcept { m_fHighlightEnabled = fEnable; }

private:
   NO_COPY_MOVE( RenderSystem )

   void BuildQueues( const FrameContext& ctx, RenderQueues& outQueues );

   void DrawTerrainPass( const FrameContext& ctx );
   void DrawOpaquePass( const FrameContext& ctx, const RenderQueues& queues );
   void DrawOverlayPass( const FrameContext& ctx, const RenderQueues& queues );

   void QueueItemDrops( const FrameContext& ctx, RenderQueues& outQueues );
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
