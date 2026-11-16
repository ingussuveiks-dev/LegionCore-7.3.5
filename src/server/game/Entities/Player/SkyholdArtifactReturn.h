#pragma once

#include "Define.h"

class Player;

namespace SkyholdArtifactReturn
{
    // Actual Skyhold presence plus the owned, earned Fury acquisition.
    // Shared by native credit admission and the scoped arrival recovery.
    TC_GAME_API bool HasPendingFuryReturn(Player* player);
}
