#ifndef LEGION_MAGNAR_START_H
#define LEGION_MAGNAR_START_H

class Creature;
class Player;

namespace MagnarStart
{
    constexpr unsigned Accepted = 1062;
    // Reattach a pending request's original quest receipt before a returning
    // player can abandon/reaccept. This never starts combat during map entry.
    void Rejoin(Creature* companion, Player* player);
}

#endif
