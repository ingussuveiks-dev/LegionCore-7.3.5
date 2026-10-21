#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ObjectAccessor.h"

namespace DarkheartTrash
{
enum Spells : uint32
{
    VileMushroom = 198799,
    MushroomModel = 198937,
    MushroomArea = 198915,
    VileBurst = 198916,
    CorruptedInfusion = 225497,
    InfusionHeal = 225503,
    RootBurst = 201123,
    RootBurstSummon = 201129,
    RootBurstVisual = 201145
};

bool IsMythic(Map const* map)
{
    return map->GetDifficultyID() == DIFFICULTY_MYTHIC_DUNGEON ||
        map->GetDifficultyID() == DIFFICULTY_MYTHIC_KEYSTONE;
}
}

// 99359. The native area dummy chooses one target (three on Mythic), then
// 220369 -> 198910 creates each mushroom. Never replace that with player casts.
struct npc_darkheart_rotheart_keeper : ScriptedAI
{
    explicit npc_darkheart_rotheart_keeper(Creature* creature) : ScriptedAI(creature), summons(me) { }
    SummonList summons;
    EventMap events;

    void Reset() override
    {
        events.Reset();
        summons.DespawnAll();
        me->RemoveAllAreaObjects();
        me->RemoveAurasDueToSpell(DarkheartTrash::CorruptedInfusion);
        if (DarkheartTrash::IsMythic(me->GetMap()))
            DoCast(me, DarkheartTrash::CorruptedInfusion, true);
    }

    void EnterCombat(Unit*) override { events.RescheduleEvent(1, 6000); }
    void JustSummoned(Creature* summon) override { summons.Summon(summon); }
    void JustDied(Unit*) override
    {
        summons.DespawnAll();
        me->RemoveAllAreaObjects();
        me->RemoveAurasDueToSpell(DarkheartTrash::CorruptedInfusion);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;
        if (events.ExecuteEvent())
        {
            DoCast(me, DarkheartTrash::VileMushroom);
            events.RescheduleEvent(1, 15000);
        }
        DoMeleeAttackIfReady();
    }
};

// 100960. Keep the native model aura and 6-yard trigger; the invisible stalker
// alone has neither a visible mushroom nor an explosion.
struct npc_darkheart_vile_mushroom : ScriptedAI
{
    explicit npc_darkheart_vile_mushroom(Creature* creature) : ScriptedAI(creature) { }
    uint32 armTimer = 3000;
    uint32 lifeTimer = 30000;
    bool exploded = false;

    void Reset() override
    {
        armTimer = 3000;
        lifeTimer = 30000;
        exploded = false;
        me->SetReactState(REACT_PASSIVE);
        DoCast(me, DarkheartTrash::MushroomModel, true);
        DoCast(me, DarkheartTrash::MushroomArea, true);
    }

    void Explode()
    {
        if (exploded)
            return;
        exploded = true;
        me->RemoveAllAreaObjects();
        me->RemoveAurasDueToSpell(DarkheartTrash::MushroomModel);
        DoCast(me, DarkheartTrash::VileBurst, true);
        me->DespawnOrUnsummon(100);
    }

    void OnAreaTriggerDespawn(uint32 spellId, Position, bool duration) override
    {
        if (duration && spellId == DarkheartTrash::MushroomArea)
            Explode();
    }

    void UpdateAI(uint32 diff) override
    {
        if (exploded)
            return;
        bool expired = lifeTimer <= diff;
        if (!expired)
            lifeTimer -= diff;
        if (armTimer > diff)
        {
            armTimer -= diff;
            return;
        }
        armTimer = 0;
        std::list<Player*> players;
        GetPlayerListInGrid(players, me, 6.0f);
        bool disturbed = std::any_of(players.begin(), players.end(), [this](Player* player)
        {
            return player->IsAlive() && !player->isGameMaster() && me->IsValidAttackTarget(player);
        });
        if (!disturbed && !expired)
            return;
        // Set this before casting: triggered effects may re-enter AI callbacks.
        Explode();
    }
};

// 101958. Only a warning; damage belongs to the flower's completed cast.
struct npc_darkheart_root_burst : ScriptedAI
{
    explicit npc_darkheart_root_burst(Creature* creature) : ScriptedAI(creature) { }
    void Reset() override
    {
        me->SetReactState(REACT_PASSIVE);
        DoCast(me, DarkheartTrash::RootBurstVisual, true);
    }
};

// 99360. Anchor the native ANY-unit Root Burst on its warning stalker so that
// leaving the warning really avoids the damage, including with a pet tank.
struct npc_darkheart_vilethorn_blossom : ScriptedAI
{
    explicit npc_darkheart_vilethorn_blossom(Creature* creature) : ScriptedAI(creature), summons(me)
    {
        SetCombatMovement(false);
    }
    EventMap events;
    SummonList summons;
    ObjectGuid rootTarget;
    uint32 infusionTimer = 0;

    void Reset() override
    {
        events.Reset();
        summons.DespawnAll();
        rootTarget.Clear();
        infusionTimer = 0;
        me->RemoveAurasDueToSpell(DarkheartTrash::InfusionHeal);
    }
    void EnterCombat(Unit*) override { events.RescheduleEvent(1, urand(21000, 22000)); }
    void JustDied(Unit*) override { summons.DespawnAll(); }
    void JustSummoned(Creature* summon) override
    {
        summons.Summon(summon);
        if (summon->GetEntry() == 101958)
            rootTarget = summon->GetGUID();
    }

    void UpdateInfusion(uint32 diff)
    {
        if (infusionTimer > diff)
        {
            infusionTimer -= diff;
            return;
        }
        infusionTimer = 500;
        bool infused = false;
        if (DarkheartTrash::IsMythic(me->GetMap()))
        {
            std::list<Creature*> keepers;
            me->GetCreatureListWithEntryInGrid(keepers, 99359, 25.0f);
            infused = std::any_of(keepers.begin(), keepers.end(), [](Creature* keeper)
            {
                return keeper->IsAlive() && keeper->HasAura(DarkheartTrash::CorruptedInfusion);
            });
        }
        if (infused && !me->HasAura(DarkheartTrash::InfusionHeal))
            DoCast(me, DarkheartTrash::InfusionHeal, true);
        else if (!infused)
            me->RemoveAurasDueToSpell(DarkheartTrash::InfusionHeal);
    }

    void UpdateAI(uint32 diff) override
    {
        UpdateInfusion(diff); // Also remove healing when the last nearby keeper dies OOC.
        if (!UpdateVictim())
            return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;
        if (events.ExecuteEvent())
        {
            if (Unit* player = SelectTarget(SELECT_TARGET_RANDOM, 0, 60.0f, true))
            {
                rootTarget.Clear();
                me->CastSpell(player, DarkheartTrash::RootBurstSummon, true);
                if (Creature* warning = ObjectAccessor::GetCreature(*me, rootTarget))
                    DoCast(warning, DarkheartTrash::RootBurst);
            }
            events.RescheduleEvent(1, urand(21000, 22000));
        }
        DoMeleeAttackIfReady();
    }
};

// 204243. Interrupting the channel must not apply its completion fear.
class spell_darkheart_tormenting_eye : public AuraScript
{
    PrepareAuraScript(spell_darkheart_tormenting_eye);
    void OnRemove(AuraEffect const*, AuraEffectHandleModes)
    {
        if (GetTargetApplication()->GetRemoveMode() == AURA_REMOVE_BY_EXPIRE)
            if (Unit* caster = GetCaster())
                caster->CastSpell(GetTarget(), 204246, true);
    }
    void Register() override
    {
        AfterEffectRemove += AuraEffectRemoveFn(spell_darkheart_tormenting_eye::OnRemove, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE, AURA_EFFECT_HANDLE_REAL);
    }
};

// 204970 references absent 204972 in the 7.3.5 catalog. Resume at its surviving
// native missile 204973 -> impact 204974 -> area 6078. Keep native tick timing.
class spell_darkheart_noxious_spit : public AuraScript
{
    PrepareAuraScript(spell_darkheart_noxious_spit);
    void Tick(AuraEffect const*)
    {
        PreventDefaultAction();
        if (Unit* caster = GetCaster())
            if (caster->IsCreature())
                if (Unit* player = caster->ToCreature()->AI()->SelectTarget(SELECT_TARGET_RANDOM, 0, 40.0f, true))
                    caster->CastSpell(player, 204973, true);
    }
    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_darkheart_noxious_spit::Tick, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
    }
};

// 212464 is the Mythic Nightfall missile. Its native impact only creates the
// pool; the missing additional summon must still be made by the boss.
class spell_darkheart_mythic_nightfall : public SpellScript
{
    PrepareSpellScript(spell_darkheart_mythic_nightfall);
    void Launch()
    {
        Unit* caster = GetCaster();
        if (caster && caster->GetEntry() == 96512 && DarkheartTrash::IsMythic(caster->GetMap()))
            if (Unit* target = GetExplTargetUnit())
                caster->CastSpell(target, 198432, true);
    }
    void Register() override { OnCast += SpellCastFn(spell_darkheart_mythic_nightfall::Launch); }
};

// 200289. Touching the growing area removes it via 200329 and fears the target.
// Only surviving the full duration transitions to the separate isolation aura.
class spell_darkheart_growing_paranoia : public AuraScript
{
    PrepareAuraScript(spell_darkheart_growing_paranoia);
    void OnRemove(AuraEffect const*, AuraEffectHandleModes)
    {
        if (GetTargetApplication()->GetRemoveMode() == AURA_REMOVE_BY_EXPIRE)
            if (Unit* target = GetTarget())
                target->CastSpell(target, 200243, true);
    }
    void Register() override
    {
        AfterEffectRemove += AuraEffectRemoveFn(spell_darkheart_growing_paranoia::OnRemove, EFFECT_0, SPELL_AURA_CREATE_AREATRIGGER, AURA_EFFECT_HANDLE_REAL);
    }
};

void AddSC_darkheart_thicket_trash()
{
    RegisterCreatureAI(npc_darkheart_rotheart_keeper);
    RegisterCreatureAI(npc_darkheart_vile_mushroom);
    RegisterCreatureAI(npc_darkheart_root_burst);
    RegisterCreatureAI(npc_darkheart_vilethorn_blossom);
    RegisterAuraScript(spell_darkheart_tormenting_eye);
    RegisterAuraScript(spell_darkheart_noxious_spit);
    RegisterSpellScript(spell_darkheart_mythic_nightfall);
    RegisterAuraScript(spell_darkheart_growing_paranoia);
}
