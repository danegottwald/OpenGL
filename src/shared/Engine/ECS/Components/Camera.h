#pragma once

#include <Engine/ECS/Registry.h>

// Stores the camera's rendering and input state (view/projection matrices, FOV, mouse info)
struct CCamera
{
   glm::mat4 view { 1.0f };
   glm::mat4 projection { 1.0f };
   glm::mat4 viewProjection { 1.0f };

   float fov { 90.0f };
   float aspectRatio { 16.0f / 9.0f };
   float nearPlane { 0.1f };
   float farPlane { 1000.0f };
   float sensitivity { 1.0f / 16.0f };
};

// Defines how a camera follows a target entity (offset, yaw/pitch following)
struct CCameraRig
{
   Entity::Entity targetEntity { Entity::NullEntity };
   glm::vec3      offset { 0.0f };

   bool followYaw { true };
   bool followPitch { false };
};
