#ifndef EBONCHILL_RETURN_H
#define EBONCHILL_RETURN_H

#include "Item.h"
#include "Player.h"

namespace EbonchillReturn
{
inline bool HasPendingReturn(Player const& player)
{
    if (player.GetMapId() != 1616 || player.getClass() != CLASS_MAGE || player.GetQuestStatus(42479) != QUEST_STATUS_COMPLETE ||
        player.GetQuestRewardStatus(42479) || player.GetQuestObjectiveData(42479, 108025) < 1 || player.GetQuestObjectiveData(42479, 108026) < 1)
        return false;

    Item const* staff = player.GetItemByEntry(128862, false);
    return staff && staff->GetOwnerGUID() == player.GetGUID() && staff->GetCount() && !staff->IsInTrade();
}
}

#endif
