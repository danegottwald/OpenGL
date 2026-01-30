#pragma once

// Forward Declarations
namespace UI
{
struct IDrawable;
}

// Create Network UI element
std::shared_ptr< UI::IDrawable > CreateNetworkUI();
