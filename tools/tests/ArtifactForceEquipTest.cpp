#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <utility>
using uint16 = uint16_t;
using uint32 = uint32_t;
using SpellEffIndex = int;
enum { SPELL_EFFECT_HANDLE_HIT_TARGET = 1, NULL_SLOT = 255 };
enum InventoryResult { EQUIP_ERR_OK, EQUIP_ERR_CANT_EQUIP, EQUIP_ERR_BAG_FULL };
struct ChildGuid { bool empty = true; bool IsEmpty() const { return empty; } };
struct Item
{
    uint16 position = 0xFF20;
    bool equipped = false;
    ChildGuid child;
    bool IsEquipped() const { return equipped; }
    ChildGuid GetChildItem() const { return child; }
    uint16 GetPos() const { return position; }
};
struct Player
{
    std::map<uint16, Item*> inventory;
    Item* artifact = nullptr;
    uint32 expectedEntry = 0, swaps = 0, errors = 0, checks = 0, childChecks = 0;
    uint16 destination = 0xFF0F;
    InventoryResult equipResult = EQUIP_ERR_OK, childResult = EQUIP_ERR_OK, lastError = EQUIP_ERR_OK;
    Item* GetItemByEntry(uint32 entry) { assert(entry == expectedEntry); return artifact; }
    InventoryResult CanEquipItem(int slot, uint16& target, Item* item, bool swap)
    {
        assert(slot == NULL_SLOT && item == artifact && swap);
        ++checks;
        target = destination;
        return equipResult;
    }
    InventoryResult CanEquipChildItem(Item* item) { assert(item == artifact); ++childChecks; return childResult; }
    void SendEquipError(InventoryResult result, Item* item) { assert(item == artifact); ++errors; lastError = result; }
    void SwapItem(uint16 source, uint16 target)
    {
        assert(source != target && inventory[source] == artifact);
        ++swaps;
        std::swap(inventory[source], inventory[target]);
        inventory[target]->position = target;
        inventory[target]->equipped = true;
        if (inventory[source])
        {
            inventory[source]->position = source;
            inventory[source]->equipped = false;
        }
    }
    void EquipItem(uint16, Item*, bool) { assert(false); }
    void AutoUnequipOffhandIfNeed() { assert(false); }
};
struct Unit { Player* player = nullptr; Player* ToPlayer() { return player; } };
struct Effect { uint32 MiscValue = 0; };
struct SpellInfo { Effect effect; Effect* GetEffect(SpellEffIndex, int) { return &effect; } };
struct Spell
{
    int effectHandleMode = SPELL_EFFECT_HANDLE_HIT_TARGET, m_diffMode = 0;
    Unit* m_caster;
    SpellInfo* m_spellInfo;
    void EffectForceEquipItem(SpellEffIndex);
};
#include "ForceEquip.inc"
int main(int argc, char** argv)
{
    _set_abort_behavior(0, _CALL_REPORTFAULT);
    assert(argc == 2);
    std::ifstream catalog(argv[1]);
    uint32 spellId, entry, targetOne, targetTwo, artifactId, count = 0;
    std::set<uint32> entries;
    while (catalog >> spellId >> entry >> targetOne >> targetTwo >> artifactId)
    {
        assert(targetOne == 1 && targetTwo == 0);
        entries.insert(entry);
        ++count;
        for (bool occupied : {false, true})
        {
            Player player;
            Item artifact, previous;
            player.expectedEntry = entry;
            player.artifact = &artifact;
            player.inventory[artifact.position] = &artifact;
            if (occupied)
                player.inventory[player.destination] = &previous;
            uint16 source = artifact.position;
            Unit caster{&player};
            SpellInfo info{{entry}};
            Spell spell{SPELL_EFFECT_HANDLE_HIT_TARGET, 0, &caster, &info};
            spell.EffectForceEquipItem(0);
            assert(player.swaps == 1 && player.inventory[player.destination] == &artifact);
            assert(player.inventory[source] == (occupied ? &previous : nullptr));
            spell.EffectForceEquipItem(0);
            assert(player.swaps == 1);
        }
        for (int failure = 0; failure < 7; ++failure)
        {
            Player player;
            Item artifact;
            player.expectedEntry = entry;
            player.artifact = &artifact;
            player.inventory[artifact.position] = &artifact;
            Unit caster{&player};
            SpellInfo info{{entry}};
            Spell spell{SPELL_EFFECT_HANDLE_HIT_TARGET, 0, &caster, &info};
            if (failure == 0) spell.effectHandleMode = 0;
            if (failure == 1) caster.player = nullptr;
            if (failure == 2) player.artifact = nullptr;
            if (failure == 3) player.equipResult = EQUIP_ERR_CANT_EQUIP;
            if (failure == 4) { artifact.child.empty = false; player.childResult = EQUIP_ERR_BAG_FULL; }
            if (failure == 5) player.destination = artifact.position;
            if (failure == 6) artifact.equipped = true;
            spell.EffectForceEquipItem(0);
            assert(!player.swaps);
            assert(player.errors == (failure == 3 || failure == 4 ? 1u : 0u));
            if (failure == 4) assert(player.lastError == EQUIP_ERR_BAG_FULL && player.childChecks == 1);
        }
        Player player;
        Item artifact;
        artifact.child.empty = false;
        player.expectedEntry = entry;
        player.artifact = &artifact;
        player.inventory[artifact.position] = &artifact;
        Unit caster{&player};
        SpellInfo info{{entry}};
        Spell spell{SPELL_EFFECT_HANDLE_HIT_TARGET, 0, &caster, &info};
        spell.EffectForceEquipItem(0);
        assert(player.swaps == 1 && player.childChecks == 1);
    }
    assert(count == 55 && entries.size() == 39);
    std::cout << "PASS: 55 native effect249 records; actual production spell handler, swap delegation, paired-item admission, errors and repeat/missing-item guards\n";
}
