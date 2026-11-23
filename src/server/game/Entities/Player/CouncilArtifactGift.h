#ifndef LEGIONCORE_COUNCIL_ARTIFACT_GIFT_H
#define LEGIONCORE_COUNCIL_ARTIFACT_GIFT_H

class Player;

namespace CouncilArtifactGift
{
    constexpr unsigned Acquire = 234679;
    constexpr unsigned PlayScene = 234681;
    // Player::SafeTeleport option carried with this ritual's near transfer.
    constexpr unsigned NearTeleport = 0x100;

    void BeginTransfer(Player& player);
    void OnArrival(Player& player);
}

#endif
