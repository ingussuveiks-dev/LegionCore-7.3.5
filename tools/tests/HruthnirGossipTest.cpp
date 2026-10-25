#include <cassert>
#include <array>
#include <atomic>
#include <cstdint>
#include <functional>
#include <iostream>
#include <limits>
#include <list>
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
    UNIT_FLAG_IMMUNE_TO_NPC = 0x200, UNIT_FLAG_NOT_ATTACKABLE_1 = 0x80, REACT_DEFENSIVE = 1, REACT_PASSIVE = 0,
    UNIT_STATE_DIED = 1, SPELL_AURA_FEIGN_DEATH = 2, UNIT_STATE_CASTING = 4, UNIT_STATE_STUNNED = 8,
    UNIT_STATE_ROAMING = 16, UNIT_STATE_ROAMING_MOVE = 32, POINT_MOTION_TYPE = 8, WAYPOINT_MOTION_TYPE = 2,
    CRITERIA_TYPE_SCRIPT_EVENT_2 = 92,
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
    using LowType = uint64;
    static ObjectGuid const Empty;
    uint32 identity = 1, kind = 1;
    explicit operator bool() const { return identity != 0; }
    bool operator==(ObjectGuid other) const { return identity == other.identity && kind == other.kind; }
    bool IsCreatureOrVehicle() const { return kind == 1; }
    bool IsGameObject() const { return kind == 2; }
    void Clear() { identity = 0; }
    std::string ToString() const { return std::to_string(identity); }
};
ObjectGuid const ObjectGuid::Empty{0,0};
struct Player;
struct Creature;
using Unit = Creature;
enum DamageEffectType { DIRECT_DAMAGE };
struct Map;
enum class HighGuid { Conversation };
struct GuidGenerator { uint64 current = 0; uint64 Generate() { return ++current; } } guidGenerator;
struct ObjectManager { template<HighGuid> GuidGenerator* GetGenerator() { return &guidGenerator; } } objectManager;
auto* sObjectMgr = &objectManager;
struct Conversation;
struct ConversationPool
{
    std::vector<Conversation*> objects;
    ~ConversationPool();
} conversationPool;
struct Conversation
{
    bool m_isUpdate = false, world = false;
    bool predelete = false;
    Map* map = nullptr;
    ObjectGuid guid = ObjectGuid::Empty;
    std::function<void()> onRemove;
    inline static uint32 attempts = 0, deletions = 0, removals = 0, queued = 0;
    ~Conversation() { ++deletions; }
    bool CreateConversation(uint64, uint32, Creature*, std::nullptr_t, Creature const&);
    bool IsInWorld() const { return world; }
    Map* GetMap() const { return map; }
    ObjectGuid GetGUID() const { return guid; }
    bool IsPreDelete() const { return predelete; }
    void RemoveFromWorld() { assert(world); world = false; ++removals; if (onRemove) { auto callback = std::move(onRemove); callback(); } }
    void AddObjectToRemoveList() { ++queued; }
#include "ConversationRemove.inc"
};
ConversationPool::~ConversationPool() { for (auto* conversation : objects) delete conversation; }
struct CreatureAI
{
    virtual ~CreatureAI() = default;
    virtual void Reset() { }
    virtual void JustDied(Unit*) { }
    virtual void DamageTaken(Unit*, uint32&, DamageEffectType) { }
    virtual void UpdateAI(uint32) { }
    virtual void MovementInform(uint32, uint32) { }
    virtual uint32 GetData(uint32) const { return 0; }
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
    ObjectGuid GetGUID() const { assert(false); return {}; }
};
struct InstanceScript
{
    bool credit = true;
    uint32 generation = 1;
    uint32 GetData(uint32 identity) const { assert(identity == 1059 || identity == 1060); return identity == 1059 ? uint32(credit) : generation; }
    void HandleGameObject(ObjectGuid, bool) { assert(false); }
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
        assert((point == 7 && positionX == 4720.80f && positionY == 347.91f && positionZ == -37.71f) ||
            (point == 6 && positionX == 4763.73f && positionY == 340.87f && positionZ == -37.19f));
        ++moves;
        if (reenter) { auto callback = std::move(reenter); callback(); }
    }
};
struct Creature
{
    struct HomeRecord
    {
        std::array<float,4> values{1.0f,2.0f,3.0f,0.5f};
        uint32 updates = 0;
        void Relocate(float positionX, float positionY, float positionZ, float orientation)
        {
            values = {positionX,positionY,positionZ,orientation}; ++updates;
        }
    } m_homePosition;
    std::array<float,4> pose{10.0f,20.0f,30.0f,1.25f};
    float GetPositionX() const { return pose[0]; }
    float GetPositionY() const { return pose[1]; }
    float GetPositionZ() const { return pose[2]; }
    float GetOrientation() const { return pose[3]; }
#include "SetHome.inc"
    struct Spline { bool finalized = true; bool Finalized() const { return finalized; } } spline;
    Spline* movespline = &spline;
    struct Info { uint32 ScriptID = 7; } info;
    bool world = true, alive = true, despawn = false, phase = true, visible = true, los = true;
    bool controlled = false, hostile = false;
    bool IsAIEnabled = true;
    uint32 state = 0, conversations = 0;
    std::function<void()> reenterConversation;
    bool conversationSucceeds = true;
    float distance = 4.0f;
    float arrivalDistanceSq = 0.0f;
    float offerDistanceSq = 0.0f;
    uint32 entry = 96468, mapId = 1495, instance = 42, flags = 1, LastUsedScriptID = 7;
    uint32 unitFlags = 0x2000380, react = 0;
    Map* map = &instanceMap;
    CreatureAI* ai = nullptr;
    Creature* boss = nullptr;
    InstanceScript request;
    InstanceScript* instanceScript = &request;
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
    GameObject* FindNearestGameObject(uint32, float) { assert(false); return nullptr; }
    InstanceScript* GetInstanceScript() const { return instanceScript; }
    void StopAttack() { }
    Player* FindNearestPlayer(float) { assert(false); return nullptr; }
    void Kill(Creature*) { assert(false); }
    void DespawnOrUnsummon(uint32) { assert(false); }
    float GetExactDistSq(float positionX, float positionY, float positionZ) const
    {
        if (positionX == 4763.73f && positionY == 340.87f && positionZ == -37.19f)
            return offerDistanceSq;
        assert(positionX == 4720.80f && positionY == 347.91f && positionZ == -37.71f);
        return arrivalDistanceSq;
    }
    bool HasUnitState(uint32 mask) const { return (state & mask) != 0; }
    void ClearUnitState(uint32 mask) { state &= ~mask; }
    Creature* FindNearestCreature(uint32 expected, float range, bool living)
    {
        assert(expected == 96034 && range == 60.0f && living);
        return boss && boss->world && boss->alive && boss->distance <= range && boss->phase &&
            boss->map == map && boss->instance == instance ? boss : nullptr;
    }
    void CreateConversation(uint32 conversation);
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
    template<typename... Args> void UpdateAchievementCriteria(Args...) { assert(false); }
};
void GetCreatureListWithEntryInGrid(std::list<Creature*>&, Creature*, uint32, float) { assert(false); }
#include "Policy.inc"
struct CreatureScript
{
    virtual ~CreatureScript() = default;
    virtual bool OnGossipSelect(Player*, Creature*, uint32, uint32) = 0;
    virtual bool OnGossipHello(Player*, Creature*) = 0;
};
bool Conversation::CreateConversation(uint64 lowGuid, uint32 identity, Creature* caster, std::nullptr_t, Creature const& position)
{
    assert(identity == 497 && caster == &position);
    ++attempts;
    if (!caster->conversationSucceeds) return false;
    guid = {uint32(lowGuid),3};
    map = caster->GetMap();
    world = true;
    conversationPool.objects.push_back(this);
    ++caster->conversations;
    if (caster->reenterConversation) { auto callback = std::move(caster->reenterConversation); callback(); }
    return true;
}
#include "UnitConversation.inc"
using WorldObject = Creature;
struct ObjectAccessor
{
    template<class Target> static Target* GetObjectInWorld(ObjectGuid guid, Target*)
    {
        for (Conversation* conversation : conversationPool.objects)
            if (conversation->guid == guid && conversation->world) return conversation;
        return nullptr;
    }
#include "ObjectInMap.inc"
    static Conversation* GetConversation(WorldObject const&, ObjectGuid);
};
#include "GetConversation.inc"
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
#include "AIDestructor.inc"
    bool UpdateVictim() { return victim; }
    void DoCast(uint32) { ++casts; }
    void DoMeleeAttackIfReady() { ++melees; }
    void AttackStart(Creature*) { assert(false); }
#include "AIFields.inc"
#include "AISelect.inc"
#include "AIReset.inc"
#include "AIDeath.inc"
#include "AIDamage.inc"
#include "Cleanup.inc"
#include "OfferContext.inc"
#include "OfferPosition.inc"
#include "Request.inc"
#include "Ready.inc"
#include "Conversation.inc"
#include "IntroContext.inc"
#include "Chamber.inc"
#include "Offer.inc"
#include "Arrival.inc"
#include "Movement.inc"
#include "AIUpdate.inc"
};
template<class T> struct PointMovementGenerator
{
    uint32 _movementId = 7;
    void DoFinalize(T& owner);
    void MovementInform(T& owner);
};
#include "PointFinalize.inc"
#include "PointInform.inc"
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
#ifdef CURRENT_REQUEST
        ai.magnarChamberStarted = true;
        ai.magnarIntroInstanceGeneration = actor.request.generation;
#endif
        actor.boss = &boss;
        boss.entry = 96034;
        boss.ai = &bossAI;
        player.talk.menu._menuItems[0] = {};
    }
    void PrepareSelection()
    {
#ifdef CURRENT_READY
        if (!ai.magnarOfferToken && !ai.magnarStartToken && !ai.magnarArrivalPending && ai.texts.empty())
        {
            ai.magnarOfferToken = std::make_shared<bool>(true);
            ai.magnarOfferReady = true;
        }
#endif
    }
    void Select(bool prepare = true) { if (prepare) PrepareSelection(); session.HandleGossipSelectOption(packet); }
    void Tick(uint32 diff) { actor.m_Functions.Update(diff); actor.ai->UpdateAI(diff); }
};
int main()
{
#ifdef DEATH_BASELINE
    Fixture fixture; fixture.Select(); fixture.ai.Arrive();
    auto* owned = conversationPool.objects.back(); auto token = fixture.ai.magnarStartToken;
    fixture.actor.alive = false; fixture.ai.JustDied(&fixture.boss);
    assert(owned->world && fixture.ai.magnarConversation == owned->GetGUID() && fixture.ai.magnarStartToken == token);
    std::cout << "PASS death baseline: pinneda16f352b/default empty CreatureAI death hook leaves accepted497 registered and retains the selected weak cue owner after companion death. Direct native-hook dispatch with controlled actor services, not a live ordinary combat death.\n";
#elif defined(CLEANUP_BASELINE)
    for (uint32 cancel = 0; cancel < 3; ++cancel)
    {
        Fixture fixture; fixture.Select(); fixture.ai.Arrive();
        auto* conversation = conversationPool.objects.back();
        if (cancel == 0) fixture.ai.Reset();
        if (cancel == 1) { ++fixture.actor.request.generation; fixture.ai.StartChamber(); }
        if (cancel == 2)
        {
            Creature actor; auto ai = std::make_unique<HruthnirAI>(&actor); actor.ai = ai.get();
            ai->magnarStartToken = std::make_shared<bool>(false); ai->magnarStartInstance = 42;
            assert(ai->StartMagnarConversation()); conversation = conversationPool.objects.back(); ai.reset();
        }
        assert(conversation->world);
    }
    std::cout << "PASS cleanup baseline: pinned127df428 leaves an accepted497 registered after Reset, new1060 epoch and actual AI destruction. Controlled registry/map services, not client traversal.\n";
#elif defined(CONVERSATION_BASELINE)
    {
        Fixture fixture; fixture.Select(); fixture.actor.conversationSucceeds = false;
        uint32 deleted = Conversation::deletions;
        fixture.ai.Arrive(); fixture.Tick(6000);
        assert(!fixture.actor.conversations && Conversation::deletions == deleted + 1);
        assert(!fixture.ai.magnarArrivalPending && fixture.bossAI.starts == 1);
    }
    {
        Fixture fixture; fixture.Select();
        uint32 removed = Conversation::removals;
        fixture.actor.reenterConversation = [&]() { fixture.ai.Reset(); };
        fixture.ai.Arrive(); fixture.Tick(6000);
        assert(conversationPool.objects.back()->world && Conversation::removals == removed && !fixture.bossAI.starts);
    }
    std::cout << "PASS conversation baseline: pinned688c16b9/native Unit void wrapper deletes a failed497 allocation but still consumes point7 and starts boss at6000ms. Reset during successful factory insertion leaves that object registered. Controlled factory/world services, not a live/client failure reproduction.\n";
#elif defined(READY_BASELINE)
    for (uint32 invalid = 0; invalid < 5; ++invalid)
    {
        Fixture fixture;
        if (invalid == 1) { fixture.ai.OfferArrive(); fixture.actor.flags = 1; }
        if (invalid == 2) { fixture.ai.OfferArrive(); fixture.Tick(13000); fixture.ai.Reset(); fixture.actor.flags = 1; }
        if (invalid == 3) ++fixture.actor.request.generation;
        if (invalid == 4) fixture.actor.IsAIEnabled = false;
        assert(!scriptManager.script.OnGossipHello(&fixture.player,&fixture.actor));
        fixture.Select(false);
        assert(fixture.actor.motion.moves == 1 && fixture.ai.magnarStartToken);
    }
    std::cout << "PASS ready baseline: pinnedda79baae accepts flag/cached-menu selection without offer, during pending voice, after Reset, on new epoch or disabled AI. Native menu dispatch, controlled actor services; no live packet exploit claimed.\n";
#elif defined(REQUEST_BASELINE)
    for (uint32 invalid = 0; invalid < 4; ++invalid)
    {
        Fixture offer; offer.actor.flags = 0;
        offer.ai.magnarChamberStarted = true; offer.ai.magnarIntroInstanceGeneration = 1;
        if (invalid == 0) offer.ai.magnarChamberStarted = false;
        if (invalid == 1) offer.actor.instanceScript = nullptr;
        if (invalid == 2) offer.actor.request.credit = false;
        if (invalid == 3) ++offer.actor.request.generation;
        offer.ai.OfferArrive(); offer.Tick(13000);
        assert(offer.actor.flags == 1 && offer.ai.texts == std::vector<uint32>({15,16}));
    }
    Fixture epoch; epoch.actor.flags = 0;
    epoch.ai.magnarChamberStarted = true; epoch.ai.magnarIntroInstanceGeneration = 1;
    epoch.ai.OfferArrive(); auto stale = epoch.actor.m_Functions.LastQueued();
    ++epoch.actor.request.generation; epoch.ai.StartChamber(); stale();
    assert(epoch.actor.flags == 1 && epoch.ai.texts == std::vector<uint32>({15,14,16}));
    std::cout << "PASS request baseline: pinned3d445173 admits point6 without chamber request/current instance credit/epoch; native action2 rearm retains an old offer and its saved callback opens immediately in the new epoch. Controlled instance marker, not a live Initialize/scene replay.\n";
#elif defined(HOME_BASELINE)
    for (uint32 point : {6u,7u})
        for (uint32 invalid = 0; invalid < 3; ++invalid)
        {
            Fixture fixture; if (point == 7) fixture.Select();
            if (invalid == 0) fixture.actor.offerDistanceSq = fixture.actor.arrivalDistanceSq = 100.0f;
            if (invalid == 1) scenarioManager.scenario.step = 2;
            if (invalid == 2) fixture.actor.IsAIEnabled = false;
            fixture.ai.MovementInform(POINT_MOTION_TYPE,point);
            assert(fixture.actor.m_homePosition.updates == 1 && fixture.actor.m_homePosition.values == fixture.actor.pose);
            assert(!fixture.ai.magnarOfferToken && !fixture.actor.conversations);
        }
    std::cout << "PASS home baseline: pinned5e944704 full MovementInform writes the home pose before rejecting point6/7 endpoint, stage or disabled-AI contexts. Actual setter body; Relocate/actor pose controlled, not a live evade/pathfinding reproduction.\n";
#elif defined(OFFER_ARRIVAL_BASELINE)
    for (uint32 moment = 0; moment < 3; ++moment)
    {
        Fixture offer; offer.actor.flags = 0;
        PointMovementGenerator<Creature> point; point._movementId = 6;
        if (moment == 0) offer.actor.offerDistanceSq = 100.0f;
        point.DoFinalize(offer.actor);
        if (moment == 1) offer.actor.offerDistanceSq = 100.0f;
        if (moment == 2) offer.ai.onTalk = [&]() { offer.actor.offerDistanceSq = 100.0f; };
        offer.Tick(13000);
        assert(offer.actor.flags == 1 && !offer.actor.unitFlags && offer.ai.texts == std::vector<uint32>({15,16}));
    }
    std::cout << "PASS offer arrival baseline: pinned9a0e4426 opens gossip/releases protection at10-yard separation after early native point6 finalization, late drift or Talk16 drift. Spline/AI/text/position services controlled, not live client movement.\n";
#elif defined(ARRIVAL_BASELINE)
    Fixture stopped; stopped.Select(); stopped.actor.arrivalDistanceSq = 100.0f;
    PointMovementGenerator<Creature> point;
    point.DoFinalize(stopped.actor); stopped.Tick(6000);
    assert(stopped.actor.conversations == 1 && stopped.bossAI.starts == 1);
    std::cout << "PASS arrival baseline: native Point DoFinalize/MovementInform forwards a controlled finalized spline at10-yard separation; pinned31d576d9 consumes point7 and requests497/queued boss action. Spline/AI bridge/boss are controlled, not actual combat progression or a client stop reproduction.\n";
#elif defined(VOICE_BASELINE)
    Fixture offer; offer.actor.flags = 0; offer.ai.OfferArrive();
    offer.Tick(6499); assert(!offer.actor.flags && offer.ai.texts == std::vector<uint32>({15}));
    offer.Tick(1); assert(offer.actor.flags == 1 && offer.ai.texts == std::vector<uint32>({15,16}));
    std::cout << "PASS voice baseline: pinned c7239252 sends Talk16 and releases gossip at6500ms, before the complete native60403 voice's12629ms boundary. Controlled text sender; actual audio mixing remains a client check.\n";
#elif defined(STAGE_BASELINE)
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
    for (uint32 faction : {0u,1u})
        for (uint32 status : {uint32(QUEST_STATUS_INCOMPLETE),uint32(QUEST_STATUS_COMPLETE)})
            for (uint32 phase = 0; phase < 4; ++phase)
                for (uint32 killer = 0; killer < 4; ++killer)
                {
                    Fixture fixture; fixture.player.faction = faction; fixture.player.status = status;
                    if (phase == 0) fixture.ai.OfferArrive();
                    else { fixture.Select(); if (phase >= 2) fixture.ai.Arrive(); if (phase == 3) fixture.Tick(6000); }
                    Conversation* owned = phase >= 2 ? conversationPool.objects.back() : nullptr;
                    auto* unrelated = new Conversation;
                    assert(unrelated->CreateConversation(guidGenerator.Generate(),497,&fixture.actor,nullptr,fixture.actor));
                    Unit playerKiller, petKiller, guardianKiller;
                    Unit* killers[] = {&playerKiller,&petKiller,&guardianKiller,nullptr};
                    uint32 damage = std::numeric_limits<uint32>::max(); fixture.ai.DamageTaken(killers[killer],damage,DIRECT_DAMAGE);
                    assert(!damage && fixture.actor.alive);
                    fixture.actor.alive = false; fixture.ai.JustDied(killers[killer]);
                    assert(!fixture.ai.magnarStartToken && !fixture.ai.magnarOfferToken && !fixture.ai.magnarArrivalPending && !fixture.ai.magnarOfferReady);
                    assert(!fixture.ai.magnarConversation && (!owned || !owned->world) && unrelated->world && !(fixture.actor.flags & UNIT_NPC_FLAG_GOSSIP));
                    fixture.ai.JustDied(killers[killer]); fixture.Tick(20000);
                    assert(fixture.bossAI.starts == uint32(phase == 3) && unrelated->world);
                    unrelated->Remove();
                    fixture.actor.alive = true; fixture.ai.Reset(); fixture.actor.flags |= 1;
                    fixture.player.talk.menu._menuItems[0] = {}; fixture.ai.OfferArrive(); fixture.Tick(13000);
                    fixture.Select(false); fixture.ai.Arrive(); fixture.Tick(5999);
                    assert(fixture.bossAI.starts == uint32(phase == 3));
                    fixture.Tick(1); assert(fixture.bossAI.starts == uint32(phase == 3) + 1);
                }
    std::cout << "PASS death:64 faction/status/offer-or-selected-or-accepted-or-started/killer-label cases retain ordinary zero-damage protection, revoke only owned dialogue/offer/cue on explicit death and preserve unrelated497. Explicitly reissued point6/selection/point7 after projected revival retains full13000/6000ms budgets, not automatic motion or actual boss Reset. Repeated/null/player/pet/guardian-pointer hooks do not credit anyone; killer labels/services controlled, not real native pet-owner combat.\n";
    for (uint32 faction : {0u,1u})
        for (uint32 status : {uint32(QUEST_STATUS_INCOMPLETE),uint32(QUEST_STATUS_COMPLETE)})
            for (uint32 cancel = 0; cancel < 2; ++cancel)
                for (uint32 delay : {0u,6000u})
                {
                    Fixture fixture; fixture.player.faction = faction; fixture.player.status = status;
                    fixture.Select(); fixture.ai.Arrive(); auto* owned = conversationPool.objects.back();
                    auto* unrelated = new Conversation;
                    assert(unrelated->CreateConversation(guidGenerator.Generate(),497,&fixture.actor,nullptr,fixture.actor));
                    fixture.Tick(delay); assert(owned->world && fixture.bossAI.starts == uint32(delay == 6000));
                    uint32 removed = Conversation::removals, queued = Conversation::queued;
                    if (!cancel) fixture.ai.Reset();
                    else { ++fixture.actor.request.generation; fixture.ai.StartChamber(); }
                    assert(!fixture.ai.magnarConversation && !owned->world && unrelated->world);
                    assert(Conversation::removals == removed + 1 && Conversation::queued == queued + 1);
                    fixture.ai.Reset(); fixture.Tick(6000);
                    assert(Conversation::removals == removed + 1 && fixture.bossAI.starts == uint32(delay == 6000));
                    unrelated->Remove();
                }
    for (uint32 invalid = 0; invalid < 5; ++invalid)
    {
        Fixture fixture; fixture.Select(); fixture.ai.Arrive(); auto* owned = conversationPool.objects.back();
        if (invalid == 0) owned->Remove();
        if (invalid == 1) owned->predelete = true;
        if (invalid == 2) owned->map = &otherMap;
        if (invalid == 3) fixture.actor.map = nullptr;
        if (invalid == 4) fixture.ai.magnarConversation = {999999,3};
        uint32 removed = Conversation::removals;
        fixture.ai.Reset(); fixture.ai.Reset();
        assert(!fixture.ai.magnarConversation && Conversation::removals == removed);
        fixture.actor.map = &instanceMap; owned->map = &instanceMap; owned->predelete = false; owned->Remove();
    }
    {
        Creature actor; auto ai = std::make_unique<HruthnirAI>(&actor); actor.ai = ai.get();
        ai->magnarStartToken = std::make_shared<bool>(false); ai->magnarStartInstance = 42;
        scenarioManager = {};
        assert(ai->StartMagnarConversation()); auto* owned = conversationPool.objects.back();
        ai.reset(); actor.ai = nullptr;
        assert(!owned->world);
    }
    for (bool epoch : {false,true})
    {
        Fixture fixture; fixture.Select(); fixture.ai.Arrive(); auto* old = conversationPool.objects.back();
        old->onRemove = [&]() {
            fixture.ai.magnarStartToken = std::make_shared<bool>(false); fixture.ai.magnarStartInstance = 42;
            assert(fixture.ai.StartMagnarConversation());
        };
        if (epoch) { ++fixture.actor.request.generation; fixture.ai.StartChamber(); }
        else fixture.ai.Reset();
        auto* fresh = conversationPool.objects.back();
        assert(fresh != old && !old->world && fresh->world && fixture.ai.magnarConversation == fresh->GetGUID());
        fixture.ai.Reset(); assert(!fresh->world && !fixture.ai.magnarConversation);
    }
    std::cout << "PASS cleanup:16 faction/status/Reset-or-new-epoch/before-or-after-cue paths remove only owned497 once; unrelated497 survives. Missing/expired/predeleted/foreign-map/null-map lookups clear only local claim; actual AI destructor and clear-before-lookup reentrant replacement tested. Actual native GetConversation/GetObjectInMap/Remove bodies, controlled registration/removal services. Natural6000ms success keeps dialogue alive; implicit abandonment/map-unload and client playback remain separate.\n";
    for (uint32 faction : {0u,1u})
        for (uint32 status : {uint32(QUEST_STATUS_INCOMPLETE),uint32(QUEST_STATUS_COMPLETE)})
        {
            Fixture fixture; fixture.player.faction = faction; fixture.player.status = status; fixture.Select();
            fixture.actor.conversationSucceeds = false;
            uint32 attempts = Conversation::attempts, deleted = Conversation::deletions;
            auto token = fixture.ai.magnarStartToken;
            fixture.ai.Arrive(); fixture.ai.Arrive(); fixture.Tick(6000);
            assert(fixture.ai.magnarArrivalPending && fixture.ai.magnarStartToken == token && !*token);
            assert(!fixture.actor.conversations && !fixture.bossAI.starts && !fixture.actor.m_Functions.SizeQueue() && !fixture.actor.m_Functions.Size());
            assert(Conversation::attempts == attempts + 2 && Conversation::deletions == deleted + 2);
            fixture.actor.conversationSucceeds = true; fixture.ai.Arrive(); fixture.ai.Arrive();
            assert(fixture.actor.conversations == 1 && !fixture.ai.magnarArrivalPending && Conversation::attempts == attempts + 3);
            fixture.Tick(5999); assert(!fixture.bossAI.starts);
            fixture.Tick(1); fixture.ai.Arrive(); fixture.Tick(6000);
            assert(fixture.bossAI.starts == 1 && Conversation::attempts == attempts + 3);
        }
    for (uint32 invalid = 0; invalid < 8; ++invalid)
    {
        Fixture fixture; fixture.Select();
        HruthnirAI replacement(&fixture.actor);
        uint32 removed = Conversation::removals, queued = Conversation::queued;
        fixture.actor.reenterConversation = [&]() {
            if (invalid == 0) fixture.ai.Reset();
            if (invalid == 1) fixture.actor.ai = &replacement;
            if (invalid == 2) scenarioManager.scenario.step = 4;
            if (invalid == 3) fixture.actor.world = false;
            if (invalid == 4) fixture.actor.instance = 43;
            if (invalid == 5) *fixture.ai.magnarStartToken = true;
            if (invalid == 6) fixture.ai.magnarStartToken = std::make_shared<bool>(false);
            if (invalid == 7) conversationPool.objects.back()->Remove();
        };
        fixture.ai.Arrive(); fixture.actor.m_Functions.Update(6000);
        assert(fixture.ai.magnarArrivalPending == (invalid == 7) && !fixture.bossAI.starts && !conversationPool.objects.back()->world);
        assert(Conversation::removals == removed + 1 && Conversation::queued == queued + 1);
        if (invalid == 7) { fixture.ai.Arrive(); fixture.Tick(6000); assert(fixture.bossAI.starts == 1); }
    }
    {
        Fixture fixture; fixture.Select();
        std::size_t first = conversationPool.objects.size();
        fixture.actor.reenterConversation = [&]() {
            fixture.ai.Reset(); fixture.actor.flags = 1; fixture.player.talk.menu._menuItems[0] = {};
            fixture.Select(); fixture.ai.Arrive();
        };
        fixture.ai.Arrive();
        assert(conversationPool.objects.size() == first + 2);
        assert(!conversationPool.objects[first]->world && conversationPool.objects[first + 1]->world);
        assert(!fixture.ai.magnarArrivalPending && fixture.ai.magnarStartToken && !*fixture.ai.magnarStartToken);
        fixture.Tick(5999); assert(!fixture.bossAI.starts);
        fixture.Tick(1); assert(fixture.bossAI.starts == 1);
    }
    std::cout << "PASS497:4 faction/status two-failure/deletion/no-cue paths retain the same unconsumed claim for explicit successful retry/full5999+1ms cue. Seven creation-time state revocations and an already-removed object roll back through actual Remove; reentrant fresh-token success survives old rollback. Controlled factory/insertion/world services; no automatic retry or client delivery certificate.\n";
    for (uint32 faction : {0u,1u})
        for (uint32 status : {uint32(QUEST_STATUS_INCOMPLETE),uint32(QUEST_STATUS_COMPLETE)})
            for (uint32 invalid = 0; invalid < 15; ++invalid)
            {
                Fixture fixture; fixture.player.faction = faction; fixture.player.status = status;
                fixture.actor.flags = 0x80; fixture.ai.OfferArrive(); fixture.Tick(13000);
                assert(fixture.ai.GetData(1061) == 1);
                HruthnirAI replacement(&fixture.actor);
                if (invalid == 0) fixture.ai.magnarOfferReady = false;
                if (invalid == 1) fixture.ai.magnarOfferToken.reset();
                if (invalid == 2) *fixture.ai.magnarOfferToken = false;
                if (invalid == 3) fixture.ai.magnarChamberStarted = false;
                if (invalid == 4) fixture.actor.instanceScript = nullptr;
                if (invalid == 5) fixture.actor.request.credit = false;
                if (invalid == 6) ++fixture.actor.request.generation;
                if (invalid == 7) fixture.actor.IsAIEnabled = false;
                if (invalid == 8) fixture.actor.offerDistanceSq = std::numeric_limits<float>::infinity();
                if (invalid == 9) fixture.actor.offerDistanceSq = 4.04f;
                if (invalid == 10) fixture.actor.offerDistanceSq = std::numeric_limits<float>::quiet_NaN();
                if (invalid == 11) fixture.ai.magnarArrivalPending = true;
                if (invalid == 12) fixture.ai.magnarStartToken = std::make_shared<bool>(false);
                if (invalid == 13) fixture.actor.ai = &replacement;
                if (invalid == 14) { fixture.ai.Reset(); assert(fixture.actor.flags == 0x80); fixture.actor.flags |= 1; }
                assert(!fixture.ai.GetData(1061));
                auto start = fixture.ai.magnarStartToken;
                assert(scriptManager.script.OnGossipHello(&fixture.player,&fixture.actor));
                assert(!fixture.player.talk.menu.GetItem(0));
                fixture.player.talk.menu._menuItems[0] = {};
                fixture.Select(false);
                assert(!fixture.actor.motion.moves && fixture.ai.magnarStartToken == start && fixture.actor.flags == 0x81);
                fixture.actor.instanceScript = &fixture.actor.request; fixture.actor.request.credit = true;
                fixture.actor.IsAIEnabled = true; fixture.actor.ai = &fixture.ai; fixture.actor.offerDistanceSq = 0;
                fixture.ai.Reset(); fixture.ai.magnarChamberStarted = false; fixture.ai.StartChamber();
                assert(fixture.actor.flags == 0x80 && !fixture.ai.GetData(1061));
                fixture.ai.OfferArrive(); fixture.Tick(12999); assert(!fixture.ai.GetData(1061));
                fixture.Tick(1); assert(fixture.ai.GetData(1061) == 1 && fixture.actor.flags == 0x81);
                assert(!scriptManager.script.OnGossipHello(&fixture.player,&fixture.actor));
                fixture.player.talk.menu._menuItems[0] = {}; fixture.Select(false);
                assert(fixture.actor.motion.moves == 2 && fixture.actor.flags == 0x80 && !fixture.ai.GetData(1061));
                fixture.actor.flags |= 1; fixture.player.talk.menu._menuItems[0] = {}; fixture.Select(false);
                assert(fixture.actor.motion.moves == 2);
            }
    {
        Fixture fixture; fixture.actor.flags = 0x80;
        fixture.ai.onTalk = [&]() {
            if (fixture.ai.texts.back() != 16) return;
            fixture.actor.flags |= 1;
            assert(!fixture.ai.GetData(1061));
            fixture.Select(false); assert(!fixture.actor.motion.moves);
        };
        fixture.ai.OfferArrive(); fixture.Tick(13000);
        assert(fixture.ai.GetData(1061) == 1 && fixture.actor.flags == 0x81);
        ++fixture.actor.request.generation; fixture.ai.StartChamber();
        assert(!fixture.ai.magnarOfferReady && fixture.actor.flags == 0x80);
    }
    std::cout << "PASS readiness:60 faction/status/authority revocations close hello and reject independently restored cached selection; actual action2/point6/full13000ms reoffers once, preserving other flag bits. Talk16 reentrant selection precedes ready publication; epoch/Reset clear readiness/gossip. Ordinary positive selection fixtures project an already-ready offer; these new cases use the actual producer. Client visibility/traversal remains separate.\n";
    {
        Fixture missing; missing.actor.ai = nullptr;
        assert(scriptManager.script.OnGossipHello(&missing.player,&missing.actor));
        assert(!missing.player.talk.menu.GetItem(0));
    }
    for (uint32 faction : {0u,1u})
        for (uint32 status : {uint32(QUEST_STATUS_INCOMPLETE),uint32(QUEST_STATUS_COMPLETE)})
            for (uint32 invalid = 0; invalid < 4; ++invalid)
            {
                Fixture offer; offer.player.faction = faction; offer.player.status = status; offer.actor.flags = 0;
                auto home = offer.actor.m_homePosition.values;
                if (invalid == 0) offer.ai.magnarChamberStarted = false;
                if (invalid == 1) offer.actor.instanceScript = nullptr;
                if (invalid == 2) offer.actor.request.credit = false;
                if (invalid == 3) ++offer.actor.request.generation;
                offer.ai.MovementInform(POINT_MOTION_TYPE,6); offer.Tick(13000);
                assert(!offer.ai.magnarOfferToken && offer.ai.texts.empty() && !offer.actor.flags && offer.actor.m_homePosition.values == home);
                offer.actor.instanceScript = &offer.actor.request; offer.actor.request.credit = true;
                offer.ai.magnarChamberStarted = false; offer.ai.StartChamber();
                assert(offer.ai.HasMagnarChamberRequest() && offer.actor.motion.moves == 1 && offer.ai.texts == std::vector<uint32>({14}));
                offer.ai.OfferArrive(); offer.Tick(12999); assert(!offer.actor.flags);
                offer.Tick(1); assert(offer.actor.flags == 1 && offer.ai.texts == std::vector<uint32>({14,15,16}));
            }
    for (uint32 moment : {0u,16u})
        for (uint32 invalid = 0; invalid < 4; ++invalid)
        {
            Fixture offer; offer.actor.flags = 0; offer.ai.OfferArrive();
            auto revoke = [&]() {
                if (invalid == 0) offer.ai.magnarChamberStarted = false;
                if (invalid == 1) offer.actor.instanceScript = nullptr;
                if (invalid == 2) offer.actor.request.credit = false;
                if (invalid == 3) ++offer.actor.request.generation;
            };
            if (!moment) revoke(); else offer.ai.onTalk = revoke;
            offer.Tick(13000);
            assert(!offer.actor.flags && offer.actor.unitFlags == 0x2000380 && offer.ai.texts.size() == (moment ? 2u : 1u));
        }
    {
        Fixture epoch; epoch.actor.flags = 0; epoch.ai.OfferArrive();
        auto stale = epoch.actor.m_Functions.LastQueued();
        auto oldToken = epoch.ai.magnarOfferToken;
        ++epoch.actor.request.generation; epoch.ai.StartChamber();
        assert(!epoch.ai.magnarOfferToken && !epoch.ai.magnarStartToken && !epoch.ai.magnarArrivalPending && epoch.ai.magnarStartInstance == 0);
        epoch.ai.OfferArrive(); auto fresh = epoch.ai.magnarOfferToken; stale();
        assert(fresh && fresh != oldToken && !*fresh && !epoch.actor.flags);
        epoch.Tick(12999); assert(!epoch.actor.flags);
        epoch.Tick(1); assert(epoch.actor.flags == 1 && epoch.ai.texts == std::vector<uint32>({15,14,15,16}));
    }
    {
        Fixture epoch; epoch.Select(); epoch.ai.Arrive();
        std::weak_ptr<bool> oldToken = epoch.ai.magnarStartToken;
        ++epoch.actor.request.generation; epoch.ai.StartChamber(); epoch.Tick(6000);
        assert(oldToken.expired() && !epoch.bossAI.starts && !epoch.ai.magnarArrivalPending && !epoch.ai.magnarStartToken && epoch.ai.magnarStartInstance == 0);
    }
    std::cout << "PASS request:16 initial request/instance/credit/epoch rejections with actual action2 readmission,8 late/Talk16 revocations, fresh epoch offer full12999+1ms budget/old callback isolation and old point7 cue cancellation. Controlled marker; same-epoch physical request identity/selected owner/client remain open.\n";
    for (uint32 point : {6u,7u})
        for (uint32 faction : {0u,1u})
            for (uint32 status : {uint32(QUEST_STATUS_INCOMPLETE),uint32(QUEST_STATUS_COMPLETE)})
            {
                for (uint32 invalid = 0; invalid < 8; ++invalid)
                {
                    Fixture fixture; fixture.player.faction = faction; fixture.player.status = status;
                    if (point == 7 && invalid != 7) fixture.Select();
                    auto home = fixture.actor.m_homePosition.values;
                    if (invalid == 0) fixture.actor.offerDistanceSq = fixture.actor.arrivalDistanceSq = 100.0f;
                    if (invalid == 1) fixture.actor.offerDistanceSq = fixture.actor.arrivalDistanceSq = std::numeric_limits<float>::quiet_NaN();
                    if (invalid == 2) fixture.actor.offerDistanceSq = fixture.actor.arrivalDistanceSq = std::numeric_limits<float>::infinity();
                    if (invalid == 3) scenarioManager.scenario.step = 2;
                    if (invalid == 4) fixture.actor.world = false;
                    if (invalid == 5) fixture.actor.IsAIEnabled = false;
                    if (invalid == 6) fixture.actor.instance = 43;
                    if (invalid == 7 && point == 6) fixture.ai.magnarOfferToken = std::make_shared<bool>(false);
                    fixture.ai.MovementInform(POINT_MOTION_TYPE,point);
                    assert(!fixture.actor.m_homePosition.updates && fixture.actor.m_homePosition.values == home);
                    assert(fixture.ai.texts.empty() && !fixture.actor.conversations);
                }
                Fixture fixture; fixture.player.faction = faction; fixture.player.status = status;
                fixture.actor.pose = point == 6 ? std::array<float,4>{4763.73f,340.87f,-37.19f,1.25f} :
                    std::array<float,4>{4720.80f,347.91f,-37.71f,1.25f};
                if (point == 7) fixture.Select();
                fixture.ai.MovementInform(POINT_MOTION_TYPE,point);
                auto home = fixture.actor.m_homePosition.values;
                assert(fixture.actor.m_homePosition.updates == 1 && home == fixture.actor.pose);
                fixture.actor.pose[3] = 2.5f;
                fixture.ai.MovementInform(POINT_MOTION_TYPE,point);
                assert(fixture.actor.m_homePosition.updates == 1 && fixture.actor.m_homePosition.values == home);
            }
    for (auto notification : {std::array<uint32,2>{POINT_MOTION_TYPE,99u},std::array<uint32,2>{WAYPOINT_MOTION_TYPE,99u},std::array<uint32,2>{123u,6u}})
    {
        Fixture legacy; legacy.ai.MovementInform(notification[0],notification[1]);
        assert(legacy.actor.m_homePosition.updates == 1 && legacy.actor.m_homePosition.values == legacy.actor.pose);
    }
    std::cout << "PASS home: full native MovementInform/setter preserve home across64 point6/7 rejections;8 admitted faction/status paths copy actual pose/orientation once, duplicates retain it;3 legacy notification kinds retain their prior home update. Pose/Relocate services controlled; no native evade/client motion claim.\n";
    for (uint32 faction : {0u,1u})
        for (uint32 status : {uint32(QUEST_STATUS_INCOMPLETE),uint32(QUEST_STATUS_COMPLETE)})
            for (float distanceSq : {0.0f,3.96f,4.0f,4.04f,100.0f,
                std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()})
            {
                Fixture offer; offer.player.faction = faction; offer.player.status = status;
                offer.actor.flags = 0; offer.actor.offerDistanceSq = distanceSq;
                PointMovementGenerator<Creature> point; point._movementId = 6;
                point.DoFinalize(offer.actor); point.DoFinalize(offer.actor);
                if (!(distanceSq <= 4.0f))
                {
                    offer.Tick(13000);
                    assert(!offer.ai.magnarOfferToken && offer.ai.texts.empty() && !offer.actor.flags && offer.actor.unitFlags == 0x2000380);
                    offer.actor.offerDistanceSq = 0.0f;
                    point.DoFinalize(offer.actor); point.DoFinalize(offer.actor);
                }
                auto token = offer.ai.magnarOfferToken;
                assert(token && !*token && offer.ai.texts == std::vector<uint32>({15}));
                offer.Tick(12999); assert(!offer.actor.flags && offer.actor.unitFlags == 0x2000380);
                offer.Tick(1); point.DoFinalize(offer.actor); offer.Tick(13000);
                assert(offer.actor.flags == 1 && !offer.actor.unitFlags && *token && offer.ai.texts == std::vector<uint32>({15,16}));
            }
    for (uint32 moment : {15u,0u,16u})
        for (float distanceSq : {4.04f,std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()})
        {
            Fixture offer; offer.actor.flags = 0;
            offer.ai.onTalk = [&]() { if (offer.ai.texts.back() == moment) offer.actor.offerDistanceSq = distanceSq; };
            offer.ai.OfferArrive();
            auto oldToken = offer.ai.magnarOfferToken;
            auto replay = offer.actor.m_Functions.LastQueued();
            if (!moment) offer.actor.offerDistanceSq = distanceSq;
            offer.Tick(13000);
            assert(!offer.actor.flags && offer.actor.unitFlags == 0x2000380 && !offer.ai.magnarOfferToken);
            assert(offer.ai.texts.size() == (moment == 16 ? 2u : 1u));
            offer.ai.onTalk = nullptr; offer.actor.offerDistanceSq = 0.0f;
            offer.ai.OfferArrive();
            auto newToken = offer.ai.magnarOfferToken;
            assert(newToken && newToken != oldToken);
            replay();
            assert(offer.ai.magnarOfferToken == newToken && !*newToken && !offer.actor.flags);
            offer.Tick(12999); assert(!offer.actor.flags);
            offer.Tick(1); assert(offer.actor.flags == 1 && !offer.actor.unitFlags && *newToken);
        }
    std::cout << "PASS offer arrival:28 faction/status/ordered-radius cases reject early point6 outside its endpoint;9 Talk15/late/Talk16 drift paths retire only the old offer token and permit explicit fresh arrival with full12999+1ms budget. Replayed old callback cannot retire a new offer; controlled services, not automatic motion recovery.\n";
    {
        Fixture moving; moving.Select(); moving.actor.spline.finalized = false;
        PointMovementGenerator<Creature> point;
        moving.actor.state |= UNIT_STATE_ROAMING | UNIT_STATE_ROAMING_MOVE;
        point.DoFinalize(moving.actor); moving.Tick(6000);
        assert(!moving.actor.state && moving.ai.magnarArrivalPending && !moving.actor.conversations && !moving.bossAI.starts);
    }
    for (uint32 faction : {0u,1u})
        for (uint32 status : {uint32(QUEST_STATUS_INCOMPLETE),uint32(QUEST_STATUS_COMPLETE)})
            for (float distanceSq : {0.0f,3.96f,4.0f,4.04f,100.0f,
                std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()})
            {
                Fixture arrival; arrival.player.faction = faction; arrival.player.status = status;
                arrival.Select(); arrival.actor.arrivalDistanceSq = distanceSq;
                auto token = arrival.ai.magnarStartToken;
                PointMovementGenerator<Creature> point;
                point.DoFinalize(arrival.actor); point.DoFinalize(arrival.actor);
                if (!(distanceSq <= 4.0f))
                {
                    arrival.Tick(6000);
                    assert(arrival.ai.magnarArrivalPending && arrival.ai.magnarStartToken == token && !*token);
                    assert(!arrival.actor.conversations && !arrival.bossAI.starts);
                    arrival.actor.arrivalDistanceSq = 0.0f;
                    arrival.ai.Arrive(); arrival.ai.Arrive();
                }
                assert(!arrival.ai.magnarArrivalPending && arrival.actor.conversations == 1);
                arrival.Tick(5999); assert(!arrival.bossAI.starts && !*token);
                arrival.Tick(1); arrival.ai.Arrive(); arrival.Tick(6000);
                assert(arrival.bossAI.starts == 1 && *token && arrival.actor.conversations == 1);
            }
    std::cout << "PASS arrival:28 faction/status/ordered-radius cases retain an invalid arrival claim and permit one later explicit valid arrival; native queue5999+1 and duplicate suppression remain. Controlled geometry; separate actual Position test required.\n";
    {
        Fixture offer;
        offer.actor.flags = 0;
        offer.ai.OfferArrive(); offer.ai.OfferArrive();
        auto replay = offer.actor.m_Functions.LastQueued();
        offer.Tick(12999);
        assert(!offer.actor.flags && offer.actor.unitFlags == 0x2000380 && offer.ai.texts == std::vector<uint32>({15}));
        offer.Tick(1); replay();
        assert(offer.actor.flags == 1 && !offer.actor.unitFlags && offer.actor.react == REACT_DEFENSIVE);
        assert(offer.ai.texts == std::vector<uint32>({15,16}));
        offer.ai.OfferArrive(); offer.Tick(13000);
        assert(offer.ai.texts == std::vector<uint32>({15,16}));
    }
    {
        Fixture offer;
        offer.actor.flags = 0; offer.ai.OfferArrive();
        auto stale = offer.actor.m_Functions.LastQueued();
        offer.ai.Reset(); offer.ai.OfferArrive(); stale();
        assert(!offer.actor.flags && offer.ai.texts == std::vector<uint32>({15,15}));
        offer.Tick(13000);
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
        offer.actor.m_Functions.Update(13000);
        assert(!offer.actor.flags && offer.actor.unitFlags == 0x2000380 && offer.ai.texts == std::vector<uint32>({15}));
        offer.actor.world = offer.actor.alive = offer.actor.IsAIEnabled = true;
        offer.actor.despawn = false; offer.actor.entry = 96468; offer.actor.mapId = 1495;
        offer.actor.instance = 42; offer.actor.map = &instanceMap; offer.actor.ai = &offer.ai; scenarioManager = {};
        offer.ai.Reset(); offer.ai.OfferArrive(); offer.Tick(13000);
        assert(offer.actor.flags == 1 && !offer.actor.unitFlags && offer.ai.texts == std::vector<uint32>({15,15,16}));
    }
    for (bool replace : {false,true})
    {
        Fixture offer;
        offer.actor.flags = 0; offer.ai.OfferArrive();
        HruthnirAI replacement(&offer.actor);
        offer.ai.onTalk = [&]() { if (replace) offer.actor.ai = &replacement; else offer.ai.Reset(); };
        offer.Tick(13000);
        assert(!offer.actor.flags && offer.actor.unitFlags == 0x2000380 && offer.ai.texts == std::vector<uint32>({15,16}));
    }
    {
        Fixture offer;
        offer.actor.flags = 0; offer.ai.OfferArrive();
        offer.actor.flags = 1; offer.Select(); offer.ai.OfferArrive(); offer.Tick(13000);
        assert(offer.actor.flags == 1 && offer.ai.texts == std::vector<uint32>({15,16}));
        assert(!offer.actor.motion.moves);
    }
    {
        Fixture offer;
        offer.actor.flags = 0;
        auto oldAI = std::make_unique<HruthnirAI>(&offer.actor);
        offer.actor.ai = oldAI.get(); oldAI->OfferArrive(); oldAI.reset(); offer.actor.ai = &offer.ai;
        offer.Tick(13000);
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
            offer.Tick(13000);
            assert(!offer.actor.flags && offer.actor.unitFlags == 0x2000380);
        }
        offer.Tick(13000);
        assert(offer.actor.flags == 1 && !offer.actor.unitFlags);
        assert(offer.ai.texts == (moment == 15 ? std::vector<uint32>({15,15,16}) : std::vector<uint32>({15,16,15,16})));
    }
    for (bool alliance : {false,true}) for (uint32 status : {uint32(QUEST_STATUS_INCOMPLETE),uint32(QUEST_STATUS_COMPLETE)})
    {
        Fixture offer;
        offer.player.faction = uint32(alliance); offer.player.status = status;
        offer.actor.flags = 0x20; offer.actor.unitFlags |= 0x40000000;
        offer.ai.OfferArrive(); offer.Tick(13000);
        assert(offer.actor.flags == 0x21 && offer.actor.unitFlags == 0x40000000);
        offer.Select();
        offer.ai.OfferArrive(); offer.Tick(13000);
        assert(offer.actor.flags == 0x20 && offer.actor.motion.moves == 1);
        assert(offer.ai.texts == std::vector<uint32>({15,16}));
    }
    std::cout << "PASS: scoped13000ms point6 offer is weak/one-shot; duplicate/replayed events,15 revoked contexts with explicit Reset/readmission, deleted or retained old-AI, gossip selection and Talk15/16 Reset/new-attempt cannot reopen gossip/release protection. Four faction/status cases preserve unrelated NPC/unit flags. Unit FunctionProcessor extracted; path/arrival/re-entry routing/group/client traversal separate.\n";
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
        fixture.PrepareSelection();
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
        assert(fixture.actor.motion.moves == 1);
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
    std::cout << "PASS:52 pre-arrival stage/actor rejection-readmission paths and5 late scenario changes gate497 and the native6000ms boss dispatch independently; no automatic arrival replay, implicit owner-abandon cleanup or client motion certification.\n";
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
    oldAI->magnarChamberStarted = true; oldAI->magnarIntroInstanceGeneration = replaced.actor.request.generation;
    oldAI->OfferArrive(); replaced.Tick(13000); replaced.Select(false); oldAI->Arrive();
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
    restarted.ai.OfferArrive(); restarted.Tick(13000);
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
    std::cout << "PASS:16 faction/status/combat cue paths, native6000ms boundary, duplicate/reentrant arrival/consumption, Reset before/after arrival/during497,17 invalid actor/instance/boss contexts, unrelated callback retention, deleted and retained old-AI replacement and fresh-attempt timing. Native497 factory/explicit cleanup covered separately; rendered delivery and implicit owner-abandon/death/map-unload lifetime remain open.\n";
    std::cout << "PASS: actual packet dispatch, native cached-menu getters/clear,39 rejected and explicitly readmitted contexts, both factions/statuses, stale-menu and reentrant/duplicate selection; gossip bit claimed before motion, other flags preserved. Controlled services, not client traversal.\n";
#endif
}
