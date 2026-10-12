#include "ScriptMgr.h"
#include "CombatAI.h"
#include "SpellInfo.h"
#include "SpellMgr.h"

// Native TARGET_UNIT_TARGET_ANY on Bash / Stone Disc is classified as SELF
// by FillAISpellInfo. Only these three quest actors need explicit victims;
// keep their other AggressorAI spells, dialogue, melee and assistance behavior.
struct npc_highmountain_mightstone : AggressorAI
{
    npc_highmountain_mightstone(Creature* creature) : AggressorAI(creature) { }
    EventMap attacks;
    void Reset() override
    {
        attacks.Reset();
        AggressorAI::Reset();
    }
    void EnterCombat(Unit* who) override
    {
        AggressorAI::EnterCombat(who);
        attacks.Reset();
        for (uint32 spell : {194019u,196402u,197483u})
            if (me->GetCreatureSpell(spell))
            {
                spellCasts.CancelEvent(spell);
                attacks.ScheduleEvent(spell,AISpellInfo[spell].cooldown);
            }
    }
    void UpdateAI(uint32 diff) override
    {
        AggressorAI::UpdateAI(diff);
        if (!me->IsInWorld() || !me->IsAlive() || !me->isInCombat() || !me->getVictim()) return;
        attacks.Update(diff);
        if (IsInControl() || me->HasUnitState(UNIT_STATE_CASTING) || me->getVictim()->HasCrowdControlAura(me)) return;
        if (uint32 spell = attacks.ExecuteEvent())
        {
            SpellInfo const* info = sSpellMgr->GetSpellInfo(spell);
            if (info && info->CanAutoCast(me,me->getVictim()))
            {
                me->CastSpell(me->getVictim(),spell,false);
                uint32 cooldown = AISpellInfo[spell].cooldown;
                attacks.ScheduleEvent(spell,cooldown + urand(0,cooldown));
            }
            else attacks.ScheduleEvent(spell,500);
        }
    }
};

void AddSC_highmountain_snowblind_combat()
{
    RegisterCreatureAI(npc_highmountain_mightstone);
}
