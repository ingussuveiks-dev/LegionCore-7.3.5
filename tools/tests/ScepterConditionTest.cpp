#include <cassert>
#include <cstdint>
#include <iostream>
using uint32 = std::uint32_t;
constexpr int CONDITION_SCENARION_STEP = 44, SMART_EVENT_DATA_SET = 38;
struct Scenario { uint32 id = 1155, step = 0;
    uint32 GetScenarioId() const { return id; }
    uint32 GetCurrentStep() const { return step; }
} scenario;
struct InstanceMap { uint32 id = 1;
    uint32 GetInstanceId() const { return id; }
    InstanceMap* ToInstanceMap() { return this; }
} map;
struct ScenarioMgr { Scenario* GetScenario(uint32 id) { return id == 1 ? &scenario : nullptr; } } manager;
auto sScenarioMgr = &manager;
struct WorldObject { bool inInstance = true;
    bool InInstance() const { return inInstance; }
    InstanceMap* GetMap() { return &map; }
};
bool evaluate(WorldObject* object, uint32 ConditionValue1, uint32 ConditionValue2)
{
    bool condMeets = false;
    if (!object) return false;
    switch (CONDITION_SCENARION_STEP) {
#include "ScenarioCondition.inc"
    }
    return condMeets;
}
struct Script {
    WorldObject base;
    int target = 1;
    uint32 wantedStep = 0;
    bool passed = false;
    void ProcessEventsFor(int event, WorldObject* invoker, uint32 id, uint32 value) {
        assert(event == SMART_EVENT_DATA_SET && !invoker && id == 1 && value == 1);
        // Native ConditionSourceInfo(unit, baseObject) binds these two slots.
        WorldObject* targets[] = {invoker, &base};
        passed = evaluate(targets[target], 1155, wantedStep);
    }
};
struct SmartAI { Script script; Script* GetScript() { return &script; }
    void SetData(uint32 id, uint32 value);
};
#include "SmartSetData.inc"
int main()
{
    SmartAI ai;
    ai.script.target = 0; ai.SetData(1, 1);
    assert(!ai.script.passed); // Reproduce the deployed broken condition.
    ai.script.target = 1;
    for (uint32 step = 0; step < 10; ++step) {
        scenario.step = step; ai.script.wantedStep = step;
        ai.SetData(1, 1); assert(ai.script.passed);
        ai.script.wantedStep = (step + 1) % 10;
        ai.SetData(1, 1); assert(!ai.script.passed);
    }
    ai.script.wantedStep = scenario.step;
    scenario.id = 962; ai.SetData(1, 1); assert(!ai.script.passed);
    scenario.id = 1155; map.id = 0; ai.SetData(1, 1); assert(!ai.script.passed);
    map.id = 1; ai.script.base.inInstance = false;
    ai.SetData(1, 1); assert(!ai.script.passed);
    std::cout << "PASS: native SetData null invoker, all 10 native condition steps, wrong scenario/map\n";
}
