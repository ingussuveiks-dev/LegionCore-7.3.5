#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>
using uint32 = uint32_t; using int32 = int32_t;
using ObjectGuid = uint32; using GuidList = std::vector<ObjectGuid>;
enum Powers { POWER_MANA, POWER_RAGE, POWER_FOCUS, POWER_ENERGY, POWER_COMBO_POINTS,
    POWER_RUNES, POWER_RUNIC_POWER, POWER_CHI = 12, POWER_HOLY_POWER = 9,
    POWER_MAELSTROM = 11, POWER_INSANITY = 13, POWER_FURY = 17, MAX_POWERS = 18, POWER_HEALTH = -2 };
constexpr int CHEAT_POWER = 1, SPELL_MISS_NONE = 0, SPELL_MISS_IMMUNE = 1,
    SPELLMOD_SPELL_COST_REFUND_ON_FAIL = 30, SPEC_DK_UNHOLY = 252, EFFECT_1 = 1;
#define TC_LOG_DEBUG(...) do {} while (false)
#define TC_LOG_ERROR(...) do {} while (false)
template<class T> T CalculatePct(T value, int percent) { return value * percent / 100; }
bool roll_chance_f(float) { return false; }
struct SpellPowerEntry { int PowerType; };
using SpellPowerData = std::vector<SpellPowerEntry const*>;
struct Effect { float BasePoints = 0; } effect;
struct SpellInfo
{
    uint32 Id = 116095;
    struct { int32 PowerCost = 0; } Power;
    Effect* Effects[2] = {&effect, &effect};
    SpellPowerData rows;
    bool NoPower() const { return rows.empty(); }
    template<class T> bool GetSpellPowerByCasterPower(T*, SpellPowerData& result) const { result = rows; return !rows.empty(); }
};
struct Aura { SpellInfo info; SpellInfo const* GetSpellInfo() { return &info; } };
struct Player
{
    std::map<int, int32> spent;
    bool cheat = false;
    bool IsPlayer() const { return true; }
    Player* ToPlayer() { return this; }
    bool GetCommandStatus(int) const { return cheat; }
    Player* GetSpellModOwner() { return this; }
    void ApplySpellMod(uint32, int, int32&) {}
    int GetSpecializationId() const { return 0; }
    bool HasSpell(uint32) const { return false; }
    GuidList* GetSummonList(uint32) { static GuidList list; return &list; }
    Aura* GetAura(uint32) { return nullptr; }
    void CastSpell(Player*, uint32, bool) {}
    bool IsAlive() const { return true; }
    void CastCustomSpell(Player*, uint32, float*, void*, void*, bool) {}
    void ModifyHealth(int32 amount) { spent[POWER_HEALTH] -= amount; }
    void ModifyPower(Powers type, int32 amount, bool, SpellInfo const*) { spent[type] -= amount; }
};
using Unit = Player;
struct ObjectAccessor { static Unit* GetUnit(Player&, ObjectGuid) { return nullptr; } };
struct Target { ObjectGuid targetGUID; int missCondition; };
struct Targets { ObjectGuid GetUnitTargetGUID() const { return 0; } };
struct Spell
{
    Player* m_caster;
    SpellInfo* m_spellInfo;
    void* m_CastItem = nullptr;
    void* m_triggeredByAuraSpell = nullptr;
    Targets m_targets;
    std::vector<Target*> m_UniqueTargetInfo;
    std::vector<int32> m_powerCost = std::vector<int32>(MAX_POWERS + 1);
    int hooks = 0;
    SpellInfo const* GetSpellInfo() const { return m_spellInfo; }
    int32 GetPowerCost(Powers type) const { return m_powerCost[type == POWER_HEALTH ? MAX_POWERS : type]; }
    void TakeRunePower() {}
    void CallScriptTakePowerHandlers(Powers, int32&) { ++hooks; }
    void TakePower();
};
#include "TakePower.inc"
int main()
{
    // DB2 can supply a conditional and an unconditional cost for one resource
    // (e.g. Disable 116095). CalcPowerCost stores one final cost per power type.
    SpellPowerEntry conditional{POWER_ENERGY}, base{POWER_ENERGY}, mana{POWER_MANA};
    SpellInfo info; info.rows = {&conditional, &base, &mana};
    Player player;
    Spell spell{&player, &info}; spell.m_powerCost[POWER_ENERGY] = 15; spell.m_powerCost[POWER_MANA] = 10;
    spell.TakePower();
    if (player.spent[POWER_ENERGY] != 15 || player.spent[POWER_MANA] != 10 || spell.hooks != 2)
    { std::cerr << "FAIL: each computed resource cost and its hook must run once per resource\n"; return 1; }
    player.spent.clear(); player.cheat = true; spell.TakePower();
    if (!player.spent.empty()) return 1;
    player.cheat = false; spell.m_CastItem = &player; spell.TakePower();
    if (!player.spent.empty()) return 1;
    std::cout << "PASS: resource payment deduplication, mixed resource costs and item/cheat exemptions\n";
}
