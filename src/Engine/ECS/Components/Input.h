#pragma once

struct CInput
{
   // Intent (Values between -1.0 and 1.0)
   glm::vec2 movement { 0.0f }; // x = right/left, y = forward/back

   // Actions
   bool fJumpRequest { false };
   bool fSprintRequest { false };

   // State tracking
   bool     fWasJumpDown { false };
   uint32_t jumpCooldown { 0 };
};

struct CLookInput
{
   float yawDelta { 0.0f };
   float pitchDelta { 0.0f };
};
