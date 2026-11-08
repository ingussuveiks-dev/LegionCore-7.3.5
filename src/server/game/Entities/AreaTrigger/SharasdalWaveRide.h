#ifndef SHARASDAL_WAVE_RIDE_H
#define SHARASDAL_WAVE_RIDE_H

#include "AreaTrigger.h"
#include "Scenario.h"
#include "ScenarioMgr.h"
#include "Unit.h"

namespace SharasdalWaveRide
{
    constexpr uint32 Period = 1000;
    constexpr uint32 Ride = 46598;
    constexpr uint32 Wave = 209132;
    constexpr uint32 Moment = AT_ACTION_MOMENT_ENTER | AT_ACTION_MOMENT_UPDATE_TARGET;

    inline bool Owns(AreaTrigger const* trigger, Unit const* owner)
    {
        if (!trigger || !owner || !owner->IsCreature() || owner->GetEntry() != 105251 ||
            !owner->IsInWorld() || owner->IsPreDelete() || !owner->IsAlive() ||
            trigger->GetMapId() != 1600 || owner->GetMap() != trigger->GetMap() ||
            trigger->GetRealEntry() != 6547 || trigger->GetCustomEntry() != 11160 ||
            trigger->GetSpellId() != Wave || trigger->GetCasterGUID() != owner->GetGUID() ||
            trigger->GetTargetGuid() != owner->GetGUID())
            return false;
        Scenario* scenario = sScenarioMgr->GetScenario(trigger->GetInstanceId());
        return scenario && scenario->GetScenarioId() == 1066;
    }

    inline bool NativeAction(AreaTriggerAction const& action)
    {
        return action.id == 1 && action.actionType == AT_ACTION_TYPE_CAST_SPELL &&
            action.spellId == Ride && action.targetFlags == 1056840 &&
            action.hitMaxCount == 1 && action.maxCharges == 0;
    }

    inline bool Enabled(AreaTrigger const* trigger, Unit const* owner, AreaTriggerInfo const& info)
    {
        return Owns(trigger, owner) && info.updateDelay == Period && info.actions.size() == 1 &&
            NativeAction(info.actions.front()) && info.actions.front().moment == Moment;
    }
}
#endif
