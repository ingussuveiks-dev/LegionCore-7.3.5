#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <vector>
using uint8 = uint8_t; using int8 = int8_t; using uint32 = uint32_t; using int32 = int32_t;
using uint64 = uint64_t; using int64 = int64_t;
constexpr int IN_MILLISECONDS = 1000, MAX_POWERS = 18, MAX_POWERS_FOR_SPELL = 4;
enum Powers { POWER_MANA = 0, POWER_RUNES = 5, POWER_HOLY_POWER = 9, POWER_HEALTH = -2 };
enum SpellCastResult { SPELL_CAST_OK, SPELL_FAILED_CASTER_AURASTATE, SPELL_FAILED_NO_POWER };
constexpr int SPELLMOD_COST = 1, SPELL_AURA_MOD_COOLDOWN = 2, SPELL_AURA_CHARGE_RECOVERY_MOD = 3,
    SPELL_AURA_CHARGE_RECOVERY_MULTIPLIER = 4, SPELL_AURA_CHARGE_RECOVERY_AFFECTED_BY_HASTE_REGEN = 5,
    UNIT_FIELD_MOD_TIME_RATE = 6;
#define TC_LOG_ERROR(...) do {} while (false)
struct SpellPowerEntry { int32 PowerType; };
struct SpellInfo
{
    uint32 Id = 1;
    struct { uint32 ChargeCategory = 10; } Categories;
    std::vector<SpellPowerEntry const*> powers;
    bool NoPower() const { return powers.empty(); }
    bool IsPowerActive(uint8 index) const { return index < powers.size(); }
    SpellPowerEntry const* GetPowerInfo(uint8 index) const { return powers.at(index); }
    template<class T> bool GetSpellPowerByCasterPower(T*, std::vector<SpellPowerEntry const*>& result) const
    { result.insert(result.end(), powers.begin(), powers.end()); return !powers.empty(); }
};
struct SpellCategoryEntry { uint32 ID = 10, MaxCharges = 2, ChargeRecoveryTime = 1000; } categoryFixture;
struct CategoryStore { SpellCategoryEntry const* LookupEntry(uint32 id) { return id == categoryFixture.ID ? &categoryFixture : nullptr; } } sSpellCategoryStore;
struct SpellManager
{
    SpellInfo spell;
    SpellInfo const* GetSpellInfo(uint32 id) { return id == spell.Id ? &spell : nullptr; }
} spellManager;
auto sSpellMgr = &spellManager;
struct SpellChargeData
{
    int8 charges; int8 maxCharges; SpellCategoryEntry const* categoryEntry;
    uint32 timer; uint32 chargeRegenTime; float speed = 1.0f; SpellInfo const* spellInfo = nullptr;
};
using SpellChargeDataMap = std::map<uint32, SpellChargeData>;
namespace WorldPackets { namespace Spells {
struct SpellChargeEntry { uint32 Category = 0, NextRecoveryTime = 0; int ConsumedCharges = 0; };
struct SetSpellCharges : SpellChargeEntry { SetSpellCharges* Write() { return this; } };
struct SendSpellCharges { std::vector<SpellChargeEntry> Entries; SendSpellCharges* Write() { return this; } };
}}
namespace GameTime {
uint64 now = 100000;
auto GetSystemTime() { return std::chrono::system_clock::time_point(std::chrono::milliseconds(now)); }
}
enum { CHAR_DEL_CHARACTER_SPELL_CHARGES, CHAR_INS_CHARACTER_SPELL_CHARGES };
struct Field
{
    uint64 value;
    uint32 GetUInt32() const { return uint32(value); }
    uint64 GetUInt64() const { return value; }
};
struct QueryResult
{
    std::vector<std::vector<Field>> rows;
    size_t cursor = 0;
    Field* Fetch() { return rows.at(cursor).data(); }
    bool NextRow() { return ++cursor < rows.size(); }
};
using PreparedQueryResult = std::shared_ptr<QueryResult>;
struct CharacterDatabasePreparedStatement
{
    int id;
    uint64 args[6]{};
    void setUInt64(int pos, uint64 value) { args[pos] = value; }
    void setUInt32(int pos, uint32 value) { args[pos] = value; }
    void setUInt8(int pos, uint8 value) { args[pos] = value; }
};
struct Database
{
    CharacterDatabasePreparedStatement* GetPreparedStatement(int id) { return new CharacterDatabasePreparedStatement{id}; }
} CharacterDatabase;
struct Transaction
{
    std::vector<CharacterDatabasePreparedStatement> statements;
    void Append(CharacterDatabasePreparedStatement* stmt) { statements.push_back(*stmt); delete stmt; }
};
using CharacterDatabaseTransaction = std::shared_ptr<Transaction>;
struct Player
{
    SpellChargeDataMap m_spellChargeData;
    int32 flat = 0, cooldown = 0;
    float multiplier = 1.0f;
    uint32 health = 100;
    std::map<int32, int32> powers;
    std::vector<WorldPackets::Spells::SpellChargeEntry> packets;
    int holyChecks = 0, ripple = 0;
    uint8 GetMaxSpellCategoryCharges(SpellCategoryEntry const* c) const { return uint8(c->MaxCharges); }
    int32 GetTotalAuraModifierByMiscValue(int, uint32) const { return flat; }
    int32 GetTotalAuraModifier(int) const { return cooldown; }
    float GetTotalAuraMultiplierByMiscValue(int, uint32) const { return 1.0f; }
    float GetFloatValue(int) const { return 1.0f; }
    float SpellCooldownModByRate(SpellInfo const*, bool) { return multiplier; }
    bool HasAura(uint32) const { return false; }
    void CastSpell(Player*, uint32, bool) { ++ripple; }
    void SendDirectMessage(WorldPackets::Spells::SetSpellCharges* p) { packets.push_back(*p); }
    void SendDirectMessage(WorldPackets::Spells::SendSpellCharges* p) { packets = p->Entries; }
    uint32 GetHealth() const { return health; }
    uint64 GetGUIDLow() const { return 42; }
    int32 GetPower(Powers p) { return powers[p]; }
    uint8 HandleHolyPowerCost(int32 cost, SpellPowerEntry const*) { ++holyChecks; return uint8(cost); }
    Player* GetSpellModOwner() { return this; }
    void ApplySpellMod(uint32, int, int32&) {}
    uint32 GetSpellCategoryChargesTimer(SpellCategoryEntry const*, SpellInfo const*, bool = false) const;
    void TakeSpellCharge(SpellInfo const*);
    void UpdateSpellCharges(uint32);
    void ModSpellCharge(uint32, int32);
    void ModSpellChargeCooldown(uint32, int32);
    void SendSpellChargeData();
    uint32 GetChargesCooldown(uint32) const;
    void _LoadSpellCharges(PreparedQueryResult);
    void _SaveSpellCharges(CharacterDatabaseTransaction&);
};
struct Spell
{
    Player* m_caster; SpellInfo const* m_spellInfo;
    void* m_CastItem = nullptr;
    std::vector<SpellPowerEntry const*> m_powerData;
    std::vector<int32> m_powerCost = std::vector<int32>(MAX_POWERS + 1);
    int32 m_failedArg[1]{};
    SpellInfo const* GetSpellInfo() { return m_spellInfo; }
    int32 GetPowerCost(int32 type) const { return m_powerCost[type == POWER_HEALTH ? MAX_POWERS : type]; }
    SpellCastResult CheckPower();
};
#include "GetChargeTimer.inc"
#include "TakeSpellCharge.inc"
#include "UpdateSpellCharges.inc"
#include "ModSpellCharge.inc"
#include "ModSpellChargeCooldown.inc"
#include "SendSpellChargeData.inc"
#include "GetChargeCooldown.inc"
#include "CheckPower.inc"
#include "LoadCharges.inc"
#include "SaveCharges.inc"
struct SpecializationSpellsEntry { uint32 ID; };
struct SpecializationStore
{
    std::set<uint32> ids;
    bool LookupEntry(uint32 id) const { return ids.count(id) != 0; }
} sSpecializationSpellsStore;
std::map<uint32, std::vector<SpecializationSpellsEntry const*>> _specializationSpellsBySpec;
void PruneSpecializationLinks()
{
#include "PruneLinks.inc"
}
int failures = 0;
void Check(bool ok, char const* message) { if (!ok) { ++failures; std::cerr << "FAIL: " << message << '\n'; } }
int main()
{
    auto info = &spellManager.spell;
    Player p;
    p.flat = -1500;
    Check(p.GetSpellCategoryChargesTimer(&categoryFixture, info) == 0, "Negative recovery modifier must clamp to zero, not wrap");
    p.flat = 0;
    p.ModSpellCharge(info->Id, -5);
    auto& data = p.m_spellChargeData.at(categoryFixture.ID);
    Check(data.spellInfo == info, "Script-created charge recovery must retain spell pointer");
    Check(data.charges == 0, "Script charge consumption must clamp at zero");
    // Do not dereference the old missing pointer while recording the remaining failures.
    data.spellInfo = info; data.charges = 0;
    p.multiplier = 2.0f;
    p.UpdateSpellCharges(1000);
    Check(data.charges == 1 && data.timer == 0 && data.chargeRegenTime == 2000,
        "Completed recovery spends the OLD interval before the next interval is recalculated");
    data.timer = 100; data.charges = 0;
    p.ModSpellChargeCooldown(info->Id, -200);
    Check(data.timer == 0 && data.charges == 0, "Negative progress cannot wrap and award a charge");
    data.timer = 0; data.charges = 0; data.chargeRegenTime = 1000; p.multiplier = 1;
    p.ModSpellChargeCooldown(info->Id, 2500);
    Check(data.charges == 2 && data.timer == 0 && p.packets.back().NextRecoveryTime == 0,
        "Large cooldown reduction must restore every elapsed charge and report no remaining cooldown at full charges");
    data.charges = 1; data.timer = 500; data.chargeRegenTime = 2000;
    p.SendSpellChargeData();
    Check(p.packets.back().NextRecoveryTime == 1500, "Charge snapshot must use effective recovery duration");
    data.charges = 2; data.timer = 0;
    Check(p.GetChargesCooldown(info->Id) == 0, "Full charge category has no remaining recovery");
    SpellPowerEntry health{POWER_HEALTH}, mana{POWER_MANA}, holy{POWER_HOLY_POWER};
    SpellInfo mixed; mixed.powers = {&health, &mana};
    Player caster; caster.powers[POWER_MANA] = 0;
    Spell spell{&caster, &mixed}; spell.m_powerCost[MAX_POWERS] = 10; spell.m_powerCost[POWER_MANA] = 20;
    Check(spell.CheckPower() == SPELL_FAILED_NO_POWER, "Health requirement must not bypass another resource requirement");
    caster.powers[POWER_MANA] = 20;
    Check(spell.CheckPower() == SPELL_CAST_OK, "Mixed health and mana requirements can succeed");
    caster.health = 10;
    Check(spell.CheckPower() == SPELL_FAILED_CASTER_AURASTATE, "Health cost cannot kill caster during preflight");
    mixed.powers = {&holy}; caster.powers[POWER_HOLY_POWER] = 3; spell.m_powerCost.assign(MAX_POWERS + 1, 0); spell.m_powerCost[POWER_HOLY_POWER] = 3;
    spell.m_powerData.clear(); caster.holyChecks = 0;
    spell.CheckPower(); spell.CheckPower();
    Check(spell.m_powerData.size() == 1 && caster.holyChecks == 2, "Repeated preflight must not accumulate power rows");
    Player saved;
    saved.TakeSpellCharge(info); saved.TakeSpellCharge(info); saved.UpdateSpellCharges(250);
    auto transaction = std::make_shared<Transaction>();
    saved._SaveSpellCharges(transaction);
    Check(transaction->statements.size() == 2 && transaction->statements[0].id == CHAR_DEL_CHARACTER_SPELL_CHARGES,
        "Charge save must replace previous rows transactionally");
    auto row = transaction->statements.back();
    Check(row.args[3] == 2 && row.args[4] == 100750 && row.args[5] == 1000,
        "Save must retain missing charges and millisecond recovery progress");
    auto result = [&]() {
        auto query = std::make_shared<QueryResult>();
        query->rows.push_back({{row.args[1]}, {row.args[2]}, {row.args[3]}, {row.args[4]}, {row.args[5]}});
        return query;
    };
    Player restored; restored._LoadSpellCharges(result());
    Check(restored.m_spellChargeData.at(10).charges == 0 && restored.GetChargesCooldown(info->Id) == 750,
        "Immediate relog must not reset charges or partial recovery");
    GameTime::now = 101100;
    Player partial; partial._LoadSpellCharges(result());
    Check(partial.m_spellChargeData.at(10).charges == 1 && partial.GetChargesCooldown(info->Id) == 650,
        "Offline time must restore only elapsed charge intervals");
    GameTime::now = 102000;
    Player full; full._LoadSpellCharges(result());
    Check(full.m_spellChargeData.empty(), "Fully recovered offline category needs no outstanding record");
    row.args[2] = 999;
    Player unknown; unknown._LoadSpellCharges(result());
    Check(unknown.m_spellChargeData.empty(), "Missing saved spell must be ignored safely");
    SpecializationSpellsEntry stale{4946}, live{5434}, other{2275};
    sSpecializationSpellsStore.ids = {5434, 2275};
    _specializationSpellsBySpec[258] = {&stale, &live};
    _specializationSpellsBySpec[71] = {&other};
    PruneSpecializationLinks();
    Check(_specializationSpellsBySpec[258] == std::vector<SpecializationSpellsEntry const*>{&live} &&
        _specializationSpellsBySpec[71] == std::vector<SpecializationSpellsEntry const*>{&other},
        "Tombstones must remove cached links without dropping live specialization entries");
    if (failures) return 1;
    std::cout << "PASS: resource preflight, charge initialization, recovery changes, clamping and packets\n";
}
