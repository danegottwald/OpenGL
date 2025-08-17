#pragma once

struct CCamera
{
   glm::mat4 viewMatrix { 1.0f };
   glm::mat4 projectionMatrix { 1.0f };
   glm::mat4 projectionViewMatrix { 1.0f };
   glm::vec3 eulerAngles { 0.0f, 0.0f, 0.0f }; // pitch, yaw, roll
   glm::vec2 lastMousePos { 0.0f, 0.0f };
   float     fov         = 90.0f;
   float     sensitivity = 1.0f / 16.0f;
   bool      firstMouse  = true;
};
