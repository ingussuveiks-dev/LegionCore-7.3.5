#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <string>
using uint32 = uint32_t;
enum { CLASS_WARRIOR = 1, QUEST_STATUS_INCOMPLETE = 3, QUEST_STATUS_COMPLETE = 1,
    UNIT_NPC_FLAG_NONE = 0, UNIT_NPC_FLAG_GOSSIP = 1, UNIT_FIELD_NPC_FLAGS = 10,
    UNIT_STATE_DIED = 1, SPELL_AURA_FEIGN_DEATH = 2 };
enum class GossipOptionNpc { None, Vendor };
struct GossipMenuItem
{
    GossipOptionNpc OptionNpc = GossipOptionNpc::None;
    uint32 Sender = 0, Action = 0, BoxMoney = 0;
    bool IsCoded = false;
};
namespace Trinity { namespace Containers {
template<typename Key, typename Value> Value const* MapGetValuePtr(std::map<Key, Value> const& values, Key key)
{
    auto found = values.find(key);
    return found == values.end() ? nullptr : &found->second;
}
} }
struct GossipMenu
{
    std::map<uint32, GossipMenuItem> _menuItems;
    std::map<uint32, uint32> _menuItemData;
    uint32 _menuId = 18743;
    uint32 GetMenuId() const;
    uint32 GetMenuItemSender(uint32) const;
    uint32 GetMenuItemAction(uint32) const;
    GossipMenuItem const* GetItem(uint32) const;
    void ClearMenu();
};
#include "Menu.inc"
struct PlayerMenu
{
    GossipMenu menu;
    uint32 closes = 0;
    GossipMenu& GetGossipMenu() { return menu; }
    uint32 GetGossipOptionSender(uint32 selection) const { return menu.GetMenuItemSender(selection); }
    uint32 GetGossipOptionAction(uint32 selection) const { return menu.GetMenuItemAction(selection); }
    void ClearMenus() { menu.ClearMenu(); }
    void SendCloseGossip() { ++closes; }
};
struct ObjectGuid
{
    uint32 identity = 1, kind = 1;
    bool IsCreatureOrVehicle() const { return kind == 1; }
    bool IsGameObject() const { return kind == 2; }
    std::string ToString() const { return std::to_string(identity); }
};
struct Player;
struct CreatureAI
{
    virtual ~CreatureAI() = default;
    virtual void sGossipSelect(Player*, uint32, uint32) { }
    virtual void sGossipSelectCode(Player*, uint32, uint32, char const*) { }
};
struct GameObjectAI
{
    void GossipSelect(Player*, uint32, uint32) { }
    void GossipSelectCode(Player*, uint32, uint32, char const*) { }
};
struct GameObject
{
    struct Info { uint32 ScriptId = 7; } info;
    GameObjectAI ai;
    uint32 LastUsedScriptID = 7;
    Info* GetGOInfo() { return &info; }
    GameObjectAI* AI() { return &ai; }
};
struct Map
{
    GameObject* GetGameObject(ObjectGuid) { return nullptr; }
} instanceMap, otherMap;
struct Scenario
{
    uint32 identity = 909, step = 3;
    uint32 GetScenarioId() const { return identity; }
    uint32 GetCurrentStep() const { return step; }
};
struct ScenarioMgr
{
    Scenario scenario;
    bool available = true;
    Scenario* GetScenario(uint32 instance) { assert(instance == 42); return available ? &scenario : nullptr; }
} scenarioManager;
ScenarioMgr* sScenarioMgr = &scenarioManager;
struct MotionMaster
{
    uint32 moves = 0;
    std::function<void()> reenter;
    void MovePoint(uint32 point, float positionX, float positionY, float positionZ)
    {
        assert(point == 7 && positionX == 4720.80f && positionY == 347.91f && positionZ == -37.71f);
        ++moves;
        if (reenter) { auto callback = std::move(reenter); callback(); }
    }
};
struct Creature
{
    struct Info { uint32 ScriptID = 7; } info;
    bool world = true, alive = true, despawn = false, phase = true, visible = true, los = true;
    bool controlled = false, hostile = false;
    float distance = 4.0f;
    uint32 entry = 96468, mapId = 1495, instance = 42, flags = 1, LastUsedScriptID = 7;
    Map* map = &instanceMap;
    CreatureAI* ai = nullptr;
    MotionMaster motion;
    bool IsInWorld() const { return world; }
    bool IsAlive() const { return alive; }
    bool IsDespawn() const { return despawn; }
    uint32 GetEntry() const { return entry; }
    uint32 GetMapId() const { return mapId; }
    uint32 GetInstanceId() const { return instance; }
    Map* GetMap() const { return map; }
    ObjectGuid GetGUID() const { return {}; }
    Info* GetCreatureTemplate() { return &info; }
    CreatureAI* AI() { return ai; }
    MotionMaster* GetMotionMaster() { return &motion; }
    void RemoveFlag(uint32 field, uint32 mask) { assert(field == UNIT_FIELD_NPC_FLAGS); flags &= ~mask; }
    void SetUInt32Value(uint32 field, uint32 value) { assert(field == UNIT_FIELD_NPC_FLAGS); flags = value; }
};
struct Player
{
    PlayerMenu talk;
    PlayerMenu* PlayerTalkClass = &talk;
    Creature* actor = nullptr;
    Map* map = &instanceMap;
    bool world = true, contact = true, alive = true, flight = false, rewarded = false;
    uint32 playerClass = 1, instance = 42, status = 3, faction = 0, genericCalls = 0;
    bool IsInWorld() const { return world; }
    bool CanContact() const { return contact; }
    uint32 getClass() const { return playerClass; }
    uint32 GetInstanceId() const { return instance; }
    Map* GetMap() const { return map; }
    bool InSamePhase(Creature* creature) const { return creature->phase; }
    bool canSeeOrDetect(Creature* creature) const { return creature->visible; }
    bool IsWithinLOSInMap(Creature* creature) const { return creature->los; }
    bool GetQuestRewardStatus(uint32 quest) const { assert(quest == 39191); return rewarded; }
    uint32 GetQuestStatus(uint32 quest) const { assert(quest == 39191); return status; }
    Creature* GetNPCIfCanInteractWith(ObjectGuid guid, uint32 mask)
    {
        if (!actor || guid.identity != 1 || !world || !alive || flight || !actor->world || !actor->alive ||
            actor->controlled || actor->hostile || actor->distance > 5.0f || (mask && !(actor->flags & mask)))
            return nullptr;
        return actor;
    }
    void CLOSE_GOSSIP_MENU() { talk.SendCloseGossip(); }
    bool HasUnitState(uint32) const { return false; }
    void RemoveAurasByType(uint32) { }
    void OnGossipSelect(Creature*, uint32, uint32) { ++genericCalls; }
    void OnGossipSelect(GameObject*, uint32, uint32) { ++genericCalls; }
};
#include "Policy.inc"
struct CreatureScript
{
    virtual ~CreatureScript() = default;
    virtual bool OnGossipSelect(Player*, Creature*, uint32, uint32) = 0;
    virtual bool OnGossipHello(Player*, Creature*) = 0;
};
struct HruthnirScript : CreatureScript
{
#include "Select.inc"
#include "Hello.inc"
};
struct HruthnirAI : CreatureAI
{
    Creature* me;
    explicit HruthnirAI(Creature* creature) : me(creature) { }
#include "AISelect.inc"
};
struct ScriptMgr
{
    HruthnirScript script;
    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action)
    {
        return script.OnGossipSelect(player, creature, sender, action);
    }
    bool OnGossipSelect(Player*, GameObject*, uint32, uint32) { return true; }
    template<typename Actor> bool OnGossipSelectCode(Player*, Actor*, uint32, uint32, char const*) { return false; }
} scriptManager;
ScriptMgr* sScriptMgr = &scriptManager;
namespace WorldPackets { namespace NPC {
struct GossipSelectOption
{
    ObjectGuid GossipUnit;
    uint32 GossipID = 18743, GossipIndex = 0;
    std::string PromotionCode;
};
} }
uint32 getMSTime() { return 0; }
uint32 GetMSTimeDiffToNow(uint32) { return 0; }
template<typename... Args> void TC_LOG_DEBUG(Args...) { }
struct Log { template<typename... Args> void outDiff(Args...) { } } logService;
Log* sLog = &logService;
struct WorldSession
{
    Player* currentPlayer;
    Player* GetPlayer() { return currentPlayer; }
    void HandleGossipSelectOption(WorldPackets::NPC::GossipSelectOption& packet);
};
#include "Dispatch.inc"
struct Fixture
{
    Creature actor;
    HruthnirAI ai{&actor};
    Player player;
    WorldSession session{&player};
    WorldPackets::NPC::GossipSelectOption packet;
    Fixture()
    {
        scenarioManager = {};
        player.actor = &actor;
        actor.ai = &ai;
        player.talk.menu._menuItems[0] = {};
    }
    void Select() { session.HandleGossipSelectOption(packet); }
};
int main()
{
#ifdef BASELINE
    for (uint32 invalid = 0; invalid < 5; ++invalid)
    {
        Fixture fixture;
        if (invalid == 0) fixture.actor.flags = 0;
        if (invalid == 1) scenarioManager.scenario.step = 2;
        if (invalid == 2) fixture.player.status = 0;
        if (invalid == 3) fixture.packet.GossipIndex = 999;
        if (invalid == 4) fixture.packet.GossipID = 999;
        fixture.Select();
        assert(fixture.actor.motion.moves == 1 && fixture.player.talk.closes == 1);
    }
    Fixture duplicate;
    duplicate.Select(); duplicate.Select();
    assert(duplicate.actor.motion.moves == 2);
    assert(duplicate.player.talk.menu.GetItem(0));
    std::cout << "PASS baseline: actual native dispatch reaches unguarded movement for gossip-off, early stage, absent quest, invalid raw index/menu and repeated selection; menu remains cached.\n";
#else
    assert(!HruthnirGossip::CanStart(nullptr, nullptr));
    for (uint32 invalid = 0; invalid < 39; ++invalid)
    {
        Fixture fixture;
        switch (invalid)
        {
            case 0: fixture.player.world = false; break;
            case 1: fixture.player.contact = false; break;
            case 2: fixture.actor.world = false; break;
            case 3: fixture.actor.alive = false; break;
            case 4: fixture.actor.despawn = true; break;
            case 5: fixture.actor.entry = 96469; break;
            case 6: fixture.actor.mapId = 1479; break;
            case 7: fixture.player.playerClass = 2; break;
            case 8: fixture.player.map = nullptr; break;
            case 9: fixture.player.map = &otherMap; break;
            case 10: fixture.player.instance = 43; break;
            case 11: fixture.actor.phase = false; break;
            case 12: fixture.actor.visible = false; break;
            case 13: fixture.actor.los = false; break;
            case 14: fixture.actor.flags = 0; break;
            case 15: fixture.actor.distance = 5.01f; break;
            case 16: fixture.player.rewarded = true; break;
            case 17: fixture.player.status = 0; break;
            case 18: fixture.player.status = 5; break;
            case 19: fixture.player.status = 6; break;
            case 20: scenarioManager.available = false; break;
            case 21: scenarioManager.scenario.identity = 1068; break;
            case 22: scenarioManager.scenario.step = 2; break;
            case 23: scenarioManager.scenario.step = 4; break;
            case 24: fixture.packet.GossipIndex = 999; break;
            case 25: fixture.packet.GossipID = 999; break;
            case 26: fixture.player.talk.menu._menuId = 999; break;
            case 27: fixture.player.talk.menu._menuItems.clear(); break;
            case 28: fixture.player.talk.menu._menuItems[0].OptionNpc = GossipOptionNpc::Vendor; break;
            case 29: fixture.player.talk.menu._menuItems[0].Sender = 1; break;
            case 30: fixture.player.talk.menu._menuItems[0].Action = 1; break;
            case 31: fixture.player.talk.menu._menuItems[0].IsCoded = true; break;
            case 32: fixture.player.talk.menu._menuItems[0].BoxMoney = 1; break;
            case 33: fixture.player.alive = false; break;
            case 34: fixture.player.flight = true; break;
            case 35: fixture.actor.controlled = true; break;
            case 36: fixture.actor.hostile = true; break;
            case 37: fixture.packet.PromotionCode = "unoffered"; break;
            case 38: fixture.actor.map = &otherMap; break;
        }
        fixture.Select();
        assert(!fixture.actor.motion.moves && !fixture.player.talk.closes && fixture.player.genericCalls == uint32(invalid == 37));
        assert(fixture.actor.flags == uint32(invalid == 14 ? 0 : 1));
        fixture.player.world = fixture.player.contact = fixture.player.alive = true;
        fixture.player.flight = fixture.player.rewarded = false;
        fixture.player.playerClass = 1;
        fixture.player.map = &instanceMap;
        fixture.player.instance = 42;
        fixture.player.status = 3;
        fixture.actor.world = fixture.actor.alive = fixture.actor.phase = fixture.actor.visible = fixture.actor.los = true;
        fixture.actor.despawn = fixture.actor.controlled = fixture.actor.hostile = false;
        fixture.actor.entry = 96468;
        fixture.actor.mapId = 1495;
        fixture.actor.map = &instanceMap;
        fixture.actor.flags = 1;
        fixture.actor.distance = 4.0f;
        scenarioManager = {};
        fixture.packet = {};
        fixture.player.talk.menu._menuId = 18743;
        fixture.player.talk.menu._menuItems[0] = {};
        fixture.Select();
        assert(fixture.actor.motion.moves == 1 && fixture.player.talk.closes == 1 && !fixture.actor.flags);
    }
    for (uint32 faction = 0; faction < 2; ++faction)
    for (uint32 status : {uint32(1), uint32(3)})
    {
        Fixture fixture;
        fixture.player.faction = faction;
        fixture.player.status = status;
        fixture.actor.flags = 0x81;
        fixture.actor.distance = 5.0f;
        assert(!scriptManager.script.OnGossipHello(&fixture.player, &fixture.actor));
        fixture.actor.motion.reenter = [&fixture]() { fixture.Select(); };
        fixture.Select(); fixture.Select();
        assert(fixture.actor.motion.moves == 1 && fixture.actor.flags == 0x80);
        assert(fixture.player.talk.closes == 1 && !fixture.player.talk.menu.GetItem(0));
        assert(!fixture.player.genericCalls);
        fixture.actor.flags |= 1;
        fixture.player.talk.menu._menuId = 18743;
        fixture.player.talk.menu._menuItems[0] = {};
        fixture.Select();
        assert(fixture.actor.motion.moves == 2);
    }
    Fixture stale;
    scenarioManager.scenario.step = 2;
    assert(scriptManager.script.OnGossipHello(&stale.player, &stale.actor));
    assert(stale.player.talk.closes == 1 && !stale.player.talk.menu.GetItem(0));
    scenarioManager.scenario.step = 3;
    stale.Select();
    assert(!stale.actor.motion.moves);
    stale.player.talk.menu._menuId = 18743;
    stale.player.talk.menu._menuItems[0] = {};
    stale.Select();
    assert(stale.actor.motion.moves == 1);
    std::cout << "PASS: actual packet dispatch, native cached-menu getters/clear,39 rejected and explicitly readmitted contexts, both factions/statuses, stale-menu and reentrant/duplicate selection; gossip bit claimed before motion, other flags preserved. Controlled services, not client traversal.\n";
#endif
}
