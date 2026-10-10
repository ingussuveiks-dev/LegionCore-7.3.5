#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SmartAI.h"
#include "SpellScript.h"
#include "Player.h"
#include "TemporarySummon.h"
#include "ObjectAccessor.h"
#include "QuestDef.h"
#include <set>

namespace ValsharahRituals
{
Position const Grove = {2610.6f,6745.28f,106.005f,0};
Position const EscortStart = {3145.67f,5774.15f,308.262f,0};
// Existing world waypoint path 103022, preserved in order.
Position const Path[] = {
    {3133.62f,5788.83f,309.58f,0},{3105.9f,5824.6f,299.07f,0},
    {3186.87f,5853.19f,265.22f,0},{3210.81f,5866.01f,277.7f,0},
    {3276.92f,5927.24f,253.7f,0},{3316.64f,5957.16f,250.45f,0}
};
Position const VigilStart = {3123.44f,5918.23f,286.13f,2.7f};
Position const Prayer = {3129.54f,5915.17f,287.85f,0};
Position const Attack = {3107.34f,5929.39f,295.73f,5.6f};

bool Ready(Player* player)
{
    return player && player->IsAlive() && player->GetMapId() == 1220 && !player->IsBeingTeleported();
}
bool EscortQuest(Player* player)
{
    return player && (player->GetQuestStatus(38675) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(41724) == QUEST_STATUS_INCOMPLETE);
}
bool VigilQuest(Player* player)
{
    return player && (player->GetQuestStatus(41708) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(41890) == QUEST_STATUS_INCOMPLETE);
}
Player* Owner(Creature* creature)
{
    Unit* current = creature;
    for (uint32 depth=0; current && depth<3; ++depth)
    {
        if (Player* player = current->ToPlayer()) return player;
        Creature* npc = current->ToCreature();
        TempSummon* summon = npc ? npc->ToTempSummon() : nullptr;
        current = summon ? summon->GetSummoner() : nullptr;
    }
    return nullptr;
}
Creature* Existing(Player* player, uint32 entry)
{
    for (ObjectGuid const& guid : *player->GetSummonList(entry))
        if (Creature* npc = ObjectAccessor::GetCreature(*player,guid))
            if (npc->IsAlive() && Owner(npc) == player) return npc;
    return nullptr;
}
void Clear(Player* player)
{
    for (uint32 entry : {103022,104739})
    {
        auto guids = *player->GetSummonList(entry);
        for (ObjectGuid const& guid : guids)
            if (Creature* npc = ObjectAccessor::GetCreature(*player,guid))
                if (Owner(npc) == player)
                {
                    npc->AI()->DoAction(-1);
                    npc->DespawnOrUnsummon();
                }
    }
}
void FinishRitual(Player* player)
{
    if (Ready(player) && player->GetQuestStatus(38377) == QUEST_STATUS_INCOMPLETE && player->GetDistance(Grove) < 80.0f)
        player->CastSpell(player,197487,true); // native credit 92742 + post-scene destination
}
void StartRitual(Player* player)
{
    if (Ready(player) && player->GetQuestStatus(38377) == QUEST_STATUS_INCOMPLETE && player->GetDistance(Grove) < 80.0f)
        // SceneScriptText 13744 lasts 1 + 2.5 + 7 + 18 seconds. A skip or
        // missing callback cannot award credit before this server-side wait.
        player->AddDelayedEvent(30000,[player]() { FinishRitual(player); });
}
void StartVigil(Player* player, Creature* source)
{
    if (!Ready(player) || !VigilQuest(player) || player->GetDistance(source) > 10.0f ||
        player->isInCombat() || player->GetVehicle() || Existing(player,104739)) return;
    player->SummonCreature(104739,VigilStart,TEMPSUMMON_MANUAL_DESPAWN,0,0,player->GetGUID());
}
}

class spell_valsharah_summon_ysera_ritual : public SpellScript
{
    PrepareSpellScript(spell_valsharah_summon_ysera_ritual);
    void Start()
    {
        if (Player* player = GetCaster()->ToPlayer()) ValsharahRituals::StartRitual(player);
    }
    void Register() override { AfterCast += SpellCastFn(spell_valsharah_summon_ysera_ritual::Start); }
};

struct npc_valsharah_path_tyrande : SmartAI
{
    npc_valsharah_path_tyrande(Creature* creature) : SmartAI(creature) { }
    uint32 point = 0, poll = 0;
    bool moving = false, mounted = false;
    void UpdateAI(uint32 diff) override
    {
        if (!me->ToTempSummon()) { SmartAI::UpdateAI(diff); return; }
        if (point >= 6) return;
        Player* player = ValsharahRituals::Owner(me);
        if (!ValsharahRituals::Ready(player) || !ValsharahRituals::EscortQuest(player) || player->GetDistance(me) > 400.0f)
        { me->DespawnOrUnsummon(); return; }
        if (poll > diff) { poll -= diff; return; }
        poll = 500;
        // Also applies to the existing custom 305067 summon: only its owner sees it.
        me->AddPlayerInPersonnalVisibilityList(player->GetGUID());
        me->RemoveFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_QUESTGIVER | UNIT_NPC_FLAG_GOSSIP);
        me->SetReactState(REACT_PASSIVE);
        if (!mounted) { me->Mount(67955); me->SetWalk(false); Talk(0,player->GetGUID()); mounted = true; }
        if (player->GetDistance(me) > 35.0f)
        { me->GetMotionMaster()->Clear(); moving = false; return; }
        if (me->GetDistance(ValsharahRituals::Path[point]) < 4.0f)
        {
            if (player->GetDistance(ValsharahRituals::Path[point]) > 20.0f) return;
            if (point == 1) Talk(1,player->GetGUID());
            if (point == 2) Talk(3,player->GetGUID());
            if (point == 3) Talk(4,player->GetGUID());
            if (++point == 6)
            {
                Talk(5,player->GetGUID());
                player->KilledMonsterCredit(103022);
                me->DespawnOrUnsummon();
                return;
            }
            moving = false;
        }
        if (!moving) { me->GetMotionMaster()->MovePoint(point+1,ValsharahRituals::Path[point]); moving = true; }
    }
};

struct npc_valsharah_vigil_tyrande : SmartAI
{
    npc_valsharah_vigil_tyrande(Creature* creature) : SmartAI(creature), summons(me) { }
    SummonList summons;
    std::set<ObjectGuid> alive;
    uint32 phase = 0, wave = 0, wait = 0, lifetime = 0;
    bool moving = false, stopping = false;
    void Stop()
    {
        if (stopping) return;
        stopping = true;
        alive.clear();
        summons.DespawnAll();
        me->RemoveAurasDueToSpell(130491);
        me->DespawnOrUnsummon();
    }
    void DoAction(int32 action) override { if (action == -1 && me->ToTempSummon()) Stop(); }
    void sGossipSelect(Player* player, uint32 menu, uint32 option) override
    {
        if (menu == 19405 && option == 1) ValsharahRituals::StartVigil(player,me);
        else SmartAI::sGossipSelect(player,menu,option);
    }
    void JustSummoned(Creature* summon) override { summons.Summon(summon); }
    void SummonedCreatureDespawn(Creature* summon) override
    {
        summons.Despawn(summon);
        if (!stopping && alive.count(summon->GetGUID())) Stop(); // disappearance is not a kill
    }
    void SummonedCreatureDies(Creature* summon, Unit*) override
    {
        Player* player = ValsharahRituals::Owner(me);
        if (stopping || phase != 1 || !ValsharahRituals::Ready(player) || !ValsharahRituals::VigilQuest(player) ||
            !alive.erase(summon->GetGUID()) || !alive.empty()) return;
        if (++wave < 3)
        {
            Talk(wave,player->GetGUID());
            wait = 10000;
        }
        else
        {
            Talk(6,player->GetGUID());
            me->RemoveAurasDueToSpell(130491);
            phase = 2;
            moving = false;
        }
    }
    void SpawnWave(Player* player)
    {
        uint32 entries[3][2] = {{104643,0},{104643,104644},{104646,0}};
        for (uint32 entry : entries[wave])
        {
            if (!entry) continue;
            Creature* enemy = me->SummonCreature(entry,ValsharahRituals::Attack,TEMPSUMMON_MANUAL_DESPAWN,0,0,player->GetGUID());
            if (!enemy) { Stop(); return; }
            alive.insert(enemy->GetGUID());
            enemy->AI()->AttackStart(player);
        }
    }
    void UpdateAI(uint32 diff) override
    {
        if (!me->ToTempSummon()) { SmartAI::UpdateAI(diff); return; }
        if (stopping) return;
        Player* player = ValsharahRituals::Owner(me);
        lifetime += diff;
        if (!ValsharahRituals::Ready(player) || !ValsharahRituals::VigilQuest(player) ||
            player->GetDistance(me) > 120.0f || lifetime > 900000) { Stop(); return; }
        me->RemoveFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_GOSSIP);
        me->SetReactState(REACT_PASSIVE);
        if (phase == 0)
        {
            if (!moving) { me->GetMotionMaster()->MovePoint(1,ValsharahRituals::Prayer); moving = true; }
            if (me->GetDistance(ValsharahRituals::Prayer) < 2.0f)
            { phase = 1; wait = 8000; DoCast(130491); Talk(0,player->GetGUID()); }
        }
        else if (phase == 1 && alive.empty())
        {
            if (wait > diff) wait -= diff;
            else { wait = 0; SpawnWave(player); }
        }
        else if (phase == 2)
        {
            if (!moving) { me->GetMotionMaster()->MovePoint(2,ValsharahRituals::VigilStart); moving = true; }
            if (me->GetDistance(ValsharahRituals::VigilStart) < 3.0f && player->GetDistance(me) < 15.0f)
            {
                // Direct owner credit intentionally avoids 207140's area targets,
                // which also awarded 103022 to uninvolved bystanders.
                Talk(7,player->GetGUID());
                Talk(8,player->GetGUID());
                player->KilledMonsterCredit(103022);
                Stop();
            }
        }
    }
};

struct npc_valsharah_vigil_enemy : ScriptedAI
{
    npc_valsharah_vigil_enemy(Creature* creature) : ScriptedAI(creature) { }
    void EnterEvadeMode() override
    {
        if (TempSummon* summon = me->ToTempSummon())
            if (Unit* owner = summon->GetSummoner())
                if (Creature* npc = owner->ToCreature()) npc->AI()->DoAction(-1);
        me->DespawnOrUnsummon();
    }
    void UpdateAI(uint32) override
    {
        Player* player = ValsharahRituals::Owner(me);
        if (!ValsharahRituals::Ready(player) || !ValsharahRituals::VigilQuest(player))
        { me->DespawnOrUnsummon(); return; }
        if (!UpdateVictim()) AttackStart(player);
        DoMeleeAttackIfReady();
    }
};

class player_valsharah_rituals : public PlayerScript
{
public:
    player_valsharah_rituals() : PlayerScript("player_valsharah_rituals") { }
    void OnUpdate(Player* player, uint32) override
    {
        if (ValsharahRituals::Ready(player) && ValsharahRituals::EscortQuest(player) &&
            player->GetDistance(ValsharahRituals::EscortStart) < 60.0f && !ValsharahRituals::Existing(player,103022))
            player->SummonCreature(103022,ValsharahRituals::EscortStart,TEMPSUMMON_MANUAL_DESPAWN,0,0,player->GetGUID());
    }
    void OnLogout(Player* player) override { ValsharahRituals::Clear(player); }
    void OnMapChanged(Player* player) override { ValsharahRituals::Clear(player); }
    void OnQuestReward(Player* player, Quest const* quest) override
    {
        if (quest->GetQuestId() == 38675 || quest->GetQuestId() == 41724 || quest->GetQuestId() == 41708 || quest->GetQuestId() == 41890)
            ValsharahRituals::Clear(player);
    }
};

void AddSC_valsharah_rituals()
{
    RegisterSpellScript(spell_valsharah_summon_ysera_ritual);
    RegisterCreatureAI(npc_valsharah_path_tyrande);
    RegisterCreatureAI(npc_valsharah_vigil_tyrande);
    RegisterCreatureAI(npc_valsharah_vigil_enemy);
    new player_valsharah_rituals();
}
