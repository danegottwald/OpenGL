#pragma once

#include <Engine/ECS/Registry.h>
#include <Engine/World/Blocks.h>
#include <Engine/World/Level.h>

// Intent-based events (no block logic in input).
// These are structured to support client prediction + server validation.
struct BlockHitEvent
{
   Entity::Entity player { Entity::NullEntity };
   WorldBlockPos  pos { 0, 0, 0 };
   BlockState     state { BlockId::Air };
   glm::ivec3     faceNormal { 0, 0, 0 };
};

struct BlockBreakEvent
{
   Entity::Entity player { Entity::NullEntity };
   WorldBlockPos  pos { 0, 0, 0 };
   BlockState     state { BlockId::Air };
};

struct BlockUseEvent
{
   Entity::Entity player { Entity::NullEntity };
   WorldBlockPos  pos { 0, 0, 0 };
   BlockState     state { BlockId::Air };
   glm::ivec3     faceNormal { 0, 0, 0 };
};

struct OpenBlockEntityEvent
{
   Entity::Entity player { Entity::NullEntity };
   Entity::Entity blockEntity { Entity::NullEntity };
};
