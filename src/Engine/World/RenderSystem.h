#pragma once

#include <Engine/World/ChunkRenderer.h>

#include <Engine/ECS/Registry.h>
#include <Engine/ECS/Components.h>

namespace Engine
{

// Used to sort draw calls to minimize state changes
struct RenderKey
{
   // Bitfields are great for sorting:
   // [ Transparent (1) | Layer (8) | Shader (8) | Material (15) ]
   uint32_t value;
   bool     operator<( const RenderKey& other ) const { return value < other.value; }
};

class RenderBuffer
{
public:
   RenderBuffer() { m_items.reserve( 1000 ); }

   struct Item
   {
      RenderKey key;
      uint32_t  vertexArrayId;
      uint32_t  textureId;
      uint32_t  indexCount;
      glm::mat4 model; // heavy, bad for sorting
   };

   void Clear() { m_items.clear(); }
   void Submit( const Item& item ) { m_items.push_back( item ); }

   void Sort()
   {
      // Sorting by key ensures similar materials/shaders are drawn together
      std::sort( m_items.begin(), m_items.end(), []( const Item& a, const Item& b ) { return a.key < b.key; } );
   }

   std::span< const Item > GetItems() const { return m_items; }

private:
   std::vector< Item > m_items;
};

// global render buffer
inline RenderBuffer g_renderBuffer;

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

      std::optional< glm::ivec3 > optHighlightBlock; // currently aiming at block
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
