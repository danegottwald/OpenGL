#pragma once

#include <Engine/Core/GameContext.h>

namespace Engine::ECS
{

enum class SystemPhase : uint8_t
{
   Input,
   Intent,
   Simulation,
   LateSimulation,
   Presentation,
};

struct TickContext
{
   Engine::GameContext& game;
   float                dt { 0.0f };
};

struct FixedTickContext
{
   Engine::GameContext& game;
   float                tickInterval { 0.0f };
};

class ISystem
{
public:
   virtual ~ISystem() = default;

   virtual SystemPhase Phase() const noexcept = 0;

   virtual void Tick( TickContext& ) {}
   virtual void FixedTick( FixedTickContext& ) {}
};

} // namespace Engine::ECS
