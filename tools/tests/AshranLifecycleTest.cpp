#include <cassert>
#include <cstdint>
#include <map>
#include <set>
#include <vector>
#include <mutex>
#include <iostream>
#include <functional>
using uint32 = uint32_t;
using uint16 = uint16_t;
using ObjectGuid = uint64_t;
constexpr uint32 AshranMapID = 1191, AshranNeutralMapID = 1116, AshranZoneID = 8485, PlayerMinLevel = 110;
constexpr uint32 AshranPreAreaAlliance = 7332, AshranPreAreaHorde = 7333;
constexpr uint32 TEAM_ALLIANCE = 0, TEAM_HORDE = 1, TEAM_NEUTRAL = 2, QUEST_STATUS_NONE = 0, MAX_QUEST_LOG_SIZE = 25;
constexpr uint32 QUEST_FLAGS_EX_REMOVE_QUEST_ON_WEEKLY_RESET = 2097152, DIFFICULTY_PVEVP_SCENARIO = 25;
struct Quest
{
    uint32 id;
    bool weekly = true, expire = false;
    bool IsWeekly() const
    {
        return weekly;
    }
    bool HasFlagEx(uint32) const
    {
        return expire;
    }
};
struct Quests
{
    std::map<uint32, Quest> quests;
    Quest const* GetQuestTemplate(uint32 id)
    {
        auto i = quests.find(id);
        return i == quests.end() ? nullptr : &i->second;
    }
} store;
auto sQuestDataStore = &store;
struct DB2
{
    uint32 GetQuestUniqueBitFlag(uint32 id)
    {
        return id;
    }
} db2;
auto& sDB2Manager = db2;
struct Player
{
    uint32 level = 110, team = 0, map = 1191, zone = 8485, area = 7279;
    ObjectGuid guid = 1;
    bool gm = false, alive = true, combat = false, flight = false, teleport = false, transport = false, vehicle = false,
         inWorld = true, canTake = true, canAdd = true;
    int additions = 0, travels = 0, worldStates = 0;
    std::map<uint32, uint32> status;
    std::set<uint32> m_weeklyquests, m_weeklyquestSaves;
    bool m_WeeklyQuestChanged = true;
    uint32 getLevel()
    {
        return level;
    }
    bool isGameMaster()
    {
        return gm;
    }
    uint32 GetTeamId()
    {
        return team;
    }
    uint32 GetQuestStatus(uint32 id)
    {
        return status[id];
    }
    bool CanTakeQuest(Quest const*, bool)
    {
        return canTake;
    }
    bool CanAddQuest(Quest const*, bool)
    {
        return canAdd;
    }
    void AddQuest(Quest const* q, void*)
    {
        status[q->id] = 1;
        ++additions;
    }
    ObjectGuid GetGUID()
    {
        return guid;
    }
    bool IsInWorld()
    {
        return inWorld;
    }
    uint32 GetAreaId()
    {
        return area;
    }
    uint32 GetMapId()
    {
        return map;
    }
    uint32 GetZoneId()
    {
        return zone;
    }
    bool IsAlive()
    {
        return alive;
    }
    bool isInCombat()
    {
        return combat;
    }
    bool isInFlight()
    {
        return flight;
    }
    bool IsBeingTeleported()
    {
        return teleport;
    }
    bool GetTransport()
    {
        return transport;
    }
    bool GetVehicle()
    {
        return vehicle;
    }
    void SafeTeleport(uint32 dest, Player*)
    {
        ++travels;
        map = dest;
    }
    void SendInitWorldStates(uint32, uint32)
    {
        ++worldStates;
    }
    uint16 FindQuestSlot(uint32 id)
    {
        return status[id] ? uint16(id == 38923 ? 0 : 1) : MAX_QUEST_LOG_SIZE;
    }
    void SetQuestSlot(uint16, uint32)
    {
    }
    void RemoveActiveQuest(uint32 id)
    {
        status.erase(id);
    }
    void SetQuestCompletedBit(uint32, bool)
    {
    }
    void ResetWeeklyQuestStatus();
};
#include "offer.inc"
#include "weekly.inc"
struct CreatureData
{
    uint32 id;
};
struct GOData
{
    uint32 mapid;
};
struct Objects
{
    uint32 deleted=0;
    void DeleteGOData(uint64_t) {++deleted;}
    std::map<uint32, GOData> rows;
    GOData const* GetGOData(uint32 id)
    {
        auto i = rows.find(id);
        return i == rows.end() ? nullptr : &i->second;
    }
} objects;
auto sObjectMgr = &objects;
struct OutdoorPvP
{
};
struct Actor
{
};
struct Reference
{
    Player* p;
    Player* getSource() const
    {
        return p;
    }
};
struct References : std::vector<Reference>
{
    bool isEmpty() const
    {
        return empty();
    }
};
struct Map
{
    uint32 GetId() {return 1191;}
    uint32 GetInstanceId() {return 7;}
    bool unloading = false;
    References players;
    std::set<OutdoorPvP*>* OutdoorPvPList = nullptr;
    References const& GetPlayers()
    {
        return players;
    }
    bool IsMapUnload()
    {
        return unloading;
    }
    Actor* GetCreature(ObjectGuid)
    {
        return nullptr;
    }
    Actor* GetGameObject(ObjectGuid)
    {
        return nullptr;
    }
};
bool IsAshranInstance(Map* map) {return map && map->GetId()==1191 && map->GetInstanceId()!=0;}
struct CaptureGuid {uint64_t value=77; void Clear(){value=0;} uint64_t GetCounter(){return value;} void operator=(uint64_t n){value=n;}};
struct CaptureOwner {Map* map;Map* GetMap(){return map;}};
struct CaptureObject {uint32 deletes=0;void SetRespawnTime(uint32){}void Delete(){++deletes;}};
struct OPvPCapturePoint {CaptureOwner* m_PvP;CaptureObject* m_capturePoint;CaptureGuid m_capturePointGUID;bool DelCapturePoint();};
#include "capture-delete.inc"
constexpr uint32 QUEST_FLAGS_EX_CLEAR_PROGRESS_OF_CRITERIA_TREE_OBJECTIVES_ON_ACCEPT=16777216;
struct CriteriaTree {std::vector<CriteriaTree const*> Children;bool Criteria=false;};
struct Progress {uint32 removed=0;void RemoveCriteriaProgress(CriteriaTree const*){++removed;}};
struct ClearCriteria {Progress* m_achievementMgr;void Accept(uint32 quest_id,Quest const* quest,CriteriaTree const* tree) {
#include "criteria-clear.inc"
}};
struct Battle : OutdoorPvP
{
    int initialized = 0, entered = 0, left = 0, areaEntered = 0, areaLeft = 0, invited = 0;
    void Initialize(uint32)
    {
        ++initialized;
    }
    void OnCreatureCreate(Actor*)
    {
    }
    void OnGameObjectCreate(Actor*)
    {
    }
    void HandlePlayerEnterMap(ObjectGuid, uint32)
    {
        ++entered;
    }
    void HandlePlayerLeaveMap(ObjectGuid, uint32)
    {
        ++left;
    }
    void HandlePlayerEnterArea(ObjectGuid, uint32)
    {
        ++areaEntered;
    }
    void HandlePlayerLeaveArea(ObjectGuid, uint32)
    {
        ++areaLeft;
    }
    void HandleBFMGREntryInviteResponse(bool, Player*)
    {
        ++invited;
    }
};
struct Instance
{
    Map* instance;
    Battle _battle;
    std::set<OutdoorPvP*> _controllers;
    std::set<uint32> _managedCreatures, _managedObjects;
    std::set<ObjectGuid> _pendingCreatures, _pendingObjects;
    std::map<ObjectGuid, uint32> _areas;
    uint32 _poll = 1000;
    bool _ready = false;
#include "instance.inc"
};
struct Entry
{
#include "entry.inc"
};
struct MapMgr
{
    uint32 next = 0;
    uint32 GenerateInstanceId()
    {
        return ++next;
    }
} manager;
auto sMapMgr = &manager;
struct Maps
{
    std::recursive_mutex m_lock;
    std::map<uint32, Map*> m_InstancedMaps;
    uint32 difficulty = 0;
    Map* CreateInstance(uint32 id, void*, uint32 diff)
    {
        difficulty = diff;
        return m_InstancedMaps[id] = new Map;
    }
    Map* Select(uint32 mapId)
    {
#include "shared.inc"
        return nullptr;
    }
    ~Maps()
    {
        for (auto const& i : m_InstancedMaps)
            delete i.second;
    }
};
int main()
{
    for (uint32 id : {38923u, 38925u, 39090u, 39096u})
        store.quests[id] = {id, true, id == 38923 || id == 38925};
    Player a, b;
    b.guid = 2;
    b.team = TEAM_HORDE;
    OfferAshranQuests(&a);
    assert(a.status[38925] == 1 && a.status[39096] == 1 && a.additions == 2);
    OfferAshranQuests(&a);
    assert(a.additions == 2);
    b.status[38923] = 2;
    OfferAshranQuests(&b);
    assert(b.additions == 1 && b.status[39090] == 1);
    Player full;
    full.canAdd = false;
    OfferAshranQuests(&full);
    assert(!full.additions);
    full.canAdd = true;
    OfferAshranQuests(&full);
    assert(full.additions == 2);
    Player low;
    low.level = 109;
    OfferAshranQuests(&low);
    assert(!low.additions);
    b.ResetWeeklyQuestStatus();
    assert(b.status[38923] == 0 && b.status[39090] == 1); // no rewarded weekly needed
    b.status[38923] = 1;
    b.m_weeklyquests.insert(39090);
    b.m_weeklyquestSaves.insert(39090);
    b.ResetWeeklyQuestStatus();
    assert(!b.status[38923] && b.status[39090] == 1 && b.m_weeklyquests.empty());
    Map map;
    Instance instance{&map};
    instance.Update(1000);
    assert(!instance._ready);
    map.players.push_back({&a});
    map.players.push_back({&b});
    instance.Update(1000);
    assert(instance._ready && instance._battle.initialized == 1);
    assert(instance._areas.size() == 2 && instance._battle.invited == 2 && map.OutdoorPvPList->size() == 1);
    instance.Update(1000);
    assert(instance._battle.initialized == 1 && instance._battle.invited == 2);
    a.area = 7099;
    instance.Update(1000);
    assert(instance._battle.areaLeft == 1 && instance._battle.areaEntered == 3);
    instance.OnPlayerLeave(&a);
    instance.OnPlayerLeave(&a);
    assert(instance._battle.left == 1 && instance._areas.size() == 1);
    instance.Update(1000);
    assert(instance._battle.invited == 3); // reentry registers exactly once
    instance._managedCreatures.insert(82876);
    CreatureData managed{82876}, unmanaged{82909};
    assert(instance.GetCreatureEntry(1, &managed) == 0 && instance.GetCreatureEntry(2, &unmanaged) == 82909);
    instance._managedObjects.insert(123);
    objects.rows[1] = {1191};
    objects.rows[2] = {1116};
    assert(instance.GetGameObjectEntry(1, 123) == 0 && instance.GetGameObjectEntry(2, 123) == 123 &&
           instance.GetGameObjectEntry(999, 123) == 123);
    Map unloading;
    unloading.unloading=true;
    CaptureOwner captureOwner{&unloading};
    OPvPCapturePoint capture{&captureOwner,reinterpret_cast<CaptureObject*>(uintptr_t(1))};
    assert(capture.DelCapturePoint() && capture.m_capturePoint==nullptr && capture.m_capturePointGUID.value==0);
    CaptureObject liveFlag;
    unloading.unloading=false;
    capture.m_capturePoint=&liveFlag;
    capture.DelCapturePoint();
    assert(liveFlag.deletes==1 && objects.deleted==0);
    CriteriaTree leaf{{},true}, middle{{&leaf},true}, root{{&middle},false};
    Progress progress;
    ClearCriteria cleaner{&progress};
    cleaner.Accept(38923,&store.quests[38923],&root);
    assert(progress.removed==2);
    cleaner.Accept(39090,&store.quests[38923],&root);
    assert(progress.removed==2); // other quest paths are unchanged
    Maps maps;
    auto first = maps.Select(1191);
    assert(first == maps.Select(1191) && maps.difficulty == 25);
    assert(maps.Select(1374) == nullptr);
    first->unloading = true;
    assert(first != maps.Select(1191));
    Entry entry;
    Player p;
    p.map = 1116;
    p.zone = 6941;
    p.area = 7332;
    entry.OnUpdate(&p, 1000);
    assert(!p.travels);
    p.area = 7279;
    p.combat = true;
    entry.OnUpdate(&p, 1000);
    assert(!p.travels);
    p.combat = false;
    p.level = 109;
    entry.OnUpdate(&p, 1000);
    assert(!p.travels);
    p.level = 110;
    entry.OnUpdate(&p, 1000);
    assert(p.travels == 1 && p.map == 1191);
    entry.OnUpdate(&p, 1000);
    assert(p.travels == 1);
    std::cout << "Ashran lifecycle, shared instance, quest reset/acquisition and entry checks passed\n";
}
