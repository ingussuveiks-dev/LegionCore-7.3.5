#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>
using uint32 = uint32_t;
using uint8 = uint8_t;
enum InventoryResult { EQUIP_ERR_OK, EQUIP_ERR_INV_FULL, EQUIP_ERR_ITEM_MAX_COUNT, EQUIP_ERR_ITEM_NOT_FOUND };
constexpr int NULL_BAG = 0, NULL_SLOT = 0, ITEM_QUALITY_EPIC = 4,
    CURRENT_EXPANSION = 0, GUILD_NEWS_ITEM_CRAFTED = 1, CRITERIA_TYPE_CRAFT_ITEMS_GUILD = 1,
    CRITERIA_TYPE_CREATE_ITEM = 2, ITEM_CLASS_CONSUMABLE = 0, ITEM_CLASS_QUEST = 12, ITEM_FIELD_CREATOR = 1;
uint32 MinNewsItemLevel[] = {100};
using ItemPosCountVec = std::vector<uint32>;
using GuidSet = std::vector<uint32>;
struct ItemTemplate
{
    uint32 ItemLevel = 110, id = 1234, quality = 4, stack = 20;
    uint32 GetMaxStackSize() const { return stack; }
    uint32 GetQuality() const { return quality; }
    uint32 GetClass() const { return 2; }
    bool IsNotAppearInGuildNews() const { return false; }
    uint32 GetId() const { return id; }
} itemTemplate;
struct Item
{
    uint32 creator = 0;
    static uint32 GenerateItemRandomPropertyId(uint32, uint32) { return 0; }
    ItemTemplate* GetTemplate() { return &itemTemplate; }
    void SetUInt32Value(int, uint32 value) { creator = value; }
};
struct ObjectManager
{
    bool valid = true;
    ItemTemplate const* GetItemTemplate(uint32) { return valid ? &itemTemplate : nullptr; }
} objectManager;
auto sObjectMgr = &objectManager;
struct Player
{
    bool isPlayer = true, storeFails = false;
    uint32 noSpace = 0, requested = 0, stored = 0, skills = 0, achievements = 0, notified = 0, errors = 0;
    InventoryResult inventory = EQUIP_ERR_OK;
    Item item;
    bool IsPlayer() { return isPlayer; }
    Player* ToPlayer() { return this; }
    uint32 GetGuildId() { return 1; }
    uint32 GetGUID() { return 10; }
    uint32 GetGUIDLow() { return 10; }
    uint32 GetLootSpecID() { return 0; }
    void SendEquipError(InventoryResult, void* = nullptr, void* = nullptr, uint32 = 0) { ++errors; }
    InventoryResult CanStoreNewItem(int, int, ItemPosCountVec& dest, uint32, uint32 count, uint32* missing)
    { requested = count; *missing = noSpace; dest = {count - noSpace}; return inventory; }
    Item* StoreNewItem(ItemPosCountVec const& dest, uint32, bool, uint32, GuidSet, std::vector<uint32> const&)
    { if (storeFails) return nullptr; stored = dest[0]; return &item; }
    void SendNewItem(Item*, uint32 count, bool, bool) { notified = count; }
    void UpdateCraftSkill(uint32) { ++skills; }
    void UpdateAchievementCriteria(int, uint32, uint32 count) { achievements += count; }
};
struct Guild
{
    uint32 news = 0, achievements = 0;
    void AddGuildNews(int, uint32, int, uint32, Item*) { ++news; }
    void UpdateAchievementCriteria(int, uint32, uint32 count, int, void*, Player*) { achievements += count; }
} testGuild;
struct GuildManager { Guild* GetGuildById(uint32) { return &testGuild; } } guildManager;
auto sGuildMgr = &guildManager;
struct ScriptManager { uint32 calls = 0; void OnCreateItem(Player*, Item*, uint32) { ++calls; } } scripts;
auto sScriptMgr = &scripts;
bool canCreateExtraItems(Player*, uint32, float&, uint8&) { return false; }
bool roll_chance_f(float) { return false; }
struct SpellInfo { uint32 Id = 1; };
struct Spell
{
    Player* unitTarget;
    SpellInfo* m_spellInfo;
    int damage = 3;
    void DoCreateItem(uint32, uint32, std::vector<uint32> const&);
};
#include "CraftCreate.inc"
void Check(bool value, char const* message) { if (!value) throw std::runtime_error(message); }
int main()
{
    try
    {
        SpellInfo info;
        Player player;
        Spell spell{&player, &info};
        player.storeFails = true;
        spell.DoCreateItem(0, 1234, {});
        Check(!testGuild.news && !testGuild.achievements, "Failed item creation must not award guild credit or news");
        Check(!scripts.calls && !player.skills && !player.achievements && player.errors == 1, "Failed creation must only report an error");
        player.storeFails = false;
        spell.DoCreateItem(0, 1234, {});
        Check(player.stored == 3 && player.notified == 3 && player.skills == 1, "Successful craft must store correct quantity and skill up once");
        Check(testGuild.news == 1 && testGuild.achievements == 3 && scripts.calls == 1 && player.achievements == 3, "Successful craft must award matching credit");
        Check(player.item.creator == 10, "Successful craft must retain creator");
        testGuild = {}; scripts = {}; player = {};
        player.inventory = EQUIP_ERR_INV_FULL;
        player.noSpace = 3;
        spell.DoCreateItem(0, 1234, {});
        Check(!player.stored && !player.skills && !testGuild.achievements && !scripts.calls, "No room must not create or award anything");
        player.noSpace = 1;
        spell.DoCreateItem(0, 1234, {});
        Check(player.stored == 2 && testGuild.achievements == 2 && player.achievements == 2, "Partial storage must credit only items actually stored");
        testGuild = {}; scripts = {}; player = {};
        objectManager.valid = false;
        spell.DoCreateItem(0, 1234, {});
        Check(player.errors == 1 && !player.stored && !testGuild.news, "Missing template must fail without rewards");
        std::cout << "PASS: crafting failure/success, full/partial inventory, quantities and reward accounting\n";
    }
    catch (std::exception const& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
