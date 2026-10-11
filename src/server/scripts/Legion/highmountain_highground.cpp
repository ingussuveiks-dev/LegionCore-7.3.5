#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "Player.h"
#include "TemporarySummon.h"
#include "ObjectAccessor.h"
#include "QuestData.h"
#include <map>
#include <mutex>

namespace HighGround
{
// Existing event 1501, existing Jale destination, and the existing Mayla spawn.
Position const Route[] = {{4029.02f,4542.88f,658.72f,0},
    {4045.83f,4482.76f,661.67f,5}, {4083.05f,4385.5f,670.709f,1.56092f}};
bool Ready(Player* player)
{
    return player && player->IsAlive() && player->GetMapId() == 1220 &&
        (player->GetZoneId() == 7503 || player->GetZoneId() == 7731) &&
        !player->IsBeingTeleported() && !player->isInFlight() && !player->IsFlying() &&
        !player->GetVehicle() && player->GetQuestStatus(38909) == QUEST_STATUS_INCOMPLETE;
}
bool Done(Player* player, uint32 entry) { return player->GetQuestObjectiveData(38909,entry) > 0; }
Player* Owner(Creature* npc)
{
    TempSummon* summon = npc->ToTempSummon();
    Unit* owner = summon ? summon->GetSummoner() : nullptr;
    return owner ? owner->ToPlayer() : nullptr;
}
void Credit(Player* player, uint32 entry, ObjectGuid guid)
{
    if (!Ready(player) || Done(player,entry)) return;
    if (Quest const* quest = sQuestDataStore->GetQuestTemplate(38909))
        for (QuestObjective const& objective : quest->GetObjectives())
            if (objective.ObjectID == entry)
            {
                player->SetQuestObjectiveData(quest,&objective,1);
                player->SendQuestUpdateAddCredit(quest,guid,objective,1);
                if (player->CanCompleteQuest(38909)) player->CompleteQuest(38909);
                else player->SetQuestStatus(38909,QUEST_STATUS_INCOMPLETE);
                return;
            }
}
Creature* Existing(Player* player)
{
    auto guids = *player->GetSummonList(95078);
    for (ObjectGuid const& guid : guids)
        if (Creature* npc = ObjectAccessor::GetCreature(*player,guid))
            if (Owner(npc) == player && npc->IsAlive()) return npc;
    return nullptr;
}
void Clear(Player* player)
{
    auto guids = *player->GetSummonList(95078);
    for (ObjectGuid const& guid : guids)
        if (Creature* npc = ObjectAccessor::GetCreature(*player,guid))
            if (Owner(npc) == player) npc->DespawnOrUnsummon();
}
}

class spell_highground_jale : public SpellScript
{
    PrepareSpellScript(spell_highground_jale);
    SpellCastResult Check()
    {
        Player* player = GetCaster()->ToPlayer();
        return HighGround::Ready(player) && !HighGround::Existing(player) ? SPELL_CAST_OK : SPELL_FAILED_DONT_REPORT;
    }
    void Register() override { OnCheckCast += SpellCheckCastFn(spell_highground_jale::Check); }
};

struct npc_highground_jale : ScriptedAI
{
    npc_highground_jale(Creature* creature) : ScriptedAI(creature) { }
    std::weak_ptr<uint8> lifetime;
    uint32 point = 0, travel = 0;
    bool leading = false, moving = false, arrived = false;
    void IsSummonedBy(Unit* summoner) override
    {
        Player* player = summoner->ToPlayer();
        if (!HighGround::Ready(player)) { me->DespawnOrUnsummon(); return; }
        player->AddQuestDelayedEvent(38909,0,[] { });
        lifetime = player->getQuestStatus(38909)->ScriptLifetime;
        me->AddPlayerInPersonnalVisibilityList(player->GetGUID());
        me->SetReactState(REACT_PASSIVE);
        me->SetWalk(false);
        me->GetMotionMaster()->MoveFollow(player,3.0f,3.14f);
    }
    void MovementInform(uint32 type, uint32 id) override
    {
        if (type != POINT_MOTION_TYPE || !moving || id != point + 1) return;
        moving = false;
        if (++point == 3) arrived = true;
    }
    void UpdateAI(uint32 diff) override
    {
        Player* player = HighGround::Owner(me);
        if (lifetime.expired() || !HighGround::Ready(player) ||
            !player->InSamePhase(me) || player->GetDistance(me) > 80.0f)
        { me->DespawnOrUnsummon(); return; }
        if (!leading)
        {
            Creature* bridge = me->FindNearestCreature(94547,15.0f,true);
            Creature* mayla = me->FindNearestCreature(93826,15.0f,true);
            bool atBridge = bridge && me->InSamePhase(bridge) && player->GetDistance(bridge) <= 15.0f;
            bool atMayla = mayla && me->InSamePhase(mayla) && player->GetDistance(mayla) <= 15.0f;
            // Native quest also allows returning by hearthstone/flight. Both
            // owner and recovered companion must actually reach Thunder Totem.
            if (player->GetDistance(me) <= 15.0f && (atBridge || atMayla))
                HighGround::Credit(player,94547,me->GetGUID());
            if (!HighGround::Done(player,94547)) return;
            // Resume the last leg after reconnect; don't send Jale back across
            // the bridge or grant the second objective merely for summoning her.
            for (uint32 i = 1; i < 3; ++i)
                if (me->GetExactDist(&HighGround::Route[i]) < me->GetExactDist(&HighGround::Route[point])) point = i;
            leading = true;
            me->GetMotionMaster()->Clear();
            me->SetWalk(true);
            Talk(0,player->GetGUID());
        }
        if (arrived)
        {
            Creature* mayla = me->FindNearestCreature(93826,10.0f,true);
            if (mayla && me->InSamePhase(mayla) && player->GetDistance(mayla) <= 15.0f &&
                player->canSeeOrDetect(mayla))
            {
                HighGround::Credit(player,95078,me->GetGUID());
                me->DespawnOrUnsummon();
            }
            return;
        }
        if (moving)
        {
            if (player->GetDistance(me) > 30.0f)
            { me->GetMotionMaster()->Clear(); moving = false; return; }
            if (travel <= diff) me->DespawnOrUnsummon();
            else travel -= diff;
            return;
        }
        if (player->GetDistance(me) > 20.0f) return;
        moving = true;
        travel = 120000;
        me->GetMotionMaster()->MovePoint(point + 1,HighGround::Route[point]);
    }
};

class player_highground_jale : public PlayerScript
{
    struct State { uint32 timer = 0; std::weak_ptr<uint8> lifetime; };
    std::mutex mutex;
    std::map<ObjectGuid,State> states;
public:
    player_highground_jale() : PlayerScript("player_highground_jale") { }
    void OnLogout(Player* player) override
    {
        bool serviced;
        { std::lock_guard<std::mutex> lock(mutex); serviced = states.erase(player->GetGUID()) != 0; }
        // Native acceptance can summon her before our first update.
        if (serviced || player->GetQuestStatus(38909) == QUEST_STATUS_INCOMPLETE) HighGround::Clear(player);
    }
    void OnMapChanged(Player* player) override { OnLogout(player); }
    void OnLogin(Player* player) override
    {
        auto status = player->GetQuestStatus(38909);
        if ((status == QUEST_STATUS_INCOMPLETE || status == QUEST_STATUS_COMPLETE) &&
            HighGround::Done(player,95078) && !HighGround::Done(player,94547))
        {
            if (Quest const* quest = sQuestDataStore->GetQuestTemplate(38909))
                for (QuestObjective const& objective : quest->GetObjectives())
                    if (objective.ObjectID == 95078) player->SetQuestObjectiveData(quest,&objective,0);
            player->IncompleteQuest(38909);
        }
    }
    void OnUpdate(Player* player, uint32 diff) override
    {
        if (!HighGround::Ready(player)) { OnLogout(player); return; }
        bool stale;
        {
            std::lock_guard<std::mutex> lock(mutex);
            auto inserted = states.emplace(player->GetGUID(),State{});
            State& state = inserted.first->second;
            stale = !inserted.second && state.lifetime.expired();
            if (inserted.second || stale)
            {
                player->AddQuestDelayedEvent(38909,0,[] { });
                state = State{};
                state.lifetime = player->getQuestStatus(38909)->ScriptLifetime;
            }
            if (state.timer > diff) { state.timer -= diff; return; }
            state.timer = 1000;
        }
        if (stale) HighGround::Clear(player);
        if (!HighGround::Existing(player)) player->CastSpell(player,188583,true);
    }
};

void AddSC_highmountain_highground()
{
    RegisterSpellScript(spell_highground_jale);
    RegisterCreatureAI(npc_highground_jale);
    new player_highground_jale();
}
