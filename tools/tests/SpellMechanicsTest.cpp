#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <vector>
using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using int16 = int16_t;
using int32 = int32_t;
constexpr int CONFIG_PLAYER_ALLOW_PVP_TALENTS_ALL_THE_TIME = 1, PLAYERSPELL_REMOVED = 3,
    PLAYER_FIELD_CURRENT_SPEC_ID = 1, MAX_MASTERY_SPELLS = 2;
constexpr int ITEM_CLASS_WEAPON = 2, ITEM_CLASS_ARMOR = 4,
    SPELL_EFFECT_ENCHANT_ITEM = 53, SPELL_EFFECT_ENCHANT_ITEM_TEMPORARY = 54,
    SPELL_EFFECT_ENCHANT_ITEM_PRISMATIC = 156, ITEM_FLAG3_CAN_STORE_ENCHANTS = 1,
    SPELL_ATTR8_ARMOR_SPECIALIZATION = 8;
constexpr int INVTYPE_WEAPON = 13, INVTYPE_2HWEAPON = 17, INVTYPE_WEAPONMAINHAND = 21, INVTYPE_WEAPONOFFHAND = 22;
constexpr uint8 INVENTORY_SLOT_BAG_0 = 0, EQUIPMENT_SLOT_START = 0, EQUIPMENT_SLOT_HEAD = 0,
    EQUIPMENT_SLOT_SHOULDERS = 2, EQUIPMENT_SLOT_CHEST = 4, EQUIPMENT_SLOT_WAIST = 5,
    EQUIPMENT_SLOT_LEGS = 6, EQUIPMENT_SLOT_FEET = 7, EQUIPMENT_SLOT_WRISTS = 8,
    EQUIPMENT_SLOT_HANDS = 9, EQUIPMENT_SLOT_MAINHAND = 15, EQUIPMENT_SLOT_OFFHAND = 16,
    EQUIPMENT_SLOT_TABARD = 18;
constexpr int AURA_ATTR_IS_USING_CHARGES = 1, SPELL_ATTR1_CU_IS_USING_STACKS = 2,
    SPELL_AURA_ADD_FLAT_MODIFIER = 107, SPELL_AURA_ADD_PCT_MODIFIER = 108,
    SPELLMOD_STACK_AMOUNT = 1, SPELLMOD_STACK_AMOUNT2 = 2, MAX_SPELL_EFFECTS = 32;
using AuraRemoveMode = int;
constexpr AuraRemoveMode AURA_REMOVE_BY_DEFAULT = 0, AURA_REMOVE_BY_ENEMY_SPELL = 1;
#define TC_LOG_ERROR(...) do {} while (false)
struct AuraOptions { uint16 CumulativeAura = 0; };
struct SpellInfo
{
    uint32 SpellLevel = 1, EffectMask = 0;
    int32 EquippedItemClass = -1, EquippedItemSubClassMask = 0, EquippedItemInventoryTypeMask = 0;
    bool enchant = false, armorSpec = false, useStacks = false, refresh = true;
    AuraOptions options;
    bool HasEffect(int) const { return enchant; }
    bool HasAttribute(int attribute) const { return attribute == SPELL_ATTR8_ARMOR_SPECIALIZATION ? armorSpec : useStacks; }
    bool IsStack() const { return options.CumulativeAura > 1; }
    bool IsRefreshTimers() const { return refresh; }
    AuraOptions const* GetAuraOptions(int) const { return &options; }
};
struct ItemTemplate
{
    uint32 itemClass = ITEM_CLASS_ARMOR, subclass = 4, inventory = 1, flags = 0;
    uint32 GetClass() const { return itemClass; }
    uint32 GetSubClass() const { return subclass; }
    uint32 GetInventoryType() const { return inventory; }
    uint32 GetFlags3() const { return flags; }
};
struct Item
{
    ItemTemplate itemTemplate;
    ItemTemplate const* GetTemplate() const { return &itemTemplate; }
    bool IsFitToSpellRequirements(SpellInfo const*) const;
};
struct SpellProcEntry { uint32 modcharges = 0; };
struct SpellManager
{
    std::map<uint32, SpellInfo> spells;
    std::map<uint32, SpellProcEntry> proc;
    SpellInfo const* GetSpellInfo(uint32 id) { auto i = spells.find(id); return i == spells.end() ? nullptr : &i->second; }
    SpellProcEntry const* GetSpellProcEntry(uint32 id) { auto i = proc.find(id); return i == proc.end() ? nullptr : &i->second; }
} spellManager;
auto sSpellMgr = &spellManager;
struct World { bool always = false; bool getBoolConfig(int) const { return always; } } world;
auto sWorld = &world;
struct Map { bool dungeon = false; bool IsDungeon() { return dungeon; } };
struct PvpTalentEntry { uint32 SpellID, OverrideSpellID; };
struct SpecializationSpellsEntry { uint32 SpellID, OverridesSpellID; };
struct ChrSpecializationEntry { uint32 ID, MasterySpellID[2]; };
struct DB2Manager
{
    std::map<uint32, PvpTalentEntry> pvp;
    std::map<uint32, ChrSpecializationEntry> specs;
    std::map<uint32, std::vector<SpecializationSpellsEntry const*>> spells;
    PvpTalentEntry const* GetPvpTalentBySpellID(uint32 id) { auto i = pvp.find(id); return i == pvp.end() ? nullptr : &i->second; }
    ChrSpecializationEntry const* GetChrSpecializationByIndex(uint32, uint32 index) { auto i = specs.find(index); return i == specs.end() ? nullptr : &i->second; }
    std::vector<SpecializationSpellsEntry const*> const* GetSpecializationSpells(uint32 id) { auto i = spells.find(id); return i == spells.end() ? nullptr : &i->second; }
} sDB2Manager;
struct Player
{
    uint32 level = 110, active = 0, spec = 71, power = 0;
    Map map;
    std::map<uint32, std::map<uint32, int>> talents;
    std::set<uint32> known, auras;
    std::map<uint32, std::set<uint32>> m_overrideSpells, m_spellOverrides;
    std::map<uint8, Item*> equipped;
    uint32 getLevel() const { return level; }
    uint32 getClass() const { return 1; }
    uint32 GetUInt32Value(int) const { return spec; }
    Map* GetMap() { return &map; }
    uint32 GetActiveTalentGroup() const { return active; }
    auto* GetPvPTalentMap(uint32 group) { return &talents[group]; }
    void learnSpell(uint32 id, bool) { known.insert(id); }
    void removeSpell(uint32 id, bool, bool = true, bool = true) { known.erase(id); auras.erase(id); }
    void RemoveAurasDueToSpell(uint32 id) { auras.erase(id); }
    uint32 GetPowerTypeBySpecId(uint32 id) { return id; }
    void SetPowerType(uint32 type) { power = type; }
    void AddOverrideSpell(uint32, uint32);
    void RemoveOverrideSpell(uint32, uint32);
    void TogglePvpTalents(bool);
    void LearnSpecializationSpells();
    void RemoveSpecializationSpells();
    Item* GetUseableItemByPos(uint8, uint8 slot) const { auto i = equipped.find(slot); return i == equipped.end() ? nullptr : i->second; }
    bool HasItemFitToSpellRequirements(SpellInfo const*, Item const* = nullptr) const;
    template<class T> void ApplySpellMod(uint32, int, T&) {}
};
struct Unit { Player* GetSpellModOwner() { return nullptr; } };
struct SpellModifier { uint8 charges = 0; };
struct AuraEffect { int GetAuraType() { return 0; } SpellModifier* GetSpellModifier() { return nullptr; } };
struct Aura
{
    SpellInfo* m_spellInfo;
    uint32 id = 1;
    uint8 m_procCharges = 2;
    uint16 m_stackAmount = 1;
    bool usingCharges = false, removed = false;
    AuraRemoveMode removedMode = -1;
    int timerRefreshes = 0;
    std::vector<int> durationByStack;
    uint32 GetId() const { return id; }
    bool HasAuraAttribute(int) const { return usingCharges; }
    AuraEffect* GetEffect(int) { return nullptr; }
    uint8 CalcMaxCharges() const { return 2; }
    void Remove(AuraRemoveMode mode) { removed = true; removedMode = mode; }
    void SetCharges(uint8 value) { m_procCharges = value; }
    uint8 GetCharges() const { return m_procCharges; }
    void SetStackAmount(uint16 value) { m_stackAmount = value; }
    uint16 GetStackAmount() const { return m_stackAmount; }
    int GetSpawnMode() const { return 0; }
    Unit* GetCaster() const { return nullptr; }
    int GetMaxDuration() const { return 10000; }
    void RefreshSpellMods() {}
    void RefreshTimers() { ++timerRefreshes; }
    void SetNeedClientUpdateForTargets() {}
    bool ModCharges(int32, AuraRemoveMode = AURA_REMOVE_BY_DEFAULT);
    bool ModStackAmount(int16, AuraRemoveMode = AURA_REMOVE_BY_DEFAULT);
};
#include "TogglePvp.inc"
#include "LearnSpec.inc"
#include "RemoveSpec.inc"
#include "AddOverride.inc"
#include "RemoveOverride.inc"
#include "ItemRequirements.inc"
#include "EquippedRequirements.inc"
#include "ModCharges.inc"
#include "ModStacks.inc"
int failures = 0;
void Check(bool ok, char const* text) { if (!ok) { ++failures; std::cerr << "FAIL: " << text << '\n'; } }
int main()
{
    sDB2Manager.pvp[100] = {100, 10};
    sDB2Manager.pvp[101] = {101, 11};
    for (bool always : {false, true})
    {
        world.always = always;
        Player p;
        p.talents[0] = {{100, 0}, {101, PLAYERSPELL_REMOVED}, {999, 0}};
        p.talents[1] = {{101, 0}};
        p.TogglePvpTalents(true);
        Check(p.known.count(100) && !p.known.count(101), "Only selected PvP talents may be learned, even with always-on configuration");
        Check(!p.m_overrideSpells.count(11), "Removed PvP talent must not restore override");
        p.TogglePvpTalents(false);
        Check(bool(p.known.count(100)) == always, "Always-on policy must preserve selected talents outside PvP");
        Check(!p.known.count(101), "Disabling must never resurrect a removed talent");
    }
    world.always = false;
    Player p;
    p.talents[0] = {{100, 0}};
    p.map.dungeon = true;
    p.TogglePvpTalents(true);
    Check(p.known.empty(), "Dungeon restriction must remain");
    p.map.dungeon = false; p.level = 109;
    p.TogglePvpTalents(true);
    Check(p.known.empty(), "Level restriction must remain");

    SpecializationSpellsEntry oldSpec{200, 20}, shared{201, 0}, newSpec{202, 20}, tooHigh{203, 0}, missing{204, 0};
    sDB2Manager.specs = {{0, {71, {300, 0}}}, {1, {72, {301, 0}}}};
    sDB2Manager.spells[71] = {&oldSpec, &shared};
    sDB2Manager.spells[72] = {&newSpec, &shared, &tooHigh, &missing};
    for (uint32 id : {200u, 201u, 202u, 203u}) spellManager.spells[id] = {};
    spellManager.spells[203].SpellLevel = 111;
    Player s;
    s.known = {20, 999}; s.auras = {300, 999};
    s.LearnSpecializationSpells();
    Check(s.known.count(200) && s.known.count(201) && s.m_overrideSpells[20].count(200), "Specialization learns spells and override");
    s.RemoveSpecializationSpells(); s.spec = 72; s.LearnSpecializationSpells();
    Check(!s.known.count(200) && s.known.count(201) && s.known.count(202) && s.known.count(999), "Spec transition removes old and restores shared spells");
    Check(!s.auras.count(300) && s.auras.count(999) && !s.known.count(203) && !s.known.count(204), "Mastery cleanup, unrelated auras and level guards");
    Check(s.m_overrideSpells[20] == std::set<uint32>{202} && !s.m_spellOverrides.count(200), "Both override indexes must match new spec");

    SpellInfo requirement;
    requirement.EquippedItemClass = ITEM_CLASS_ARMOR;
    requirement.EquippedItemSubClassMask = 1 << 4;
    requirement.armorSpec = true;
    Player wearer;
    Item armor[8];
    uint8 slots[] = {0, 2, 4, 5, 6, 7, 8, 9};
    for (size_t i = 0; i < 8; ++i) wearer.equipped[slots[i]] = &armor[i];
    Check(wearer.HasItemFitToSpellRequirements(&requirement), "Eight matching armor pieces must satisfy armor specialization");
    Check(!wearer.HasItemFitToSpellRequirements(&requirement, &armor[0]), "Unequipped/ignored armor must break specialization");
    armor[0].itemTemplate.subclass = 3;
    Check(!wearer.HasItemFitToSpellRequirements(&requirement), "One wrong armor subclass must fail");
    armor[0].itemTemplate.subclass = 4;
    wearer.equipped.erase(0);
    Check(!wearer.HasItemFitToSpellRequirements(&requirement), "Missing armor slot must fail");
    requirement.armorSpec = false;
    Check(wearer.HasItemFitToSpellRequirements(&requirement), "Ordinary armor requirement needs only one matching piece");
    requirement.enchant = true; requirement.EquippedItemInventoryTypeMask = 1 << 11;
    Check(!armor[0].IsFitToSpellRequirements(&requirement), "Enchant target must match inventory mask");
    armor[0].itemTemplate.inventory = 11;
    Check(armor[0].IsFitToSpellRequirements(&requirement), "Matching enchant target must pass");
    armor[0].itemTemplate.flags = ITEM_FLAG3_CAN_STORE_ENCHANTS; armor[0].itemTemplate.itemClass = 7;
    Check(armor[0].IsFitToSpellRequirements(&requirement), "Enchant vellum exception must remain");

    SpellInfo auraInfo;
    auraInfo.options.CumulativeAura = 3;
    Aura aura{&auraInfo};
    aura.ModStackAmount(9);
    Check(aura.GetStackAmount() == 3, "Stack increase must respect DB2 limit");
    int refreshes = aura.timerRefreshes;
    aura.ModStackAmount(-1);
    Check(aura.GetStackAmount() == 2 && aura.timerRefreshes == refreshes, "Stack loss must not refresh duration");
    Check(aura.ModStackAmount(-2) && aura.removed, "Last stack removal must report removal");
    auraInfo.useStacks = true;
    aura = Aura{&auraInfo}; aura.usingCharges = true;
    Check(aura.ModCharges(-1, AURA_REMOVE_BY_ENEMY_SPELL) && aura.removed && aura.removedMode == AURA_REMOVE_BY_ENEMY_SPELL,
        "Charge-to-stack redirection must preserve removal result and mode");
    auraInfo.useStacks = false; auraInfo.options.CumulativeAura = 0;
    aura = Aura{&auraInfo}; aura.m_procCharges = 1; aura.usingCharges = true;
    spellManager.proc[1] = {1};
    Check(aura.ModStackAmount(-1, AURA_REMOVE_BY_ENEMY_SPELL) && aura.removed && aura.removedMode == AURA_REMOVE_BY_ENEMY_SPELL,
        "Stack-to-charge redirection must report last charge removal");
    spellManager.proc.clear();
    aura = Aura{&auraInfo}; aura.ModCharges(50);
    Check(aura.GetCharges() == 2 && !aura.removed, "Charge increase must respect limit");
    if (failures) return 1;
    std::cout << "PASS: PvP selection, specialization/override lifecycle, equipment/enchant requirements and stack/charge removal\n";
}
