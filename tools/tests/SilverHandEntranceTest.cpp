#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <vector>
using uint32=uint32_t;
using uint64=uint64_t;
enum { CLASS_PALADIN=2, QUEST_STATUS_INCOMPLETE=3, TYPEID_PLAYER=4 };
struct ObjectGuid
{
    uint64 low=1,high=7;
    bool operator<(ObjectGuid const& other) const {return high<other.high || (high==other.high && low<other.low);}
    bool operator==(ObjectGuid const& other) const {return high==other.high && low==other.low;}
};
struct Map {} liveMap,otherMap;
struct WorldObject
{
    virtual ~WorldObject()=default;
    Map* map=&liveMap;
    bool world=true,preDelete=false;
    Map* GetMap() const {return map;}
    bool IsInWorld() const {return world;}
    bool IsPreDelete() const {return preDelete;}
};
struct Player;
struct Creature;
struct Unit : WorldObject
{
    virtual Player* ToPlayer() {return nullptr;}
    virtual uint32 GetTypeId() const {return 0;}
};
struct Player : Unit
{
    ObjectGuid guid;
    bool contact=true,alive=true,teleport=false,phase=true,visible=true,los=true,rewarded=false;
    uint32 klass=2,status=3,instance=42,credits=0;
    float distance=1;
    Player* ToPlayer() override {return this;}
    uint32 GetTypeId() const override {return TYPEID_PLAYER;}
    bool CanContact() const {return contact;}
    bool IsAlive() const {return alive;}
    bool IsBeingTeleported() const {return teleport;}
    uint32 getClass() const {return klass;}
    uint32 GetInstanceId() const {return instance;}
    bool InSamePhase(Creature*) const {return phase;}
    bool canSeeOrDetect(Creature*) const {return visible;}
    bool IsWithinLOSInMap(Creature*) const {return los;}
    bool IsWithinDistInMap(Creature*,float radius) const {return distance<=radius;}
    bool GetQuestRewardStatus(uint32 quest) const {assert(quest==42120);return rewarded;}
    uint32 GetQuestStatus(uint32 quest) const {assert(quest==42120);return status;}
    ObjectGuid GetGUID() const {return guid;}
    void KilledMonsterCredit(uint32 entry) {assert(entry==106416);++credits;}
};
struct Motion
{
    uint32 moves=0;
    void MovePoint(uint32 point,float positionX,float positionY,float positionZ)
    {assert(point==1 && positionX==2137.70f && positionY==2397.60f && positionZ==118.54f);++moves;}
};
struct Creature : WorldObject
{
    bool alive=true;
    uint32 entry=106429,mapId=1539,instance=42;
    Motion motion;
    std::vector<std::function<void()>> pending;
    bool IsAlive() const {return alive;}
    uint32 GetEntry() const {return entry;}
    uint32 GetMapId() const {return mapId;}
    uint32 GetInstanceId() const {return instance;}
    bool IsWithinDistInMap(Unit* who,float radius) const {return static_cast<Player*>(who)->distance<=radius;}
    Motion* GetMotionMaster() {return &motion;}
    void AddDelayedEvent(uint32 delay,std::function<void()> callback) {assert(delay==5000);pending.push_back(std::move(callback));}
    void Run() {auto callbacks=std::move(pending);pending.clear();for(auto& callback:callbacks)callback();}
};
template<class Object>struct HashMapHolder
{
    static std::map<ObjectGuid,Object*> objects;
    static Object* Find(ObjectGuid guid) {auto found=objects.find(guid);return found==objects.end()?nullptr:found->second;}
};
template<class Object>std::map<ObjectGuid,Object*> HashMapHolder<Object>::objects;
class ObjectAccessor
{
public:
#include "WorldPlayer.inc"
#include "MapPlayer.inc"
    static Player* GetPlayer(WorldObject const&,ObjectGuid);
};
#include "AccessorPlayer.inc"
struct Scenario
{
    uint32 id=1092,step=0;
    uint32 GetScenarioId() const {return id;}
    uint32 GetCurrentStep() const {return step;}
};
struct ScenarioManager
{
    bool present=true;
    Scenario scenario;
    Scenario* GetScenario(uint32 instance) {return present && instance==42?&scenario:nullptr;}
} manager;
auto* sScenarioMgr=&manager;
#include "EntrancePolicy.inc"
struct EntranceAI
{
    Creature* me;
    bool _introDone=false;
    std::vector<uint32> talks;
    void Talk(uint32 line) {talks.push_back(line);}
#include "EntranceLos.inc"
};

void Invalidate(uint32 invalid,Player& player,Creature& actor)
{
    switch(invalid)
    {
        case 0:player.world=false;break;case 1:player.contact=false;break;case 2:player.alive=false;break;
        case 3:player.teleport=true;break;case 4:player.klass=1;break;case 5:actor.world=false;break;
        case 6:actor.alive=false;break;case 7:actor.entry=106430;break;case 8:actor.mapId=1220;break;
        case 9:player.map=nullptr;break;case 10:player.map=&otherMap;break;case 11:player.instance=43;break;
        case 12:actor.instance=43;break;case 13:player.phase=false;break;case 14:player.visible=false;break;
        case 15:player.los=false;break;case 16:player.distance=20.01f;break;case 17:player.rewarded=true;break;
        case 18:player.status=0;break;case 19:player.status=1;break;case 20:player.status=5;break;
        case 21:manager.present=false;break;case 22:manager.scenario.id=991;break;case 23:manager.scenario.id=1037;break;
        case 24:manager.scenario.step=2;break;case 25:manager.scenario.step=11;break;
    }
}
int main()
{
#ifdef BASELINE
    for(uint32 invalid:{0u,10u,18u,25u})
    {
        manager={};Player player;Creature actor;EntranceAI ai{&actor};ai.MoveInLineOfSight(&player);
        Invalidate(invalid,player,actor);actor.Run();assert(player.credits==1 && actor.motion.moves==1 && ai._introDone);
    }
    std::cout<<"PASS pinned4685321c: delayed Travard callback retains raw invoker and attempts credit/movement after logout/foreign map/abandon/completed scenario in controlled services. Not a native UAF crash or proof of persisted invalid quest credit.\n";
#else
    for(uint32 invalid=0;invalid<26;++invalid)for(bool delayed:{false,true})
    {
        manager={};Player player;Creature actor;EntranceAI ai{&actor};HashMapHolder<Player>::objects={{player.guid,&player}};
        if(delayed)ai.MoveInLineOfSight(&player);
        Invalidate(invalid,player,actor);
        if(delayed)actor.Run();else ai.MoveInLineOfSight(&player);
        assert(!player.credits && !actor.motion.moves && !ai._introDone && actor.pending.empty());
        assert(ai.talks==(delayed?std::vector<uint32>{0}:std::vector<uint32>{}));
    }
    for(uint32 step:{0u,1u})
    {
        manager={};manager.scenario.step=step;Player player;player.distance=20;Creature actor;EntranceAI ai{&actor};Unit pet,guardian;
        HashMapHolder<Player>::objects={{player.guid,&player}};
        ai.MoveInLineOfSight(nullptr);ai.MoveInLineOfSight(&pet);ai.MoveInLineOfSight(&guardian);assert(!ai._introDone);
        ai.MoveInLineOfSight(&player);ai.MoveInLineOfSight(&player);assert(actor.pending.size()==1);
        actor.Run();ai.MoveInLineOfSight(&player);actor.Run();
        assert(player.credits==1 && actor.motion.moves==1 && ai.talks==std::vector<uint32>({0,1}));
    }
    for(bool preDelete:{false,true})
    {
        manager={};Player player,other;other.guid.low=2;Creature actor;EntranceAI ai{&actor};
        HashMapHolder<Player>::objects={{player.guid,&player},{other.guid,&other}};ai.MoveInLineOfSight(&player);
        if(preDelete)player.preDelete=true;else HashMapHolder<Player>::objects.erase(player.guid);
        actor.Run();assert(!player.credits && !other.credits && !ai._introDone);
        player.preDelete=false;HashMapHolder<Player>::objects[player.guid]=&player;
        ai.MoveInLineOfSight(&player);actor.Run();assert(player.credits==1 && !other.credits);
    }
    {
        manager={};auto player=std::make_unique<Player>();Creature actor;EntranceAI ai{&actor};
        HashMapHolder<Player>::objects={{player->guid,player.get()}};ai.MoveInLineOfSight(player.get());
        HashMapHolder<Player>::objects.clear();player.reset();actor.Run();assert(!ai._introDone && !actor.motion.moves);
        Player retry;HashMapHolder<Player>::objects={{retry.guid,&retry}};ai.MoveInLineOfSight(&retry);actor.Run();assert(retry.credits==1);
    }
    std::cout<<"PASS: extracted first-Travard LOS/policy and native GUID/world/pre-delete/map lookup;26 invalid contexts before/after5s, destroyed/unregistered invoker, no nearest-player substitution, null/pet/guardian, retry and exactly-once valid boundary checked. Native quest persistence, later movement/gossip/group storyline and client traversal remain separate.\n";
#endif
}
