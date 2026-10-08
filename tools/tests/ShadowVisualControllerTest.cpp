#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <type_traits>
using uint32 = uint32_t;
using int32 = int32_t;
using AuraEffectHandleModes = int;
struct AuraEffect {};
struct SpellInfo {};
constexpr int POWER_INSANITY = 13;
struct Hook { void operator+=(int) {} };
struct Unit
{
    int32 insanity = 0;
    std::set<uint32> auras;
    std::map<uint32, int> casts;
    int32 GetPower(int) const { return insanity; }
    bool HasAura(uint32 id) const { return auras.count(id) != 0; }
    void CastSpell(Unit*, uint32 id, bool) { auras.insert(id); ++casts[id]; }
    void RemoveAurasDueToSpell(uint32 id) { auras.erase(id); }
};
struct AuraScript
{
    Unit* casterFixture = nullptr;
    Hook OnAuraUpdate, AfterEffectRemove, AfterEffectApply;
    virtual void Register() {}
    Unit* GetCaster() { return casterFixture; }
};
#define PrepareAuraScript(name) public:
#define AuraUpdateFn(...) 0
#define AuraEffectRemoveFn(...) 0
#define AuraEffectApplyFn(...) 0
#include "Shadowform.inc"
template<class T, class = void> struct HasApply : std::false_type {};
template<class T> struct HasApply<T, std::void_t<decltype(&T::OnApply)>> : std::true_type {};
template<class T> void Apply(T& script)
{
    if constexpr (HasApply<T>::value) script.OnApply(nullptr, 0);
}
int failures = 0;
void Check(bool ok, char const* message)
{
    if (!ok) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
}
int main()
{
    Unit priest;
    priest.insanity = 7500;
    spell_pri_shadowform script;
    script.casterFixture = &priest;
    Apply(script);
    Check(priest.auras == std::set<uint32>({185908, 185909, 185910, 185911}),
        "Shadowform must initialize its replacement visual controller immediately, including current Insanity");
    for (int32 power : {0, 2499, 2500, 4999, 5000, 7499, 7500, 10000, 7499, 4999, 2499, 0})
    {
        priest.insanity = power;
        script.OnUpdate(1000);
        Check(priest.HasAura(185908), "Base visual must remain while Shadowform is active");
        Check(priest.HasAura(185909) == (power >= 2500), "25 Insanity visual threshold");
        Check(priest.HasAura(185910) == (power >= 5000), "50 Insanity visual threshold");
        Check(priest.HasAura(185911) == (power >= 7500), "75 Insanity visual threshold");
        auto casts = priest.casts;
        script.OnUpdate(1000);
        Check(priest.casts == casts, "Unchanged visuals must not be cast repeatedly");
    }
    priest.insanity = 10000;
    script.OnUpdate(1000);
    script.OnRemove(nullptr, 0);
    Check(priest.auras.empty(), "Leaving Shadowform must remove all four staged visuals");
    priest.insanity = 0;
    Apply(script);
    Check(priest.auras == std::set<uint32>({185908}), "Re-entry must initialize from current power, not stale visuals");
    script.casterFixture = nullptr;
    Apply(script); script.OnUpdate(1000); script.OnRemove(nullptr, 0);
    if (failures) return 1;
    std::cout << "PASS: immediate Shadowform visuals, ascending/descending thresholds, cleanup and re-entry\n";
}
