#pragma once

#include <Engine/ECS/ISystem.h>

namespace Engine::ECS
{

class FurnaceSystem final : public ISystem
{
public:
   SystemPhase Phase() const noexcept override { return SystemPhase::Simulation; }
   void        FixedTick( FixedTickContext& ctx ) override;
};

} // namespace Engine::ECS
