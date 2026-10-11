#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "Player.h"
#include "TemporarySummon.h"
#include "ObjectAccessor.h"
#include "QuestData.h"
#include "Vehicle.h"
#include <map>
#include <mutex>

namespace Dargrul
{
constexpr uint32 QuestId = 39027;
Position const Trail[] = {
    {3486.07f,4596.64f,681.78f,0}, {3491.36f,4586.78f,681.089f,0},
    {3486.92f,4574.84f,680.463f,0}, {3480.15f,4555.07f,680.623f,0},
    {3474.18f,4535.13f,680.924f,0}, {3470.04f,4519.79f,680.203f,0},
    {3463.97f,4501.73f,679.853f,0}
};
bool Ready(Player* player)
{
    return player && player->IsAlive() && player->GetMapId() == 1220 &&
        !player->IsBeingTeleported() && player->GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE;
}
bool Done(Player* player, uint32 entry) { return player->GetQuestObjectiveData(QuestId,entry) > 0; }
bool InChamber(Player* player)
{
    return player->GetExactDist(&Trail[6]) < 40.0f;
}
Player* Owner(Creature* creature)
{
    TempSummon* summon = creature ? creature->ToTempSummon() : nullptr;
    Unit* owner = summon ? summon->GetSummoner() : nullptr;
    return owner ? owner->ToPlayer() : nullptr;
}
std::weak_ptr<uint8> Lifetime(Player* player)
{
    player->AddQuestDelayedEvent(QuestId,0,[] { });
    return player->getQuestStatus(QuestId)->ScriptLifetime;
}
Creature* Existing(Player* player, uint32 entry)
{
    auto guids = *player->GetSummonList(entry);
    for (ObjectGuid const& guid : guids)
        if (Creature* npc = ObjectAccessor::GetCreature(*player,guid))
            if (Owner(npc) == player && npc->IsAlive()) return npc;
    return nullptr;
}
void Clear(Player* player)
{
    for (uint32 entry : {97552u,97781u,94610u,97827u})
    {
        auto guids = *player->GetSummonList(entry);
        for (ObjectGuid const& guid : guids)
            if (Creature* npc = ObjectAccessor::GetCreature(*player,guid))
                if (Owner(npc) == player) npc->DespawnOrUnsummon();
    }
    player->RemoveAurasDueToSpell(205051);
}
void Credit(Player* player, uint32 entry, ObjectGuid guid)
{
    if (!Ready(player) || Done(player,entry)) return;
    if (Quest const* quest = sQuestDataStore->GetQuestTemplate(QuestId))
        for (QuestObjective const& objective : quest->GetObjectives())
            if (objective.ObjectID == entry)
            {
                // Includes the native TALKTO objective for the stone grasp.
                player->SetQuestObjectiveData(quest,&objective,1);
                player->SendQuestUpdateAddCredit(quest,guid,objective,1);
                if (player->CanCompleteQuest(QuestId)) player->CompleteQuest(QuestId);
                else player->SetQuestStatus(QuestId,QUEST_STATUS_INCOMPLETE); // objective-dependent phases/area spells
                return;
            }
}
void Grasp(Player* player)
{
    if (Ready(player) && Done(player,97871) && !Done(player,97781) &&
        InChamber(player) && !player->GetVehicle() && !Existing(player,97781))
        player->CastSpell(player,192936,true); // native vehicle summon and boarding
}
}

struct npc_dargrul_escort : ScriptedAI
{
    npc_dargrul_escort(Creature* creature) : ScriptedAI(creature) { }
    std::weak_ptr<uint8> lifetime;
    uint32 point = 0;
    uint32 travel = 0;
    bool moving = false;
    void IsSummonedBy(Unit* summoner) override
    {
        Player* player = summoner->ToPlayer();
        if (!Dargrul::Ready(player) || Dargrul::Done(player,97871))
        { me->DespawnOrUnsummon(); return; }
        lifetime = Dargrul::Lifetime(player);
        me->AddPlayerInPersonnalVisibilityList(player->GetGUID());
        me->SetReactState(REACT_PASSIVE);
        me->SetWalk(true);
        // Reconnect in the cave resumes at the nearest existing waypoint.
        for (uint32 i = 1; i < 7; ++i)
            if (me->GetExactDist(&Dargrul::Trail[i]) < me->GetExactDist(&Dargrul::Trail[point])) point = i;
        Talk(0,player->GetGUID());
    }
    void MovementInform(uint32 type, uint32 id) override
    {
        if (type == POINT_MOTION_TYPE && moving && id == point + 1)
        { moving = false; ++point; }
    }
    void UpdateAI(uint32 diff) override
    {
        Player* player = Dargrul::Owner(me);
        if (lifetime.expired() || !Dargrul::Ready(player) || Dargrul::Done(player,97871) ||
            !player->InSamePhase(me) || player->GetDistance(me) > 60.0f)
        { me->DespawnOrUnsummon(); return; }
        if (moving)
        {
            if (travel <= diff) me->DespawnOrUnsummon();
            else travel -= diff;
            return;
        }
        if (player->GetDistance(me) > 15.0f) return;
        if (point == 7)
        {
            Dargrul::Credit(player,97871,me->GetGUID());
            player->RemoveAurasDueToSpell(205051);
            me->DespawnOrUnsummon();
            return;
        }
        moving = true;
        travel = 30000;
        me->GetMotionMaster()->MovePoint(point + 1,Dargrul::Trail[point]);
    }
};

struct npc_dargrul_grasp : ScriptedAI
{
    npc_dargrul_grasp(Creature* creature) : ScriptedAI(creature) { }
    std::weak_ptr<uint8> lifetime;
    uint32 boardingGrace = 3000;
    uint32 GetData(uint32) const override { return lifetime.expired() ? 0 : 1; }
    void IsSummonedBy(Unit* summoner) override
    {
        Player* player = summoner->ToPlayer();
        if (!Dargrul::Ready(player) || !Dargrul::Done(player,97871) || Dargrul::Done(player,97781))
        { me->DespawnOrUnsummon(); return; }
        lifetime = Dargrul::Lifetime(player);
        me->AddPlayerInPersonnalVisibilityList(player->GetGUID());
        me->SetReactState(REACT_PASSIVE);
    }
    void PassengerBoarded(Unit* passenger, int8 seat, bool apply) override
    {
        Player* player = passenger->ToPlayer();
        if (apply && seat == 0 && !lifetime.expired() && Dargrul::Owner(me) == player &&
            Dargrul::Ready(player) && Dargrul::Done(player,97871))
            Dargrul::Credit(player,94610,me->GetGUID());
    }
    void UpdateAI(uint32 diff) override
    {
        Player* player = Dargrul::Owner(me);
        if (lifetime.expired() || !Dargrul::Ready(player) || Dargrul::Done(player,97781) ||
            player->GetDistance(me) > 60.0f)
        { me->DespawnOrUnsummon(); return; }
        if (player->GetVehicleBase() != me)
        {
            if (boardingGrace <= diff) me->DespawnOrUnsummon();
            else boardingGrace -= diff;
        }
        else boardingGrace = 3000;
    }
};

class spell_dargrul_escape : public SpellScript
{
    PrepareSpellScript(spell_dargrul_escape);
    void Escape(SpellEffIndex index)
    {
        // 193017's native 500% damage must not kill another player's grasp.
        PreventHitDefaultEffect(index);
        Player* player = GetCaster()->ToPlayer();
        Creature* grasp = GetHitCreature();
        if (!grasp || grasp->GetEntry() != 97781 || !grasp->IsAlive() ||
            !Dargrul::Ready(player) || Dargrul::Owner(grasp) != player || !grasp->AI()->GetData(1) ||
            !Dargrul::Done(player,97871) || !Dargrul::Done(player,94610) ||
            Dargrul::Done(player,97781) || player->GetVehicleBase() != grasp ||
            !grasp->GetVehicleKit() || grasp->GetVehicleKit()->GetPassenger(0) != player) return;
        player->ExitVehicle();
        if (player->GetVehicle()) return;
        Dargrul::Credit(player,97781,grasp->GetGUID());
        grasp->Kill(grasp);
        grasp->DespawnOrUnsummon(500);
    }
    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_dargrul_escape::Escape,EFFECT_0,SPELL_EFFECT_DAMAGE_FROM_MAX_HEALTH_PCT);
    }
};

class player_dargrul_event : public PlayerScript
{
    struct State { uint32 timer = 0; bool started = false; std::weak_ptr<uint8> lifetime; ObjectGuid actor; };
    std::mutex mutex;
    std::map<ObjectGuid,State> states;
public:
    player_dargrul_event() : PlayerScript("player_dargrul_event") { }
    void OnLogin(Player* player) override
    {
        auto status = player->GetQuestStatus(Dargrul::QuestId);
        if (status != QUEST_STATUS_INCOMPLETE && status != QUEST_STATUS_COMPLETE) return;
        if (Dargrul::Done(player,93805) && (!Dargrul::Done(player,97871) ||
            !Dargrul::Done(player,94610) || !Dargrul::Done(player,97781)))
        {
            if (Quest const* quest = sQuestDataStore->GetQuestTemplate(Dargrul::QuestId))
                for (QuestObjective const& objective : quest->GetObjectives())
                    if (objective.ObjectID == 93805) player->SetQuestObjectiveData(quest,&objective,0);
            player->IncompleteQuest(Dargrul::QuestId);
        }
    }
    void OnLogout(Player* player) override
    {
        bool serviced;
        { std::lock_guard<std::mutex> lock(mutex); serviced = states.erase(player->GetGUID()) != 0; }
        if (serviced) Dargrul::Clear(player);
    }
    void OnMapChanged(Player* player) override { OnLogout(player); }
    void OnUpdate(Player* player, uint32 diff) override
    {
        if (!Dargrul::Ready(player)) { OnLogout(player); return; }
        bool started, stale;
        {
            std::lock_guard<std::mutex> lock(mutex);
            auto inserted = states.emplace(player->GetGUID(),State{});
            State& state = inserted.first->second;
            stale = !inserted.second && state.lifetime.expired();
            if (inserted.second || stale) { state = State{}; state.lifetime = Dargrul::Lifetime(player); }
            if (state.timer > diff) { state.timer -= diff; return; }
            state.timer = 1000;
            started = state.started;
        }
        if (stale) Dargrul::Clear(player);
        if (Dargrul::Done(player,97781)) return;
        if (!Dargrul::Done(player,97871))
        {
            if (player->GetVehicle() || player->isInFlight() || Dargrul::Existing(player,97552)) return;
            for (Position const& point : Dargrul::Trail)
                if (player->GetExactDist(&point) < 30.0f)
                {
                    // Removing a stale controller aura also removes its old summon.
                    player->RemoveAurasDueToSpell(205051);
                    player->CastSpell(player,205051,true);
                    break;
                }
            return;
        }
        if (!Dargrul::InChamber(player))
        {
            if (started) OnLogout(player);
            return;
        }
        if (Dargrul::Done(player,94610)) { Dargrul::Grasp(player); return; }
        if (started || player->GetVehicle()) return;
        // Each player gets the existing two SmartAI timelines, independently of
        // the static NPCs' one-shot flags or another player's current scene.
        Creature* boss = player->SummonCreature(94610,3438.76f,4456.35f,710.192f,0,
            TEMPSUMMON_TIMED_DESPAWN,30000,player->GetGUID());
        Creature* shaper = player->SummonCreature(97827,3448.72f,4442.41f,710.261f,1.43828f,
            TEMPSUMMON_TIMED_DESPAWN,30000,player->GetGUID());
        if (!boss || !shaper) { Dargrul::Clear(player); return; }
        boss->SetReactState(REACT_PASSIVE);
        shaper->SetReactState(REACT_PASSIVE);
        boss->AI()->SetData(1,1);
        shaper->AI()->SetData(1,1);
        ObjectGuid bossGuid = boss->GetGUID();
        {
            std::lock_guard<std::mutex> lock(mutex);
            states[player->GetGUID()].started = true;
            states[player->GetGUID()].actor = bossGuid;
        }
        player->AddQuestDelayedEvent(Dargrul::QuestId,5500,[player,bossGuid]
        {
            if (Creature* actor = ObjectAccessor::GetCreature(*player,bossGuid))
                if (Dargrul::Owner(actor) == player && actor->IsAlive()) Dargrul::Grasp(player);
        });
        // Failed boarding/summoning must permit another real encounter attempt.
        player->AddQuestDelayedEvent(Dargrul::QuestId,30000,[player,bossGuid,this]
        {
            std::lock_guard<std::mutex> lock(mutex);
            auto itr = states.find(player->GetGUID());
            if (itr != states.end() && itr->second.actor == bossGuid) itr->second.started = false;
        });
    }
};

void AddSC_highmountain_dargrul()
{
    RegisterCreatureAI(npc_dargrul_escort);
    RegisterCreatureAI(npc_dargrul_grasp);
    RegisterSpellScript(spell_dargrul_escape);
    new player_dargrul_event();
}
