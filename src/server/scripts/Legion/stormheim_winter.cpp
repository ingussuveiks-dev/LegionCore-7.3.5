#include "ScriptMgr.h"
#include "SmartScript.h"
#include "EventObject.h"
#include "Player.h"
#include "QuestData.h"
#include "MoveSpline.h"
#include "Map.h"
#include <cmath>

namespace StormheimWinter
{
struct Branch { uint32 quest, team; };
Branch const branches[] = {{38412, ALLIANCE}, {38618, HORDE}};
struct Stage { uint32 entry, spawn, credit; uint8 slot; float radius; };
Stage const stages[] = {{611, 410363, 91728, 0, 5.0f}, {612, 410364, 91729, 1, 10.0f}};
Quest const* Definition(Branch const& branch)
{
    Quest const* quest = sQuestDataStore->GetQuestTemplate(branch.quest);
    if (!quest) return nullptr;
    bool across = false, top = false;
    for (QuestObjective const& objective : quest->GetObjectives())
        if (objective.Type == QUEST_OBJECTIVE_MONSTER && objective.Amount == 1 && !(objective.Flags & QUEST_OBJECTIVE_FLAG_OPTIONAL))
        {
            across |= objective.ObjectID == 91728 && objective.StorageIndex == 0;
            top |= objective.ObjectID == 91729 && objective.StorageIndex == 1 && (objective.Flags & QUEST_OBJECTIVE_FLAG_SEQUENCED);
        }
    return across && top ? quest : nullptr;
}
Stage const* Scoped(EventObject* event)
{
    if (!event || !event->IsInWorld() || event->GetMapId() != 1220 || event->GetZoneId() != 7541 || event->GetInstanceId() != 0 ||
        !event->eventTemplate || event->eventTemplate->SpellID || event->eventTemplate->WorldSafeLocID || event->eventTemplate->Flags) return nullptr;
    for (Stage const& stage : stages)
        if (event->GetEntry() == stage.entry && event->GetDBTableGUIDLow() == stage.spawn && event->eventTemplate->radius == stage.radius)
            return &stage;
    return nullptr;
}
Branch const* Eligible(Player* player, EventObject* event, Stage const& stage)
{
    if (!player || !player->IsInWorld() || !player->IsAlive() || player->IsBeingTeleported() ||
        player->GetMapId() != 1220 || player->GetZoneId() != 7541 || player->GetInstanceId() != 0 ||
        player->GetVehicle() || player->GetTransport() || player->isInFlight() ||
        player->HasUnitMovementFlag(MOVEMENTFLAG_FALLING | MOVEMENTFLAG_FALLING_FAR | MOVEMENTFLAG_FLYING) ||
        !player->movespline->Finalized() || !player->InSamePhase(event) || !player->IsWithinDistInMap(event, stage.radius)) return nullptr;
    float floor = player->GetMap()->GetHeight(player->GetPositionX(), player->GetPositionY(), player->GetPositionZ() + 2.0f, true, 5.0f);
    if (!std::isfinite(floor) || std::fabs(player->GetPositionZ() - floor) > 1.0f) return nullptr;
    for (Branch const& branch : branches)
        if (player->GetTeam() == branch.team && Definition(branch) && player->GetQuestStatus(branch.quest) == QUEST_STATUS_INCOMPLETE &&
            !player->GetQuestObjectiveData(branch.quest, stage.credit) &&
            (!stage.slot || player->GetQuestObjectiveData(branch.quest, 91728))) return &branch;
    return nullptr;
}
void RepairLegacy(Player* player)
{
    for (Branch const& branch : branches)
        if (player->GetTeam() == branch.team && Definition(branch) && player->GetQuestStatus(branch.quest) == QUEST_STATUS_COMPLETE &&
            (!player->GetQuestObjectiveData(branch.quest, 91728) || !player->GetQuestObjectiveData(branch.quest, 91729)))
            player->SetQuestStatus(branch.quest, QUEST_STATUS_INCOMPLETE);
}
}

class eventobject_stormheim_winter : public EventObjectScript
{
public:
    eventobject_stormheim_winter() : EventObjectScript("eventobject_stormheim_winter") { }
    bool IsTriggerMeets(Player* player, EventObject* event) override
    {
        auto stage = StormheimWinter::Scoped(event);
        // Check before EventObject::Update inserts affectedPlayers. A rejected
        // entry (airborne, no quest, or out of order) can retry in place.
        return !stage || StormheimWinter::Eligible(player, event, *stage);
    }
    bool OnTrigger(Player* player, EventObject* event, bool enter) override
    {
        auto stage = StormheimWinter::Scoped(event);
        if (!stage)
        {
            if (!player || !player->IsAlive()) return false;
            SmartScript script;
            script.OnInitialize(event, nullptr);
            script.ProcessEventsFor(enter ? SMART_EVENT_EVENTOBJECT_ONTRIGGER : SMART_EVENT_EVENTOBJECT_OFFTRIGGER, player, event->GetEntry());
            return true;
        }
        if (!enter) return false;
        auto branch = StormheimWinter::Eligible(player, event, *stage);
        if (!branch) return false;
        Quest const* quest = StormheimWinter::Definition(*branch);
        for (QuestObjective const& objective : quest->GetObjectives())
            if (objective.ObjectID == stage->credit && objective.StorageIndex == stage->slot)
            {
                player->SetQuestObjectiveData(quest, &objective, 1);
                player->SendQuestUpdateAddCredit(quest, ObjectGuid::Empty, objective, 1);
                if (player->CanCompleteQuest(branch->quest)) player->CompleteQuest(branch->quest);
                // Existing Vethir conversation is independent of progression.
                if (stage->slot) player->CreateConversation(2549);
                return true;
            }
        return false;
    }
};
class player_stormheim_winter : public PlayerScript
{
public:
    player_stormheim_winter() : PlayerScript("player_stormheim_winter") { }
    void OnLogin(Player* player) override { StormheimWinter::RepairLegacy(player); }
};
void AddSC_stormheim_winter()
{
    new eventobject_stormheim_winter();
    new player_stormheim_winter();
}
