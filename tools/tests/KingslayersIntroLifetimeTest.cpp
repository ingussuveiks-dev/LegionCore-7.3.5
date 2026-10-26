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
enum {TYPEID_PLAYER=4,CRITERIA_TYPE_SCRIPT_EVENT_2=92};
enum class HighGuid {Conversation};
struct ObjectGuid
{
    uint64 high=1,low=1;
    uint32 GetGUIDLow() const{return uint32(low);}
    bool operator==(ObjectGuid const& other) const{return high==other.high && low==other.low;}
};
struct Map {};
struct WorldObject
{
    Map* map=nullptr;
    bool inWorld=true,preDelete=false;
    uint32 phase=1;
    ObjectGuid guid;
    Map* GetMap() const{return map;}
    ObjectGuid GetGUID() const{return guid;}
    bool IsInWorld() const{return inWorld;}
    bool IsPreDelete() const{return preDelete;}
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
    std::vector<uint32> criteria;
    Player(){type=TYPEID_PLAYER;}
    bool CanContact() const{return contact;}
    void UpdateAchievementCriteria(uint32 kind,uint32 asset){assert(kind==92);criteria.push_back(asset);}
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
struct Conversation
{
    inline static std::set<Conversation*> live;
    inline static std::vector<std::pair<uint32,ObjectGuid>> delivered;
    inline static uint32 failedEntry=0,attempts=0;
    Conversation(){live.insert(this);}
    ~Conversation(){live.erase(this);}
    bool CreateConversation(uint64,uint32 entry,Unit* recipient,std::nullptr_t,WorldObject& position)
    {
        assert(recipient==&position);++attempts;
        if(entry==failedEntry)return false;
        delivered.emplace_back(entry,recipient->GetGUID());return true;
    }
    static void Clear(){while(!live.empty())delete *live.begin();delivered.clear();failedEntry=0;attempts=0;}
};
#include "GaronaIntro.inc"
using Garona=npc_garona_halforcenAI;
void Advance(Garona& script,uint32 milliseconds){script.UpdateAI(milliseconds);script.me->functions.Update(milliseconds);}
struct Fixture
{
    Map original,other;
    Creature garona,first,second,unrelated;
    Player player,secondPlayer;
    Garona script;
    Fixture():script(&garona)
    {
        Conversation::Clear();garona.map=&original;player.map=&original;secondPlayer.map=&original;
        secondPlayer.guid.low=2;
        HashMapHolder<Player>::_objectVector.assign(32,nullptr);
        HashMapHolder<Player>::_objectVector[1]=&player;HashMapHolder<Player>::_objectVector[2]=&secondPlayer;
        first.entry=98094;second.entry=108058;unrelated.entry=1;
        garona.neighbors={&first,&second,&unrelated};
    }
    void Start(){script.MoveInLineOfSight(&player);}
    ~Fixture(){Conversation::Clear();}
};
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
        Advance(fixture.script,30000);assert(fixture.player.criteria.empty() && fixture.secondPlayer.criteria.empty() && Conversation::delivered.size()==1);CheckCleanup(fixture);
    }
    {
        Fixture fixture;fixture.Start();HashMapHolder<Player>::_objectVector[1]=nullptr;Advance(fixture.script,15000);
        Player reconnected;reconnected.map=&fixture.original;HashMapHolder<Player>::_objectVector[1]=&reconnected;
        Advance(fixture.script,15000);assert(reconnected.criteria.size()==2 && fixture.player.criteria.empty() && Conversation::delivered.size()==2);
    }
    {
        Fixture fixture;auto player=std::make_unique<Player>();player->map=&fixture.original;HashMapHolder<Player>::_objectVector[1]=player.get();
        fixture.script.MoveInLineOfSight(player.get());HashMapHolder<Player>::_objectVector[1]=nullptr;player.reset();Advance(fixture.script,30000);
        assert(Conversation::delivered.size()==1);CheckCleanup(fixture);
    }
    {
        Fixture fixture;auto oldScript=std::make_unique<Garona>(&fixture.garona);oldScript->MoveInLineOfSight(&fixture.player);
        assert(fixture.garona.functions.SizeQueue()==0);oldScript.reset();
        Garona replacement(&fixture.garona);Advance(replacement,60000);
        assert(fixture.player.criteria.empty() && Conversation::delivered.size()==1 && fixture.garona.despawn==0);
    }
    for(uint32 failed:{0u,2361u,2377u,2384u})
    {
        Fixture fixture;Conversation::failedEntry=failed;fixture.garona.neighbors.clear();fixture.Start();Advance(fixture.script,60000);
        assert(fixture.player.criteria.size()==2 && Conversation::attempts==3 && Conversation::delivered.size()==(failed?2u:3u));CheckCleanup(fixture);
    }
    {
        Fixture fixture;fixture.garona.entry=107979;fixture.Start();Advance(fixture.script,60000);
        assert(!fixture.script.introDone && fixture.player.criteria.empty() && Conversation::delivered.empty() && fixture.garona.motion.points.empty());
    }
    std::cout<<"PASS actual Garona AI/native EventMap/Player lookup and low-GUID Find: timings, actual actors, one-shot recipient, absent/destructed/relocated/predelete/contact/phase/full-GUID rejection, reconnect, missing actors/conversation failure and shared107979 preserved. No real client/quest-attempt/group eligibility or pickup/VFX proof\n";
#endif
}
