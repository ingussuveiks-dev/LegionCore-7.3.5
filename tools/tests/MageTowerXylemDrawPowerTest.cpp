#include <cassert>
#include <cstdint>
#include <iostream>

using uint32 = std::uint32_t;
constexpr unsigned CURRENT_CHANNELED_SPELL = 2;

struct Player
{
    bool IsAlive() const { return alive; }
    bool IsInWorld() const { return online; }
    bool HasPendingTeleport() const { return pendingTransfer; }
    bool alive = true, online = true, pendingTransfer = false, sameMap = true, samePhase = true;
};

struct Unit
{
    Player* owner = nullptr;
    Player* GetCharmerOrOwnerPlayerOrPlayerItself() { return owner; }
};

struct SpellInfo
{
    uint32 Id = 231522;
};

struct Spell
{
    SpellInfo info;
    SpellInfo const* GetSpellInfo() const { return &info; }
};

struct Creature
{
    uint32 entry = 64693, guid = 14534039, instance = 5;
    bool alive = true, present = true, inCombat = true, sameMap = true, samePhase = true;
    Unit* victim = nullptr;
    Spell* channel = nullptr;
    unsigned stacks = 5;

    uint32 GetEntry() const { return entry; }
    uint32 GetDBTableGUIDLow() const { return guid; }
    uint32 GetInstanceId() const { return instance; }
    bool IsAlive() const { return alive; }
    bool IsInWorld() const { return present; }
    bool isInCombat() const { return inCombat; }
    template<class Object> bool IsInMap(Object const* object) const { return object->sameMap; }
    template<class Object> bool InSamePhase(Object const* object) const { return object->samePhase; }
    Unit* getVictim() const { return victim; }
    Spell const* GetCurrentSpell(unsigned slot) const { assert(slot == CURRENT_CHANNELED_SPELL); return channel; }
    void RemoveAurasDueToSpell(uint32 spell) { assert(spell == 231610); stacks = 0; }
};

struct SmartAI
{
    explicit SmartAI(Creature* creature) : me(creature) {}
    virtual void Reset() { ++resets; }
    Creature* me;
    unsigned resets = 0;
};

struct Scenario
{
    uint32 id = 1204, step = 1;
    bool completed = false;
    uint32 GetScenarioId() const { return id; }
    uint32 GetCurrentStep() const { return step; }
    bool IsCompleted(bool) const { return completed; }
};

struct ScenarioManager
{
    Scenario state;
    Scenario* GetScenario(uint32 instance) { return instance == 5 ? &state : nullptr; }
};

ScenarioManager manager;
auto* sScenarioMgr = &manager;

#include "XylemDrawPower.inc"

int main()
{
    Creature boss, focus;
    Player player;
    Unit pet{ &player };
    Spell channel;
    boss.channel = &channel;
    boss.victim = &pet;
    ClosingTheEye::XylemAI ai(&boss);
    assert(ai.CanDrawPower(&focus));
    assert(!ai.CanDrawPower(nullptr));
    ++focus.guid;
    assert(!ai.CanDrawPower(&focus));
    --focus.guid;
    focus.samePhase = false;
    assert(!ai.CanDrawPower(&focus));
    focus.samePhase = true;
    player.pendingTransfer = true;
    assert(!ai.CanDrawPower(&focus));
    player.pendingTransfer = false;
    player.alive = false;
    assert(!ai.CanDrawPower(&focus));
    player.alive = true;
    boss.channel = nullptr;
    assert(!ai.CanDrawPower(&focus));
    boss.channel = &channel;
    channel.info.Id = 242734;
    assert(!ai.CanDrawPower(&focus));
    channel.info.Id = 231522;
    manager.state.step = 2;
    assert(!ai.CanDrawPower(&focus));
    manager.state.step = 1;
    ++boss.instance;
    assert(!ai.CanDrawPower(&focus));
    --boss.instance;
    assert(ai.CanDrawPower(&focus));
    ai.Reset();
    assert(boss.stacks == 0 && ai.resets == 1);
    std::cout << "PASS native channel gate, focus identity, exact scenario and phase, pet owner, interrupted or stale attempts, and reset.\n";
}
