#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SmartAI.h"
#include "Player.h"
#include "TemporarySummon.h"
#include "ObjectAccessor.h"
#include "QuestDef.h"

namespace ValsharahFinale
{
// Recovery placements: native quest POI regions and existing world spawn heights.
// These are not a captured retail escort spline.
Position const Entrance = {3527.78f,6125.69f,185.758f,0};
Position const Searches[] = {
    {3599.18f,6146.14f,180.755f,0},
    {3719.23f,6162.69f,183.577f,0},
    {3752.63f,6318.8f,185.554f,0}
};
Position const Found = {3555.88f,6303.12f,165.54f,0};
Position const Temple = {2929.38f,6592.05f,215.18f,1.8f};
Position const Ysera = {2911.69f,6648.32f,214.987f,5.04898f};
Position const Ender = {2910.21f,6650.52f,215.004f,0};
uint32 const Illusions[2][3] = {{111258,111260,111259},{111203,111198,111204}};
uint32 const Actors[] = {104728,111258,111260,111259,111203,111198,111204,93065,104921};

Player* Owner(Creature* npc)
{
    TempSummon* summon = npc ? npc->ToTempSummon() : nullptr;
    Unit* owner = summon ? summon->GetSummoner() : nullptr;
    return owner ? owner->ToPlayer() : nullptr;
}
bool Ready(Player* player)
{
    return player && player->IsAlive() && player->GetMapId() == 1220 && !player->IsBeingTeleported();
}
uint32 SearchQuest(Player* player)
{
    for (uint32 quest : {38687,41763})
        if (player->GetQuestStatus(quest) == QUEST_STATUS_INCOMPLETE) return quest;
    return 0;
}
bool Searched(Player* player, uint32 quest)
{
    if (!quest || !player->GetQuestObjectiveData(quest,104799)) return false;
    for (uint32 entry : Illusions[quest == 41763])
        if (!player->GetQuestObjectiveData(quest,entry)) return false;
    return true;
}
Creature* Existing(Player* player, uint32 entry)
{
    for (ObjectGuid const& guid : *player->GetSummonList(entry))
        if (Creature* npc = ObjectAccessor::GetCreature(*player,guid))
            if (npc->IsAlive() && Owner(npc) == player) return npc;
    return nullptr;
}
Creature* Spawn(Player* player, uint32 entry, Position const& position)
{
    if (Creature* npc = Existing(player,entry)) return npc;
    return player->SummonCreature(entry,position,TEMPSUMMON_MANUAL_DESPAWN,0,0,player->GetGUID());
}
void Clear(Player* player)
{
    for (uint32 entry : Actors)
    {
        auto guids = *player->GetSummonList(entry);
        for (ObjectGuid const& guid : guids)
            if (Creature* npc = ObjectAccessor::GetCreature(*player,guid))
                if (Owner(npc) == player) npc->DespawnOrUnsummon();
    }
}
void Search(Player* player, Creature* source)
{
    if (!Ready(player) || !SearchQuest(player) || player->isInCombat() || player->GetVehicle() ||
        player->GetDistance(source) > 10.0f) return;
    Spawn(player,104728,player->GetPosition());
}
void TempleRecovery(Player* player)
{
    if (!Ready(player) || player->isWatchingMovie() || player->GetDistance(Temple) > 180.0f) return;
    if (player->GetQuestStatus(38743) == QUEST_STATUS_INCOMPLETE &&
        player->GetQuestObjectiveData(38743,104799) && !player->GetQuestObjectiveData(38743,93065))
        Spawn(player,93065,Ysera);
    else if (player->GetQuestStatus(38743) == QUEST_STATUS_COMPLETE)
    {
        if (Creature* existing = player->FindNearestCreature(104921,180.0f))
            if (player->canSeeOrDetect(existing)) return;
        Spawn(player,104921,Ender);
    }
}
void Arrive(Player* player, uint32 retries)
{
    if (player->GetQuestStatus(38743) != QUEST_STATUS_INCOMPLETE) return;
    if (!Ready(player) || player->GetDistance(Temple) > 15.0f)
    {
        if (retries) player->AddQuestDelayedEvent(38743,500,[player,retries]() { Arrive(player,retries-1); });
        return;
    }
    if (!player->GetQuestObjectiveData(38743,104799)) player->KilledMonsterCredit(104799);
    TempleRecovery(player);
}
void StartBattle(Player* player, Creature* source)
{
    if (!Ready(player) || player->GetQuestStatus(38743) != QUEST_STATUS_INCOMPLETE ||
        player->isInCombat() || player->GetVehicle() || player->GetDistance(source) > 10.0f) return;
    // Preserve the existing transport destination; acknowledge arrival before credit/spawn.
    if (player->TeleportTo(1220,Temple.GetPositionX(),Temple.GetPositionY(),Temple.GetPositionZ(),Temple.GetOrientation()))
        player->AddQuestDelayedEvent(38743,500,[player]() { Arrive(player,40); });
}
}

struct npc_valsharah_search_tyrande : SmartAI
{
    npc_valsharah_search_tyrande(Creature* creature) : SmartAI(creature) { }
    uint32 poll = 0, sceneWait = 0;
    bool moving = false, sceneStarted = false;
    void sGossipSelect(Player* player, uint32 menu, uint32 option) override
    {
        if (menu == 19419 && option == 1) ValsharahFinale::Search(player,me);
        else SmartAI::sGossipSelect(player,menu,option);
    }
    void DoAction(int32 action) override
    {
        Player* player = ValsharahFinale::Owner(me);
        uint32 quest = player ? ValsharahFinale::SearchQuest(player) : 0;
        if (action == 1 && sceneStarted && ValsharahFinale::Ready(player) &&
            ValsharahFinale::Searched(player,quest) && player->GetDistance(ValsharahFinale::Found) < 50.0f)
        {
            sceneStarted = false;
            player->CastSpell(player,208446,true);
            me->DespawnOrUnsummon(1000);
        }
    }
    void UpdateAI(uint32 diff) override
    {
        if (!me->ToTempSummon()) { SmartAI::UpdateAI(diff); return; }
        Player* player = ValsharahFinale::Owner(me);
        uint32 quest = player ? ValsharahFinale::SearchQuest(player) : 0;
        if (!ValsharahFinale::Ready(player) || !quest || player->GetDistance(me) > 500.0f)
        { me->DespawnOrUnsummon(); return; }
        if (sceneStarted)
        {
            if (sceneWait <= diff) { DoAction(1); sceneWait = 1000; }
            else sceneWait -= diff;
            return;
        }
        if (poll > diff) { poll -= diff; return; }
        poll = 1000;
        me->SetReactState(REACT_PASSIVE);
        me->RemoveFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_QUESTGIVER);
        if (!player->GetQuestObjectiveData(quest,104799))
        {
            if (player->GetDistance(me) > 40.0f)
            { me->GetMotionMaster()->Clear(); moving = false; return; }
            if (!moving)
            { me->GetMotionMaster()->MovePoint(1,ValsharahFinale::Entrance); moving = true; }
            if (me->GetDistance(ValsharahFinale::Entrance) < 8.0f && player->GetDistance(ValsharahFinale::Entrance) < 20.0f)
            {
                player->KilledMonsterCredit(104799);
                player->CastSpell(player,221449,true);
            }
            return;
        }
        for (uint32 i=0; i<3; ++i)
        {
            uint32 entry = ValsharahFinale::Illusions[quest == 41763][i];
            if (!player->GetQuestObjectiveData(quest,entry))
                ValsharahFinale::Spawn(player,entry,ValsharahFinale::Searches[i]);
        }
        if (ValsharahFinale::Searched(player,quest) && player->GetDistance(ValsharahFinale::Found) < 20.0f)
        {
            sceneStarted = true;
            sceneWait = 90000; // recovery if the native scene's callback is lost
            player->CastSpell(player,208444,true);
        }
    }
};

struct npc_valsharah_malfurion_search : ScriptedAI
{
    npc_valsharah_malfurion_search(Creature* creature) : ScriptedAI(creature) { }
    void OnSpellClick(Unit* clicker) override
    {
        Player* player = clicker->ToPlayer();
        if (!ValsharahFinale::Ready(player) || ValsharahFinale::Owner(me) != player || player->GetDistance(me) > 5.0f) return;
        uint32 quest = ValsharahFinale::SearchQuest(player);
        if (!quest || !player->GetQuestObjectiveData(quest,104799)) return;
        for (uint32 entry : ValsharahFinale::Illusions[quest == 41763])
            if (me->GetEntry() == entry && !player->GetQuestObjectiveData(quest,entry))
            {
                player->KilledMonsterCredit(entry);
                me->DespawnOrUnsummon(1000);
            }
    }
    void UpdateAI(uint32) override
    {
        Player* player = ValsharahFinale::Owner(me);
        if (!ValsharahFinale::Ready(player) || !ValsharahFinale::SearchQuest(player)) me->DespawnOrUnsummon();
    }
};

class scene_valsharah_choice : public SceneTriggerScript
{
public:
    scene_valsharah_choice() : SceneTriggerScript("scene_valsharah_choice") { }
    bool OnTrigger(Player* player, SpellScene const* scene, std::string trigger) override
    {
        if (scene->MiscValue != 1246) return false;
        if (trigger == "complete")
            if (Creature* npc = ValsharahFinale::Existing(player,104728)) npc->AI()->DoAction(1);
        return true;
    }
};

struct npc_valsharah_temple_departure : SmartAI
{
    npc_valsharah_temple_departure(Creature* creature) : SmartAI(creature) { }
    void sGossipSelect(Player* player, uint32 menu, uint32 option) override
    {
        if (menu == 19474 && option == 0) ValsharahFinale::StartBattle(player,me);
        else SmartAI::sGossipSelect(player,menu,option);
    }
};

struct npc_valsharah_ysera_finale : ScriptedAI
{
    npc_valsharah_ysera_finale(Creature* creature) : ScriptedAI(creature) { }
    uint32 breath = 8000, cries = 15000;
    bool participated = false;
    void DamageTaken(Unit* attacker, uint32&, DamageEffectType) override
    {
        if (attacker && attacker->GetCharmerOrOwnerPlayerOrPlayerItself() == ValsharahFinale::Owner(me)) participated = true;
    }
    void EnterEvadeMode() override
    {
        if (ValsharahFinale::Owner(me)) me->DespawnOrUnsummon();
        else ScriptedAI::EnterEvadeMode();
    }
    void JustDied(Unit*) override
    {
        Player* player = ValsharahFinale::Owner(me);
        if (!participated || !ValsharahFinale::Ready(player) || player->GetDistance(me) > 100.0f ||
            !player->GetQuestObjectiveData(38743,104799)) return;
        if (player->GetQuestStatus(38743) == QUEST_STATUS_INCOMPLETE) player->KilledMonsterCredit(93065);
        if (player->GetQuestStatus(38743) == QUEST_STATUS_COMPLETE)
        {
            player->CastSpell(player,194213,true);
            // The quest progress is persistent. Login/nearby recovery supplies
            // the ender even if the movie is skipped or the connection is lost.
            player->AddDelayedEvent(1000,[player]() { ValsharahFinale::TempleRecovery(player); });
        }
        me->DespawnOrUnsummon(30000);
    }
    void UpdateAI(uint32 diff) override
    {
        if (me->ToTempSummon())
        {
            Player* player = ValsharahFinale::Owner(me);
            if (!ValsharahFinale::Ready(player) || player->GetQuestStatus(38743) != QUEST_STATUS_INCOMPLETE ||
                player->GetDistance(me) > 180.0f)
            { me->DespawnOrUnsummon(); return; }
        }
        if (!UpdateVictim()) return;
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;
        if (breath <= diff) { DoCast(208292); breath = 14000; } else breath -= diff;
        if (cries <= diff) { DoCast(190406); cries = 20000; } else cries -= diff;
        DoMeleeAttackIfReady();
    }
};

class player_valsharah_finale : public PlayerScript
{
public:
    player_valsharah_finale() : PlayerScript("player_valsharah_finale") { }
    void OnUpdate(Player* player, uint32) override
    {
        // This hook is player-owned; no global player pointers or retry loops.
        // Only inspect the relevant quest near its encounter, never grant credit.
        if (player->GetMapId() == 1220 && player->GetQuestStatus(38743) != QUEST_STATUS_NONE &&
            !player->GetQuestRewardStatus(38743)) ValsharahFinale::TempleRecovery(player);
        uint32 quest = ValsharahFinale::SearchQuest(player);
        if (ValsharahFinale::Ready(player) && quest && player->GetQuestObjectiveData(quest,104799) &&
            player->GetDistance(ValsharahFinale::Entrance) < 500.0f)
            ValsharahFinale::Spawn(player,104728,ValsharahFinale::Entrance);
        if (ValsharahFinale::Existing(player,104921) &&
            (player->GetQuestStatus(38743) == QUEST_STATUS_NONE || player->GetQuestRewardStatus(38743)))
            ValsharahFinale::Clear(player);
    }
    void OnLogout(Player* player) override { ValsharahFinale::Clear(player); }
    void OnMapChanged(Player* player) override { ValsharahFinale::Clear(player); }
    void OnQuestReward(Player* player, Quest const* quest) override
    {
        if (quest->GetQuestId() == 38743 || quest->GetQuestId() == 38687 || quest->GetQuestId() == 41763)
            ValsharahFinale::Clear(player);
    }
};

void AddSC_valsharah_finale()
{
    RegisterCreatureAI(npc_valsharah_search_tyrande);
    RegisterCreatureAI(npc_valsharah_malfurion_search);
    RegisterCreatureAI(npc_valsharah_temple_departure);
    RegisterCreatureAI(npc_valsharah_ysera_finale);
    new scene_valsharah_choice();
    new player_valsharah_finale();
}
