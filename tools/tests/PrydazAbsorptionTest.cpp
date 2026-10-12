#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <vector>
using uint32 = uint32_t;
using int32 = int32_t;
using int64 = int64_t;
using uint8 = uint8_t;
using SpellSchoolMask = unsigned;
constexpr int SPELL_AURA_SCHOOL_ABSORB = 69, SPELL_ATTR6_NOT_LIMIT_ABSORB = 1;
constexpr int SPELL_AURA_MANA_SHIELD = 97, POWER_MANA = 0;
constexpr int SCORE_HEALING_DONE = 0, CRITERIA_TYPE_HEALING_DONE = 0, CRITERIA_TYPE_HIGHEST_HEAL_CASTED = 0, AURA_REMOVE_BY_ENEMY_SPELL = 0;
struct DamageInfo {
    uint32 m_damage, m_absorb = 0;
    uint32 GetDamage() const { return m_damage; }
    void ModifyDamage(int32 amount);
    void AbsorbDamage(int32 amount);
};
#include "DamageAbsorb.inc"
struct AuraApplication {};
struct AuraEffect;
struct Unit;
using Player = Unit;
struct Battleground { void UpdatePlayerScore(Player*, int, float) {} };
struct SpellInfo {
    struct Effect { float CalcValueMultiplier(Unit*) const { return 1; } } effect;
    Effect* GetEffect(int, int) { return &effect; }
    bool HasAttribute(int) const { return false; } // Native 207472 has no unlimited-absorb flag.
    bool IsPassive() const { return false; }
};
struct Aura {
    bool active = true;
    AuraApplication application;
    AuraApplication* GetApplicationOfTarget(int) { return active ? &application : nullptr; }
    void CallScriptEffectAbsorbHandlers(AuraEffect*, AuraApplication const*, DamageInfo&, float&, bool&) {}
    void CallScriptEffectAfterAbsorbHandlers(AuraEffect*, AuraApplication const*, DamageInfo&, float) {}
    void CallScriptEffectManaShieldHandlers(AuraEffect*, AuraApplication const*, DamageInfo&, float&, bool&) {}
    void CallScriptEffectAfterManaShieldHandlers(AuraEffect*, AuraApplication const*, DamageInfo&, float&) {}
    void Remove(int) { active = false; }
};
struct AuraEffect {
    float amount;
    Aura aura;
    SpellInfo spell;
    bool manaShield = false;
    int GetEffIndex() const { return 0; }
    Aura* GetBase() { return &aura; }
    unsigned GetMiscValue() const { return 127; } // Native 207472: all seven damage schools.
    float GetAmount() const { return amount; }
    void SetAmount(float value) { amount = value; }
    Unit* GetCaster() { return nullptr; }
    SpellInfo* GetSpellInfo() { return &spell; }
};
struct Unit {
    std::vector<AuraEffect*> effects;
    int mana = 1000000;
    int32 ModifyPower(int, int32 delta) { int32 taken = std::max(delta, -mana); mana += taken; return taken; }
    std::vector<AuraEffect*> GetAuraEffectsByType(int type) {
        std::vector<AuraEffect*> result;
        for (auto effect : effects) if (effect->manaShield == (type == SPELL_AURA_MANA_SHIELD)) result.push_back(effect);
        return result;
    }
    int GetGUID() const { return 1; }
    Player* ToPlayer() { return this; }
    Battleground* GetBattleground() { return nullptr; }
    template<class... Args> void UpdateAchievementCriteria(Args...) {}
};
auto CopyAuraEffectList(std::vector<AuraEffect*> const& value) { return value; }
void AddPct(float& value, float percent) { value += value * percent / 100.f; }
float RoundToInterval(float value, float lo, float hi) { return std::clamp(value, lo, hi); }
float RoundingFloatValue(float value) { return std::round(value); }
int GetSpawnMode() { return 0; }
void Resolve(Unit* victim, SpellSchoolMask schoolMask, DamageInfo& dmgInfo, float auraAbsorbMod = 0) {
#include "SchoolAbsorb.inc"
}
int main() {
    for (unsigned school : {1u, 2u, 4u, 8u, 16u, 32u, 64u, 127u}) {
        AuraEffect shield{250000}; Unit player{{&shield}};
        DamageInfo first{100000}; Resolve(&player, school, first);
        assert(first.m_damage == 0 && first.m_absorb == 100000 && shield.amount == 150000 && shield.aura.active);
        DamageInfo second{200000}; Resolve(&player, school, second);
        assert(second.m_damage == 50000 && second.m_absorb == 150000 && shield.amount == 0 && !shield.aura.active);
        DamageInfo third{1234}; Resolve(&player, school, third);
        assert(third.m_damage == 1234 && third.m_absorb == 0);
    }
    AuraEffect shield{250000}; Unit player{{&shield}};
    DamageInfo zero{0}; Resolve(&player, 1, zero); assert(shield.amount == 250000);
    DamageInfo bypass{100000}; Resolve(&player, 1, bypass, 50);
    assert(bypass.m_damage == 50000 && bypass.m_absorb == 50000 && shield.amount == 200000);
    AuraEffect small{10000}, secondShield{250000}; Unit stacked{{&small, &secondShield}};
    DamageInfo sharedBudget{100000}; Resolve(&stacked, 1, sharedBudget, 50);
    assert(sharedBudget.m_damage == 50000 && sharedBudget.m_absorb == 50000 && secondShield.amount == 210000);
    AuraEffect physical{10000}, mana{250000}; mana.manaShield = true;
    Unit hybrid{{&physical, &mana}};
    DamageInfo mixed{100000}; Resolve(&hybrid, 1, mixed, 50);
    assert(mixed.m_damage == 50000 && mixed.m_absorb == 50000 && mana.amount == 210000 && hybrid.mana == 960000);
    DamageInfo fullBypass{100000}; Resolve(&player, 1, fullBypass, 100);
    assert(fullBypass.m_damage == 100000 && fullBypass.m_absorb == 0 && shield.amount == 200000);
    Unit empty;
    DamageInfo maximum{std::numeric_limits<uint32>::max()}; Resolve(&empty, 1, maximum, 100);
    assert(maximum.m_damage == std::numeric_limits<uint32>::max() && maximum.m_absorb == 0);
    DamageInfo modification{0}; modification.ModifyDamage(123); assert(modification.m_damage == 123);
    modification.ModifyDamage(-999); assert(modification.m_damage == 0);
    maximum.ModifyDamage(1); assert(maximum.m_damage == std::numeric_limits<uint32>::max());
    std::cout << "PASS: production school-absorb loop consumes Prydaz capacity across all schools, caps absorption to damage, removes depleted shield, preserves overflow and handles bypass.\n";
}
