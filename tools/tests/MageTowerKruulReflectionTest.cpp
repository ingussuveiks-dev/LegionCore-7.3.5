#include <cassert>
#include <iostream>

struct Player;

struct Unit
{
    Player* owner = nullptr;
    int instance = 4;
    int phase = 1;
    bool alive = true;
    bool combat = true;

    bool IsAlive() const { return alive; }
    bool isInCombat() const { return combat; }
    bool IsInMap(Unit const* other) const { return instance == other->instance; }
    bool InSamePhase(Unit const* other) const { return phase == other->phase; }
    int GetInstanceId() const { return instance; }
    Player* GetCharmerOrOwnerPlayerOrPlayerItself() const { return owner; }
};

struct Creature : Unit {};

struct Player : Unit
{
    Player() { owner = this; }

    bool inWorld = true;
    bool leaving = false;
    bool teleporting = false;

    bool IsInWorld() const { return inWorld; }
    bool IsPreDelete() const { return leaving; }
    bool IsBeingTeleportedFar() const { return teleporting; }
};

struct Scenario
{
    int id = 1259;
    int step = 2;
    bool completed = false;

    int GetScenarioId() const { return id; }
    int GetCurrentStep() const { return step; }
    bool IsCompleted(bool) const { return completed; }
};

struct ScenarioManager
{
    Scenario scenario;
    Scenario* GetScenario(int instance) { return instance == 4 ? &scenario : nullptr; }
};

ScenarioManager manager;
ScenarioManager* sScenarioMgr = &manager;

struct DamageInfo
{
    Unit* attacker;
    Unit* victim;
    unsigned damage;

    Unit* GetAttacker() const { return attacker; }
    Unit* GetVictim() const { return victim; }
    unsigned GetDamage() const { return damage; }
};

struct ProcEventInfo
{
    DamageInfo* hit;
    DamageInfo const* GetDamageInfo() const { return hit; }
};

struct Reflection
{
    Creature* boss;
    Unit* target;

    Creature* GetKruul() const { return boss; }
    Unit* GetTarget() const { return target; }

#include "KruulReflection.inc"
};

int main()
{
    Creature kruul;
    Unit ally;
    Player warrior;
    Unit guardian;
    guardian.owner = &warrior;
    Reflection curse{ &kruul, &warrior };
    DamageInfo hit{ &kruul, &warrior, 100 };
    ProcEventInfo event{ &hit };

    assert(curse.GetHealRecipient(event) == &kruul);
    hit.damage = 0;
    assert(!curse.GetHealRecipient(event));
    hit.damage = 100;
    hit.attacker = &ally;
    assert(!curse.GetHealRecipient(event));
    hit.attacker = &kruul;
    hit.victim = &guardian;
    assert(!curse.GetHealRecipient(event));
    curse.target = &guardian;
    assert(curse.GetHealRecipient(event) == &kruul);

    curse.target = &kruul;
    hit.victim = &kruul;
    assert(!curse.GetHealRecipient(event));
    hit.attacker = &warrior;
    assert(curse.GetHealRecipient(event) == &warrior);
    hit.attacker = &guardian;
    assert(curse.GetHealRecipient(event) == &warrior);
    hit.attacker = &ally;
    assert(!curse.GetHealRecipient(event));
    hit.attacker = &guardian;

    warrior.teleporting = true;
    assert(!curse.GetHealRecipient(event));
    warrior.teleporting = false;
    warrior.phase = 2;
    assert(!curse.GetHealRecipient(event));
    warrior.phase = 1;
    warrior.alive = false;
    assert(!curse.GetHealRecipient(event));
    warrior.alive = true;
    guardian.instance = 1;
    assert(!curse.GetHealRecipient(event));
    guardian.instance = 4;
    assert(curse.GetHealRecipient(event) == &warrior);

    manager.scenario.step = 1;
    assert(!curse.GetHealRecipient(event));
    manager.scenario.step = 2;
    manager.scenario.completed = true;
    assert(!curse.GetHealRecipient(event));
    manager.scenario.completed = false;
    kruul.alive = false;
    assert(!curse.GetHealRecipient(event));
    kruul.alive = true;
    kruul.combat = false;
    assert(!curse.GetHealRecipient(event));
    kruul.combat = true;
    event.hit = nullptr;
    assert(!curse.GetHealRecipient(event));
    event.hit = &hit;
    curse.boss = nullptr;
    assert(!curse.GetHealRecipient(event));

    std::cout << "PASS Twisted Reflection normal and reflected recipients, controlled hit, absorption, attempt and wipe gates\n";
}
