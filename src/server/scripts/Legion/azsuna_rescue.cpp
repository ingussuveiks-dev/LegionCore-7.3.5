#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "Player.h"
#include "TemporarySummon.h"
#include "ObjectAccessor.h"
#include "SpellMgr.h"
#include "Vehicle.h"
#include "QuestDef.h"

namespace AzsunaRescue
{
uint32 const QuestId = 37530;
uint32 const Credits[] = {89325,91395,91396,91397,91399,91400,89323};
// Start: existing 178285 destination. Other X/Y: saved quest POIs.
// Heights: existing nearby spawns; cave: native scene Path 15600.
Position const Start = {-62.06f,6252.16f,3.48f,0.366868f};
Position const Academy = {8.0f,6284.0f,3.2f,0};
Position const Pursuit = {7.0f,6150.0f,9.20659f,0};
Position const Cave = {-19.482639f,5975.091309f,0.698100f,2.630191f};
Position const Prisoner = {-98.3438f,6018.33f,0.503117f,2.59999f};

Player* Owner(Creature* creature)
{
    TempSummon* summon = creature ? creature->ToTempSummon() : nullptr;
    Unit* owner = summon ? summon->GetSummoner() : nullptr;
    return owner ? owner->ToPlayer() : nullptr;
}
bool Active(Player* player)
{
    return player && player->IsAlive() && player->GetMapId() == 1220 &&
        player->GetZoneId() == 7334 && player->GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE;
}
uint32 Next(Player* player)
{
    for (uint32 i = 0; i < 7; ++i)
        if (!player->GetQuestObjectiveData(QuestId, Credits[i])) return i;
    return 7;
}
void Credit(Player* player, uint32 step)
{
    if (Active(player) && Next(player) == step) player->KilledMonsterCredit(Credits[step]);
}
bool Near(Creature* creature, Position const& position, float radius)
{
    return creature->GetDistance(position) < radius &&
        std::abs(creature->GetPositionZ() - position.GetPositionZ()) < 10.0f;
}
Creature* Ride(Player* player)
{
    Creature* ride = player ? player->GetVehicleCreatureBase() : nullptr;
    return ride && ride->GetEntry() == 89089 && Owner(ride) == player ? ride : nullptr;
}
void Clear(Player* player)
{
    auto guids = *player->GetSummonList(89089);
    for (ObjectGuid const& guid : guids)
        if (Creature* ride = ObjectAccessor::GetCreature(*player, guid))
            if (Owner(ride) == player)
            {
                ride->AI()->DoAction(2);
                if (player->GetVehicleBase() == ride) player->ExitVehicle();
                ride->DespawnOrUnsummon();
            }
    player->RemoveAurasDueToSpell(178284);
    player->RemoveAurasDueToSpell(197936);
}
void BoardAfterTeleport(Player* player, uint32 retries)
{
    if (!Active(player) || player->GetVehicle()) return;
    if (player->IsBeingTeleported() || player->GetDistance(Start) > 10.0f)
    {
        if (retries) player->AddDelayedEvent(500, [player, retries]() { BoardAfterTeleport(player, retries - 1); });
        return;
    }
    for (ObjectGuid const& guid : *player->GetSummonList(89089))
        if (Creature* existing = ObjectAccessor::GetCreature(*player, guid))
            if (Owner(existing) == player && existing->IsAlive()) return;
    if (Creature* ride = player->SummonCreature(89089, Start, TEMPSUMMON_MANUAL_DESPAWN, 0, 3972, player->GetGUID()))
    {
        ride->setFaction(player->getFaction());
        player->CastSpell(ride, 178284, true);
    }
}
}

// Personal replay: actual movement and actual combat, with persisted quest
// objectives as checkpoints. A retry never awards an unperformed step.
struct npc_azsuna_rescue_farondis : ScriptedAI
{
    npc_azsuna_rescue_farondis(Creature* creature) : ScriptedAI(creature), summons(me)
    {
        me->SetReactState(REACT_PASSIVE);
    }
    SummonList summons;
    ObjectGuid athissa, parjesh, queen;
    bool athissaDefeated = false, parjeshDefeated = false, boarded = false;
    uint32 poll = 0, boardingWait = 0, finale = 0, lifetime = 0;
    bool finaleRunning = false, prisonerSpawned = false;

    void JustSummoned(Creature* summon) override { summons.Summon(summon); }
    void JustDied(Unit*) override { summons.DespawnAll(); }
    void SummonedCreatureDespawn(Creature* summon) override { summons.Despawn(summon); }
    void PassengerBoarded(Unit* passenger, int8, bool apply) override
    {
        Player* player = AzsunaRescue::Owner(me);
        if (passenger != player) return;
        boarded = apply;
        if (apply) AzsunaRescue::Credit(player, 0);
        else
        {
            summons.DespawnAll();
            player->RemoveAurasDueToSpell(197936);
            me->DespawnOrUnsummon(1000);
        }
    }
    Creature* Actor(uint32 entry, Position const& position)
    {
        Player* player = AzsunaRescue::Owner(me);
        return player ? me->SummonCreature(entry, position, TEMPSUMMON_TIMED_DESPAWN, 900000, 0, player->GetGUID()) : nullptr;
    }
    void SummonedCreatureDies(Creature* summon, Unit*) override
    {
        Player* player = AzsunaRescue::Owner(me);
        if (!boarded || AzsunaRescue::Ride(player) != me || !AzsunaRescue::Active(player) ||
            AzsunaRescue::Next(player) != 3) return;
        if (summon->GetGUID() == athissa) athissaDefeated = true;
        if (summon->GetGUID() == parjesh) parjeshDefeated = true;
        if (athissaDefeated && parjeshDefeated) AzsunaRescue::Credit(player, 3);
    }
    void SetGUID(ObjectGuid const& guid, int32 id) override
    {
        Player* player = AzsunaRescue::Owner(me);
        if (id != 1 || guid != queen || finaleRunning || !AzsunaRescue::Active(player) ||
            AzsunaRescue::Ride(player) != me || AzsunaRescue::Next(player) != 5 ||
            !AzsunaRescue::Near(me, AzsunaRescue::Cave, 40.0f)) return;
        finale = 0;
        finaleRunning = true;
        if (Creature* actor = ObjectAccessor::GetCreature(*me, queen)) actor->SetVisible(false);
        player->CastSpell(player, 197936, true);
    }
    void DoAction(int32 action) override
    {
        if (action == 2) { summons.DespawnAll(); return; }
        // Scene cancellation uses the same core callback as completion.
        // An early callback must never count as defeating Azshara.
        if (action == 1 && finaleRunning && finale < 20000)
        {
            finaleRunning = false;
            finale = 0;
            if (Creature* actor = ObjectAccessor::GetCreature(*me, queen)) actor->SetVisible(true);
        }
    }
    void UpdateAI(uint32 diff) override
    {
        using namespace AzsunaRescue;
        Player* player = Owner(me);
        lifetime += diff;
        if (lifetime >= 900000 || !player || !player->IsAlive() || player->GetMapId() != 1220 || player->GetZoneId() != 7334 ||
            (player->GetQuestStatus(QuestId) != QUEST_STATUS_INCOMPLETE && player->GetQuestStatus(QuestId) != QUEST_STATUS_COMPLETE))
        {
            summons.DespawnAll();
            me->DespawnOrUnsummon();
            return;
        }
        if (!boarded || Ride(player) != me)
        {
            boardingWait += diff;
            if (boardingWait >= 5000) { summons.DespawnAll(); me->DespawnOrUnsummon(); }
            return;
        }
        if (finaleRunning)
        {
            if (!Near(me, Cave, 40.0f))
            {
                finaleRunning = false;
                player->RemoveAurasDueToSpell(197936);
                if (Creature* actor = ObjectAccessor::GetCreature(*me, queen)) actor->SetVisible(true);
            }
            else
            {
                finale += diff;
                // Native scene: defeat at 14+6 seconds, end after 2 more.
                // Server-side defeat
                // has already been triggered by the player's offensive spell.
                if (finale >= 22000)
                {
                    finaleRunning = false;
                    Credit(player, 5);
                    player->RemoveAurasDueToSpell(197936);
                    if (Creature* actor = ObjectAccessor::GetCreature(*me, queen)) actor->DespawnOrUnsummon();
                }
            }
        }
        poll += diff;
        if (poll < 500) return;
        poll = 0;
        switch (Next(player))
        {
            case 1:
                if (Near(me, Academy, 18.0f)) Credit(player, 1);
                break;
            case 2:
                if (Near(me, Pursuit, 25.0f)) Credit(player, 2);
                break;
            case 3:
                if (!Near(me, Pursuit, 55.0f)) break;
                if (!athissaDefeated && !ObjectAccessor::GetCreature(*me, athissa))
                    if (Creature* actor = Actor(89116, {15.3004f,6149.94f,9.20659f,1.44427f}))
                    { athissa = actor->GetGUID(); actor->AI()->AttackStart(me); }
                if (!parjeshDefeated && !ObjectAccessor::GetCreature(*me, parjesh))
                    if (Creature* actor = Actor(89117, {12.3004f,6149.94f,9.20659f,1.44427f}))
                    { parjesh = actor->GetGUID(); actor->AI()->AttackStart(me); }
                break;
            case 4:
                if (Near(me, Cave, 25.0f)) Credit(player, 4);
                break;
            case 5:
                if (Near(me, Cave, 45.0f) && !ObjectAccessor::GetCreature(*me, queen))
                    if (Creature* actor = Actor(91402, Cave)) queen = actor->GetGUID();
                break;
            case 6:
            case 7:
                if (!prisonerSpawned)
                    if (Creature* actor = Actor(89090, Prisoner))
                    {
                        actor->SetDisplayId(player->GetNativeDisplayId());
                        // Native clone appearance effect, including equipment.
                        player->CastSpell(actor, 45204, true);
                        prisonerSpawned = true;
                    }
                if (Near(me, Prisoner, 8.0f)) Credit(player, 6);
                break;
            default: break;
        }
    }
};

class npc_azsuna_rescue_gossip : public CreatureScript
{
public:
    npc_azsuna_rescue_gossip() : CreatureScript("npc_azsuna_rescue_gossip") { }
    bool OnGossipHello(Player* player, Creature* creature) override
    {
        player->PrepareQuestMenu(creature->GetGUID());
        if (AzsunaRescue::Active(player) && !player->GetVehicle())
            player->ADD_GOSSIP_ITEM(GossipOptionNpc::None, "What happened, Farondis?", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
        return true;
    }
    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action) override
    {
        if (sender != GOSSIP_SENDER_MAIN || action != GOSSIP_ACTION_INFO_DEF + 1 || !AzsunaRescue::Active(player) ||
            player->GetVehicle() || player->isInCombat() || !player->IsWithinDistInMap(creature, 6.0f) ||
            !player->IsWithinLOSInMap(creature)) return true;
        player->PlayerTalkClass->SendCloseGossip();
        AzsunaRescue::Clear(player);
        auto const& pos = AzsunaRescue::Start;
        player->NearTeleportTo(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation());
        // Wait for the client's teleport acknowledgement before casting the
        // short-range boarding spell. Player-owned events die with the player.
        player->AddDelayedEvent(500, [player]() { AzsunaRescue::BoardAfterTeleport(player, 20); });
        return true;
    }
};

struct npc_azsuna_rescue_enemy : ScriptedAI
{
    npc_azsuna_rescue_enemy(Creature* creature) : ScriptedAI(creature) { }
    uint32 timer = 2000;
    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim()) return;
        if (timer <= diff)
        {
            if (me->GetEntry() == 89116) DoCastVictim(15497);
            timer = 4000;
        }
        else timer -= diff;
        DoMeleeAttackIfReady();
    }
};

struct npc_azsuna_rescue_azshara : ScriptedAI
{
    npc_azsuna_rescue_azshara(Creature* creature) : ScriptedAI(creature) { me->SetReactState(REACT_PASSIVE); }
    // This is a vision. The native Tidestone scene defeats her; ordinary
    // damage must not kill the actor before that scene can run.
    void DamageTaken(Unit*, uint32& damage, DamageEffectType) override { damage = 0; }
    void SpellHit(Unit* caster, SpellInfo const* spell) override
    {
        TempSummon* summon = me->ToTempSummon();
        if (summon && caster == summon->GetSummoner() && caster->IsCreature() &&
            (spell->Id == 178784 || spell->Id == 179217))
            caster->ToCreature()->AI()->SetGUID(me->GetGUID(), 1);
    }
};

class scene_azsuna_rescue : public SceneTriggerScript
{
public:
    scene_azsuna_rescue() : SceneTriggerScript("scene_azsuna_rescue") { }
    bool OnTrigger(Player* player, SpellScene const* scene, std::string trigger) override
    {
        if (scene->MiscValue != 1148) return false;
        if (trigger == "complete")
            if (Creature* ride = AzsunaRescue::Ride(player)) ride->AI()->DoAction(1);
        return true;
    }
};

// The native button is DUMMY; its matching 7.3.5 damage spell is 179217.
class spell_azsuna_rescue_meteor : public SpellScript
{
    PrepareSpellScript(spell_azsuna_rescue_meteor);
    void Launch(SpellEffIndex)
    {
        Creature* caster = GetCaster()->ToCreature();
        Player* player = AzsunaRescue::Owner(caster);
        if (!caster || caster != AzsunaRescue::Ride(player) || !AzsunaRescue::Active(player) || !GetExplTargetDest()) return;
        auto const* pos = GetExplTargetDest();
        caster->CastSpell(pos->GetPositionX(), pos->GetPositionY(), pos->GetPositionZ(), 179217, true);
    }
    void Register() override { OnEffectHit += SpellEffectFn(spell_azsuna_rescue_meteor::Launch, EFFECT_0, SPELL_EFFECT_DUMMY); }
};

class player_azsuna_rescue : public PlayerScript
{
public:
    player_azsuna_rescue() : PlayerScript("player_azsuna_rescue") { }
    void OnLogout(Player* player) override { AzsunaRescue::Clear(player); }
    void OnMapChanged(Player* player) override { if (player->GetMapId() != 1220) AzsunaRescue::Clear(player); }
    void OnQuestReward(Player* player, Quest const* quest) override
    {
        if (quest->GetQuestId() != AzsunaRescue::QuestId) return;
        if (AzsunaRescue::Ride(player)) player->ExitVehicle(&AzsunaRescue::Prisoner);
        AzsunaRescue::Clear(player);
    }
};

void AddSC_azsuna_rescue()
{
    RegisterCreatureAI(npc_azsuna_rescue_farondis);
    new npc_azsuna_rescue_gossip();
    RegisterCreatureAI(npc_azsuna_rescue_enemy);
    RegisterCreatureAI(npc_azsuna_rescue_azshara);
    new scene_azsuna_rescue();
    RegisterSpellScript(spell_azsuna_rescue_meteor);
    new player_azsuna_rescue();
}
