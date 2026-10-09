#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <vector>
using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;
using int32 = int32_t;
constexpr uint32 SKILL_RIDING = 762, SKILL_FIRST_AID = 129, SKILL_DEMON_HUNTER = 1848;
constexpr uint32 SKILL_LINE_ABILITY_LEARNED_ON_SKILL_VALUE = 1;
constexpr uint32 SKILL_LINE_ABILITY_LEARNED_ON_SKILL_LEARN = 2;
constexpr uint32 SKILL_LINE_ABILITY_NOT_AUTO_LEARN = 4;
constexpr int PLAYERSPELL_REMOVED = 3;
struct SkillLineAbilityEntry
{
    uint32 Spell, SkillLine, MinSkillLineRank, AcquireMethod;
    uint64 RaceMask = 0;
    uint32 ClassMask = 0;
};
struct SpellInfo { uint32 Id, SpellLevel = 1; };
struct SpellMgr
{
    std::map<uint32, SpellInfo> spells;
    SpellInfo const* GetSpellInfo(uint32 id) { auto i = spells.find(id); return i == spells.end() ? nullptr : &i->second; }
} spellMgr;
auto sSpellMgr = &spellMgr;
struct DB2Manager { std::map<uint32, std::vector<SkillLineAbilityEntry const*>> _skillLineAbilityContainer; } db2;
auto& sDB2Manager = db2;
enum EnchantmentSlot { PERM_ENCHANTMENT_SLOT, SOCK_ENCHANTMENT_SLOT, SOCK_ENCHANTMENT_SLOT_2,
    SOCK_ENCHANTMENT_SLOT_3, PRISMATIC_ENCHANTMENT_SLOT, MAX_ENCHANTMENT_SLOT };
constexpr uint8 INVENTORY_SLOT_BAG_END = 3;
struct SpellItemEnchantmentEntry { uint16 RequiredSkillID, RequiredSkillRank; };
struct EnchantmentStore
{
    std::map<uint32, SpellItemEnchantmentEntry> rows;
    SpellItemEnchantmentEntry const* LookupEntry(uint32 id) { auto i = rows.find(id); return i == rows.end() ? nullptr : &i->second; }
} sSpellItemEnchantmentStore;
struct Item
{
    std::map<int, uint32> enchants;
    uint32 GetEnchantmentId(EnchantmentSlot slot) { return enchants[slot]; }
    Item* GetTemplate() { return this; }
    uint32 GetSocketType(int) { return 0; }
};
struct SkillLineEntry { uint32 ID, ParentSkillLineID; };
struct SkillStore : std::vector<SkillLineEntry const*>
{
    SkillLineEntry const* LookupEntry(uint32 id) { for (auto v : *this) if (v->ID == id) return v; return nullptr; }
} sSkillLineStore;
namespace WorldPackets
{
namespace Misc
{
struct ShowTradeSkill { uint32 SkillLineID, SpellID, PlayerGUID; };
struct ShowTradeSkillResponse
{
    uint32 PlayerGUID = 0, SpellId = 0;
    std::vector<uint32> KnownAbilitySpellIDs, SkillLineIDs, SkillRanks, SkillMaxRanks;
    ShowTradeSkillResponse* Write() { return this; }
};
}
}
struct PlayerSpell { int state = 0; bool active = true, disabled = false; };
struct Player
{
    std::set<uint32> known, auras;
    std::vector<uint32> removed;
    std::map<uint32, PlayerSpell> spellMap;
    std::map<uint32, uint32> skills;
    int32 skillTempBonus = 0, skillPermBonus = 0;
    int32 GetSkillTempBonusValue(uint32) { return skillTempBonus; }
    int32 GetSkillPermBonusValue(uint32) { return skillPermBonus; }
    Item* m_items[INVENTORY_SLOT_BAG_END]{};
    struct EnchantChange { Item* item; EnchantmentSlot slot; bool apply; };
    std::vector<EnchantChange> changes;
    WorldPackets::Misc::ShowTradeSkillResponse response;
    uint64 getRaceMask() { return 1; }
    uint32 getClassMask() { return 1; }
    uint32 getLevel() { return 110; }
    uint32 GetMapId() { return 0; }
    bool IsLoXpMap(uint32) { return false; }
    bool IsInWorld() { return true; }
    bool HasSpell(uint32 id) { return known.count(id) != 0; }
    void removeSpell(uint32 id) { known.erase(id); auras.erase(id); removed.push_back(id); }
    void learnSpell(uint32 id, bool, uint32) { known.insert(id); }
    void addSpell(uint32 id, bool, bool, bool, bool, bool, uint32) { known.insert(id); }
    void ApplyEnchantment(Item* item, EnchantmentSlot slot, bool apply) { changes.push_back({item, slot, apply}); }
    void learnSkillRewardedSpells(uint32, uint32);
    void UpdateSkillEnchantments(uint16, uint16, uint16);
    void SwitchMastery(uint32 spellId)
    {
        switch (spellId)
#include "ProfessionMastery.inc"
        known.insert(spellId);
    }
    bool CanContact() { return true; }
    bool HasSkill(uint32 id) { return skills.count(id) != 0; }
    auto const& GetSpellMapConst() { return spellMap; }
    uint32 GetSkillValue(uint32 id) { return skills[id]; }
    uint32 GetMaxSkillValue(uint32 id) { return HasSkill(id) ? 800 : 0; }
    void SendDirectMessage(WorldPackets::Misc::ShowTradeSkillResponse* packet) { response = *packet; }
};
struct ObjectAccessor { Player* player; Player* FindPlayer(uint32) { return player; } } accessor;
auto sObjectAccessor = &accessor;
bool IsPartOfSkillLine(uint32 skill, uint32 spell)
{
    for (auto a : db2._skillLineAbilityContainer[skill]) if (a->Spell == spell) return true;
    return false;
}
struct WorldSession
{
    Player* _player;
    void HandleShowTradeSkill(WorldPackets::Misc::ShowTradeSkill&);
};
#include "ProfessionRewards.inc"
// The item-set updater is compiled and tested by ItemEffectLifecycleTest.
void UpdateItemSetSkill(Player*, uint32, uint32) {}
#include "ProfessionEnchantments.inc"
#include "ProfessionShare.inc"
int failures = 0;
#define CHECK(expr) do { if (!(expr)) { std::cerr << "Failed: " #expr "\n"; ++failures; } } while (false)
int main()
{
    // All transitions and relearning the current mastery must keep exactly one.
    for (uint32 old : {28672u, 28675u, 28677u})
        for (uint32 next : {28672u, 28675u, 28677u})
        {
            Player player;
            player.known.insert(old);
            player.auras.insert(old);
            player.SwitchMastery(next);
            CHECK(player.known == std::set<uint32>{next});
            CHECK(old == next || !player.auras.count(old));
            CHECK(old != next || player.auras.count(old));
        }
    // Real 26972 skill-value rewards: inscription research at 75 and 100.
    SkillLineAbilityEntry reward{165304, 773, 75, 1}, higher{165456, 773, 100, 1};
    SkillLineAbilityEntry recipe{48247, 773, 1, 0}; // Mysterious Tarot, explicitly learned
    db2._skillLineAbilityContainer[773] = {&reward, &higher, &recipe};
    for (auto a : {reward, higher, recipe}) spellMgr.spells[a.Spell] = {a.Spell};
    Player scribe;
    scribe.known = {165304, 165456, 48247};
    scribe.auras.insert(165456);
    scribe.learnSkillRewardedSpells(773, 75);
    CHECK(scribe.known == (std::set<uint32>{165304, 48247}));
    CHECK(!scribe.auras.count(165456));
    scribe.learnSkillRewardedSpells(773, 74);
    CHECK(scribe.known == std::set<uint32>{48247});
    scribe.learnSkillRewardedSpells(773, 100);
    CHECK(scribe.known == (std::set<uint32>{165304, 165456, 48247}));
    // An invalid enchant on an earlier item must not skip later valid enchants.
    // IDs 4223/3717 are Nitro Boosts (engineering 250)/Socket Bracer (smithing 400).
    sSpellItemEnchantmentStore.rows = {{4223, {202, 250}}, {3717, {164, 400}}, {100, {0, 0}}};
    Item invalid, boots, bracer;
    invalid.enchants[PERM_ENCHANTMENT_SLOT] = 999999;
    boots.enchants[PERM_ENCHANTMENT_SLOT] = 4223;
    bracer.enchants[PRISMATIC_ENCHANTMENT_SLOT] = 3717;
    bracer.enchants[SOCK_ENCHANTMENT_SLOT] = 100;
    Player crafter;
    crafter.m_items[0] = &invalid; crafter.m_items[1] = &boots; crafter.m_items[2] = &bracer;
    crafter.UpdateSkillEnchantments(202, 800, 0);
    CHECK(crafter.changes.size() == 1 && !crafter.changes[0].apply && crafter.changes[0].item == &boots);
    crafter.changes.clear();
    crafter.UpdateSkillEnchantments(202, 249, 250);
    CHECK(crafter.changes.size() == 1 && crafter.changes[0].apply);
    crafter.changes.clear();
    crafter.UpdateSkillEnchantments(164, 400, 399);
    CHECK(crafter.changes.size() == 2);
    for (auto change : crafter.changes) CHECK(!change.apply && change.item == &bracer);
    crafter.changes.clear();
    crafter.UpdateSkillEnchantments(202, 300, 250);
    CHECK(crafter.changes.empty());
    // ApplyEnchantment uses skill including bonuses. Gnome +15 engineering
    // must not leave Nitro Boosts active when base rank 235 is unlearned.
    crafter.skillPermBonus = 15;
    crafter.UpdateSkillEnchantments(202, 235, 0);
    CHECK(crafter.changes.size() == 1 && !crafter.changes[0].apply);
    crafter.changes.clear();
    crafter.UpdateSkillEnchantments(202, 234, 235);
    CHECK(crafter.changes.size() == 1 && crafter.changes[0].apply);
    crafter.changes.clear();
    crafter.UpdateSkillEnchantments(202, 235, 234);
    CHECK(crafter.changes.size() == 1 && !crafter.changes[0].apply);
    // Cooking's six Ways are child skills in the 7.3.5 DB2, not expansion skills.
    SkillLineEntry cooking{185, 0}, grill{975, 185}, wok{976, 185};
    sSkillLineStore = {{&cooking, &grill, &wok}};
    SkillLineAbilityEntry grillRecipe{104298, 975, 1, 0};
    db2._skillLineAbilityContainer[975] = {&grillRecipe};
    Player cook, viewer;
    cook.skills = {{185, 600}, {975, 550}};
    cook.spellMap[104298] = {};
    accessor.player = &cook;
    spellMgr.spells[2550] = {2550};
    WorldSession session{&viewer};
    WorldPackets::Misc::ShowTradeSkill packet{185, 2550, 1};
    session.HandleShowTradeSkill(packet);
    CHECK(viewer.response.KnownAbilitySpellIDs == std::vector<uint32>{104298});
    CHECK(viewer.response.SkillLineIDs == (std::vector<uint32>{185, 975}));
    CHECK(viewer.response.SkillRanks == (std::vector<uint32>{600, 550}));
    if (failures) return 1;
    std::cout << "Profession regressions: mastery transitions, rank loss/relearn, enchant cleanup and cooking sharing passed\n";
}
