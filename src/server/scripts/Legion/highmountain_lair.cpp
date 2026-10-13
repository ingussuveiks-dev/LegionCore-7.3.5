#include "ScriptMgr.h"
#include "Player.h"
#include "QuestData.h"
#include <algorithm>

class player_highmountain_lair : public PlayerScript
{
public:
    player_highmountain_lair() : PlayerScript("player_highmountain_lair") { }
    void OnLogin(Player* player) override
    {
        auto status = player->GetQuestStatus(39781);
        if (status != QUEST_STATUS_INCOMPLETE && status != QUEST_STATUS_COMPLETE) return;
        if (Quest const* quest = sQuestDataStore->GetQuestTemplate(39781))
        {
            for (QuestObjective const& objective : quest->GetObjectives())
                if (objective.Type == QUEST_OBJECTIVE_ITEM && objective.ObjectID == 128765)
                {
                    // Repair completion from the former optional-only objective
                    // list. The normal loot/item/reward code owns later progress.
                    player->SetQuestObjectiveData(quest,&objective,
                        std::min(player->GetItemCount(128765,false),uint32(objective.Amount)));
                    if (status == QUEST_STATUS_COMPLETE && !player->HasQuestObjectiveComplete(quest,objective))
                        player->IncompleteQuest(39781);
                    if (player->CanCompleteQuest(39781)) player->CompleteQuest(39781);
                    return;
                }
        }
    }
};

void AddSC_highmountain_lair()
{
    new player_highmountain_lair();
}
