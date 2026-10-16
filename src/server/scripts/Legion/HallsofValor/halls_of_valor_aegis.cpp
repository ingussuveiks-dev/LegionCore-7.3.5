#include "ScriptMgr.h"
#include "GameObjectAI.h"
#include "Player.h"
#include "QuestData.h"
#include "ConditionMgr.h"
#include "halls_of_valor.h"

namespace HallsOfValorAegis
{
constexpr uint32 QuestId = 40072, ClaimCredit = 110425;
Position const Pedestal = {2375.19f, 559.552f, 754.515f, 5.72213f};
bool Definition()
{
    auto q = sQuestDataStore->GetQuestTemplate(QuestId);
    if (!q || q->StartScript || q->HasSpecialFlag(QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT)) return false;
    uint32 mask = 0;
    for (auto const& o : q->GetObjectives())
    {
        if (o.Type != QUEST_OBJECTIVE_MONSTER || o.Amount != 1) return false;
        if (o.ObjectID == ClaimCredit && o.StorageIndex == 0 && !o.Flags && !(mask & 1)) mask |= 1;
        else if (o.ObjectID == NPC_ODYN && o.StorageIndex == 1 &&
            o.Flags == (QUEST_OBJECTIVE_FLAG_TRACKED_ON_MINIMAP | QUEST_OBJECTIVE_FLAG_SEQUENCED) && !(mask & 2)) mask |= 2;
        else return false;
    }
    return mask == 3;
}
bool CanClaim(Player* p, GameObject* go)
{
    if (!p || !go || !p->IsInWorld() || !go->IsInWorld() || !p->IsAlive() || !go->isSpawned() ||
        go->HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE | GO_FLAG_NOT_SELECTABLE) ||
        go->GetEntry() != 251991 || go->GetDBTableGUIDLow() != 25178659 || go->GetMapId() != 1477 ||
        !go->GetInstanceId() || p->GetMap() != go->GetMap() || !p->HasPhaseId(3554) ||
        go->GetExactDist(&Pedestal) >= 1 || !Definition() || p->GetQuestStatus(QuestId) != QUEST_STATUS_INCOMPLETE ||
        p->GetQuestObjectiveData(QuestId, ClaimCredit) || p->GetQuestObjectiveData(QuestId, NPC_ODYN) != 1 ||
        p->IsBeingTeleported() || p->isInCombat() || p->isInFlight() || p->GetVehicle() || p->GetTransport() ||
        !p->InSamePhase(go) || !p->canSeeOrDetect(go) || !p->IsWithinLOSInMap(go) ||
        p->GetGameObjectIfCanInteractWith(go->GetGUID(), GAMEOBJECT_TYPE_QUESTGIVER) != go ||
        !sConditionMgr->IsPlayerMeetingCondition(p, go->GetGOInfo()->questgiver.conditionID1)) return false;

    auto instance = go->GetInstanceScript();
    return instance && instance->GetBossState(DATA_SKOVALD) == DONE && instance->GetBossState(DATA_ODYN) == DONE;
}
}

struct go_halls_of_valor_quest_aegis : GameObjectAI
{
    explicit go_halls_of_valor_quest_aegis(GameObject* object) : GameObjectAI(object) { }
    bool GossipUse(Player* p) override
    {
        using namespace HallsOfValorAegis;
        if (CanClaim(p, go))
        {
            auto q = sQuestDataStore->GetQuestTemplate(QuestId);
            for (auto const& o : q->GetObjectives()) if (o.ObjectID == ClaimCredit)
            {
                p->SetQuestObjectiveData(q, &o, 1);
                p->SendQuestUpdateAddCredit(q, go->GetGUID(), o, 1);
                if (p->CanCompleteQuest(QuestId)) p->CompleteQuest(QuestId);
                break;
            }
        }
        // Continue native questgiver use: the completed quest can now be turned
        // in here, then this same object offers the existing follow-up43349.
        return false;
    }
};

void AddSC_halls_of_valor_aegis()
{
    RegisterGameObjectAI(go_halls_of_valor_quest_aegis);
}
