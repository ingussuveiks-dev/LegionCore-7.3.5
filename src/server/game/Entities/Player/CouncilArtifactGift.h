#ifndef LEGIONCORE_COUNCIL_ARTIFACT_GIFT_H
#define LEGIONCORE_COUNCIL_ARTIFACT_GIFT_H

class Player;

namespace CouncilArtifactGift
{
    constexpr unsigned Acquire = 234679;
    constexpr unsigned Button = 234670;
    constexpr unsigned PlayScene = 234681;
    constexpr unsigned Scene = 1499;
    constexpr unsigned RitualEvent = 1516;
    constexpr unsigned RitualSpawn = 10016;
    // Player::SafeTeleport option carried with this ritual's near transfer.
    constexpr unsigned NearTeleport = 0x100;

    constexpr bool MatchesJump(unsigned spell, unsigned arrival)
    {
        return spell == Button && arrival == Acquire;
    }

    void BeginTransfer(Player& player);
    void OnArrival(Player& player);
    bool MaintainRitual(Player& player);
    void RemoveRecoveredButton(Player& player);
}

#endif
