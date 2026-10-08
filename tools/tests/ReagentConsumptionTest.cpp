#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>
using uint32 = uint32_t; using int32 = int32_t;
constexpr int MAX_SPELL_REAGENTS = 8, ITEM_FLAG_NO_REAGENT_COST = 1,
    SPELL_AURA_SHOW_CONFIRMATION_PROMPT_WITH_DIFFICULTY = 1;
struct ItemEffectEntry { int32 Charges, LegacySlotIndex; };
struct ItemTemplate
{
    uint32 id = 100, flags = 0;
    std::vector<ItemEffectEntry const*> Effects;
    uint32 GetId() const { return id; }
    uint32 GetFlags() const { return flags; }
};
struct Item
{
    ItemTemplate data;
    bool inWorld = true, inUse = true;
    int32 charges = -1;
    bool IsInWorld() const { return inWorld; }
    ItemTemplate const* GetTemplate() const { return &data; }
    int32 GetSpellCharges(int) const { return charges; }
    void SetInUse(bool value) { inUse = value; }
};
struct SpellInfo
{
    struct { int32 Reagent[8]{}; uint32 ReagentCount[8]{}; uint32 CurrencyID = 0, CurrencyCount = 0; } Reagents;
    bool confirmation = false;
    bool HasAura(int) const { return confirmation; }
};
struct Player
{
    bool player = true, exempt = false;
    std::map<uint32, uint32> consumed;
    std::map<uint32, int32> currencies;
    bool IsPlayer() const { return player; }
    Player* ToPlayer() { return this; }
    bool CanNoReagentCast(SpellInfo const*) const { return exempt; }
    void DestroyItemCount(uint32 id, uint32 count, bool) { consumed[id] += count; }
    void ModifyCurrency(uint32 id, int32 count) { currencies[id] += count; }
};
struct Targets
{
    Item* item = nullptr;
    uint32 GetItemTargetEntry() const { return item ? item->data.id : 0; }
    void SetItemTarget(Item* value) { item = value; }
};
struct Guid { bool empty = false; void Clear() { empty = true; } };
struct Spell
{
    Player* m_caster;
    SpellInfo* m_spellInfo;
    Item* m_CastItem = nullptr;
    Guid m_castItemGUID;
    uint32 m_castItemEntry = 0;
    Targets m_targets;
    void TakeReagents();
};
#include "TakeReagents.inc"
int failures = 0;
void Check(bool ok, char const* message) { if (!ok) { ++failures; std::cerr << "FAIL: " << message << '\n'; } }
int main()
{
    SpellInfo info;
    info.Reagents.Reagent[0] = 100; info.Reagents.ReagentCount[0] = 2;
    info.Reagents.Reagent[1] = 101; info.Reagents.ReagentCount[1] = 3;
    info.Reagents.CurrencyID = 1200; info.Reagents.CurrencyCount = 5;
    Player p;
    Spell spell{&p, &info};
    spell.TakeReagents();
    Check(p.consumed[100] == 2 && p.consumed[101] == 3 && p.currencies[1200] == -5, "Ordinary cast consumes exact item and currency costs");
    p.consumed.clear(); p.currencies.clear(); p.exempt = true;
    spell.TakeReagents();
    Check(p.consumed.empty() && p.currencies.empty(), "Reagent exemption must include currency");
    p.exempt = false;
    Item item; ItemEffectEntry effect{-1, 0}; item.data.Effects = {&effect};
    spell.m_CastItem = &item; spell.m_targets.item = &item; spell.m_castItemEntry = 100;
    item.data.flags = ITEM_FLAG_NO_REAGENT_COST;
    spell.TakeReagents();
    Check(p.consumed.empty() && p.currencies.empty(), "No-reagent cast item must not charge materials");
    item.data.flags = 0;
    spell.TakeReagents();
    Check(p.consumed[100] == 3, "Last-charge cast item used as a reagent must consume one cast item plus recipe cost");
    Check(!spell.m_CastItem && !spell.m_targets.item && spell.m_castItemGUID.empty && !spell.m_castItemEntry && !item.inUse,
        "Consuming a reagent target must clear every cast-item reference");
    if (failures) return 1;
    std::cout << "PASS: reagent/currency quantities, exemptions and consumed cast-item pointer cleanup\n";
}
