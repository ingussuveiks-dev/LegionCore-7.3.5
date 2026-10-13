#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "Player.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "ObjectAccessor.h"
#include <set>

void StartHighmountainUnderkingEscape(Creature* boss);

namespace Underking
{
constexpr uint32 Quest = 39780, Wrath = 97266, Meeting = 97352;
// Native quest POI; the existing quest-giver spawn is 1.5 m away.
Position const CaveMayla = {3662.03f,3928.39f,906.681f,0.0f};
bool Eligible(Player* player)
{
    return player && player->IsInWorld() && player->IsAlive() && player->GetMapId() == 1220 &&
        player->GetQuestStatus(Quest) == QUEST_STATUS_INCOMPLETE &&
        !player->IsBeingTeleported() && !player->GetVehicle() && !player->GetTransport() &&
        !player->isInFlight() && !player->IsFlying() && player->GetExactDist(&CaveMayla) < 15.0f;
}
bool IsWarbrave(Unit* unit)
{
    return unit && unit->GetEntry() == 97498 && unit->GetMapId() == 1220 && unit->HasPhaseId(9180);
}
}

class player_highmountain_underking : public PlayerScript
{
public:
    player_highmountain_underking() : PlayerScript("player_highmountain_underking") { }
    void OnUpdate(Player* player, uint32) override
    {
        if (!Underking::Eligible(player) || player->GetQuestObjectiveData(Underking::Quest,Underking::Meeting)) return;
        Creature* mayla = player->FindNearestCreature(93846,15.0f);
        if (!mayla || !mayla->IsAlive() || mayla->GetExactDist(&Underking::CaveMayla) > 3.0f ||
            !player->InSamePhase(mayla) || !player->canSeeOrDetect(mayla) || !player->IsWithinLOSInMap(mayla)) return;
        player->KilledMonsterCredit(Underking::Meeting);
    }
};

struct npc_highmountain_underking : ScriptedAI
{
    npc_highmountain_underking(Creature* creature) : ScriptedAI(creature) { }
    EventMap attacks;
    std::set<ObjectGuid> affected;
    bool escaped = false;
    void Cleanup()
    {
        for (ObjectGuid guid : affected)
            if (Unit* unit = ObjectAccessor::GetUnit(*me,guid))
                for (uint32 spell : {192270u,192322u,216101u}) unit->RemoveAurasDueToSpell(spell,me->GetGUID());
        affected.clear();
    }
    void Reset() override { attacks.Reset(); Cleanup(); escaped = false; }
    void EnterCombat(Unit*) override
    {
        attacks.Reset();
        attacks.ScheduleEvent(192269,4000);
        attacks.ScheduleEvent(213577,15000);
    }
    void SpellHitTarget(Unit* target, SpellInfo const* spell) override
    {
        if (spell->Id == 192270) affected.insert(target->GetGUID());
    }
    void JustDied(Unit*) override
    {
        attacks.Reset(); Cleanup();
        if (!escaped)
        {
            escaped = true;
            StartHighmountainUnderkingEscape(me);
        }
        // Ordinary kill rewards own the 97266 credit. Never grant it on spawn,
        // a spell hit, an ally death or a reset.
    }
    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim()) return;
        attacks.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;
        if (uint32 spell = attacks.ExecuteEvent())
        {
            if (spell == 192269) me->SetFacingToObject(me->getVictim());
            DoCast(me,spell);
            attacks.ScheduleEvent(spell,spell == 192269 ? 8000 : 20000);
        }
        DoMeleeAttackIfReady();
    }
};

class spell_highmountain_underking_breath : public SpellScript
{
    PrepareSpellScript(spell_highmountain_underking_breath);
    void SuppressTrigger(SpellEffIndex index)
    {
        if (GetCaster()->GetEntry() == Underking::Wrath) PreventHitDefaultEffect(index);
    }
    void After()
    {
        Unit* target = GetHitUnit();
        Unit* caster = GetCaster();
        if (caster->GetEntry() != Underking::Wrath || !target || !target->IsAlive()) return;
        Aura* stacks = target->GetAura(192270,caster->GetGUID());
        if (!stacks || stacks->GetStackAmount() < 10) return;
        uint32 petrify = Underking::IsWarbrave(target) ? 216101 : 192322;
        if (!target->HasAura(petrify,caster->GetGUID()))
            target->CastSpell(target,petrify,true,nullptr,nullptr,caster->GetGUID());
        // Both native petrify spells remove 192270 from their self target.
    }
    void Register() override
    {
        // EffectTriggerSpell runs at LAUNCH_TARGET, before the slow is applied.
        // Decide after the actual hit so stack ten petrifies on this tick.
        OnEffectLaunchTarget += SpellEffectFn(spell_highmountain_underking_breath::SuppressTrigger,EFFECT_1,SPELL_EFFECT_TRIGGER_SPELL_4);
        OnEffectLaunchTarget += SpellEffectFn(spell_highmountain_underking_breath::SuppressTrigger,EFFECT_2,SPELL_EFFECT_TRIGGER_SPELL_4);
        AfterHit += SpellHitFn(spell_highmountain_underking_breath::After);
    }
};

class spell_highmountain_underking_smash : public SpellScript
{
    PrepareSpellScript(spell_highmountain_underking_smash);
    void After()
    {
        Unit* target = GetHitUnit();
        Unit* caster = GetCaster();
        if (caster->GetEntry() == Underking::Wrath && Underking::IsWarbrave(target) && target->IsAlive() &&
            target->HasAura(216101,caster->GetGUID())) caster->Kill(target);
    }
    void Register() override { AfterHit += SpellHitFn(spell_highmountain_underking_smash::After); }
};

void AddSC_highmountain_underking()
{
    new player_highmountain_underking();
    RegisterCreatureAI(npc_highmountain_underking);
    RegisterSpellScript(spell_highmountain_underking_breath);
    RegisterSpellScript(spell_highmountain_underking_smash);
}
