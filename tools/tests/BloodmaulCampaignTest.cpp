#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <vector>
using uint32 = uint32_t;
struct ObjectGuid { unsigned value = 0; void Clear() { value = 0; } bool operator<(ObjectGuid b) const { return value < b.value; } bool operator!=(ObjectGuid b) const { return value != b.value; } };
enum { TEAM_HORDE=1, TEAM_ALLIANCE=0, QUEST_STATUS_NONE=0, QUEST_STATUS_INCOMPLETE=1, QUEST_STATUS_COMPLETE=2,
    UNIT_FIELD_NPC_FLAGS=1, UNIT_NPC_FLAG_QUESTGIVER=2, REACT_PASSIVE=0, TEMPSUMMON_TIMED_DESPAWN=1,
    GAMEOBJECT_FIELD_FLAGS=1, GO_FLAG_INTERACT_COND=4 };
constexpr float INVALID_HEIGHT = -100000;
struct TempSummon;
struct Creature {
    uint32 entry=0; ObjectGuid guid, viewer; unsigned map=1116; bool removed=false;
    virtual ~Creature() = default;
    virtual TempSummon* ToTempSummon() { return nullptr; }
    void DespawnOrUnsummon() { removed=true; }
    void AddPlayerInPersonnalVisibilityList(ObjectGuid who) { viewer=who; }
    void SetFlag(int,int) { }
    void SetReactState(int) { }
    ObjectGuid GetGUID() { return guid; }
};
struct TempSummon : Creature {
    unsigned timer=60000;
    TempSummon* ToTempSummon() override { return this; }
    uint32 GetTimer() { return timer; }
    void AddDuration(uint32 n) { timer+=n; }
};
struct Map { float ground=50; float GetHeight(float,float,float) { return ground; } };
struct GameObject {
    ObjectGuid guid,owner; bool removed=false; unsigned timer=0;
    void Delete() { removed=true; }
    void RemoveFlag(int,int) { }
    void SetRespawnTime(unsigned t) { timer=t; }
    ObjectGuid GetGUID() { return guid; } ObjectGuid GetOwnerGUID() { return owner; }
};
struct Player {
    ObjectGuid guid{1}; unsigned map=1116, team=TEAM_HORDE; float x=7591.93f,y=4337.79f,z=48.5104f;
    bool alive=true, flight=false, los=true, prisoner=true;
    Map world; std::set<unsigned> rewarded; std::map<unsigned,int> quests;
    std::map<ObjectGuid,TempSummon> summons; std::vector<unsigned> credit; unsigned serial=100, spawns=0;
    std::map<ObjectGuid,GameObject> objects; bool key=false,unlocked=false;
    ObjectGuid GetGUID() { return guid; }
    unsigned GetMapId() { return map; } unsigned GetTeamId() { return team; }
    Map* GetMap() { return &world; }
    bool GetQuestRewardStatus(unsigned q) { return rewarded.count(q)!=0; }
    int GetQuestStatus(unsigned q) { return quests[q]; }
    float GetPositionX() { return x; } float GetPositionY() { return y; } float GetPositionZ() { return z; }
    float GetDistance2d(float a,float b) { return std::hypot(x-a,y-b); }
    bool IsAlive() { return alive; } bool isInFlight() { return flight; }
    bool HasItemCount(unsigned,unsigned) { return key; }
    int GetQuestObjectiveData(unsigned,unsigned) { return unlocked ? 1 : 0; }
    bool IsWithinLOSInMap(Creature*) { return los; }
    Creature* FindNearestCreature(unsigned,float) { static Creature c; return prisoner ? &c : nullptr; }
    void KilledMonsterCredit(unsigned entry) { credit.push_back(entry); }
    TempSummon* SummonCreature(uint32 entry,float,float,float,float,int,unsigned,ObjectGuid viewer) {
        ObjectGuid id{++serial}; auto& s=summons[id]; s.entry=entry; s.guid=id; s.viewer=viewer; s.map=map; ++spawns; return &s;
    }
    GameObject* SummonGameObject(unsigned,float,float,float,float,float,float,float,float,unsigned,ObjectGuid) {
        ObjectGuid id{++serial}; auto& go=objects[id]; go.guid=id; go.owner=guid; return &go;
    }
    GameObject* Shackle() { for (auto& go:objects) if(!go.second.removed) return &go.second; return nullptr; }
    unsigned Count(unsigned entry) { unsigned n=0; for(auto& s:summons) if(!s.second.removed && s.second.entry==entry) ++n; return n; }
};
struct ObjectAccessor {
    static GameObject* GetGameObject(Player& p,ObjectGuid id) {
        auto i=p.objects.find(id); return i==p.objects.end() || i->second.removed ? nullptr : &i->second;
    }
    static Creature* GetCreature(Player& p,ObjectGuid id) {
        auto i=p.summons.find(id); return i==p.summons.end() || i->second.removed || i->second.map!=p.map ? nullptr : &i->second;
    }
};
struct PlayerScript {
    PlayerScript(char const*) {} virtual ~PlayerScript()=default;
    virtual void OnLogout(Player*) {} virtual void OnMapChanged(Player*) {} virtual void OnUpdate(Player*,uint32) {}
};
struct GameObjectScript {
    GameObjectScript(char const*) {} virtual ~GameObjectScript()=default;
    virtual bool OnGossipHello(Player*,GameObject*) { return false; }
};

// Compile the actual production actor/recovery implementation against a small
// world boundary. This is not an imitation of its update or cleanup handler.
#include "../../src/server/scripts/Draenor/draenor_campaign_recovery.cpp"

int main()
{
    using namespace BloodmaulCampaign;
    player_bloodmaul_campaign_recovery script;
    Player p;
    script.OnUpdate(&p,2000); assert(p.Count(78746)==0 && p.credit.empty());
    p.quests[SearchForBwuja]=QUEST_STATUS_INCOMPLETE;
    p.prisoner=false; script.OnUpdate(&p,2000); assert(p.credit.empty());
    p.prisoner=true; p.los=false; script.OnUpdate(&p,2000); assert(p.credit.empty());
    p.los=true; script.OnUpdate(&p,2000); assert(p.credit.back()==78060);
    p.quests[SearchForBwuja]=QUEST_STATUS_NONE; p.credit.clear();
    p.quests[OutOfTheChains]=QUEST_STATUS_INCOMPLETE;
    script.OnUpdate(&p,2000); assert(!p.Shackle()); // no key: no unlock
    p.key=true; script.OnUpdate(&p,2000); assert(p.Shackle() && p.credit.empty());
    go_bwuja_shackle shackleScript;
    GameObject* shackle=p.Shackle();
    assert(!shackleScript.OnGossipHello(&p,shackle)); // allow native GO handler
    p.key=false; assert(shackleScript.OnGossipHello(&p,shackle)); // key lost after spawn
    p.key=true; Player intruder; intruder.guid={9}; intruder.key=true;
    intruder.quests[OutOfTheChains]=QUEST_STATUS_INCOMPLETE;
    assert(shackleScript.OnGossipHello(&intruder,shackle));
    p.quests[OutOfTheChains]=QUEST_STATUS_NONE;
    assert(shackleScript.OnGossipHello(&p,shackle));
    script.OnUpdate(&p,2000); assert(!p.Shackle()); // abandon cleanup
    p.quests[OutOfTheChains]=QUEST_STATUS_INCOMPLETE;
    script.OnUpdate(&p,2000); assert(p.Shackle());
    p.unlocked=true; script.OnUpdate(&p,2000); assert(!p.Shackle()); // no respawn after use
    p.quests[OutOfTheChains]=QUEST_STATUS_COMPLETE;
    p.rewarded.insert(GearingUp); script.OnUpdate(&p,2000);
    assert(p.Count(78746)==1 && p.credit.empty());
    unsigned created=p.spawns;
    for(auto& s:p.summons) s.second.timer=1000;
    script.OnUpdate(&p,2000); assert(p.spawns==created);
    for(auto& s:p.summons) assert(s.second.timer==60000 && s.second.viewer.value==p.guid.value);
    p.map=1; script.OnMapChanged(&p); p.map=1116; script.OnMapChanged(&p);
    script.OnUpdate(&p,2000); assert(p.spawns==created); // quick map return: no duplicates
    script.OnLogout(&p); assert(p.Count(78746)==0);
    script.OnUpdate(&p,2000); assert(p.Count(78746)==1); // login/abandon recovery
    p.quests[SeekingTruth]=QUEST_STATUS_INCOMPLETE;
    script.OnUpdate(&p,2000); assert(p.Count(78746)==0 && p.credit.empty());
    p.x=7384; p.y=5027; p.z=150; p.world.ground=50;
    script.OnUpdate(&p,2000); assert(p.credit.empty() && p.Count(78785)==0); // flying over target
    p.z=50; p.flight=true; script.OnUpdate(&p,2000); assert(p.credit.empty());
    p.flight=false; p.alive=false; script.OnUpdate(&p,2000); assert(p.credit.empty());
    p.alive=true; p.world.ground=INVALID_HEIGHT; script.OnUpdate(&p,2000); assert(p.credit.empty());
    p.world.ground=50; script.OnUpdate(&p,2000); assert(p.Count(78785)==1 && p.credit.back()==78252);
    p.credit.clear(); p.x=7500; script.OnUpdate(&p,2000); assert(p.credit.empty() && p.Count(78785)==0);
    p.x=7384; p.quests[SeekingTruth]=QUEST_STATUS_COMPLETE;
    script.OnUpdate(&p,2000); assert(p.Count(78785)==1 && p.credit.empty());
    p.quests[SeekingTruth]=QUEST_STATUS_NONE; p.rewarded.insert(SeekingTruth);
    script.OnUpdate(&p,2000); assert(p.Count(78785)==1 && p.credit.empty()); // next quest giver retained
    p.rewarded.insert(ShadowGate); script.OnUpdate(&p,2000); assert(p.Count(78785)==0);
    Player other; other.guid={2}; other.team=TEAM_ALLIANCE; other.rewarded.insert(GearingUp);
    script.OnUpdate(&other,2000); assert(other.spawns==0 && other.credit.empty());
    assert(!InsideBorgalPOI(7320,4998)); // bounding-box corner is outside polygon
    std::cout << "PASS: production Bloodmaul actor recovery, ownership, map/login/abandon lifecycle, discovery location and ground checks; no acceptance/spawn credit.\n";
}
