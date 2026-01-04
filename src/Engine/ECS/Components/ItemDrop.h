#pragma once

// Project Dependencies
#include <Engine/World/Blocks.h>

struct CItemDrop
{
   BlockId  blockId;
   uint64_t tickLifetime { 0 };
};
