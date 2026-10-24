#include "WarswordsFinalVisibilityTest.cpp"

struct FirstAI : ScriptedAI
{
    explicit FirstAI(Creature* actor) : ScriptedAI(actor) { }
#include "FirstPhase.inc"
#include "FirstActive.inc"
#include "FirstReady.inc"
#include "FirstVisibility.inc"
#include "FirstCanAttack.inc"
#include "FirstAttack.inc"
};

int main()
{
#ifdef BASELINE
    scenarioManager = {}; scenarioManager.scenario.step = 5;
    Creature actor; actor.entry = 98007; FirstAI ai(&actor); Unit player;
    Map map; Instance instance(&map); map.actors[actor.guid] = &actor;
    instance.OnCreatureCreate(&actor); actor.UpdateObjectVisibility();
    assert(actor.observedVisible && instance.objects.empty() && actor.canCreatureAttack(&player));
    ai.AttackStart(&player); assert(actor.attacks == 1);
    std::cout << "BASELINE: pinnedbaca33c7 fresh first98007 remains natively visible/attack-capable at944/5 after re-entry, with no first-actor registration/refresh. Controlled map/notifier/combat services, not a captured client failure.\n";
#else
    uint32 cases = 0;
    for (uint32 step = 0; step < 9; ++step) for (bool retreat : {false, true})
        for (bool corpse : {false, true}) for (bool suspended : {false, true})
    {
        scenarioManager = {}; scenarioManager.scenario.step = step;
        Creature actor; actor.entry = 98007; FirstAI ai(&actor); Unit player;
        assert(!ai.phase);
        ai.phase = retreat; actor.alive = !corpse;
        ScriptedAI temporary(&actor);
        if (suspended) actor.i_disabledAI = &ai; else actor.ai = actor.i_AI = &ai;
        bool visible = step == 4 || (step == 5 && retreat);
        bool combat = step == 4 && !retreat;
        assert(actor.IsNeverVisible() == !visible);
        assert(ai.CanAIAttack(&player) == combat);
        if (!suspended) assert(actor.canCreatureAttack(&player) == combat);
        ai.AttackStart(&player); assert(actor.attacks == uint32(combat));
        actor.UpdateObjectVisibility(); assert(actor.observedVisible == visible && actor.visibilityRequests == 1);
        actor.world = false; assert(actor.IsNeverVisible());
        ++cases;
    }
    for (uint32 invalid = 0; invalid < 3; ++invalid)
    {
        scenarioManager = {}; scenarioManager.scenario.step = 4;
        Creature actor; actor.entry = 98007; FirstAI ai(&actor); Unit player;
        if (invalid == 0) scenarioManager.available = false;
        if (invalid == 1) actor.instance = 99;
        if (invalid == 2) scenarioManager.scenario.identity = 973;
        assert(actor.IsNeverVisible() && !actor.canCreatureAttack(&player));
        ai.AttackStart(&player); assert(!actor.attacks);
    }
    for (uint32 unrelated = 0; unrelated < 2; ++unrelated)
    {
        scenarioManager = {}; scenarioManager.available = false;
        Creature actor; actor.entry = 98007; FirstAI ai(&actor); Unit player; ai.phase = true;
        if (unrelated) actor.mapId = 1220; else actor.entry = 98602;
        assert(!actor.IsNeverVisible() && actor.canCreatureAttack(&player));
        ai.AttackStart(&player); assert(actor.attacks == 1);
    }
    {
        scenarioManager = {}; scenarioManager.scenario.step = 4;
        Creature first; first.entry = 98007; FirstAI firstAI(&first);
        Creature final; final.guid = 2; FinalAI finalAI(&final);
        Map map; Instance instance(&map); map.actors = {{1, &first}, {2, &final}};
        instance.OnCreatureCreate(&first); instance.OnCreatureCreate(&final);
        assert(instance.objects.size() == 2 && !instance.myscenario);
        instance.onScenarionNextStep(4); assert(first.observedVisible && !final.observedVisible);
        firstAI.phase = true; scenarioManager.scenario.step = 5; instance.onScenarionNextStep(5);
        assert(first.observedVisible && final.observedVisible && first.visibilityRequests == 2 && final.visibilityRequests == 2);
        firstAI.phase = false; first.UpdateObjectVisibility(); assert(!first.observedVisible);
        scenarioManager.scenario.step = 6; instance.onScenarionNextStep(6);
        assert(!first.observedVisible && final.observedVisible);
        Creature replacement; replacement.entry = 98007; replacement.guid = 3; FirstAI replacementAI(&replacement);
        map.actors.erase(1); map.actors[3] = &replacement; instance.OnCreatureCreate(&replacement);
        instance.OnCreatureRemove(&first); assert(instance.objects.at(98007) == 3 && instance.baseRemovals == 1);
        scenarioManager.scenario.step = 5; instance.onScenarionNextStep(5);
        assert(!replacement.observedVisible && final.observedVisible);
        instance.OnCreatureRemove(&replacement); assert(!instance.objects.count(98007) && instance.objects.count(98602));
        Creature helya; helya.entry = 99043; instance.OnCreatureCreate(&helya); instance.OnCreatureRemove(&helya);
        assert(instance.objects.size() == 1 && !helya.visibilityRequests);
        map.actors.clear(); instance.onScenarionNextStep(4);
    }
    std::cout << "PASS: " << cases << " actual current/suspended-AI visibility and scoped first98007 active-retreat/combat policies; fresh944/5 cannot reappear or engage, live4->5 retreat remains visible,6+ hides first but preserves final. Missing scenario, native default phase initialization, unrelated entry/map reuse and pre-bootstrap two-actor GUID lifecycle/forced refresh covered. Damage/Reset/update gates have separate production-handler/PE tests; no client packet/animation proof.\n";
#endif
}
