#pragma once

struct CCamera
{
   glm::mat4 view { 1.0f };
   glm::mat4 projection { 1.0f };
   glm::mat4 viewProjection { 1.0f };
   glm::vec2 lastMousePos { 0.0f, 0.0f };
   float     fov         = 90.0f;
   float     sensitivity = 1.0f / 16.0f;
   bool      firstMouse  = true;
};
