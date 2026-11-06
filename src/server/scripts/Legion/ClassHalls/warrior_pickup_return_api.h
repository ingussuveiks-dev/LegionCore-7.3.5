#pragma once
class Player;
namespace WarriorPickupReturn
{
    // Called only by the authenticated native scene-completion bridges.
    // True preserves a legitimate off-spec pair in storage under native use admission.
    bool Complete(Player* player, unsigned int scene);
}
