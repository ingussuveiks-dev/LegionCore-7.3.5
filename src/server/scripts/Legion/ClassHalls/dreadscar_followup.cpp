#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "Player.h"
#include "TemporarySummon.h"

namespace DreadscarPlans
{
constexpr uint32 QuestId = 40824;
Position const Plans = {3038.36f, 887.68f, 247.333f, 0.55016f};
// Supported floor beside the existing board, rather than the resurrection site.
Position const Ritssyn = {3043.24f, 890.639f, 247.241f, 3.69f};

bool Active(Player* player)
{
    return player && player->getClass() == CLASS_WARLOCK &&
        !player->GetQuestRewardStatus(QuestId) &&
        (player->GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE ||
         player->GetQuestStatus(QuestId) == QUEST_STATUS_COMPLETE);
}

void Recover(Player* player)
{
    if (!Active(player) || !player->IsInWorld() || player->IsBeingTeleported() ||
        player->GetMapId() != 1107 || player->GetExactDist(&Plans) > 35.0f)
        return;

    std::list<Creature*> actors;
    player->GetCreatureListWithEntryInGrid(actors, 101921, 60.0f);
    Creature* found = nullptr;
    for (Creature* actor : actors)
        if (TempSummon* summon = actor->ToTempSummon())
            if (actor->IsAlive() && actor->HasQuestObjectScope() && !actor->IsQuestObjectExpired() &&
                summon->GetSummonerGUID() == player->GetGUID())
            {
                if (!found)
                    found = actor;
                else
                    actor->DespawnOrUnsummon();
            }
    if (found)
        return;

    if (Creature* actor = player->SummonCreature(101921, Ritssyn, TEMPSUMMON_MANUAL_DESPAWN, 0, 0, player->GetGUID()))
    {
        player->BindQuestObject(QuestId, actor);
        actor->SetPhaseId(player->GetPhases(), true);
        actor->SetUInt32Value(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
        TC_LOG_INFO("entities.player", "Dreadscar plans: restored personal Ritssyn for player %u", player->GetGUIDLow());
    }
}
}

class player_dreadscar_plans : public PlayerScript
{
public:
    player_dreadscar_plans() : PlayerScript("player_dreadscar_plans") { }
    void OnUpdate(Player* player, uint32) override { DreadscarPlans::Recover(player); }
    void OnLogout(Player* player) override { player->CancelQuestObjects(DreadscarPlans::QuestId); }
    void OnMapChanged(Player* player) override { player->CancelQuestObjects(DreadscarPlans::QuestId); }
};

void AddSC_dreadscar_followup()
{
    new player_dreadscar_plans();
}
