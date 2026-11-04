#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <shared_mutex>
#include <utility>
#include <vector>
using uint8=uint8_t;
using uint16=uint16_t;
using uint32=uint32_t;
using uint64=uint64_t;
using Milliseconds=std::chrono::milliseconds;
#include "EventMapClass.inc"
#include "EventMapMethods.inc"
#include "FunctionProcessor.inc"
enum {TYPEID_PLAYER=4,CRITERIA_TYPE_SCRIPT_EVENT_2=92,CLASS_ROGUE=4,QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_COMPLETE=1};
enum class HighGuid {Conversation};
struct ObjectGuid
{
    uint64 high=1,low=1;
    uint32 GetGUIDLow() const{return uint32(low);}
    bool operator==(ObjectGuid const& other) const{return high==other.high && low==other.low;}
};
struct Progress{uint64 Counter=0;};
struct Scenario{uint32 id=1123,stage=0;std::map<uint32,Progress> progress;uint32 GetScenarioId()const{return id;}auto& GetAchievementMgr(){return *this;}auto GetCriteriaProgressMap()const{return &progress;}bool CanUpdateCriteria(uint32 leaf)const{auto at=progress.find(leaf);return stage==0&&(at==progress.end()||!at->second.Counter);}};
struct ScenarioManager{Scenario* current=nullptr;Scenario* GetScenario(uint32){return current;}}scenarioManager;
auto sScenarioMgr=&scenarioManager;
struct Map {uint32 id=1620;Scenario scenario;};
struct WorldObject
{
    Map* map=nullptr;
    bool inWorld=true,preDelete=false,alive=true,expired=false,scope=true;
    uint32 phase=1;
    ObjectGuid guid;
    Map* GetMap() const{return map;}
    ObjectGuid GetGUID() const{return guid;}
    bool IsInWorld() const{return inWorld;}
    bool IsPreDelete() const{return preDelete;}
    bool IsAlive()const{return alive;}bool IsQuestObjectExpired()const{return expired;}bool CanTargetQuestObject(WorldObject const* other)const{return scope&&other->scope;}
    uint32 GetMapId()const{return map?map->id:0;}uint32 GetInstanceId()const{return 1;}
    bool InSamePhase(WorldObject const* other) const{return phase==other->phase;}
};
struct Player;
struct Unit:WorldObject
{
    virtual ~Unit()=default;
    uint32 type=0;
    uint32 GetTypeId() const{return type;}
    Player* ToPlayer();
};
struct Player:Unit
{
    bool contact=true;
    bool gm=false,pending=false,rewarded=false,grantFirst=true,grantSecond=true;uint32 klass=CLASS_ROGUE,quest=42504,status=QUEST_STATUS_INCOMPLETE;
    std::function<void(uint32)> creditHook;
    std::vector<uint32> criteria;
    Player(){type=TYPEID_PLAYER;}
    bool CanContact() const{return contact;}
    bool isGameMaster()const{return gm;}bool HasPendingTeleport()const{return pending;}uint32 getClass()const{return klass;}bool GetQuestRewardStatus(uint32 id)const{return id==quest&&rewarded;}uint32 GetQuestStatus(uint32 id)const{return id==quest?status:0;}
    void UpdateAchievementCriteria(uint32 kind,uint32 asset){assert(kind==92);criteria.push_back(asset);if(creditHook)creditHook(asset);if(map&&!gm&&((asset==51418&&grantFirst)||(asset==52672&&grantSecond))){auto& scene=map->scenario;scene.progress[asset==51418?48526:49374].Counter=1;if(scene.progress[48526].Counter&&scene.progress[49374].Counter)scene.stage=1;}}
};
Player* Unit::ToPlayer(){return type==TYPEID_PLAYER?static_cast<Player*>(this):nullptr;}
template<class T>struct HashMapHolder
{
    inline static bool _checkLock=false;
    inline static std::shared_mutex i_lockVector;
    inline static uint32 _size=32;
    inline static std::vector<T*> _objectVector=std::vector<T*>(32,nullptr);
#include "PlayerFind.inc"
};
struct ObjectAccessor
{
#include "PlayerLookup.inc"
};
#include "PlayerLookupMethod.inc"
struct MotionMaster
{
    struct Point {uint32 id;float x,y,z;};
    std::vector<Point> points;
    void MovePoint(uint32 id,float x,float y,float z){points.push_back({id,x,y,z});}
};
struct Creature:Unit
{
    uint32 entry=108015,despawn=0,baseUpdates=0,removedAura=0;
    float distance=1;
    bool walk=false;
    MotionMaster motion;
    FunctionProcessor functions;
    std::vector<Creature*> neighbors;
    uint32 GetEntry() const{return entry;}
    float GetExactDist2d(Unit*) const{return distance;}
    MotionMaster* GetMotionMaster(){return &motion;}
    void RemoveAura(uint32 aura){removedAura=aura;}
    void SetWalk(bool enabled){walk=enabled;}
    void DespawnOrUnsummon(uint32 delay){despawn=delay;}
    void AddDelayedEvent(uint64 delay,std::function<void()>&& function){functions.AddDelayedEvent(delay,std::move(function));}
};
struct ScriptedAI
{
    Creature* me;
    explicit ScriptedAI(Creature* creature):me(creature){}
    virtual ~ScriptedAI()=default;
    virtual void MoveInLineOfSight(Unit*){}
    virtual void UpdateAI(uint32){++me->baseUpdates;}
};
void GetCreatureListWithEntryInGrid(std::list<Creature*>& output,Creature* center,uint32 entry,float range)
{
    assert(range==70.0f);
    for(Creature* actor:center->neighbors)if(actor->entry==entry && actor->distance<=range)output.push_back(actor);
}
struct Generator {uint64 Generate(){return 1;}};
struct ObjectManager {Generator generator;template<HighGuid>Generator* GetGenerator(){return &generator;}};
ObjectManager manager;
ObjectManager* sObjectMgr=&manager;
static uint32 warnings=0;
#define TC_LOG_WARN(...) (++warnings)
struct Conversation
{
    inline static std::set<Conversation*> live;
    inline static std::vector<std::pair<uint32,ObjectGuid>> delivered;
    inline static uint32 failedEntry=0,attempts=0;
    inline static std::map<uint32,int32_t> durations;
    int32_t duration=0;
    int32_t GetDuration() const{return duration;}
    Conversation(){live.insert(this);}
    ~Conversation(){live.erase(this);}
    bool CreateConversation(uint64,uint32 entry,Unit* recipient,std::nullptr_t,WorldObject& position)
    {
        assert(recipient==&position);++attempts;
        if(entry==failedEntry)return false;
        duration=durations[entry];delivered.emplace_back(entry,recipient->GetGUID());return true;
    }
    static void Clear(){while(!live.empty())delete *live.begin();delivered.clear();failedEntry=0;attempts=0;durations.clear();}
};
#include "GaronaIntro.inc"
using Garona=npc_garona_halforcenAI;
void Advance(Garona& script,uint32 milliseconds){script.UpdateAI(milliseconds);script.me->functions.Update(milliseconds);}
struct Fixture
{
    inline static uint32 contexts=0;
    Map original,other;
    Creature garona,first,second,unrelated;
    Player player,secondPlayer;
    Garona script;
    Fixture():script(&garona)
    {
        ++contexts;
        Conversation::Clear();garona.map=&original;player.map=&original;secondPlayer.map=&original;
        scenarioManager.current=&original.scenario;warnings=0;
        secondPlayer.guid.low=2;
        HashMapHolder<Player>::_objectVector.assign(32,nullptr);
        HashMapHolder<Player>::_objectVector[1]=&player;HashMapHolder<Player>::_objectVector[2]=&secondPlayer;
        first.entry=98094;second.entry=108058;unrelated.entry=1;
        garona.neighbors={&first,&second,&unrelated};
    }
    void Start(){script.MoveInLineOfSight(&player);}
    ~Fixture(){Conversation::Clear();scenarioManager.current=nullptr;}
};
void Finish(Garona& script){Advance(script,15000);Advance(script,15000);}
void CheckCleanup(Fixture const& fixture)
{
    assert(fixture.garona.despawn==4000 && fixture.garona.walk && fixture.garona.motion.points.size()==2);
    auto destination=fixture.garona.motion.points.back();assert(destination.x==-9074.46f && destination.y==427.19f && destination.z==92.07f);
}
int main()
{
#ifdef BASELINE
    Fixture fixture;fixture.Start();fixture.player.map=&fixture.other;Advance(fixture.script,30000);
    assert(fixture.player.criteria==std::vector<uint32>({51418,52672}) && Conversation::delivered.size()==3);
    CheckCleanup(fixture);
    std::cout<<"PASS pinned3eadb186 counterfactual: original native Unit queue sends both personal events and all conversations after recipient map transfer; no freed-pointer dereference attempted\n";
#else
    {
        Fixture fixture;Unit pet;fixture.script.MoveInLineOfSight(&pet);assert(!fixture.script.introDone);
        fixture.garona.distance=20;fixture.Start();assert(!fixture.script.introDone);
        fixture.garona.distance=19.99f;fixture.Start();assert(fixture.script.introDone && Conversation::delivered.size()==1);
        fixture.script.MoveInLineOfSight(&fixture.secondPlayer);fixture.Start();assert(Conversation::delivered.size()==1);
        Advance(fixture.script,14999);assert(Conversation::delivered.size()==1 && fixture.player.criteria.empty());
        Advance(fixture.script,1);assert(Conversation::delivered.size()==2 && fixture.first.despawn==6000 && fixture.second.despawn==6000);
        for(Creature* actor:{&fixture.first,&fixture.second}){assert(actor->walk && actor->motion.points.size()==1);auto point=actor->motion.points.front();assert(point.x==-8996.06f && point.y==487.59f && point.z==96.62f);}
        assert(fixture.unrelated.despawn==0);Advance(fixture.script,14999);assert(fixture.player.criteria.empty());
        Advance(fixture.script,1);assert(fixture.player.criteria==std::vector<uint32>({51418,52672}));CheckCleanup(fixture);
        assert(Conversation::delivered.size()==3 && Conversation::delivered[0].first==2361 && Conversation::delivered[1].first==2377 && Conversation::delivered[2].first==2384);
        for(auto const& delivery:Conversation::delivered)assert(delivery.second==fixture.player.guid);
        Advance(fixture.script,60000);assert(fixture.player.criteria.size()==2 && Conversation::delivered.size()==3 && fixture.secondPlayer.criteria.empty());
        assert(fixture.garona.baseUpdates==5 && fixture.garona.functions.SizeQueue()==0 && fixture.garona.removedAura==132653);
    }
    for(uint32 condition=0;condition<10;++condition)
    {
        Fixture fixture;fixture.Start();
        switch(condition)
        {
            case 0:HashMapHolder<Player>::_objectVector[1]=nullptr;break;
            case 1:fixture.player.inWorld=false;break;
            case 2:fixture.player.preDelete=true;break;
            case 3:fixture.player.contact=false;break;
            case 4:fixture.player.map=&fixture.other;break;
            case 5:fixture.player.phase=2;break;
            case 6:fixture.garona.inWorld=false;break;
            case 7:fixture.garona.map=nullptr;break;
            case 8:fixture.secondPlayer.guid={2,1};HashMapHolder<Player>::_objectVector[1]=&fixture.secondPlayer;break;
            case 9:fixture.player.guid.high=2;break;
        }
        Advance(fixture.script,30000);assert(fixture.player.criteria.empty() && fixture.secondPlayer.criteria.empty() && Conversation::delivered.size()==1);assert(!fixture.garona.despawn);
    }
    {
        Fixture fixture;fixture.Start();HashMapHolder<Player>::_objectVector[1]=nullptr;Advance(fixture.script,15000);
        Player reconnected;reconnected.map=&fixture.original;HashMapHolder<Player>::_objectVector[1]=&reconnected;
        Advance(fixture.script,15000);assert(reconnected.criteria.empty() && !fixture.garona.despawn);Advance(fixture.script,15000);assert(reconnected.criteria.size()==2 && fixture.player.criteria.empty() && Conversation::delivered.size()==3);
    }
    {
        Fixture fixture;auto player=std::make_unique<Player>();player->map=&fixture.original;HashMapHolder<Player>::_objectVector[1]=player.get();
        fixture.script.MoveInLineOfSight(player.get());HashMapHolder<Player>::_objectVector[1]=nullptr;player.reset();Advance(fixture.script,30000);
        assert(Conversation::delivered.size()==1 && !fixture.garona.despawn);
    }
    {
        Fixture fixture;auto oldScript=std::make_unique<Garona>(&fixture.garona);oldScript->MoveInLineOfSight(&fixture.player);
        assert(fixture.garona.functions.SizeQueue()==0);oldScript.reset();
        Garona replacement(&fixture.garona);Advance(replacement,60000);
        assert(fixture.player.criteria.empty() && Conversation::delivered.size()==1 && fixture.garona.despawn==0);
    }
    for(uint32 failed:{0u,2361u,2377u,2384u})
    {
        Fixture fixture;Conversation::failedEntry=failed;fixture.garona.neighbors.clear();fixture.Start();Finish(fixture.script);
        assert(fixture.player.criteria.size()==2 && Conversation::attempts==3 && Conversation::delivered.size()==(failed?2u:3u));CheckCleanup(fixture);
    }
    {
        Fixture fixture;fixture.garona.entry=107979;fixture.Start();Advance(fixture.script,60000);
        assert(!fixture.script.introDone && fixture.player.criteria.empty() && Conversation::delivered.empty() && fixture.garona.motion.points.empty());
    }
    for(uint32 mode=0;mode<18;++mode){Fixture f;
        switch(mode){case 0:f.player.gm=true;break;case 1:f.player.klass=1;break;case 2:f.player.status=0;break;case 3:f.player.rewarded=true;break;case 4:f.player.alive=false;break;case 5:f.player.pending=true;break;case 6:f.player.expired=true;break;case 7:f.player.scope=false;break;case 8:f.garona.expired=true;break;case 9:f.garona.scope=false;break;case 10:f.garona.alive=false;break;case 11:f.garona.preDelete=true;break;case 12:f.original.id=1;break;case 13:f.original.scenario.id=1;break;case 14:f.original.scenario.stage=1;break;case 15:f.player.inWorld=false;break;case 16:f.player.contact=false;break;case 17:f.player.phase=2;break;}
        f.Start();Advance(f.script,60000);assert(!f.script.introDone&&f.player.criteria.empty()&&Conversation::delivered.empty()&&!f.garona.despawn&&f.garona.motion.points.empty());
    }
    for(uint32 mode=0;mode<5;++mode){Fixture f;f.player.quest=mode==0?42627:42504;if(mode==1)f.player.status=QUEST_STATUS_COMPLETE;f.Start();
        if(mode==2)f.player.grantFirst=false;if(mode==3)f.player.grantSecond=false;if(mode==4)f.player.grantFirst=f.player.grantSecond=false;
        Finish(f.script);if(mode<2){CheckCleanup(f);continue;}
        assert(!f.garona.despawn&&warnings==1);auto first=f.original.scenario.progress[48526].Counter,second=f.original.scenario.progress[49374].Counter;
        Advance(f.script,999);assert(warnings==1&&!f.garona.despawn);f.player.grantFirst=f.player.grantSecond=true;Advance(f.script,1);CheckCleanup(f);
        assert(f.original.scenario.stage==1&&f.player.criteria.size()==(first&&second?2:first||second?3:4));
    }
    {Fixture f;f.Start();f.player.status=0;Finish(f.script);assert(!f.garona.despawn&&f.player.criteria.empty());f.script.MoveInLineOfSight(&f.secondPlayer);Advance(f.script,1000);assert(f.secondPlayer.criteria.empty()&&!f.garona.despawn);Advance(f.script,15000);assert(f.secondPlayer.criteria.size()==2&&f.player.criteria.empty());CheckCleanup(f);}
    {Fixture f;f.Start();f.player.creditHook=[&](uint32){Advance(f.script,60000);};Finish(f.script);assert(f.player.criteria.size()==2);CheckCleanup(f);}
    {Fixture f;f.Start();f.player.creditHook=[&](uint32 asset){if(asset==51418)f.player.status=0;};Finish(f.script);assert(f.player.criteria.size()==1&&!f.garona.despawn&&warnings==1);f.player.status=QUEST_STATUS_INCOMPLETE;f.player.creditHook={};Advance(f.script,1000);assert(f.player.criteria.size()==2);CheckCleanup(f);}
    for(uint32 mode=0;mode<8;++mode){Fixture f;Conversation::durations={{2361,40840},{2377,47679}};
        if(mode==1)Conversation::failedEntry=2361;if(mode==2)Conversation::failedEntry=2377;
        if(mode==3)Conversation::durations={{2361,1},{2377,1}};
        if(mode==4)Conversation::durations={{2361,-1},{2377,-1}};
        f.Start();uint32 first=mode==1||mode==3||mode==4?15000:40840,second=mode==2||mode==3||mode==4?15000:47679;
        if(mode==5){Advance(f.script,200000);assert(f.script.openingMiddle&&!f.script.openingComplete&&f.player.criteria.empty());}
        else{Advance(f.script,first-1);assert(!f.script.openingMiddle&&!f.first.despawn&&f.player.criteria.empty());
            if(mode==6)f.player.status=0;Advance(f.script,1);
            if(mode==6){assert(!f.script.openingMiddle&&!f.first.despawn);f.script.MoveInLineOfSight(&f.secondPlayer);Advance(f.script,1000);}
            assert(f.script.openingMiddle&&f.first.despawn==6000&&f.second.despawn==6000&&!f.garona.despawn);
        }
        if(mode==7)f.player.grantSecond=false;
        Advance(f.script,second-1);assert(!f.script.openingComplete&&!f.garona.despawn&&f.player.criteria.empty()&&f.secondPlayer.criteria.empty());Advance(f.script,1);
        if(mode==7){assert(!f.script.openingComplete&&warnings==1&&!f.garona.despawn);f.player.grantSecond=true;Advance(f.script,1000);}
        assert(f.script.openingComplete&&f.original.scenario.stage==1);CheckCleanup(f);
        assert(Conversation::attempts==3&&Conversation::delivered.size()==(mode==1||mode==2?2u:3u));
        Advance(f.script,200000);assert(Conversation::attempts==3);
    }
    std::cout<<"PASS "<<Fixture::contexts<<" exact opening AI/native EventMap/native lookup ASAN contexts: minimum15+15s/actors/optional voices;8 accepted-payload timing/coarse/refusal/recovery contexts, qualified first and delayed recipients,18 admission refusals, both quests, complete quest, three native-ack failures/recovery, no repeated acknowledged leaf, abandonment/rebinding/reconnect, reentrant/abandoning credit, no departure without both receipts and shared107979 preserved. Native services otherwise controlled; no client traversal claim\n";
#endif
}
