#pragma once

#include "AreaTrigger.h"
#include "AreaTriggerAI.h"
#include "DB2Stores.h"
#include "StromkarGroundCrackData.h"

namespace StromkarGroundCracks
{
struct State { ObjectGuid area; uint32 retry = 0; bool warned = false; };

bool Source(Creature* actor)
{
    if (!actor || !actor->IsInWorld() || !actor->IsAlive() || actor->GetMapId() != 1539 ||
        actor->GetEntry() != 103398 || actor->GetDBTableGUIDLow() != 370991)
        return false;
    Scenario* scenario = sScenarioMgr->GetScenario(actor->GetInstanceId());
    return scenario && scenario->GetScenarioId() == 1037 && scenario->GetCurrentStep() == 7;
}

void Clear(Creature* actor, State& state)
{
    if (actor)
        if (AreaTrigger* area = actor->GetMap()->GetAreaTrigger(state.area))
            if (area->GetCasterGUID() == actor->GetGUID() && area->GetCustomEntry() == StromkarGroundCrackData::Area)
                area->Remove();
    state.area.Clear();
    state.retry = 0;
    state.warned = false;
}

void Update(Creature* actor, uint32 diff, uint32 remaining, State& state)
{
    if (!remaining || !Source(actor))
    {
        Clear(actor, state);
        return;
    }
    if (AreaTrigger* area = actor->GetMap()->GetAreaTrigger(state.area))
        if (area->IsInWorld() && area->GetCasterGUID() == actor->GetGUID() && area->GetCustomEntry() == StromkarGroundCrackData::Area)
            return;
    if (state.retry > diff) { state.retry -= diff; return; }
    state.retry = 500;
    bool dataReady = StromkarGroundCrackData::Supported(sDecalPropertiesStore.LookupEntry(StromkarGroundCrackData::Record),
        sTextureBlendSetStore.LookupEntry(StromkarGroundCrackData::Record));
    AreaTrigger* area = nullptr;
    if (dataReady)
    {
        area = new AreaTrigger;
        Position position = actor->GetPosition();
        if (!area->CreateAreaTrigger(sObjectMgr->GetGenerator<HighGuid::AreaTrigger>()->Generate(),
            0, actor, nullptr, position, position, nullptr, ObjectGuid::Empty, StromkarGroundCrackData::Area))
        {
            delete area;
            area = nullptr;
        }
    }
    if (area)
        state.area = area->GetGUID();
    else if (!state.warned)
    {
        state.warned = true;
        TC_LOG_WARN("scripts", "Stromkar: projected ground crack could not be created (native decal data ready=%u); retrying only during the real burst", uint32(dataReady));
    }
}
}

struct at_stromkar_ground_cracks : AreaTriggerAI
{
    explicit at_stromkar_ground_cracks(AreaTrigger* area) : AreaTriggerAI(area) { }
    void CalculateDuration(int32& duration) override { duration = StromkarGroundCrackData::Duration; }
    bool CreateUnitList(std::list<Unit*>& units) override
    {
        // This new visual owns no actions, auras, collision or quest progress.
        units.clear();
        return true;
    }
};
