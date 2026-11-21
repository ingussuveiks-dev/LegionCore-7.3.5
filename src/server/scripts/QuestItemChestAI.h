#ifndef LEGIONCORE_QUEST_ITEM_CHEST_AI_H
#define LEGIONCORE_QUEST_ITEM_CHEST_AI_H

#include "GameObject.h"
#include "GameObjectAI.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "WorldSession.h"

// For a shared chest that contains only one quest's gathering item.
class QuestItemChestAI : public GameObjectAI
{
public:
    QuestItemChestAI(GameObject* object, uint32 quest, uint32 item)
        : GameObjectAI(object), _quest(quest), _item(item) { }

    bool GossipHello(Player* player, bool isUse) override
    {
        if (!isUse)
            return false;

        // Opening has validated the lock. Reject before SendLoot can
        // activate the chest without giving this player a quest item.
        if (!player->IsAlive() || !player->CanContact() || player->IsSpectator() ||
            !go->IsWithinDistInMap(player, INTERACTION_DISTANCE) ||
            player->GetQuestStatus(_quest) != QUEST_STATUS_INCOMPLETE || !player->HasQuestForItem(_item))
            return true;

        if (go->getLootState() == GO_ACTIVATED)
        {
            // A second window would be empty and could discard the
            // first opener's quest item when the second player closes it.
            if (Player* opener = ObjectAccessor::GetPlayer(*go, _opener))
                if (opener->GetLootGUID() == go->GetGUID())
                {
                    if (go->IsWithinDistInMap(opener, INTERACTION_DISTANCE))
                        return true;
                    opener->GetSession()->DoLootRelease(go->GetGUID());
                }

            // The native release cannot reset a chest from outside
            // interaction range. The next valid opener can rearm it.
            go->SetLootState(GO_READY);
        }
        return go->getLootState() != GO_READY;
    }

    void OnStateChanged(uint32 state, Unit* user) override
    {
        _opener = state == GO_ACTIVATED && user ? user->GetGUID() : ObjectGuid::Empty;
    }

private:
    uint32 const _quest;
    uint32 const _item;
    ObjectGuid _opener;
};

#endif
