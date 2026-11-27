#ifndef LEGION_VIOLET_HOLD_BARRIER_H
#define LEGION_VIOLET_HOLD_BARRIER_H

#include "GameObject.h"
#include "Player.h"

namespace VioletHoldBarrier
{
constexpr uint32 Map = 1494;
constexpr uint32 Scenario = 1034;
constexpr uint32 Phase = 5015;
constexpr uint32 Quest = 41036;
constexpr uint32 FinalStage = 5;

inline bool IsBarrier(GameObject const* object)
{
    if (!object || object->GetMapId() != Map)
        return false;

    uint64 const spawn = object->GetDBTableGUIDLow();
    return spawn >= 3101120 && spawn <= 3101140 &&
        (object->GetEntry() == 246866 || object->GetEntry() == 246935);
}

inline bool IsParticipant(Player* player)
{
    return player && player->CanContact() && player->IsAlive() && !player->isGameMaster() &&
        player->GetMapId() == Map && player->HasPhaseId(Phase) && player->getClass() == CLASS_MAGE &&
        player->GetQuestStatus(Quest) == QUEST_STATUS_INCOMPLETE;
}

inline bool IsInCourtyard(Position const& position)
{
    // The balcony's last wall segments end before this part of the lower
    // chamber. The original Blink in the recording lands at (1345, -267).
    return position.GetPositionX() >= 1327.0f && position.GetPositionX() <= 1410.0f &&
        position.GetPositionY() >= -323.0f && position.GetPositionY() <= -206.0f &&
        position.GetPositionZ() >= 36.0f && position.GetPositionZ() <= 58.0f;
}
}

#endif
