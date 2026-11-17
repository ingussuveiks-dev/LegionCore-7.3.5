#ifndef EARTHWARDER_WINDS_H
#define EARTHWARDER_WINDS_H

#include "AreaTrigger.h"
#include "AreaTriggerData.h"
#include "ObjectMgr.h"
#include "Spell.h"
#include "SpellScript.h"

// Native 4727 is shared by 190842 and the non-pushing 190845. The loader
// indexes it by entry, so the latter overwrites the former's force vector.
// Give only the two original Warrior sources a separate, owned definition.
class spell_earthwarder_spirit_winds : public SpellScriptLoader
{
public:
    spell_earthwarder_spirit_winds() : SpellScriptLoader("spell_earthwarder_spirit_winds") { }

    class Script : public SpellScript
    {
        PrepareSpellScript(Script);

        void CreateWind(SpellEffIndex effect)
        {
            Creature* source = GetCaster() ? GetCaster()->ToCreature() : nullptr;
            if (!source || source->GetEntry() != 96740 || source->GetMapId() != 1495 ||
                !source->GetPhases().count(6999) ||
                (source->GetDBTableGUIDLow() != 361407 && source->GetDBTableGUIDLow() != 361409))
                return;
            Scenario* scenario = sScenarioMgr->GetScenario(source->GetInstanceId());
            if (!scenario || scenario->GetScenarioId() != 909)
                return;

            PreventHitDefaultEffect(effect);
            if (!source->IsInWorld() || !source->IsAlive() || scenario->GetCurrentStep() > 2)
                return;

            // The original sources are in opposite side niches, 24 yards
            // apart. Center two adjacent 12.5-yard strips on the corridor,
            // instead of two 25-yard boxes centered on those niches.
            constexpr float facing = 4.54211f;
            float side = source->GetDBTableGUIDLow() == 361407 ? 6.25f : -6.25f;
            Position position = { 4831.615f - std::sin(facing) * side,
                285.154f + std::cos(facing) * side, source->GetPositionZ(), facing };
            AreaTrigger* wind = new AreaTrigger;
            if (!wind->CreateAreaTrigger(sObjectMgr->GetGenerator<HighGuid::AreaTrigger>()->Generate(),
                4727, source, GetSpellInfo(), position, position, GetSpell(),
                ObjectGuid::Empty, 190842, GetSpell()->m_castGuid[0]))
            {
                delete wind;
                // The instance retains its aura/area acknowledgement and
                // diagnostic/retry; never accept a visual-only fallback.
            }
        }

        void Register() override
        {
            OnEffectHit += SpellEffectFn(Script::CreateWind, EFFECT_1, SPELL_EFFECT_CREATE_AREATRIGGER);
        }
    };

    SpellScript* GetSpellScript() const override { return new Script(); }
};

#endif
