#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <vector>
using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using int32 = int32_t;
using int16 = int16_t;
constexpr uint32 PLAYER_FIELD_SKILL = 10, SKILL_ID_OFFSET = 0, SKILL_STEP_OFFSET = 10,
    SKILL_RANK_OFFSET = 20, SKILL_MAX_RANK_OFFSET = 30, SKILL_TEMP_BONUS_OFFSET = 40,
    SKILL_PERM_BONUS_OFFSET = 50, PLAYER_FIELD_PROFESSION_SKILL_LINE = 100, PLAYER_MAX_SKILLS = 8;
enum { SKILL_NEW, SKILL_CHANGED, SKILL_DELETED };
constexpr int SKILL_CATEGORY_PROFESSION = 11, CRITERIA_TYPE_REACH_SKILL_LEVEL = 1,
    CRITERIA_TYPE_LEARN_SKILL_LEVEL = 2, SPELL_AURA_MOD_SKILL = 1, SPELL_AURA_MOD_SKILL_2 = 2,
    SPELL_AURA_MOD_SKILL_TALENT = 3, AURA_EFFECT_HANDLE_SKILL = 1;
#define TC_LOG_ERROR(...) do {} while (false)
struct SkillStatusData
{
    uint16 pos;
    int uState;
    SkillStatusData(uint16 p = 0, int s = SKILL_NEW) : pos(p), uState(s) {}
};
struct SkillLineEntry { int CategoryID = SKILL_CATEGORY_PROFESSION; };
struct SkillStore : std::map<uint32, SkillLineEntry>
{
    SkillLineEntry* LookupEntry(uint32 id) { auto it = find(id); return it == end() ? nullptr : &it->second; }
} sSkillLineStore;
struct SkillLineAbilityEntry { uint32 Spell; };
struct DB2Manager { std::map<uint32, std::vector<SkillLineAbilityEntry const*>> _skillLineAbilityContainer; } sDB2Manager;
struct SpellManager { uint32 GetFirstSpellInChain(uint32 id) { return id; } } manager;
auto sSpellMgr = &manager;
struct Player;
struct AuraEffect
{
    int32 skill, amount;
    bool talent;
    int32 GetMiscValue() const { return skill; }
    void HandleEffect(Player*, int, bool) const;
};
using AuraEffectList = std::vector<AuraEffect const*>;
enum EnchantmentSlot { PERM_ENCHANTMENT_SLOT, SOCK_ENCHANTMENT_SLOT, SOCK_ENCHANTMENT_SLOT_2,
    SOCK_ENCHANTMENT_SLOT_3, PRISMATIC_ENCHANTMENT_SLOT, MAX_ENCHANTMENT_SLOT };
constexpr uint8 INVENTORY_SLOT_BAG_END = 1;
struct SpellItemEnchantmentEntry { uint16 RequiredSkillID = 202, RequiredSkillRank = 250; };
struct EnchantmentStore
{
    SpellItemEnchantmentEntry enchant;
    SpellItemEnchantmentEntry const* LookupEntry(uint32 id) { return id == 1 ? &enchant : nullptr; }
} sSpellItemEnchantmentStore;
struct Item
{
    uint32 GetEnchantmentId(EnchantmentSlot slot) { return slot == PERM_ENCHANTMENT_SLOT ? 1 : 0; }
    Item* GetTemplate() { return this; }
    uint32 GetSocketType(int) { return 0; }
};
struct Player
{
    std::map<uint32, SkillStatusData*> statusVector;
    decltype(statusVector)* mSkillStatusVector = &statusVector;
    std::map<uint32, SkillStatusData> mSkillStatus;
    std::map<std::pair<uint32, uint8>, uint16> shorts;
    std::map<uint32, uint32> fields;
    std::map<int, AuraEffectList> bonuses;
    std::set<uint32> known, removed;
    Item equipped;
    Item* m_items[INVENTORY_SLOT_BAG_END] = {&equipped};
    struct Transition { bool apply; uint16 visible; };
    std::vector<Transition> transitions;
    bool enchantActive = false;
    uint16 GetUInt16Value(uint32 field, uint8 offset) { return shorts[{field, offset}]; }
    void SetUInt16Value(uint32 field, uint8 offset, uint16 value) { shorts[{field, offset}] = value; }
    uint32 GetUInt32Value(uint32 field) { return fields[field]; }
    void SetUInt32Value(uint32 field, uint32 value) { fields[field] = value; }
    AuraEffectList const& GetAuraEffectsByType(int type) { return bonuses[type]; }
    void UpdateAchievementCriteria(int, uint32) {}
    void learnSkillRewardedSpells(uint32, uint16) {}
    void removeSpell(uint32 spell) { known.erase(spell); removed.insert(spell); }
    void ApplyEnchantment(Item*, EnchantmentSlot, bool apply)
    {
        uint16 visible = GetSkillValue(202);
        transitions.push_back({apply, visible});
        // ApplyEnchantment reads GetSkillValue; a missing/deleted skill returns 0.
        if (visible >= sSpellItemEnchantmentStore.enchant.RequiredSkillRank)
            enchantActive = apply;
    }
    void UpdateSkillEnchantments(uint16, uint16, uint16);
    int16 GetSkillPermBonusValue(uint32);
    int16 GetSkillTempBonusValue(uint32);
    void SetSkill(uint16, uint16 = 0, uint16 = 0, uint16 = 0);
    uint16 GetSkillValue(uint32);
    void ModifySkillBonus(uint32, int32, bool);
};
void AuraEffect::HandleEffect(Player* player, int, bool apply) const
{
    player->ModifySkillBonus(skill, apply ? amount : -amount, talent);
}
// ItemEffectLifecycleTest compiles the production bonus updater. Here capture
// the ranks passed from production skill handlers, including pre-clear removal.
std::map<uint32, uint32> itemSetSkills;
void UpdateItemSetSkill(Player*, uint32 skill, uint32 value) { itemSetSkills[skill] = value; }
#include "SkillSet.inc"
#include "SkillValue.inc"
#include "SkillBonus.inc"
#include "SkillEnchantments.inc"
#include "SkillPermBonus.inc"
#include "SkillTempBonus.inc"
void Check(bool value, char const* message) { if (!value) throw std::runtime_error(message); }
int main()
{
    try
    {
        sSkillLineStore[202] = {};
        sSkillLineStore[773] = {};
        SkillLineAbilityEntry recipe{127123};
        sDB2Manager._skillLineAbilityContainer[202] = {&recipe};
        AuraEffect racial{202, 15, true};
        for (bool saved : {false, true})
        {
            Player player;
            player.bonuses[SPELL_AURA_MOD_SKILL_TALENT] = {&racial};
            player.SetSkill(773, 1, 100, 800);
            player.transitions.clear();
            player.SetSkill(202, 1, 235, 800);
            Check(player.GetSkillValue(202) == 250, "New skill must include racial bonus");
            Check(player.transitions.size() == 1 && player.transitions[0].visible == 250,
                "Enchant restoration must run after skill status and racial bonus are initialized");
            Check(player.enchantActive, "Eligible equipped enchant must activate on learning");
            Check(itemSetSkills[202] == 250, "Item sets must receive the effective learned rank");
            if (saved) player.mSkillStatus[202].uState = SKILL_CHANGED;
            player.known = {127123, 192846};
            player.SetSkill(202);
            Check(player.GetSkillValue(202) == 0 && !player.enchantActive, "Unlearning must remove skill and enchant effect");
            Check(itemSetSkills[202] == 0, "Unlearning must deactivate equipped profession set bonuses");
            Check(player.transitions.back().visible == 250 && !player.transitions.back().apply,
                "Enchant removal must see old skill before it is cleared");
            Check(!player.known.count(127123) && player.known.count(192846), "Unlearning must remove only this skill's recipes");
            Check(player.GetSkillValue(773) == 100 && player.fields[PLAYER_FIELD_PROFESSION_SKILL_LINE] == 773,
                "Unlearning must preserve the other profession");
            Check(player.fields[PLAYER_FIELD_PROFESSION_SKILL_LINE + 1] == 0, "Profession slot must be cleared");
            player.transitions.clear();
            player.SetSkill(202, 1, 235, 800);
            Check(player.enchantActive && player.transitions.size() == 1 && player.transitions[0].visible == 250,
                "Relearning must restore enchant once for both deleted and unsaved skills");
        }
        std::cout << "PASS: new/saved profession removal, recipe isolation, slots, racial bonuses and enchant restoration\n";
    }
    catch (std::exception const& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
