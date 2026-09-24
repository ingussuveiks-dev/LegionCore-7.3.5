#include <cassert>
#include <iostream>

enum SpellCastResult { SPELL_CAST_OK, SPELL_FAILED_DONT_REPORT };
enum { AURA_REMOVE_BY_CANCEL };
struct ObjectGuid { static constexpr int Empty = 0; };
struct AuraEffect { };
struct Unit
{
    bool active = false;
    unsigned health = 100;
    unsigned cancels = 0;
    bool HasAura(unsigned id) const { assert(id == 111400); return active; }
    void RemoveAurasDueToSpell(unsigned id, int, int, int mode)
    {
        assert(id == 111400 && mode == AURA_REMOVE_BY_CANCEL);
        active = false;
        ++cancels;
    }
    bool HealthAbovePct(unsigned pct) const { return health > pct; }
};
struct Aura
{
    Unit& target;
    void Remove() { target.active = false; }
};
struct BurningRush
{
    Unit& caster;
    Unit& target;
    Aura aura{target};
    bool prevented = false;
    Unit* GetCaster() { return &caster; }
    Unit* GetTarget() { return &target; }
    Aura* GetAura() { return &aura; }
    void PreventDefaultAction() { prevented = true; }

    // These are extracted from the production script, not duplicate logic.
#include "BurningRushCheckCast.inc"
#include "BurningRushTick.inc"

    void Cast()
    {
        if (CheckCast() == SPELL_CAST_OK)
            target.active = true;
    }
    void Tick()
    {
        prevented = false;
        if (!target.active)
            return;
        OnTick(nullptr);
        // The core still performs the current tick after Remove() unless the
        // script explicitly prevents it. This reproduces the reported bug.
        if (!prevented)
            target.health = target.health > 4 ? target.health - 4 : 0;
    }
};

int main()
{
    Unit player;
    BurningRush rush{player, player};
    rush.Cast();
    assert(player.active);
    rush.Tick();
    assert(player.health == 96 && player.active);
    rush.Cast();
    assert(!player.active && player.cancels == 1);
    for (unsigned i = 0; i < 5; ++i)
        rush.Tick();
    assert(player.health == 96);
    rush.Cast();
    assert(player.active);

    for (unsigned hp : {1u, 4u, 8u, 9u})
    {
        player.health = hp;
        player.active = true;
        rush.Tick();
        assert(!player.active && rush.prevented && player.health == hp);
    }
    player.health = 10;
    player.active = true;
    rush.Tick();
    assert(player.health == 6 && player.active);
    rush.Tick();
    assert(player.health == 6 && !player.active);

    Unit healthyCaster;
    player.active = true;
    player.health = 4;
    BurningRush differentTarget{healthyCaster, player};
    differentTarget.Tick();
    assert(!player.active && player.health == 4);
    std::cout << "Burning Rush toggle, cancelled ticks, and low-health boundaries passed.\n";
}
