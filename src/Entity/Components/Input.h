#pragma once

struct CInput
{
    // True when the player is currently touching the ground (set by physics system)
    bool  grounded { false };
    // Flight mode toggle
    bool  flightMode { false };
    // Countdown timer for detecting double-tap of space (seconds remaining)
    float spaceTapTimer { 0.0f };
    // Previous frame space key state for edge detection
    bool  prevSpacePressed { false };
};
