#pragma once

#include <Engine/ECS/Registry.h>

struct CProjectile
{
   int            damage { 10 };
   Entity::Entity owner { Entity::NullEntity }; // optional: ignore hits on owner
   bool           fDestroyOnHit { true };
};
