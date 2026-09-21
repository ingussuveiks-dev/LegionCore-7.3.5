/*
 * Isle of Thunder scenario ambient actors.
 *
 * These actors are present in the 5.4.8 map 1126 population but their
 * original AI lived in zone_isle_of_thunder.cpp rather than in the scenario
 * directory. Keep the required subset local to the restored scenario map.
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "Player.h"
#include "Random.h"

namespace
{
enum IotTargetPriority
{
    IOT_PRIORITY_VICTIM,
    IOT_PRIORITY_NOT_VICTIM,
    IOT_PRIORITY_SELF,
    IOT_PRIORITY_ALLY,
};

enum IotSpells
{
    IOT_SPELL_STATUE_FROZEN_SHEATH     = 123825,
    IOT_SPELL_CURSE_OF_STONE           = 140207,
    IOT_SPELL_BODY_SLAM_25Y            = 140202,
    IOT_SPELL_BODY_SLAM_5Y             = 140199,
    IOT_SPELL_GASEOUS_ERUPTION         = 140325,
    IOT_SPELL_VALOROUS_SPIN            = 140349,
    IOT_SPELL_THUNDERCLEAVE             = 139035,
    IOT_SPELL_SERRATED_SLASH            = 87395,
    IOT_SPELL_SHIELD_BASH               = 140659,
    IOT_SPELL_PIERCING_CRY              = 144039,
    IOT_SPELL_PIERCING_CRY_2            = 140408,
    IOT_SPELL_SHADOW_NOVA               = 75073,
    IOT_SPELL_SHADOW_BOLT               = 91997,
    IOT_SPELL_KICK                      = 137708,
    IOT_SPELL_FOCUSED_STRIKE            = 82615,
    IOT_SPELL_SKULL_KICK                = 137707,
    IOT_SPELL_ZANDALARI_POTION          = 136772,
    IOT_SPELL_SUNDER_ARMOR              = 76622,
    IOT_SPELL_SLAM                      = 79881,
    IOT_SPELL_THUNDER_CLAP              = 81140,
    IOT_SPELL_RUNE_OF_THE_STORM         = 136737,
    IOT_SPELL_LIGHTNING_BOLT            = 9532,
    IOT_SPELL_STORM_SHIELD              = 136748,
    IOT_SPELL_BITE                      = 140374,
    IOT_SPELL_SHELLWIND                 = 140356,
    IOT_SPELL_TOSS_ENERGIZED_METAL      = 137547,
    IOT_SPELL_CHARGED_BOLT_PERIODIC     = 137543,
    IOT_SPELL_POWER_SURGE_ACHIEVEMENT   = 136842,
    IOT_CRITERIA_ASSET_SPEED_METAL      = 139370,
};

enum IotEvents
{
    IOT_EVENT_PRIMARY = 1,
    IOT_EVENT_SECONDARY,
    IOT_EVENT_TERTIARY,
    IOT_EVENT_QUATERNARY,
};

enum IotCreatures
{
    IOT_NPC_JUVENILE_SKYSCREAMER    = 69162,
    IOT_NPC_SHANZE_SOULRIPPER       = 69236,
    IOT_NPC_SHANZE_GRAVEKEEPER      = 69235,
    IOT_NPC_SHANZE_THUNDERCALLER    = 69264,
    IOT_NPC_ANCIENT_STONE_CONQUEROR = 69238,
};

struct IotAmbientAI : public ScriptedAI
{
    explicit IotAmbientAI(Creature* creature) : ScriptedAI(creature), summons(me) { }

    virtual ObjectGuid GetLowestFriendGUID() { return ObjectGuid::Empty; }

    void ExecuteTargetEvent(uint32 spellId, uint32 repeat, uint32 eventId, uint32 currentEventId,
        IotTargetPriority priority = IOT_PRIORITY_VICTIM)
    {
        if (eventId != currentEventId)
            return;

        Unit* target = nullptr;
        switch (priority)
        {
            case IOT_PRIORITY_SELF:
                target = me;
                break;
            case IOT_PRIORITY_ALLY:
                target = ObjectAccessor::GetUnit(*me, GetLowestFriendGUID());
                break;
            case IOT_PRIORITY_NOT_VICTIM:
                target = SelectTarget(SELECT_TARGET_RANDOM, 0, NonTankTargetSelector(me));
                if (!target)
                    target = SelectTarget(SELECT_TARGET_RANDOM, 0, 150.0f, true);
                break;
            case IOT_PRIORITY_VICTIM:
                target = me->getVictim();
                break;
        }

        if (target)
            DoCast(target, spellId);

        me->_AddCreatureSpellCooldown(spellId, time(nullptr) + 2);
        if (repeat)
            events.ScheduleEvent(eventId, repeat);
    }

protected:
    EventMap events;
    SummonList summons;
};

struct npc_animated_warrior : public IotAmbientAI
{
    explicit npc_animated_warrior(Creature* creature) : IotAmbientAI(creature) { ResetGuard(); }

    void Reset() override
    {
        events.Reset();
        ResetGuard();
        if (roll_chance_i(20))
            ActivateGuard(nullptr);
    }

    void ActivateGuard(Unit* invoker)
    {
        me->RemoveAurasDueToSpell(IOT_SPELL_STATUE_FROZEN_SHEATH);
        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_SELECTABLE);
        me->GetMotionMaster()->MoveRandom(10.0f);
        if (invoker)
            AttackStart(invoker);
    }

    void ResetGuard()
    {
        DoCast(me, IOT_SPELL_STATUE_FROZEN_SHEATH, true);
        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_SELECTABLE);
        me->GetMotionMaster()->MoveIdle();
    }

    void MoveInLineOfSight(Unit* who) override
    {
        if (!me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC) || who->GetTypeId() != TYPEID_PLAYER ||
            !me->IsWithinDistInMap(who, 5.0f, false))
            return;

        if (roll_chance_i(20))
            ActivateGuard(who);
    }

    void JustReachedHome() override
    {
        if (!me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC))
            me->GetMotionMaster()->MoveRandom(10.0f);
    }

    void EnterCombat(Unit* /*who*/) override
    {
        events.ScheduleEvent(IOT_EVENT_PRIMARY, 2500);
        events.ScheduleEvent(IOT_EVENT_SECONDARY, 8000);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;
        while (uint32 eventId = events.ExecuteEvent())
        {
            ExecuteTargetEvent(IOT_SPELL_CURSE_OF_STONE, urand(10500, 12000), IOT_EVENT_PRIMARY, eventId);
            ExecuteTargetEvent(urand(0, 1) ? IOT_SPELL_BODY_SLAM_25Y : IOT_SPELL_BODY_SLAM_5Y, 6000, IOT_EVENT_SECONDARY, eventId);
            break;
        }
        DoMeleeAttackIfReady();
    }
};

struct npc_creeping_moor_beast : public IotAmbientAI
{
    explicit npc_creeping_moor_beast(Creature* creature) : IotAmbientAI(creature) { }
    void Reset() override { events.Reset(); }
    void EnterCombat(Unit*) override { events.ScheduleEvent(IOT_EVENT_PRIMARY, 5000); }
    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim()) return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;
        while (uint32 eventId = events.ExecuteEvent())
        {
            ExecuteTargetEvent(IOT_SPELL_GASEOUS_ERUPTION, urand(15000, 30000), IOT_EVENT_PRIMARY, eventId);
            break;
        }
        DoMeleeAttackIfReady();
    }
};

struct npc_slateshell_wanderer : public IotAmbientAI
{
    explicit npc_slateshell_wanderer(Creature* creature) : IotAmbientAI(creature) { }
    void Reset() override { events.Reset(); }
    void EnterCombat(Unit*) override { events.ScheduleEvent(IOT_EVENT_PRIMARY, 3000); }
    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim()) return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;
        while (uint32 eventId = events.ExecuteEvent())
        {
            ExecuteTargetEvent(IOT_SPELL_VALOROUS_SPIN, urand(11000, 17500), IOT_EVENT_PRIMARY, eventId);
            break;
        }
        DoMeleeAttackIfReady();
    }
};

struct npc_zandalari_stoneshield : public IotAmbientAI
{
    explicit npc_zandalari_stoneshield(Creature* creature) : IotAmbientAI(creature) { }
    void Reset() override { events.Reset(); }
    void EnterCombat(Unit*) override
    {
        events.ScheduleEvent(IOT_EVENT_PRIMARY, 2500);
        events.ScheduleEvent(IOT_EVENT_SECONDARY, 4500);
        events.ScheduleEvent(IOT_EVENT_TERTIARY, 7000);
    }
    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim()) return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;
        while (uint32 eventId = events.ExecuteEvent())
        {
            ExecuteTargetEvent(IOT_SPELL_THUNDERCLEAVE, urand(6000, 12000), IOT_EVENT_PRIMARY, eventId);
            ExecuteTargetEvent(IOT_SPELL_SERRATED_SLASH, 7500, IOT_EVENT_SECONDARY, eventId);
            ExecuteTargetEvent(IOT_SPELL_SHIELD_BASH, 10000, IOT_EVENT_TERTIARY, eventId);
            break;
        }
        DoMeleeAttackIfReady();
    }
};

struct npc_juvenile_skyscreamer : public IotAmbientAI
{
    explicit npc_juvenile_skyscreamer(Creature* creature) : IotAmbientAI(creature) { }
    void Reset() override { events.Reset(); }
    void EnterCombat(Unit*) override { events.ScheduleEvent(IOT_EVENT_PRIMARY, 3000); }
    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim()) return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;
        while (uint32 eventId = events.ExecuteEvent())
        {
            uint32 spellId = me->GetEntry() == IOT_NPC_JUVENILE_SKYSCREAMER ? IOT_SPELL_PIERCING_CRY : IOT_SPELL_PIERCING_CRY_2;
            ExecuteTargetEvent(spellId, 9500, IOT_EVENT_PRIMARY, eventId, IOT_PRIORITY_SELF);
            break;
        }
        DoMeleeAttackIfReady();
    }
};

struct npc_zandalari_spiritbinder : public IotAmbientAI
{
    explicit npc_zandalari_spiritbinder(Creature* creature) : IotAmbientAI(creature) { }
    void Reset() override { events.Reset(); }
    void EnterCombat(Unit*) override
    {
        events.ScheduleEvent(IOT_EVENT_PRIMARY, 1500);
        events.ScheduleEvent(IOT_EVENT_SECONDARY, 8000);
    }
    void AttackStart(Unit* target) override
    {
        if (target && me->Attack(target, false))
            DoStartNoMovement(target);
    }
    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim()) return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;
        while (uint32 eventId = events.ExecuteEvent())
        {
            ExecuteTargetEvent(IOT_SPELL_SHADOW_BOLT, 3500, IOT_EVENT_PRIMARY, eventId);
            ExecuteTargetEvent(IOT_SPELL_SHADOW_NOVA, 11500, IOT_EVENT_SECONDARY, eventId, IOT_PRIORITY_SELF);
            break;
        }
        DoMeleeAttackIfReady();
    }
};

struct npc_zandalari_jaguar_warrior : public IotAmbientAI
{
    explicit npc_zandalari_jaguar_warrior(Creature* creature) : IotAmbientAI(creature) { }
    void Reset() override { events.Reset(); }
    void EnterCombat(Unit*) override
    {
        events.ScheduleEvent(IOT_EVENT_PRIMARY, 3000);
        events.ScheduleEvent(IOT_EVENT_SECONDARY, 6500);
    }
    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim()) return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;
        while (uint32 eventId = events.ExecuteEvent())
        {
            uint32 attacks[] = { IOT_SPELL_KICK, IOT_SPELL_FOCUSED_STRIKE, IOT_SPELL_SKULL_KICK };
            ExecuteTargetEvent(attacks[urand(0, 2)], 4500, IOT_EVENT_PRIMARY, eventId);
            ExecuteTargetEvent(IOT_SPELL_ZANDALARI_POTION, 15000, IOT_EVENT_SECONDARY, eventId, IOT_PRIORITY_SELF);
            break;
        }
        DoMeleeAttackIfReady();
    }
};

struct npc_shanze_gravekeeper : public IotAmbientAI
{
    explicit npc_shanze_gravekeeper(Creature* creature) : IotAmbientAI(creature) { }
    void Reset() override { events.Reset(); }
    void EnterCombat(Unit*) override
    {
        events.ScheduleEvent(IOT_EVENT_PRIMARY, 6000);
        events.ScheduleEvent(IOT_EVENT_SECONDARY, 2500);
        events.ScheduleEvent(IOT_EVENT_TERTIARY, 8000);
        events.ScheduleEvent(IOT_EVENT_QUATERNARY, urand(12000, 14000));
    }
    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim()) return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;
        while (uint32 eventId = events.ExecuteEvent())
        {
            ExecuteTargetEvent(IOT_SPELL_SUNDER_ARMOR, urand(12000, 16000), IOT_EVENT_PRIMARY, eventId);
            ExecuteTargetEvent(IOT_SPELL_SLAM, 9500, IOT_EVENT_SECONDARY, eventId);
            ExecuteTargetEvent(IOT_SPELL_THUNDER_CLAP, 10000, IOT_EVENT_TERTIARY, eventId, IOT_PRIORITY_SELF);
            ExecuteTargetEvent(IOT_SPELL_RUNE_OF_THE_STORM, 25000, IOT_EVENT_QUATERNARY, eventId, IOT_PRIORITY_SELF);
            break;
        }
        DoMeleeAttackIfReady();
    }
};

struct npc_shanze_soulripper : public IotAmbientAI
{
    explicit npc_shanze_soulripper(Creature* creature) : IotAmbientAI(creature) { }
    void Reset() override { events.Reset(); }
    void EnterCombat(Unit*) override
    {
        events.ScheduleEvent(IOT_EVENT_PRIMARY, 6000);
        events.ScheduleEvent(IOT_EVENT_SECONDARY, 8500);
    }
    ObjectGuid GetLowestFriendGUID() override
    {
        std::list<Creature*> targets;
        GetCreatureListWithEntryInGrid(targets, me, IOT_NPC_SHANZE_GRAVEKEEPER, 80.0f);
        GetCreatureListWithEntryInGrid(targets, me, IOT_NPC_SHANZE_SOULRIPPER, 80.0f);
        GetCreatureListWithEntryInGrid(targets, me, IOT_NPC_SHANZE_THUNDERCALLER, 80.0f);
        GetCreatureListWithEntryInGrid(targets, me, IOT_NPC_ANCIENT_STONE_CONQUEROR, 80.0f);
        if (targets.empty()) return ObjectGuid::Empty;
        targets.sort(Trinity::HealthPctOrderPred());
        return targets.front()->GetGUID();
    }
    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim()) return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;
        while (uint32 eventId = events.ExecuteEvent())
        {
            ExecuteTargetEvent(IOT_SPELL_LIGHTNING_BOLT, urand(8000, 16000), IOT_EVENT_PRIMARY, eventId);
            ExecuteTargetEvent(IOT_SPELL_STORM_SHIELD, 12500, IOT_EVENT_SECONDARY, eventId, IOT_PRIORITY_ALLY);
            break;
        }
        DoMeleeAttackIfReady();
    }
};

struct npc_slateshell_ancient : public IotAmbientAI
{
    explicit npc_slateshell_ancient(Creature* creature) : IotAmbientAI(creature) { }
    void Reset() override { events.Reset(); }
    void EnterCombat(Unit*) override
    {
        events.ScheduleEvent(IOT_EVENT_PRIMARY, 3000);
        events.ScheduleEvent(IOT_EVENT_SECONDARY, 8500);
    }
    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim()) return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;
        while (uint32 eventId = events.ExecuteEvent())
        {
            ExecuteTargetEvent(IOT_SPELL_BITE, urand(6500, 11000), IOT_EVENT_PRIMARY, eventId);
            ExecuteTargetEvent(IOT_SPELL_SHELLWIND, urand(12500, 17500), IOT_EVENT_SECONDARY, eventId, IOT_PRIORITY_SELF);
            break;
        }
        DoMeleeAttackIfReady();
    }
};

struct npc_master_caller : public IotAmbientAI
{
    explicit npc_master_caller(Creature* creature) : IotAmbientAI(creature) { }
    TaskScheduler scheduler;

    Creature* FindDinosaur()
    {
        if (Creature* dinosaur = me->FindNearestCreature(69277, 50.0f))
            return dinosaur;
        return me->FindNearestCreature(67477, 50.0f);
    }

    void Reset() override
    {
        events.Reset();
        scheduler.CancelAll();
        scheduler.Schedule(Seconds(1), [this](TaskContext)
        {
            if (Creature* dinosaur = FindDinosaur())
                DoCast(dinosaur, 136613);
        });
    }
    void JustReachedHome() override
    {
        if (Creature* dinosaur = FindDinosaur())
            DoCast(dinosaur, 136613);
    }
    void EnterCombat(Unit*) override
    {
        me->InterruptNonMeleeSpells(true);
        events.ScheduleEvent(IOT_EVENT_PRIMARY, 5000);
        events.ScheduleEvent(IOT_EVENT_SECONDARY, 15000);
    }
    void JustDied(Unit*) override
    {
        if (Creature* dinosaur = FindDinosaur())
            if (!dinosaur->HasAura(136613))
            {
                std::list<Player*> players;
                GetPlayerListInGrid(players, me, 40.0f);
                for (Player* player : players)
                    player->KilledMonsterCredit(69276);
            }
    }
    void UpdateAI(uint32 diff) override
    {
        scheduler.Update(diff);
        if (!UpdateVictim()) return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;
        while (uint32 eventId = events.ExecuteEvent())
        {
            ExecuteTargetEvent(140411, 5000, IOT_EVENT_PRIMARY, eventId);
            ExecuteTargetEvent(140409, 15000, IOT_EVENT_SECONDARY, eventId);
            break;
        }
        DoMeleeAttackIfReady();
    }
};

struct npc_metal_lord_meno_han : public IotAmbientAI
{
    explicit npc_metal_lord_meno_han(Creature* creature) : IotAmbientAI(creature) { }
    void Reset() override { events.Reset(); }
    void EnterCombat(Unit*) override { events.ScheduleEvent(IOT_EVENT_PRIMARY, 6500); }
    void EnterEvadeMode() override
    {
        summons.DespawnAll();
        ScriptedAI::EnterEvadeMode();
    }
    void JustSummoned(Creature* summon) override
    {
        summons.Summon(summon);
        summon->CastSpell(summon, IOT_SPELL_CHARGED_BOLT_PERIODIC, true);
    }
    void JustDied(Unit*) override
    {
        summons.DespawnAll();
        std::list<Player*> players;
        me->GetPlayerListInGrid(players, 50.0f);
        for (Player* player : players)
            if (Aura* surge = player->GetAura(IOT_SPELL_POWER_SURGE_ACHIEVEMENT))
                if (surge->GetStackAmount() > 9)
                    player->UpdateAchievementCriteria(CRITERIA_TYPE_BE_SPELL_TARGET, IOT_CRITERIA_ASSET_SPEED_METAL);
    }
    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim()) return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;
        while (uint32 eventId = events.ExecuteEvent())
        {
            ExecuteTargetEvent(IOT_SPELL_TOSS_ENERGIZED_METAL, 12000, IOT_EVENT_PRIMARY, eventId, IOT_PRIORITY_SELF);
            break;
        }
        DoMeleeAttackIfReady();
    }
};
}

void AddSC_isle_of_thunder_ambient()
{
    RegisterCreatureAI(npc_animated_warrior);
    RegisterCreatureAI(npc_creeping_moor_beast);
    RegisterCreatureAI(npc_slateshell_wanderer);
    RegisterCreatureAI(npc_zandalari_stoneshield);
    RegisterCreatureAI(npc_juvenile_skyscreamer);
    RegisterCreatureAI(npc_zandalari_spiritbinder);
    RegisterCreatureAI(npc_zandalari_jaguar_warrior);
    RegisterCreatureAI(npc_shanze_gravekeeper);
    RegisterCreatureAI(npc_shanze_soulripper);
    RegisterCreatureAI(npc_slateshell_ancient);
    RegisterCreatureAI(npc_master_caller);
    RegisterCreatureAI(npc_metal_lord_meno_han);
}
