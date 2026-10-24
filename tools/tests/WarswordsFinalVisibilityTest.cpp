#include <functional>
#include <list>
#include <map>
#include "CombatHarness.inc"
struct VisibilityUpdateTask
{
    Unit* actor;
    explicit VisibilityUpdateTask(Unit* unit) : actor(unit) { }
    static void UpdateVisibility(Unit* actor)
    {
        ++actor->visibilityRequests;
        actor->observedVisible = !actor->IsNeverVisible();
    }
};
void VisibilityEvents::AddEvent(VisibilityUpdateTask* task, uint32 delay) { assert(delay == 100); pending.push_back(task); }
struct AINotifyTask { static void Schedule(Unit* actor) { ++actor->notifications; } };
#include "VisibilityRefresh.inc"
struct Player : Unit
{
    bool contact = true;
    uint32 casts = 0;
    std::vector<std::function<void()>> pending;
    bool CanContact() const { return contact; }
    void AddDelayedEvent(uint32 delay, std::function<void()> callback) { assert(delay == 3000); pending.push_back(std::move(callback)); }
    void CastSpell(Player* target, uint32 spell) { assert(target == this && spell == 189740); ++casts; }
};
struct Map
{
    struct Reference { Player* player; Player* getSource() const { return player; } };
    struct PlayerList : std::list<Reference> { bool isEmpty() const { return empty(); } } players;
    std::map<ObjectGuid, Creature*> actors;
    PlayerList const& GetPlayers() const { return players; }
    Creature* GetCreature(ObjectGuid guid) const { auto found = actors.find(guid); return found == actors.end() ? nullptr : found->second; }
};
struct InstanceScript
{
    Map* instance;
    uint32 baseRemovals = 0;
    explicit InstanceScript(Map* map) : instance(map) { }
    virtual ~InstanceScript() = default;
    virtual void OnCreatureCreate(Creature*) { }
    virtual void OnCreatureRemove(Creature*) { ++baseRemovals; }
    virtual void onScenarionNextStep(uint32) { }
    uint32 getScenarionStep() const { return scenarioManager.scenario.step; }
};
struct Instance : InstanceScript
{
    explicit Instance(Map* map) : InstanceScript(map) { }
    std::map<uint32, ObjectGuid> objects;
    bool myscenario = false;
    uint32 count_wave = 0, waves = 0;
    void DoSummonAdds(uint32 wave) { assert(wave == count_wave - 1); ++waves; }
#include "Create.inc"
#include "Step.inc"
#include "Remove.inc"
};
#ifndef FIRST_REENTRY
int main()
{
#ifdef BASELINE
    scenarioManager = {}; scenarioManager.scenario.step = 4;
    Creature actor; FinalAI ai(&actor); Map map; Instance instance(&map); map.actors[actor.guid] = &actor;
    instance.OnCreatureCreate(&actor); actor.UpdateObjectVisibility();
    assert(actor.observedVisible && instance.objects.empty());
    uint32 requests = actor.visibilityRequests;
    scenarioManager.scenario.step = 5; instance.onScenarionNextStep(5);
    assert(actor.visibilityRequests == requests);
    std::cout << "BASELINE: pinned e50ea50d native creature visibility admits final98602 at4; no registered actor/explicit visibility refresh on unchanged-phase4->5. Controlled notifier, not client pixels.\n";
#else
    uint32 cases = 0;
    for (uint32 step = 0; step < 9; ++step) for (bool corpse : {false, true}) for (bool suspended : {false, true})
    {
        scenarioManager = {}; scenarioManager.scenario.step = step;
        Creature actor; FinalAI ai(&actor); actor.alive = !corpse;
        ScriptedAI temporary(&actor);
        if (suspended) actor.i_disabledAI = &ai;
        else actor.ai = actor.i_AI = &ai;
        WorldObject seer;
        bool expected = step >= 5 && step <= 7;
        assert(actor.IsNeverVisible(&seer) == !expected && actor.IsNeverVisible() == !expected);
        actor.UpdateObjectVisibility();
        assert(actor.visibilityRequests == 1 && actor.observedVisible == expected);
        actor.world = false;
        assert(actor.IsNeverVisible(&seer));
        ++cases;
    }
    for (uint32 invalid = 0; invalid < 3; ++invalid)
    {
        scenarioManager = {}; Creature actor; FinalAI ai(&actor);
        if (invalid == 0) scenarioManager.available = false;
        if (invalid == 1) actor.instance = 99;
        if (invalid == 2) scenarioManager.scenario.identity = 973;
        assert(actor.IsNeverVisible());
    }
    for (uint32 unrelated = 0; unrelated < 2; ++unrelated)
    {
        scenarioManager = {}; scenarioManager.available = false; Creature actor; FinalAI ai(&actor);
        if (unrelated == 0) actor.entry = 98007;
        else actor.mapId = 1220;
        assert(!actor.IsNeverVisible());
    }
    {
        scenarioManager = {}; scenarioManager.scenario.step = 4;
        Creature actor; FinalAI ai(&actor); Map map; Instance instance(&map); map.actors[actor.guid] = &actor;
        instance.OnCreatureCreate(&actor);
        assert(instance.objects.at(98602) == actor.guid && !actor.visibilityRequests);
        instance.onScenarionNextStep(4); assert(actor.visibilityRequests == 1 && !actor.observedVisible);
        scenarioManager.scenario.step = 5; instance.onScenarionNextStep(5);
        assert(actor.visibilityRequests == 2 && actor.observedVisible && !instance.myscenario);
        actor.alive = false; scenarioManager.scenario.step = 6; instance.onScenarionNextStep(6);
        assert(actor.visibilityRequests == 3 && actor.observedVisible);
        scenarioManager.scenario.step = 7;
        assert(!actor.IsNeverVisible() && actor.visibilityRequests == 3 && actor.observedVisible);
        scenarioManager.scenario.step = 4; instance.onScenarionNextStep(4);
        assert(actor.visibilityRequests == 4 && !actor.observedVisible);
        map.actors.clear(); instance.onScenarionNextStep(5); assert(actor.visibilityRequests == 4);
        instance.OnCreatureRemove(&actor); assert(instance.objects.empty() && instance.baseRemovals == 1);
        Creature replacement; replacement.guid = 2; FinalAI replacementAI(&replacement);
        map.actors[2] = &replacement; instance.OnCreatureCreate(&replacement);
        instance.OnCreatureRemove(&actor); assert(instance.objects.at(98602) == 2 && instance.baseRemovals == 2);
        scenarioManager.scenario.step = 5; instance.onScenarionNextStep(5);
        assert(replacement.visibilityRequests == 1 && replacement.observedVisible);
        Creature helya; helya.entry = 99043; helya.guid = 3; instance.OnCreatureCreate(&helya);
        assert(instance.objects.size() == 1 && !helya.visibilityRequests);
        instance.OnCreatureRemove(&helya); assert(instance.objects.at(98602) == 2 && instance.baseRemovals == 3);
    }
    {
        scenarioManager = {}; scenarioManager.scenario.step = 1;
        Map map; Instance instance(&map); instance.myscenario = true;
        Creature unrelated; unrelated.entry = 97992; Player player; unrelated.nearby = &player;
        ScriptedAI ai(&unrelated); instance.OnCreatureCreate(&unrelated);
        assert(unrelated.attacks == 1 && instance.objects.empty());
        instance.onScenarionNextStep(1); assert(instance.waves == 1 && instance.count_wave == 1);
        scenarioManager.scenario.step = 4; map.players.push_back({&player}); instance.onScenarionNextStep(4);
        assert(player.pending.size() == 1 && player.casts == 0); player.pending.front()(); assert(player.casts == 1);
    }
    {
        scenarioManager = {}; scenarioManager.scenario.step = 4;
        Creature actor; FinalAI ai(&actor); actor.UpdateObjectVisibility();
        assert(!actor.observedVisible && actor.notifications == 1 && actor.m_Events.pending.empty());
        scenarioManager.scenario.step = 5; actor.UpdateObjectVisibility(false);
        assert(!actor.observedVisible && actor.visibilityRequests == 1 && actor.notifications == 2 && actor.m_Events.pending.size() == 1);
        VisibilityUpdateTask::UpdateVisibility(actor.m_Events.pending.front()->actor);
        delete actor.m_Events.pending.front(); actor.m_Events.pending.clear();
        assert(actor.observedVisible && actor.visibilityRequests == 2);
    }
    std::cout << "PASS: " << cases << " actual creature current/suspended-AI visibility/absent-world stage/corpse paths; exact5..7 policy including terminal7 without callback, null seer, missing scenario, unrelated reuse, pre-bootstrap registration and unchanged-phase refresh, unload/replacement/stale-remove, preserved native base cleanup/wave/scene requests. Controlled task/notifier, not live packets or client pixels.\n";
#endif
}
#endif
