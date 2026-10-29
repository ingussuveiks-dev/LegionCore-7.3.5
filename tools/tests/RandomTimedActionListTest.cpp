#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <vector>
using uint32 = uint32_t;
constexpr uint32 SMART_ACTION_PARAM_COUNT = 6;
constexpr uint32 SMART_ACTION_CALL_RANDOM_TIMED_ACTIONLIST = 87;
constexpr uint32 SMART_TARGET_NONE = 0;
static unsigned errors, draws;
static uint32 draw, lastMax;
static void Require(bool value) { if (!value) std::abort(); }
uint32 urand(uint32 min, uint32 max)
{
    Require(min == 0 && max < 7 && draw <= max);
    ++draws;
    lastMax = max;
    return draw;
}
struct SmartScriptHolder
{
    struct { struct { uint32 entry1, entry2, entry3, entry4, entry5, entry6; } randTimedActionList{}; } action;
    long entryOrGuid = 1;
    uint32 target = 1;
    uint32 GetTargetType() const { return target; }
    uint32 GetScriptType() const { return 0; }
    uint32 GetEventType() const { return 62; }
    uint32 GetActionType() const { return SMART_ACTION_CALL_RANDOM_TIMED_ACTIONLIST; }
};
struct Unit;
struct FakeAI
{
    unsigned calls = 0;
    uint32 selected = 0;
    Unit* invoker = nullptr;
    SmartScriptHolder const* event = nullptr;
    void SetScript9(SmartScriptHolder const& e, uint32 id, Unit* source)
    { ++calls; selected = id; invoker = source; event = &e; }
};
struct Creature;
struct GameObject;
struct Unit
{
    virtual ~Unit() = default;
    virtual Creature* ToCreature() { return nullptr; }
    virtual GameObject* ToGameObject() { return nullptr; }
};
struct Creature : Unit
{
    FakeAI ai;
    bool smart = true;
    Creature* ToCreature() override { return this; }
    FakeAI* AI() { return &ai; }
};
struct GameObject : Unit
{
    FakeAI ai;
    bool smart = true;
    GameObject* ToGameObject() override { return this; }
    FakeAI* AI() { return &ai; }
};
bool IsSmart(Creature* target) { return target->smart; }
bool IsSmartGO(GameObject* target) { return target->smart; }
using ObjectList = std::vector<Unit*>;
#define CAST_AI(type, ai) (ai)
#define TC_LOG_ERROR(...) (++errors)
struct SmartScript
{
    ObjectList targetObjects;
    Unit invoker;
    bool missing = false;
    ObjectList* GetTargets(SmartScriptHolder const&, Unit*) { return missing ? nullptr : new ObjectList(targetObjects); }
    Unit* GetLastInvoker() { return &invoker; }
    void ProcessAction(SmartScriptHolder const& e)
    {
        Unit* unit = nullptr;
        switch (e.GetActionType())
        {
#include "RandomTimedActionList.inc"
        }
    }
};
int main(int argc, char**)
{
    if (argc > 1)
    {
        // Inclusive maximum deterministically reads temp[6] in the pinned old handler.
        SmartScript script;
        Creature target;
        script.targetObjects = {&target};
        SmartScriptHolder e;
        e.action.randTimedActionList = {101,102,103,104,105,106};
        draw = 6;
        script.ProcessAction(e);
        return 2; // The broken baseline must be diagnosed by ASan, not accepted here.
    }
    unsigned cases = 0;
    for (unsigned mask = 1; mask < 64; ++mask)
    {
        std::array<uint32,6> values{};
        std::vector<uint32> expected;
        for (unsigned i = 0; i < 6; ++i)
            if (mask & (1u << i)) { values[i] = 100 + i; expected.push_back(values[i]); }
        for (unsigned i = 0; i < expected.size(); ++i)
        {
            SmartScript script;
            Creature creature, ordinary;
            GameObject go, ordinaryGo;
            Unit unrelated;
            ordinary.smart = ordinaryGo.smart = false;
            script.targetObjects = {&creature,&go,&ordinary,&ordinaryGo,&unrelated};
            SmartScriptHolder e;
            e.action.randTimedActionList = {values[0],values[1],values[2],values[3],values[4],values[5]};
            draw = i; errors = draws = 0;
            script.ProcessAction(e);
            Require(!errors && draws == 1 && lastMax == expected.size()-1);
            for (auto ai : {&creature.ai,&go.ai})
                Require(ai->calls == 1 && ai->selected == expected[i] && ai->invoker == &script.invoker && ai->event == &e);
            Require(!ordinary.ai.calls && !ordinaryGo.ai.calls);
            ++cases;
        }
    }
    SmartScript script;
    Creature creature;
    script.targetObjects = {&creature};
    SmartScriptHolder e;
    errors = draws = 0;
    script.ProcessAction(e);
    Require(errors == 1 && !draws && !creature.ai.calls); // Empty pool fails visibly before RNG.
    e.action.randTimedActionList = {9,0,9,0,12,0};
    for (auto i : {0u,1u,2u})
    {
        draw = i;
        script.ProcessAction(e);
        Require(lastMax == 2 && creature.ai.selected == (i == 2 ? 12u : 9u));
    } // Repeated IDs keep their native weighting.
    auto calls = creature.ai.calls;
    errors = draws = 0;
    e.target = SMART_TARGET_NONE; draw = 0;
    script.ProcessAction(e);
    Require(errors == 1 && draws == 1 && creature.ai.calls == calls);
    errors = 0; e.target = 1;
    script.missing = true;
    script.ProcessAction(e);
    Require(!errors && creature.ai.calls == calls);
    script.missing = false; script.targetObjects.clear();
    script.ProcessAction(e);
    Require(!errors && creature.ai.calls == calls);
    std::printf("PASS %u exhaustive nonempty mask/choice cases, empty pool diagnostic, duplicate weighting, Creature/GO/invoker dispatch, TARGET_NONE, missing/empty targets; extracted production action87 under ASan.\n", cases);
}
