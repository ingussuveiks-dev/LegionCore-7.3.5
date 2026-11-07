#ifndef LEGION_MAGNAR_START_H
#define LEGION_MAGNAR_START_H

class Creature;
class Player;

namespace MagnarStart
{
    constexpr unsigned Accepted = 1062;
    // Instance-owned receipt: original guard victory AND chamber delivery.
    constexpr unsigned ChamberDelivered = 1066;
    // Instance-owned next native corridor leg. SetData takes the epoch and
    // pulls the cursor from the actual original companion's current AI.
    constexpr unsigned RouteCursor = 1067;
    // Reattach a pending request's original quest receipt before a returning
    // player can abandon/reaccept. This never starts combat during map entry.
    void Rejoin(Creature* companion, Player* player);
    // Called only after the acknowledged first-phase boss finishes Reset.
    // Release that start claim; the ordinary point6 offer still gates retries.
    void ResetOffer(Creature* companion, Creature* boss);
}

#endif
