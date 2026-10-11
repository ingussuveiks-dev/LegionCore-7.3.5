#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "Vehicle.h"
#include "QuestData.h"
#include "Group.h"
#include <cmath>
#include <set>

namespace Riverbend
{
bool Ready(Player* player, uint32 quest)
{
    return player && player->IsAlive() && player->GetMapId() == 1220 &&
        !player->IsBeingTeleported() && player->GetQuestStatus(quest) == QUEST_STATUS_INCOMPLETE;
}
void EvacuateCredit(Player* player, ObjectGuid guid)
{
    // This is a warning/rescue, not a creature kill or an achievement kill.
    if (Quest const* quest = sQuestDataStore->GetQuestTemplate(42104))
        for (QuestObjective const& objective : quest->GetObjectives())
            if (objective.Type == QUEST_OBJECTIVE_MONSTER && objective.ObjectID == 100520)
            {
                uint32 count = player->GetQuestObjectiveData(42104,100520) + 1;
                if (count > uint32(objective.Amount)) return;
                player->SetQuestObjectiveData(quest,&objective,count);
                player->SendQuestUpdateAddCredit(quest,guid,objective,count);
                if (player->CanCompleteQuest(42104)) player->CompleteQuest(42104);
                return;
            }
}
uint32 CaptiveAt(Creature* grasp)
{
    // Existing Riverbend grasp placements; tolerate DB float round-off.
    struct Site { float x, y, z; uint32 entry; };
    static Site const sites[] = {
        {3736.11f,4887.47f,675.735f,95051},
        {3649.89f,4908.65f,685.170f,94255},
        {3575.87f,4839.52f,700.372f,94286}
    };
    for (auto const& site : sites)
        if (grasp->GetExactDist2d(site.x,site.y) < 1.0f && std::abs(grasp->GetPositionZ()-site.z) < 2.0f)
            return site.entry;
    return 0;
}
}

struct npc_riverbend_evacuee : ScriptedAI
{
    npc_riverbend_evacuee(Creature* creature) : ScriptedAI(creature) { }
    std::set<ObjectGuid> warned;
    bool leaving = false;
    void Reset() override { warned.clear(); leaving = false; }
    void Warn(Player* player)
    {
        if (Riverbend::Ready(player,42104) && player->canSeeOrDetect(me) &&
            warned.insert(player->GetGUID()).second)
            Riverbend::EvacuateCredit(player,me->GetGUID());
    }
    void SpellHit(Unit* caster, SpellInfo const* spell) override
    {
        Player* player = caster->ToPlayer();
        if (spell->Id != 210891 || !Riverbend::Ready(player,42104) || !me->IsAlive() ||
            player->GetAreaId() != 7709 || player->GetDistance(me) > 12.0f ||
            !player->canSeeOrDetect(me) || warned.count(player->GetGUID())) return;
        Creature* refuge = me->FindNearestCreature(93805,250.0f,true);
        if (!refuge || !me->InSamePhase(refuge)) return;
        Warn(player);
        // Preserve nearby group credit without counting a second group member's
        // horn as another evacuation of the same citizen.
        if (Group* group = player->GetGroup())
            for (GroupReference* itr = group->GetFirstMember(); itr; itr = itr->next())
                if (Player* member = itr->getSource())
                    if (member->IsAtGroupRewardDistance(me)) Warn(member);
        Talk(0,player->GetGUID());
        if (!leaving)
        {
            leaving = true;
            me->SetReactState(REACT_PASSIVE);
            me->SetWalk(false);
            me->GetMotionMaster()->MovePoint(1,refuge->GetPosition());
            // Bound failed-path lifetime; ordinary arrival despawns sooner.
            me->DespawnOrUnsummon(60000);
        }
    }
    void MovementInform(uint32 type, uint32 id) override
    {
        if (leaving && type == POINT_MOTION_TYPE && id == 1)
            me->DespawnOrUnsummon(2000);
    }
};

// 97648: keep the existing binding, but rescue the actual seat-zero passenger.
struct npc_grasp_of_underking_quest : ScriptedAI
{
    npc_grasp_of_underking_quest(Creature* creature) : ScriptedAI(creature) { }
    ObjectGuid captive;
    uint32 poll = 0;
    bool released = false;
    void Reset() override
    {
        if (Creature* old = ObjectAccessor::GetCreature(*me,captive)) old->DespawnOrUnsummon();
        captive.Clear(); poll = 2000; released = false;
    }
    void UpdateAI(uint32 diff) override
    {
        if (released || !me->IsAlive()) return;
        if (poll > diff) { poll -= diff; return; }
        poll = 2000;
        Vehicle* vehicle = me->GetVehicleKit();
        uint32 entry = Riverbend::CaptiveAt(me);
        if (!vehicle || !entry || vehicle->GetPassenger(0)) return;
        Creature* prisoner = ObjectAccessor::GetCreature(*me,captive);
        if (prisoner && !prisoner->IsAlive())
        {
            prisoner->DespawnOrUnsummon();
            prisoner = nullptr;
            captive.Clear();
        }
        if (!prisoner)
        {
            prisoner = me->SummonCreature(entry,me->GetPosition());
            if (!prisoner) return; // retry after a failed summon, without a permanent 'boarded' latch
            captive = prisoner->GetGUID();
        }
        if (prisoner->IsAlive() && !prisoner->GetVehicle()) prisoner->EnterVehicle(me,0);
    }
    void sGossipHello(Player* player) override
    {
        player->PlayerTalkClass->SendCloseGossip();
        if (released || !me->IsAlive() || !Riverbend::Ready(player,39025) ||
            player->GetDistance(me) > 5.0f || !player->canSeeOrDetect(me)) return;
        Vehicle* vehicle = me->GetVehicleKit();
        Unit* passenger = vehicle ? vehicle->GetPassenger(0) : nullptr;
        Creature* prisoner = passenger ? passenger->ToCreature() : nullptr;
        uint32 entry = Riverbend::CaptiveAt(me);
        if (!entry || !prisoner || !prisoner->IsAlive() || prisoner->GetEntry() != entry ||
            prisoner->GetGUID() != captive) return;
        prisoner->SetDisableGravity(false);
        prisoner->ExitVehicle();
        if (prisoner->GetVehicle()) return;
        released = true;
        // Preserve existing nearby group credit, after successful release only.
        player->RewardPlayerAndGroupAtEvent(entry,player);
        prisoner->AI()->Talk(0,player->GetGUID());
        prisoner->SetWalk(false);
        prisoner->GetMotionMaster()->MoveRandom(12.0f);
        prisoner->DespawnOrUnsummon(6500);
        me->Kill(me);
    }
};

class player_riverbend_objectives : public PlayerScript
{
public:
    player_riverbend_objectives() : PlayerScript("player_riverbend_objectives") { }
    void OnLogin(Player* player) override
    {
        auto status = player->GetQuestStatus(39043);
        if (status != QUEST_STATUS_INCOMPLETE && status != QUEST_STATUS_COMPLETE) return;
        if (Quest const* quest = sQuestDataStore->GetQuestTemplate(39043))
            for (QuestObjective const& objective : quest->GetObjectives())
                if (objective.Type == QUEST_OBJECTIVE_MONSTER &&
                    (objective.ObjectID == 94261 || objective.ObjectID == 98653) &&
                    player->GetQuestObjectiveData(39043,objective.ObjectID) > objective.Amount)
                    player->SetQuestObjectiveData(quest,&objective,objective.Amount);
        // Existing 3/5 + 3/5 saves must become complete under the restored 3+3
        // requirements. Further kills would otherwise skip already-full slots.
        if (status == QUEST_STATUS_INCOMPLETE && player->CanCompleteQuest(39043))
            player->CompleteQuest(39043);
    }
};

void AddSC_highmountain_riverbend()
{
    RegisterCreatureAI(npc_riverbend_evacuee);
    RegisterCreatureAI(npc_grasp_of_underking_quest);
    new player_riverbend_objectives();
}
