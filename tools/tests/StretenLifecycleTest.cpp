#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>
using uint32 = uint32_t;
using uint8 = uint8_t;
using AuraEffectHandleModes = int;
constexpr int EFFECT_0 = 0;
struct AuraEffect {
    int amount = 0;
    int GetAmount() const { return amount; }
    void SetAmount(int value) { amount = value; }
};
struct Aura {
    uint8 stacks = 0;
    void SetStackAmount(uint8 value) { stacks = value; }
};
struct Unit {
    uint8 targets = 0;
    Aura buff;
    uint8 TargetsWhoHasMyAuras(std::vector<uint32> const& ids) {
        assert(ids == std::vector<uint32>({233490, 233496, 233497, 233498, 233499}));
        return targets;
    }
    Aura* GetAura(uint32 id) { assert(id == 208822); return buff.stacks ? &buff : nullptr; }
    void RemoveAurasDueToSpell(uint32 id) { assert(id == 208822); buff.stacks = 0; }
    void AddAura(uint32 id, Unit* owner, void*, uint8 stacks) {
        assert(id == 208822 && owner == this); buff.stacks = stacks;
    }
};
struct Driver {
    Unit* owner;
    AuraEffect effect;
    Unit* GetUnitOwner() { return owner; }
    AuraEffect* GetEffect(int index) { assert(index == EFFECT_0); return &effect; }
#include "StretenApply.inc"
#include "StretenUpdate.inc"
#include "StretenRemove.inc"
    void Tick() { OnUpdate(100, &effect); }
};
int main() {
    Unit player;
    Driver driver{&player};
    player.targets = 3; // UA predates equipping the item.
    driver.OnApply(&driver.effect, 0);
    driver.Tick();
    assert(player.buff.stacks == 3 && driver.effect.amount == 0);
    player.targets = 2;
    driver.effect.SetAmount(1); // Production UA removal callback.
    driver.Tick();
    assert(player.buff.stacks == 2);
    driver.OnRemove(&driver.effect, 0); // Unequip while UA still ticks.
    assert(player.buff.stacks == 0 && player.targets == 2);
    driver.OnApply(&driver.effect, 0);
    driver.Tick();
    assert(player.buff.stacks == 2);
    player.targets = 0;
    driver.effect.SetAmount(1);
    driver.Tick();
    assert(player.buff.stacks == 0);
    driver.OnRemove(&driver.effect, 0);
    driver.owner = nullptr;
    driver.OnApply(&driver.effect, 0);
    driver.Tick();
    driver.OnRemove(&driver.effect, 0);
    std::cout << "PASS: existing UA on equip, target changes, unequip, re-equip, final UA removal and absent owner.\n";
}
