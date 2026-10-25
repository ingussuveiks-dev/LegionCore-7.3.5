#include <cassert>
#include <atomic>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
using uint32 = uint32_t;
using uint64 = uint64_t;
using int32 = int32_t;
enum { CLASS_WARRIOR = 1, QUEST_STATUS_INCOMPLETE = 3, QUEST_STATUS_COMPLETE = 1,
    UNIT_NPC_FLAG_NONE = 0, UNIT_NPC_FLAG_GOSSIP = 1, UNIT_FIELD_NPC_FLAGS = 10,
    UNIT_FIELD_FLAGS = 11, UNIT_FLAG_NOT_SELECTABLE = 0x2000000, UNIT_FLAG_IMMUNE_TO_PC = 0x100,
    UNIT_FLAG_IMMUNE_TO_NPC = 0x200, UNIT_FLAG_NOT_ATTACKABLE_1 = 0x80, REACT_DEFENSIVE = 1,
    UNIT_STATE_DIED = 1, SPELL_AURA_FEIGN_DEATH = 2, UNIT_STATE_CASTING = 4, UNIT_STATE_STUNNED = 8,
    EVENT_1 = 1, EVENT_2, EVENT_3, EVENT_4, EVENT_5, EVENT_6 };
#include "FunctionsClass.inc"
#include "Functions.inc"
struct InspectableFunctions : FunctionProcessor
{
    std::function<void()> LastQueued() const { assert(!m_functions_queue.empty()); return m_functions_queue.rbegin()->second; }
};
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
    virtual void Reset() { }
    virtual void UpdateAI(uint32) { }
    uint32 starts = 0;
    std::vector<uint32> texts;
    std::function<void()> onTalk;
    void Talk(uint32 text) { texts.push_back(text); auto callback = onTalk; if (callback) callback(); }
    std::function<void()> reenterStart;
    virtual void DoAction(int32 action)
    {
        assert(action == 1);
        ++starts;
        if (reenterStart) { auto callback = std::move(reenterStart); callback(); }
    }
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
    Scenario* GetScenario(uint32 instance) { return available && instance == 42 ? &scenario : nullptr; }
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
    bool IsAIEnabled = true;
    uint32 state = 0, conversations = 0;
    std::function<void()> reenterConversation;
    float distance = 4.0f;
    uint32 entry = 96468, mapId = 1495, instance = 42, flags = 1, LastUsedScriptID = 7;
    uint32 unitFlags = 0x2000380, react = 0;
    Map* map = &instanceMap;
    CreatureAI* ai = nullptr;
    Creature* boss = nullptr;
    MotionMaster motion;
    InspectableFunctions m_Functions;
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
    bool HasUnitState(uint32 mask) const { return (state & mask) != 0; }
    Creature* FindNearestCreature(uint32 expected, float range, bool living)
    {
        assert(expected == 96034 && range == 60.0f && living);
        return boss && boss->world && boss->alive && boss->distance <= range && boss->phase &&
            boss->map == map && boss->instance == instance ? boss : nullptr;
    }
    void CreateConversation(uint32 conversation)
    {
        assert(conversation == 497);
        ++conversations;
        if (reenterConversation) { auto callback = std::move(reenterConversation); callback(); }
    }
#include "AddDelayed.inc"
    MotionMaster* GetMotionMaster() { return &motion; }
    void RemoveFlag(uint32 field, uint32 mask) { if (field == UNIT_FIELD_NPC_FLAGS) flags &= ~mask; else { assert(field == UNIT_FIELD_FLAGS); unitFlags &= ~mask; } }
    void SetUInt32Value(uint32 field, uint32 value) { assert(field == UNIT_FIELD_NPC_FLAGS); flags = value; }
    void SetFlag(uint32 field, uint32 mask) { assert(field == UNIT_FIELD_NPC_FLAGS); flags |= mask; }
    void SetReactState(uint32 reaction) { react = reaction; }
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
    void CancelMagnarIntro() { }
    void UpdateMagnarIntro(uint32) { }
    Creature* me;
    struct EventMap
    {
        uint32 updates = 0;
        void Reset() { updates = 0; }
        void Update(uint32) { ++updates; }
        uint32 ExecuteEvent() { return 0; }
        void RescheduleEvent(uint32, uint32) { }
    };
    bool victim = false;
    uint32 casts = 0, melees = 0;
    explicit HruthnirAI(Creature* creature) : me(creature) { }
    bool UpdateVictim() { return victim; }
    void DoCast(uint32) { ++casts; }
    void DoMeleeAttackIfReady() { ++melees; }
#include "AIFields.inc"
#include "AISelect.inc"
#include "AIReset.inc"
#include "OfferContext.inc"
#include "Offer.inc"
#include "Arrival.inc"
#include "AIUpdate.inc"
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
    Creature boss;
    CreatureAI bossAI;
    Player player;
    WorldSession session{&player};
    WorldPackets::NPC::GossipSelectOption packet;
    Fixture()
    {
        scenarioManager = {};
        player.actor = &actor;
        actor.ai = &ai;
        actor.boss = &boss;
        boss.entry = 96034;
        boss.ai = &bossAI;
        player.talk.menu._menuItems[0] = {};
    }
    void Select() { session.HandleGossipSelectOption(packet); }
    void Tick(uint32 diff) { actor.m_Functions.Update(diff); actor.ai->UpdateAI(diff); }
};
int main()
{
#ifdef STAGE_BASELINE
    for (uint32 afterArrival = 0; afterArrival < 2; ++afterArrival)
    {
        Fixture fixture; fixture.Select();
        if (afterArrival) fixture.ai.Arrive();
        scenarioManager.scenario.step = 4;
        fixture.ai.Arrive(); fixture.Tick(6000);
        assert(fixture.actor.conversations == 1 && fixture.bossAI.starts == 1);
    }
    std::cout << "PASS stage baseline: pinned c4c026d1 requests497 after leaving909/3 and dispatches the queued boss cue after a late stage change; boss acceptance is a controlled service, not actual combat progression.\n";
#elif defined(BASELINE)
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
#elif defined(CUE_BASELINE)
    Fixture reset;
    reset.Select(); reset.ai.Arrive();
    reset.ai.Reset();
    reset.actor.m_Functions.Update(6000);
    assert(reset.bossAI.starts == 1 && reset.actor.conversations == 1);
    Fixture duplicate;
    duplicate.Select(); duplicate.ai.Arrive(); duplicate.ai.Arrive();
    duplicate.actor.m_Functions.Update(6000);
    assert(duplicate.bossAI.starts == 2 && duplicate.actor.conversations == 2);
    Fixture replaced;
    replaced.Select(); replaced.ai.Arrive();
    HruthnirAI replacement(&replaced.actor);
    replaced.actor.ai = &replacement;
    replacement.Reset();
    replaced.actor.m_Functions.Update(6000);
    assert(replaced.bossAI.starts == 1);
    std::cout << "PASS cue baseline: native Unit FunctionProcessor retains point7 callback after AI Reset/replacement and duplicate arrivals request497/start twice. Old AI kept alive in host, not a reproduced freed-pointer crash.\n";
#elif defined(OFFER_BASELINE)
    Fixture reset;
    reset.actor.flags = 0;
    reset.ai.OfferArrive(); reset.ai.Reset(); reset.Tick(6500);
    assert(reset.actor.flags == 1 && reset.actor.react == REACT_DEFENSIVE && !reset.actor.unitFlags);
    assert(reset.ai.texts == std::vector<uint32>({15,16}));
    Fixture replaced;
    replaced.actor.flags = 0;
    replaced.ai.OfferArrive();
    HruthnirAI replacement(&replaced.actor);
    replaced.actor.ai = &replacement; replacement.Reset(); replaced.Tick(6500);
    assert(replaced.actor.flags == 1 && !replaced.actor.unitFlags && replaced.ai.texts == std::vector<uint32>({15,16}));
    Fixture early;
    early.actor.flags = 0; early.ai.OfferArrive(); scenarioManager.scenario.step = 4; early.Tick(6500);
    assert(early.actor.flags == 1 && !early.actor.unitFlags);
    std::cout << "BASELINE: pinnedbbf4455c/native Unit queue executes raw-this point6 offer after Reset, AI replacement and phase advance, releasing gossip/protection on a revoked actor. Old AI kept alive in host; no freed-pointer crash or client visibility claim.\n";
#else
    {
        Fixture offer;
        offer.actor.flags = 0;
        offer.ai.OfferArrive(); offer.ai.OfferArrive();
        auto replay = offer.actor.m_Functions.LastQueued();
        offer.Tick(6499);
        assert(!offer.actor.flags && offer.actor.unitFlags == 0x2000380 && offer.ai.texts == std::vector<uint32>({15}));
        offer.Tick(1); replay();
        assert(offer.actor.flags == 1 && !offer.actor.unitFlags && offer.actor.react == REACT_DEFENSIVE);
        assert(offer.ai.texts == std::vector<uint32>({15,16}));
        offer.ai.OfferArrive(); offer.Tick(6500);
        assert(offer.ai.texts == std::vector<uint32>({15,16}));
    }
    {
        Fixture offer;
        offer.actor.flags = 0; offer.ai.OfferArrive();
        auto stale = offer.actor.m_Functions.LastQueued();
        offer.ai.Reset(); offer.ai.OfferArrive(); stale();
        assert(!offer.actor.flags && offer.ai.texts == std::vector<uint32>({15,15}));
        offer.Tick(6500);
        assert(offer.actor.flags == 1 && offer.ai.texts == std::vector<uint32>({15,15,16}));
    }
    for (uint32 invalid = 0; invalid < 15; ++invalid)
    {
        Fixture offer;
        offer.actor.flags = 0; offer.ai.OfferArrive();
        HruthnirAI replacement(&offer.actor);
        if (invalid == 0) offer.ai.Reset();
        if (invalid == 1) offer.actor.world = false;
        if (invalid == 2) offer.actor.alive = false;
        if (invalid == 3) offer.actor.despawn = true;
        if (invalid == 4) offer.actor.entry = 96469;
        if (invalid == 5) offer.actor.mapId = 1479;
        if (invalid == 6) offer.actor.instance = 43;
        if (invalid == 7) offer.actor.map = nullptr;
        if (invalid == 8) offer.actor.IsAIEnabled = false;
        if (invalid == 9) offer.actor.ai = &replacement;
        if (invalid == 10) scenarioManager.available = false;
        if (invalid == 11) scenarioManager.scenario.step = 4;
        if (invalid == 12) scenarioManager.scenario.identity = 944;
        if (invalid == 13) scenarioManager.scenario.step = 2;
        if (invalid == 14) offer.actor.ai = nullptr;
        offer.actor.m_Functions.Update(6500);
        assert(!offer.actor.flags && offer.actor.unitFlags == 0x2000380 && offer.ai.texts == std::vector<uint32>({15}));
        offer.actor.world = offer.actor.alive = offer.actor.IsAIEnabled = true;
        offer.actor.despawn = false; offer.actor.entry = 96468; offer.actor.mapId = 1495;
        offer.actor.instance = 42; offer.actor.map = &instanceMap; offer.actor.ai = &offer.ai; scenarioManager = {};
        offer.ai.Reset(); offer.ai.OfferArrive(); offer.Tick(6500);
        assert(offer.actor.flags == 1 && !offer.actor.unitFlags && offer.ai.texts == std::vector<uint32>({15,15,16}));
    }
    for (bool replace : {false,true})
    {
        Fixture offer;
        offer.actor.flags = 0; offer.ai.OfferArrive();
        HruthnirAI replacement(&offer.actor);
        offer.ai.onTalk = [&]() { if (replace) offer.actor.ai = &replacement; else offer.ai.Reset(); };
        offer.Tick(6500);
        assert(!offer.actor.flags && offer.actor.unitFlags == 0x2000380 && offer.ai.texts == std::vector<uint32>({15,16}));
    }
    {
        Fixture offer;
        offer.actor.flags = 0; offer.ai.OfferArrive();
        offer.actor.flags = 1; offer.Select(); offer.ai.OfferArrive(); offer.Tick(6500);
        assert(!offer.actor.flags && offer.ai.texts == std::vector<uint32>({15}));
        assert(offer.actor.motion.moves == 1);
    }
    {
        Fixture offer;
        offer.actor.flags = 0;
        auto oldAI = std::make_unique<HruthnirAI>(&offer.actor);
        offer.actor.ai = oldAI.get(); oldAI->OfferArrive(); oldAI.reset(); offer.actor.ai = &offer.ai;
        offer.Tick(6500);
        assert(!offer.actor.flags && offer.actor.unitFlags == 0x2000380 && offer.ai.texts.empty());
    }
    for (uint32 moment : {15u,16u})
    {
        Fixture offer;
        offer.actor.flags = 0;
        offer.ai.onTalk = [&]() {
            if (offer.ai.texts.back() != moment) return;
            offer.ai.onTalk = nullptr; offer.ai.Reset(); offer.ai.OfferArrive();
        };
        offer.ai.OfferArrive();
        if (moment == 16)
        {
            offer.Tick(6500);
            assert(!offer.actor.flags && offer.actor.unitFlags == 0x2000380);
        }
        offer.Tick(6500);
        assert(offer.actor.flags == 1 && !offer.actor.unitFlags);
        assert(offer.ai.texts == (moment == 15 ? std::vector<uint32>({15,15,16}) : std::vector<uint32>({15,16,15,16})));
    }
    for (bool alliance : {false,true}) for (uint32 status : {uint32(QUEST_STATUS_INCOMPLETE),uint32(QUEST_STATUS_COMPLETE)})
    {
        Fixture offer;
        offer.player.faction = uint32(alliance); offer.player.status = status;
        offer.actor.flags = 0x20; offer.actor.unitFlags |= 0x40000000;
        offer.ai.OfferArrive(); offer.Tick(6500);
        assert(offer.actor.flags == 0x21 && offer.actor.unitFlags == 0x40000000);
        offer.Select();
        offer.ai.OfferArrive(); offer.Tick(6500);
        assert(offer.actor.flags == 0x20 && offer.actor.motion.moves == 1);
        assert(offer.ai.texts == std::vector<uint32>({15,16}));
    }
    std::cout << "PASS: native6500ms point6 offer is weak/one-shot; duplicate/replayed events,15 revoked contexts with explicit Reset/readmission, deleted or retained old-AI, gossip selection and Talk15/16 Reset/new-attempt cannot reopen gossip/release protection. Four faction/status cases preserve unrelated NPC/unit flags. Unit FunctionProcessor extracted; path/arrival/re-entry routing/group/client traversal separate.\n";
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
    for (uint32 faction = 0; faction < 2; ++faction)
    for (uint32 status : {uint32(1), uint32(3)})
    for (uint32 combat = 0; combat < 4; ++combat)
    {
        Fixture fixture;
        fixture.player.faction = faction;
        fixture.player.status = status;
        fixture.ai.victim = (combat & 1) != 0;
        fixture.actor.state = (combat & 2) ? UNIT_STATE_CASTING | UNIT_STATE_STUNNED : 0;
        fixture.ai.Arrive();
        assert(!fixture.actor.conversations && !fixture.ai.magnarStartToken);
        fixture.Select();
        assert(fixture.ai.magnarArrivalPending && fixture.ai.magnarStartInstance == 42);
        fixture.actor.reenterConversation = [&fixture]() { fixture.ai.Arrive(); };
        fixture.ai.Arrive(); fixture.ai.Arrive();
        assert(!fixture.ai.magnarArrivalPending && fixture.ai.magnarStartToken && fixture.actor.conversations == 1);
        assert(fixture.ai.magnarStartToken.use_count() == 1 && !*fixture.ai.magnarStartToken);
        auto callback = fixture.actor.m_Functions.LastQueued();
        fixture.Tick(0); fixture.Tick(5999);
        assert(!fixture.bossAI.starts && !*fixture.ai.magnarStartToken);
        fixture.bossAI.reenterStart = [&fixture, callback]() { callback(); fixture.ai.Arrive(); };
        fixture.Tick(1); fixture.Tick(6000); fixture.ai.Arrive(); callback();
        assert(fixture.bossAI.starts == 1 && *fixture.ai.magnarStartToken && fixture.actor.conversations == 1);
    }
    for (uint32 faction = 0; faction < 2; ++faction)
    for (uint32 status : {uint32(1), uint32(3)})
    for (uint32 invalid = 0; invalid < 13; ++invalid)
    {
        Fixture fixture; fixture.player.faction = faction; fixture.player.status = status;
        fixture.Select();
        if (invalid == 0) scenarioManager.available = false;
        if (invalid == 1) scenarioManager.scenario.identity = 1068;
        if (invalid == 2) scenarioManager.scenario.identity = 1082;
        if (invalid == 3) scenarioManager.scenario.step = 2;
        if (invalid == 4) scenarioManager.scenario.step = 4;
        if (invalid == 5) fixture.actor.world = false;
        if (invalid == 6) fixture.actor.alive = false;
        if (invalid == 7) fixture.actor.despawn = true;
        if (invalid == 8) fixture.actor.entry = 96469;
        if (invalid == 9) fixture.actor.mapId = 1479;
        if (invalid == 10) fixture.actor.instance = 43;
        if (invalid == 11) fixture.actor.IsAIEnabled = false;
        HruthnirAI replacement(&fixture.actor);
        if (invalid == 12) fixture.actor.ai = &replacement;
        fixture.ai.Arrive(); fixture.Tick(6000);
        assert(!fixture.actor.conversations && !fixture.bossAI.starts && fixture.ai.magnarArrivalPending && fixture.ai.magnarStartToken && !*fixture.ai.magnarStartToken);
        scenarioManager = {}; fixture.actor.world = fixture.actor.alive = fixture.actor.IsAIEnabled = true;
        fixture.actor.despawn = false; fixture.actor.entry = 96468; fixture.actor.mapId = 1495; fixture.actor.instance = 42; fixture.actor.ai = &fixture.ai;
        fixture.ai.Arrive(); fixture.Tick(5999);
        assert(fixture.actor.conversations == 1 && !fixture.bossAI.starts);
        fixture.Tick(1); assert(fixture.bossAI.starts == 1);
    }
    for (uint32 invalid = 0; invalid < 5; ++invalid)
    {
        Fixture fixture; fixture.Select(); fixture.ai.Arrive(); fixture.Tick(5999);
        if (invalid == 0) scenarioManager.available = false;
        if (invalid == 1) scenarioManager.scenario.identity = 1068;
        if (invalid == 2) scenarioManager.scenario.identity = 1082;
        if (invalid == 3) scenarioManager.scenario.step = 2;
        if (invalid == 4) scenarioManager.scenario.step = 4;
        fixture.Tick(1); assert(fixture.actor.conversations == 1 && !fixture.bossAI.starts && *fixture.ai.magnarStartToken);
        scenarioManager = {}; fixture.Tick(6000); fixture.ai.Arrive();
        assert(fixture.actor.conversations == 1 && !fixture.bossAI.starts);
    }
    std::cout << "PASS:52 pre-arrival stage/actor rejection-readmission paths and5 late scenario changes gate497 and the native6000ms boss dispatch independently; no automatic arrival replay,497 cleanup or client motion certification.\n";
    for (uint32 afterArrival = 0; afterArrival < 2; ++afterArrival)
    {
        Fixture fixture;
        fixture.Select();
        if (afterArrival) { fixture.ai.Arrive(); fixture.Tick(5999); }
        uint32 unrelated = 0;
        fixture.actor.AddDelayedEvent(1, [&unrelated]() { ++unrelated; });
        fixture.ai.Reset(); fixture.ai.Arrive(); fixture.Tick(6000);
        assert(!fixture.bossAI.starts && unrelated == 1 && fixture.actor.conversations == afterArrival);
        assert(!fixture.ai.magnarArrivalPending && !fixture.ai.magnarStartToken && !fixture.ai.magnarStartInstance);
    }
    for (uint32 invalid = 0; invalid < 17; ++invalid)
    {
        Fixture fixture;
        fixture.Select(); fixture.ai.Arrive();
        if (invalid == 0) fixture.actor.world = false;
        if (invalid == 1) fixture.actor.alive = false;
        if (invalid == 2) fixture.actor.despawn = true;
        if (invalid == 3) fixture.actor.entry = 96469;
        if (invalid == 4) fixture.actor.mapId = 1511;
        if (invalid == 5) fixture.actor.instance = 43;
        if (invalid == 6) fixture.actor.boss = nullptr;
        if (invalid == 7) fixture.boss.IsAIEnabled = false;
        if (invalid == 8) fixture.boss.ai = nullptr;
        if (invalid == 9) fixture.boss.despawn = true;
        if (invalid == 10) fixture.boss.distance = 60.01f;
        if (invalid == 11) fixture.boss.world = false;
        if (invalid == 12) fixture.boss.alive = false;
        if (invalid == 13) fixture.boss.phase = false;
        if (invalid == 14) fixture.boss.map = &otherMap;
        if (invalid == 15) fixture.boss.instance = 43;
        if (invalid == 16) fixture.actor.IsAIEnabled = false;
        fixture.Tick(6000);
        assert(!fixture.bossAI.starts && *fixture.ai.magnarStartToken);
    }
    Fixture replaced;
    auto oldAI = std::make_unique<HruthnirAI>(&replaced.actor);
    replaced.actor.ai = oldAI.get();
    replaced.Select(); oldAI->Arrive();
    oldAI.reset();
    auto newAI = std::make_unique<HruthnirAI>(&replaced.actor);
    replaced.actor.ai = newAI.get();
    newAI->Reset(); newAI->Arrive(); replaced.Tick(6000);
    assert(!replaced.bossAI.starts && replaced.actor.conversations == 1);
    assert(!replaced.actor.m_Functions.SizeQueue() && !replaced.actor.m_Functions.Size());
    Fixture suspended;
    suspended.Select(); suspended.ai.Arrive();
    HruthnirAI temporary(&suspended.actor);
    suspended.actor.ai = &temporary;
    suspended.Tick(6000);
    assert(!suspended.bossAI.starts && *suspended.ai.magnarStartToken);
    Fixture restored;
    restored.Select(); restored.ai.Arrive();
    HruthnirAI restoredTemporary(&restored.actor);
    restored.actor.ai = &restoredTemporary;
    restored.Tick(1000);
    restored.actor.ai = &restored.ai;
    restored.Tick(5000);
    assert(restored.bossAI.starts == 1);
    Fixture restarted;
    restarted.Select(); restarted.ai.Arrive(); restarted.Tick(1000);
    restarted.ai.Reset();
    restarted.actor.flags |= 1;
    restarted.player.talk.menu._menuItems[0] = {};
    restarted.Select(); restarted.ai.Arrive();
    restarted.Tick(5000);
    assert(!restarted.bossAI.starts);
    restarted.Tick(1000);
    assert(restarted.bossAI.starts == 1 && restarted.actor.conversations == 2);
    Fixture duringConversation;
    duringConversation.Select();
    duringConversation.actor.reenterConversation = [&duringConversation]() { duringConversation.ai.Reset(); };
    duringConversation.ai.Arrive(); duringConversation.Tick(6000);
    assert(!duringConversation.bossAI.starts);
    std::cout << "PASS:16 faction/status/combat cue paths, native6000ms boundary, duplicate/reentrant arrival/consumption, Reset before/after arrival/during497,17 invalid actor/instance/boss contexts, unrelated callback retention, deleted and retained old-AI replacement and fresh-attempt timing. Explicit retry/497 factory acknowledgement not certified.\n";
    std::cout << "PASS: actual packet dispatch, native cached-menu getters/clear,39 rejected and explicitly readmitted contexts, both factions/statuses, stale-menu and reentrant/duplicate selection; gossip bit claimed before motion, other flags preserved. Controlled services, not client traversal.\n";
#endif
}
