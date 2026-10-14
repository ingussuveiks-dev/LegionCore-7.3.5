#include "ScriptMgr.h"
#include "SmartScript.h"
#include "SmartAI.h"
#include "ObjectAccessor.h"
#include "EventObject.h"
#include "Player.h"
#include "QuestData.h"
#include "MoveSpline.h"
#include "Map.h"
#include <cmath>

namespace StormheimHeart
{
struct Branch { uint32 quest, team; };
Branch const branches[] = {{38414, ALLIANCE}, {38414, HORDE}};
struct Stage { uint32 entry, spawn, credit; uint8 slot; float radius; };
Stage const stages[] = {{613, 410365, 91734, 0, 10.0f}};
Quest const* Definition(Branch const& branch)
{
    Quest const* quest = sQuestDataStore->GetQuestTemplate(branch.quest);
    if (!quest) return nullptr;
    bool found = false, killed = false;
    for (QuestObjective const& objective : quest->GetObjectives())
        if (objective.Type == QUEST_OBJECTIVE_MONSTER && objective.Amount == 1 && !(objective.Flags & QUEST_OBJECTIVE_FLAG_OPTIONAL))
        {
            found |= objective.ObjectID == 91734 && objective.StorageIndex == 0;
            killed |= objective.ObjectID == 91737 && objective.StorageIndex == 1 && (objective.Flags & QUEST_OBJECTIVE_FLAG_SEQUENCED);
        }
    return found && killed ? quest : nullptr;
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
            !player->GetQuestObjectiveData(branch.quest, stage.credit)) return &branch;
    return nullptr;
}
void RepairLegacy(Player* player)
{
    for (Branch const& branch : branches)
        if (player->GetTeam() == branch.team && Definition(branch) && player->GetQuestStatus(branch.quest) == QUEST_STATUS_COMPLETE &&
            (!player->GetQuestObjectiveData(branch.quest, 91734) || !player->GetQuestObjectiveData(branch.quest, 91737)))
            player->SetQuestStatus(branch.quest, QUEST_STATUS_INCOMPLETE);
}
}

class eventobject_stormheim_heart : public EventObjectScript
{
public:
    eventobject_stormheim_heart() : EventObjectScript("eventobject_stormheim_heart") { }
    bool IsTriggerMeets(Player* player, EventObject* event) override
    {
        auto stage = StormheimHeart::Scoped(event);
        // Check before EventObject::Update inserts affectedPlayers. A rejected
        // entry (airborne or no quest) can retry in place.
        return !stage || StormheimHeart::Eligible(player, event, *stage);
    }
    bool OnTrigger(Player* player, EventObject* event, bool enter) override
    {
        auto stage = StormheimHeart::Scoped(event);
        if (!stage)
        {
            if (!player || !player->IsAlive()) return false;
            SmartScript script;
            script.OnInitialize(event, nullptr);
            script.ProcessEventsFor(enter ? SMART_EVENT_EVENTOBJECT_ONTRIGGER : SMART_EVENT_EVENTOBJECT_OFFTRIGGER, player, event->GetEntry());
            return true;
        }
        if (!enter) return false;
        auto branch = StormheimHeart::Eligible(player, event, *stage);
        if (!branch) return false;
        Quest const* quest = StormheimHeart::Definition(*branch);
        for (QuestObjective const& objective : quest->GetObjectives())
            if (objective.ObjectID == stage->credit && objective.StorageIndex == stage->slot)
            {
                player->SetQuestObjectiveData(quest, &objective, 1);
                player->SendQuestUpdateAddCredit(quest, ObjectGuid::Empty, objective, 1);
                if (player->CanCompleteQuest(branch->quest)) player->CompleteQuest(branch->quest);

                return true;
            }
        return false;
    }
};
class player_stormheim_heart : public PlayerScript
{
public:
    player_stormheim_heart() : PlayerScript("player_stormheim_heart") { }
    void OnLogin(Player* player) override { StormheimHeart::RepairLegacy(player); }
};
// Existing shared dragon; quest credit remains in the native kill handler.
struct npc_stormheim_heart_mother : SmartAI
{
    explicit npc_stormheim_heart_mother(Creature* creature) : SmartAI(creature) { }
    bool released = false;
    uint32 timer = 0;
    bool Scoped() const
    {
        return me->GetEntry() == 91570 && me->GetDBTableGUIDLow() == 294116 && me->GetMapId() == 1220 &&
            me->GetZoneId() == 7541 && me->GetInstanceId() == 0;
    }
    Creature* Boss()
    {
        Creature* boss = ObjectAccessor::GetCreature(*me, ObjectGuid::Create<HighGuid::Creature>(1220, 91737, 294111));
        return boss && boss->GetDBTableGUIDLow() == 294111 ? boss : nullptr;
    }
    void Release()
    {
        if (released) return;
        released = true;
        me->RemoveAurasDueToSpell(182091);
        me->SetStandState(UNIT_STAND_STATE_STAND);
        SmartAI::SetData(1, 1); // Existing free/retaliation dialogue, with a null invoker.
    }
    void Reset() override
    {
        SmartAI::Reset();
        if (!Scoped()) return;
        released = false;
        timer = 0;
        me->SetStandState(UNIT_STAND_STATE_SLEEP);
    }
    void SetData(uint32 id, uint32 value) override
    {
        if (!Scoped() || id != 1 || value != 1) { SmartAI::SetData(id, value); return; }
        if (Creature* boss = Boss())
            if (!boss->IsAlive()) Release();
    }
    void UpdateAI(uint32 diff) override
    {
        SmartAI::UpdateAI(diff);
        if (!Scoped()) return;
        if (timer > diff) { timer -= diff; return; }
        timer = 1000;
        if (Creature* boss = Boss())
        {
            if (!boss->IsAlive()) Release(); // Re-entry/failed signal recovery, never a kill credit.
            else
            {
                released = false;
                me->SetStandState(UNIT_STAND_STATE_SLEEP);
                // Retry the existing out-of-combat native channel after both
                // actors load/respawn. Keep the original boss as aura caster.
                if (!boss->isInCombat() && !me->HasAura(182091))
                    boss->CastSpell(boss, 182091, true);
            }
        }
    }
};
void AddSC_stormheim_heart()
{
    new eventobject_stormheim_heart();
    new player_stormheim_heart();
    RegisterCreatureAI(npc_stormheim_heart_mother);
}
