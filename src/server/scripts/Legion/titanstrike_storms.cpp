#include "ScriptMgr.h"
#include "AreaTriggerAI.h"
#include "Creature.h"
#include "Player.h"
#include "Scenario.h"
#include "ScenarioMgr.h"
#include "ObjectAccessor.h"

// Two overlapping native Spirit Winds boxes share a corridor. Only remove a
// force owned by this box; an exit must not clear the other box's force.
struct at_titanstrike_spirit_winds : AreaTriggerAI
{
    explicit at_titanstrike_spirit_winds(AreaTrigger* trigger) : AreaTriggerAI(trigger) { }

    bool Active() const
    {
        auto progress = at->GetMapId() == 1495 ? sScenarioMgr->GetScenario(at->GetInstanceId()) : nullptr;
        return progress && progress->GetScenarioId() == 1068 && progress->GetCurrentStep() == 1;
    }

    void CalculateDuration(int32& /*duration*/) override
    {
        // Each native kit58494 is the entire corridor, not one force box.
        // Keep the upper emitter's visual; a second copy from the lower box
        // extends past the tornado endpoints onto the entrance stairs.
        // This callback runs after visual selection and before AddToMap.
        if (Active())
            if (auto caster = at->GetCaster())
                if (auto helper = caster->ToCreature())
                    if (helper->GetDBTableGUIDLow() == 365439)
                    {
                        at->SetUInt32Value(AREATRIGGER_FIELD_SPELL_FOR_VISUALS, 0);
                        at->SetUInt32Value(AREATRIGGER_FIELD_SPELL_XSPELL_VISUAL_ID, 0);
                    }
        // Retain both actual spell210880 force fields, their geometry, native
        // duration, independent cleanup and the already-correct force vector.
    }

    bool CreateUnitList(std::list<Unit*>& units) override
    {
        for (auto const& ref : at->GetMap()->GetPlayers())
            if (auto player = ref.getSource())
                if (at->InSamePhaseId(player) && player->IsAlive())
                    units.push_back(player);
        return true; // native IsInArea still checks the oriented box
    }

    void Remove(Unit* unit)
    {
        if (auto player = unit->ToPlayer())
            if (player->GetForceGUID() == at->GetGUID())
                player->SendMovementForce(at);
    }

    void Apply(Unit* unit)
    {
        auto player = unit->ToPlayer();
        if (!player)
            return;
        if (!Active() || !player->IsAlive() || player->GetVehicle() || player->isInFlight() ||
            player->HasAuraType(SPELL_AURA_DISABLE_MOVEMENT_FORCE) || player->HasAura(210942))
        {
            Remove(player);
            return;
        }
        if (player->GetForceGUID().IsEmpty())
        {
            auto data = at->GetAreaTriggerInfo();
            player->SendMovementForce(at, Position(data.windX, data.windY, data.windZ), data.windSpeed, data.windType, true);
        }
    }

    void OnUnitEnter(Unit* unit) override { Apply(unit); }
    void OnUnitExit(Unit* unit) override { Remove(unit); }
    void BeforeRemove(Unit* unit) override { Remove(unit); }
    void OnUpdate(uint32 /*diff*/) override
    {
        for (auto guid : *at->GetAffectedPlayers())
            if (auto unit = ObjectAccessor::GetUnit(*at, guid))
                Apply(unit); // retry after leaving a tornado or an overlapping box
    }
};

void AddSC_titanstrike_storms()
{
    RegisterAreaTriggerAI(at_titanstrike_spirit_winds);
}
