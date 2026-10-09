#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <vector>
using uint8 = uint8_t;
using uint32 = uint32_t;
constexpr int PLAYER_FIELD_CURRENT_SPEC_ID = 1, ITEM_SET_FLAG_LEGACY_INACTIVE = 1;
constexpr int ITEM_SPELLTRIGGER_ON_EQUIP = 1, SPELL_AURA_MOD_XP_PCT = 1;
constexpr uint8 INVENTORY_SLOT_BAG_END = 1;
#define ASSERT assert
struct ItemSetEntry { uint32 RequiredSkill = 197, RequiredSkillRank = 300, SetFlags = 0; };
struct ItemSetSpellEntry { uint32 SpellID, Threshold, ChrSpecID = 0; };
struct ItemEffectEntry { uint32 SpellID; int TriggerType = 1; uint32 ChrSpecializationID = 0; };
struct ItemTemplate {
    uint32 set = 421;
    std::vector<ItemEffectEntry const*> Effects;
    uint32 GetItemSet() const { return set; }
};
struct Item {
    ItemTemplate data;
    ItemTemplate const* GetTemplate() const { return &data; }
    uint32 GetEntry() const { return 100; }
    bool CantBeUse() const { return false; }
};
struct SpellInfo { uint32 Id; bool xp = false; bool HasAura(int) const { return xp; } };
struct SpellManager {
    std::map<uint32, SpellInfo> spells;
    SpellInfo const* GetSpellInfo(uint32 id) { auto i = spells.find(id); return i == spells.end() ? nullptr : &i->second; }
    SpellInfo const* AssertSpellInfo(uint32 id) { auto s = GetSpellInfo(id); assert(s); return s; }
} spellManager;
auto sSpellMgr = &spellManager;
struct SetStore {
    std::map<uint32, ItemSetEntry> rows;
    ItemSetEntry const* LookupEntry(uint32 id) { auto i = rows.find(id); return i == rows.end() ? nullptr : &i->second; }
} sItemSetStore;
struct DB2Manager {
    using ItemSetSpells = std::vector<ItemSetSpellEntry const*>;
    std::map<uint32, ItemSetSpells> _itemSetSpells;
    bool heirloom = false;
    bool GetHeirloomByItemId(uint32) const { return heirloom; }
} sDB2Manager;
struct ItemSetEffect { uint32 ItemSetID = 0, EquippedItemCount = 0; std::set<ItemSetSpellEntry const*> SetBonuses; };
struct Collection { bool eligible = true; bool CanApplyHeirloomXpBonus(uint32, uint32) const { return eligible; } };
struct Player {
    std::vector<ItemSetEffect*> sets;
    std::vector<ItemSetEffect*>* ItemSetEff = &sets;
    std::map<uint32, uint32> skills;
    uint32 spec = 258;
    Collection collection;
    std::set<uint32> active;
    uint32 casts = 0;
    Item* m_items[INVENTORY_SLOT_BAG_END]{};
    ~Player() { for (auto s : sets) delete s; }
    uint32 GetSkillValue(uint32 id) const { auto i = skills.find(id); return i == skills.end() ? 0 : i->second; }
    uint32 GetUInt32Value(int) const { return spec; }
    Collection* GetCollectionMgr() { return &collection; }
    uint32 getLevel() const { return 110; }
    int GetAttackBySlot(uint8) const { return 0; }
    bool CanUseAttackType(int) const { return true; }
    void ApplyEquipSpell(SpellInfo const* s, Item*, bool apply, bool form = false) {
        if (apply) { if (!form || !active.count(s->Id)) ++casts; active.insert(s->Id); }
        else if (!form) active.erase(s->Id);
    }
    void ApplyItemEquipSpell(Item*, bool, bool = false);
    void UpdateEquipSpellsAtFormChange();
};
#include "AddSet.inc"
#include "RemoveSet.inc"
#include "SetSkillBonus.inc"
#include "ItemEquip.inc"
#include "RefreshSet.inc"
int failures = 0;
#define CHECK(x) do { if (!(x)) { ++failures; std::cerr << "FAIL: " #x "\n"; } } while (false)
int main() {
    sItemSetStore.rows[421] = {};
    ItemSetSpellEntry two{1000, 2}, three{1001, 3, 258}, otherSpec{1002, 2, 259};
    sDB2Manager._itemSetSpells[421] = {&two, &three, &otherSpec};
    for (uint32 id : {1000u, 1001u, 1002u}) spellManager.spells[id] = {id};
    Item item;
    for (uint32 rank : {0u, 197u, 299u, 300u, 350u}) {
        Player p; p.skills[197] = rank;
        AddItemsSetItem(&p, &item); AddItemsSetItem(&p, &item); AddItemsSetItem(&p, &item);
        CHECK(p.active.count(1000) == (rank >= 300));
        CHECK(p.active.count(1001) == (rank >= 300));
        CHECK(!p.active.count(1002));
        RemoveItemsSetItem(&p, item.GetTemplate());
        CHECK(!p.active.count(1001));
        CHECK(p.active.count(1000) == (rank >= 300));
        RemoveItemsSetItem(&p, item.GetTemplate()); RemoveItemsSetItem(&p, item.GetTemplate());
        CHECK(p.active.empty());
    }
    ItemEffectEntry xp{2000}, equip{2001}, use{2002, 0};
    {
        Player p; p.skills[197] = 299;
        AddItemsSetItem(&p, &item); AddItemsSetItem(&p, &item); AddItemsSetItem(&p, &item);
        CHECK(p.sets[0]->EquippedItemCount == 3 && p.active.empty());
        p.skills[197] = 300; UpdateItemSetSkill(&p, 197, 300);
        CHECK(p.active == std::set<uint32>({1000, 1001}));
        auto casts = p.casts; UpdateItemSetSkill(&p, 197, 315);
        CHECK(p.casts == casts); // bonus changes above the threshold do not refresh
        UpdateItemSetSkill(&p, 164, 0); CHECK(p.active.size() == 2);
        UpdateItemSetSkill(&p, 197, 0); // removal runs before the stored rank is cleared
        CHECK(p.active.empty() && p.sets[0]->EquippedItemCount == 3);
        p.skills[197] = 0; p.UpdateEquipSpellsAtFormChange(); CHECK(p.active.empty());
        p.skills[197] = 300; UpdateItemSetSkill(&p, 197, 300);
        CHECK(p.active.size() == 2);
        p.spec = 259; p.UpdateEquipSpellsAtFormChange();
        CHECK(p.active == std::set<uint32>({1000, 1002}));
        UpdateItemSetSkill(&p, 197, 299); CHECK(p.active.empty());
        RemoveItemsSetItem(&p, item.GetTemplate()); RemoveItemsSetItem(&p, item.GetTemplate());
        UpdateItemSetSkill(&p, 197, 300); CHECK(p.active.empty());
        RemoveItemsSetItem(&p, item.GetTemplate()); CHECK(p.sets[0] == nullptr);
    }
    spellManager.spells[2000] = {2000, true};
    spellManager.spells[2001] = {2001}; spellManager.spells[2002] = {2002};
    item.data.Effects = {&xp, &equip, &use};
    Player p; sDB2Manager.heirloom = true;
    p.ApplyItemEquipSpell(&item, true); CHECK(p.active.count(2000) && p.active.count(2001) && !p.active.count(2002));
    p.collection.eligible = false;
    p.ApplyItemEquipSpell(&item, false); CHECK(p.active.empty());
    p.ApplyItemEquipSpell(&item, true); CHECK(!p.active.count(2000) && p.active.count(2001));
    if (!failures) std::cout << "PASS: item-set skill ranks, thresholds, spec isolation and expired heirloom removal.\n";
    return failures ? 1 : 0;
}
