#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
using uint32 = uint32_t;
using AuraEffectHandleModes = int;
constexpr int EFFECT_0 = 0, CLASS_MAGE = 8, CLASS_PRIEST = 5, CLASS_WARLOCK = 9;
struct AuraEffect {};
struct Aura {
    int duration = -1, maximum = -1;
    void SetDuration(int value) { duration = value; }
    void SetMaxDuration(int value) { maximum = value; }
};
struct Unit {
    int cls = CLASS_WARLOCK;
    bool moving = false, player = true, tank = false;
    float maxHealth = 1000000;
    std::map<uint32, Aura> auras;
    std::map<uint32, unsigned> casts;
    int getClass() const { return cls; }
    bool isMoving() const { return moving; }
    Unit* ToPlayer() { return player ? this : nullptr; }
    bool isInTankSpec() const { return tank; }
    float CountPctFromMaxHealth(float percent) { return maxHealth * percent / 100.f; }
    Aura* GetAura(uint32 id) { auto i = auras.find(id); return i == auras.end() ? nullptr : &i->second; }
    void RemoveAurasDueToSpell(uint32 id) { auras.erase(id); }
    void CastSpell(Unit* target, uint32 id, bool triggered) {
        assert(target == this && triggered);
        ++casts[id];
        auras[id] = id == 234797 ? Aura{6000, 6000} : Aura{-1, -1};
    }
    void Tick(uint32 diff) {
        for (auto i = auras.begin(); i != auras.end();) {
            if (i->second.duration >= 0) {
                i->second.duration = std::max(0, i->second.duration - int(diff));
                if (!i->second.duration) { i = auras.erase(i); continue; }
            }
            ++i;
        }
    }
};
struct Norgannon {
    Unit* unit;
    bool prevented = false;
    Unit* GetUnitOwner() { return unit; }
    void PreventDefaultAction() { prevented = true; }
#include "Norgannon.inc"
    void Tick(uint32 diff) { unit->Tick(diff); OnUpdate(diff, nullptr); }
};
struct EffectData { float CalcValue(Unit*) const { return 25; } };
struct SpellInfo { EffectData data; EffectData* Effects[1]{&data}; };
struct Prydaz {
    Unit* unit;
    SpellInfo info;
    Unit* GetCaster() { return unit; }
    SpellInfo* GetSpellInfo() { return &info; }
#include "Prydaz.inc"
};
int main() {
    for (auto [cls, spell] : {std::pair<int, uint32>{8, 236380}, {5, 236430}, {9, 236431}}) {
        Unit p; p.cls = cls;
        // Old saved helper auras must not bypass the fresh six-second charge.
        p.auras[236431] = {}; p.auras[236375] = {};
        Norgannon n{&p}; n.OnApply(nullptr, 0);
        assert(!p.GetAura(236431) && !p.GetAura(236375));
        n.Tick(5999); assert(!p.GetAura(spell));
        n.Tick(1); assert(p.GetAura(spell) && p.GetAura(spell)->duration == -1);
        n.Tick(60000); assert(p.GetAura(spell)->duration == -1 && p.casts[spell] == 1);
        p.moving = true; n.Tick(1);
        assert(p.GetAura(spell)->duration == 4000);
        n.Tick(3999); assert(p.GetAura(spell)->duration == 1);
        n.Tick(1); assert(!p.GetAura(spell));
        n.Tick(60000); assert(!p.GetAura(spell) && p.casts[spell] == 1);
        p.moving = false; n.Tick(50000); // A delayed update must not count previous movement as stillness.
        assert(!p.GetAura(spell));
        n.Tick(5999); assert(!p.GetAura(spell));
        p.moving = true; n.Tick(1);
        p.moving = false; n.Tick(1);
        n.Tick(5999); assert(!p.GetAura(spell));
        n.Tick(1); assert(p.GetAura(spell));
        p.moving = true; n.Tick(1); n.Tick(1000);
        p.moving = false; n.Tick(1); n.Tick(2999);
        assert(!p.GetAura(spell)); // Stopping does not pause the already-started four-second window.
        n.OnRemove(nullptr, 0); assert(p.auras.empty());
        p.moving = true; n.OnApply(nullptr, 0); n.Tick(60000);
        assert(!p.GetAura(spell)); // Equipping while moving grants nothing.
        p.moving = false; n.Tick(1); n.Tick(6000);
        assert(p.GetAura(spell));
        n.OnRemove(nullptr, 0); assert(p.auras.empty()); // Remove while ready.
        n.OnTick(nullptr); assert(n.prevented);
    }
    Unit unsupported; unsupported.cls = 1;
    Norgannon bad{&unsupported}; bad.OnApply(nullptr, 0); bad.Tick(60000); assert(unsupported.auras.empty());
    Norgannon absent{nullptr}; absent.OnApply(nullptr, 0); absent.OnUpdate(6000, nullptr); absent.OnRemove(nullptr, 0);
    Unit warlock;
    Prydaz shield{&warlock};
    bool recalculate = true;
    for (float hp : {100.f, 1000000.f, 5307600.f}) {
        warlock.maxHealth = hp;
        float amount = 0;
        shield.CalculateAmount(nullptr, amount, recalculate);
        assert(std::fabs(amount - hp * .25f) < 1.f);
        warlock.tank = true;
        shield.CalculateAmount(nullptr, amount, recalculate);
        assert(std::fabs(amount - hp * .15f) < 1.f);
        warlock.tank = false;
    }
    shield.unit = nullptr;
    float amount = 0; shield.CalculateAmount(nullptr, amount, recalculate); assert(amount == 0);
    std::cout << "PASS: Norgannon six-second charge, four-second movement lifetime, all three classes, movement resets, long updates, cleanup/re-equip; Prydaz 25%/15% health calculation.\n";
}
