#pragma once

#include <Engine/ECS/ISystem.h>
#include <Engine/ECS/Resources/BlockInteractionResource.h>
#include <Engine/World/BlockDefs.h>

namespace Engine::ECS
{

class BlockIntentSystem final : public ISystem
{
public:
   explicit BlockIntentSystem( BlockInteractionResource& res, Level& level ) : m_res( res ), m_level( level ) {}

   SystemPhase Phase() const noexcept override { return SystemPhase::Intent; }
   void        FixedTick( FixedTickContext& ctx ) override;

private:
   BlockInteractionResource& m_res;
   Level&                    m_level;
};

class BlockHitSystem final : public ISystem
{
public:
   explicit BlockHitSystem( BlockInteractionResource& res, Level& level ) : m_res( res ), m_level( level ) {}

   SystemPhase Phase() const noexcept override { return SystemPhase::Simulation; }
   void        FixedTick( FixedTickContext& ctx ) override;

private:
   BlockInteractionResource& m_res;
   Level&                    m_level;
};

class BlockBreakSystem final : public ISystem
{
public:
   explicit BlockBreakSystem( BlockInteractionResource& res, Level& level ) : m_res( res ), m_level( level ) {}

   SystemPhase Phase() const noexcept override { return SystemPhase::Simulation; }
   void        FixedTick( FixedTickContext& ctx ) override;

private:
   BlockInteractionResource& m_res;
   Level&                    m_level;
};

class BlockUseSystem final : public ISystem
{
public:
   explicit BlockUseSystem( BlockInteractionResource& res, Level& level ) : m_res( res ), m_level( level ) {}

   SystemPhase Phase() const noexcept override { return SystemPhase::LateSimulation; }
   void        FixedTick( FixedTickContext& ctx ) override;

private:
   BlockInteractionResource& m_res;
   Level&                    m_level;
};

class BlockEntityInteractSystem final : public ISystem
{
public:
   explicit BlockEntityInteractSystem( BlockInteractionResource& res ) : m_res( res ) {}

   SystemPhase Phase() const noexcept override { return SystemPhase::Presentation; }
   void        Tick( TickContext& ctx ) override;

private:
   BlockInteractionResource& m_res;
};

} // namespace Engine::ECS
