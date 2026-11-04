#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <vector>
using uint32=uint32_t;using int32=int32_t;using uint64=uint64_t;
enum {CLASS_ROGUE=4,TYPEID_PLAYER=4,QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_COMPLETE=1,REACT_AGGRESSIVE=2};
enum class HighGuid {Conversation};
struct ObjectGuid {uint64 high=1,low=0;bool operator==(ObjectGuid const& o)const{return high==o.high&&low==o.low;}bool operator!=(ObjectGuid const& o)const{return !(*this==o);}bool IsEmpty()const{return !low;}void Clear(){high=1;low=0;}};
struct Scenario {uint32 id=1123;uint32 GetScenarioId()const{return id;}};
struct Map {uint32 id=1620;Scenario scenario;};
struct WorldObject {
    virtual ~WorldObject()=default;
    Map* map=nullptr;ObjectGuid guid;uint32 entry=0,phase=1,phaseId=1;float x=0,z=100;
    bool inWorld=true,preDelete=false,alive=true,expired=false,scope=true;
    auto GetGUID()const{return guid;}auto GetMap()const{return map;}uint32 GetMapId()const{return map?map->id:0;}uint32 GetInstanceId()const{return 1;}
    bool IsInWorld()const{return inWorld;}bool IsPreDelete()const{return preDelete;}bool IsAlive()const{return alive;}bool IsQuestObjectExpired()const{return expired;}
    bool InSamePhase(WorldObject const* o)const{return phase==o->phase;}bool InSamePhaseId(WorldObject const* o)const{return phaseId==o->phaseId;}
    bool CanTargetQuestObject(WorldObject const* o)const{return scope&&o->scope;}float GetVisibilityRange()const{return 170;}
    bool IsWithinDistInMap(WorldObject const* o,float range)const{return map==o->map&&std::abs(x-o->x)<=range;}
    float GetExactDist2d(WorldObject const* o)const{return std::abs(x-o->x);}float GetPositionZ()const{return z;}
    bool InheritQuestObjectScope(WorldObject* source){return !source->expired&&source->scope;}
};
struct Player;struct Unit:WorldObject {uint32 kind=0;uint32 GetTypeId()const{return kind;}Player* ToPlayer();};
struct Player:Unit {
    bool contact=true,gm=false,pending=false,rewarded=false;uint32 klass=4,quest=42504,status=3,auras=0,casts=0;
    Player(){kind=4;}bool CanContact()const{return contact;}bool isGameMaster()const{return gm;}bool HasPendingTeleport()const{return pending;}uint32 getClass()const{return klass;}
    bool GetQuestRewardStatus(uint32 id)const{return id==quest&&rewarded;}uint32 GetQuestStatus(uint32 id)const{return id==quest?status:0;}
    void AddAura(uint32,Player*){++auras;}void CastSpell(Player*,uint32){++casts;}
};
Player* Unit::ToPlayer(){return kind==4?static_cast<Player*>(this):nullptr;}
struct Motion {uint32 moves=0;void MovePoint(uint32,float,float,float){++moves;}};
struct Pose {float GetOrientation()const{return 0;}};
struct Creature:Unit {
    Unit* victim=nullptr;bool moving=false,canAttack=false;uint32 react=0,faces=0,baseUpdates=0,attacks=0,melees=0,talks=0;Motion motion;Player* nearby=nullptr;
    uint32 GetEntry()const{return entry;}Unit* getVictim()const{return victim;}bool canStartAttack(Unit*,bool){return canAttack;}
    bool isMoving()const{return moving;}void SetReactState(uint32 value){react=value;}void SetFacingToObject(Unit*){++faces;}void SetOrientation(float){}Pose GetHomePosition()const{return {};}
    Motion* GetMotionMaster(){return &motion;}void AddDelayedEvent(uint32,std::function<void()> callback){delayed.push_back(std::move(callback));}
    Player* FindNearestPlayer(float){return nearby;}std::vector<std::function<void()>> delayed;
};
struct Registry {Scenario* scenario=nullptr;Scenario* GetScenario(uint32){return scenario;}} registry;auto sScenarioMgr=&registry;
struct ObjectAccessor {
    inline static std::map<uint32,Player*> players;inline static std::map<uint32,Creature*> creatures;
    static Player* GetPlayer(WorldObject const& source,ObjectGuid guid){auto at=players.find(uint32(guid.low));return at!=players.end()&&at->second->map==source.map?at->second:nullptr;}
    static Creature* GetCreature(WorldObject const& source,ObjectGuid guid){auto at=creatures.find(uint32(guid.low));return at!=creatures.end()&&at->second->map==source.map?at->second:nullptr;}
};
struct ScriptedAI {
    Creature* me;explicit ScriptedAI(Creature* actor):me(actor){}virtual ~ScriptedAI()=default;
    virtual void Reset(){}virtual void MoveInLineOfSight(Unit*){}virtual void UpdateAI(uint32){++me->baseUpdates;}virtual void SetGUID(ObjectGuid const&,int32=0){}
    bool UpdateVictim(){return me->victim;}void AttackStart(Unit* target){me->victim=target;++me->attacks;}void DoMeleeAttackIfReady(){++me->melees;}void Talk(uint32){++me->talks;}
};
struct Generator {uint64 Generate(){return 1;}};
struct ObjectManager {Generator generator;template<HighGuid>Generator* GetGenerator(){return &generator;}} manager;auto sObjectMgr=&manager;
uint32 randomValue=1;uint32 urand(uint32,uint32){return randomValue;}uint32 warnings=0;
#define TC_LOG_WARN(...) (++warnings)
struct Conversation:WorldObject {
    inline static std::set<Conversation*> allocations;inline static uint32 attempts=0,accepted=0,failEntry=0;
    inline static std::vector<ObjectGuid> recipients,bound;inline static std::function<void()> callback;
    Conversation(){inWorld=false;allocations.insert(this);}virtual ~Conversation(){allocations.erase(this);}
    virtual void AddToWorld(){inWorld=true;}
    virtual Creature* FindCreatureActor(uint32,uint32 identity,WorldObject* searcher){for(auto row:ObjectAccessor::creatures)if(row.second->entry==identity&&row.second->map==searcher->map)return row.second;return nullptr;}
    bool CreateConversation(uint64,uint32 id,Unit* recipient,std::nullptr_t,WorldObject& position) {
        assert(recipient==&position);++attempts;map=recipient->map;entry=id;if(failEntry==id)return false;
        Creature* actor=nullptr;if(id==2384){actor=FindCreatureActor(0,108057,recipient);if(!actor)return false;}
        if(callback)callback();AddToWorld();if(!inWorld)return false;
        ++accepted;recipients.push_back(recipient->guid);if(actor)bound.push_back(actor->guid);return true;
    }
    static void Clear(){callback={};while(!allocations.empty())delete *allocations.begin();attempts=accepted=failEntry=0;recipients.clear();bound.clear();warnings=0;}
};
#include "SmallAI.inc"
using AI=npc_kingslayers_small_eventsAI;
struct Fixture {
    inline static uint32 cases=0;
    Map map,other;Creature clay;Player rogue,visitor;AI ai;
    Fixture():ai(&clay){++cases;Conversation::Clear();ObjectAccessor::players.clear();ObjectAccessor::creatures.clear();clay.map=&map;clay.entry=108057;clay.guid.low=11;rogue.map=visitor.map=&map;rogue.guid.low=1;visitor.guid.low=2;rogue.x=visitor.x=10;registry.scenario=&map.scenario;ObjectAccessor::players={{1,&rogue},{2,&visitor}};ObjectAccessor::creatures={{11,&clay}};}
    ~Fixture(){Conversation::Clear();ObjectAccessor::players.clear();ObjectAccessor::creatures.clear();registry.scenario=nullptr;}
    void Start(){ai.MoveInLineOfSight(&rogue);}void Received(){assert(ai.introDone&&Conversation::accepted==1&&Conversation::bound[0]==clay.guid);}
};
int main(){_set_abort_behavior(0,_CALL_REPORTFAULT);
    for(uint32 mode=0;mode<24;++mode){Fixture f;
        switch(mode){case 0:f.rogue.gm=true;break;case 1:f.rogue.klass=1;break;case 2:f.rogue.status=0;break;case 3:f.rogue.rewarded=true;break;case 4:f.rogue.contact=false;break;case 5:f.rogue.pending=true;break;case 6:f.rogue.alive=false;break;case 7:f.rogue.preDelete=true;break;case 8:f.rogue.inWorld=false;break;case 9:f.rogue.expired=true;break;case 10:f.rogue.scope=false;break;case 11:f.rogue.map=&f.other;break;case 12:f.rogue.phase=2;break;case 13:f.rogue.phaseId=2;break;case 14:f.clay.inWorld=false;break;case 15:f.clay.preDelete=true;break;case 16:f.clay.alive=false;break;case 17:f.clay.expired=true;break;case 18:f.clay.scope=false;break;case 19:f.map.scenario.id=1;break;case 20:registry.scenario=nullptr;break;case 21:f.clay.entry=108059;break;case 22:f.clay.map=&f.other;f.other.id=1;break;case 23:f.rogue.x=171;break;}
        f.ai.RequestGateOrder(&f.rogue);f.ai.UpdateGateOrder(2000);assert(!f.ai.introDone&&!Conversation::attempts);
    }
    for(uint32 quest:{42504u,42627u})for(uint32 status:{3u,1u}){Fixture f;f.rogue.quest=quest;f.rogue.status=status;f.Start();f.Received();f.ai.SetGUID(f.visitor.guid,2384);f.ai.Reset();f.ai.UpdateAI(2000);f.Start();assert(Conversation::accepted==1);}
    {Fixture f;Unit pet;pet.map=&f.map;pet.x=10;f.ai.MoveInLineOfSight(&pet);f.ai.RequestGateOrder(nullptr);assert(!f.ai.introDone&&!Conversation::attempts);f.Start();f.Received();}
    {Fixture f;f.rogue.x=14;f.Start();assert(!f.ai.introDone);f.rogue.x=13.99f;f.Start();f.Received();}
    {Fixture f;auto wrong=f.rogue.guid;wrong.high^=1;f.ai.SetGUID(wrong,2384);assert(!Conversation::attempts);f.ai.SetGUID(f.rogue.guid,1);assert(!Conversation::attempts);f.ai.SetGUID(f.rogue.guid,2384);f.Received();}
    {Fixture f;Conversation::failEntry=2384;f.Start();assert(!f.ai.introDone&&warnings==1&&Conversation::attempts==1);f.ai.SetGUID(f.visitor.guid,2384);assert(f.ai.gateOrderRecipient==f.rogue.guid);f.ai.UpdateAI(999);assert(Conversation::attempts==1);f.ai.Reset();f.ai.UpdateAI(1);assert(Conversation::attempts==2&&warnings==1);Conversation::failEntry=0;f.ai.UpdateAI(1000);f.Received();assert(Conversation::recipients[0]==f.rogue.guid);}
    for(uint32 mode=0;mode<5;++mode){Fixture f;Conversation::failEntry=2384;f.Start();Conversation::failEntry=0;
        switch(mode){case 0:f.rogue.status=0;break;case 1:ObjectAccessor::players.erase(1);break;case 2:f.rogue.inWorld=false;break;case 3:f.rogue.phaseId=2;break;case 4:f.clay.inWorld=false;break;}
        f.ai.UpdateAI(1000);assert(!f.ai.introDone&&f.ai.gateOrderRecipient.IsEmpty()&&Conversation::accepted==0);
    }
    {Fixture f;Conversation::callback=[&]{f.ai.SetGUID(f.visitor.guid,2384);f.ai.UpdateAI(200000);};f.Start();f.Received();assert(Conversation::attempts==1&&!f.ai.gateOrderBusy);}
    {Fixture f;Conversation::callback=[&]{f.clay.expired=true;};f.Start();assert(!f.ai.introDone&&warnings==1&&!Conversation::accepted&&Conversation::allocations.empty());}
    for(uint32 mode=0;mode<6;++mode){Fixture f;auto guid=f.clay.guid;
        if(mode==0)guid.high^=1;if(mode==1)guid.low^=uint64(1)<<40;if(mode==2)f.clay.alive=false;if(mode==3)f.clay.phaseId=2;if(mode==4)f.clay.scope=false;if(mode==5)ObjectAccessor::creatures.clear();
        auto speech=new AI::GateConversation(guid);assert(!speech->CreateConversation(1,2384,&f.rogue,nullptr,f.rogue));if(mode!=3){speech->AddToWorld();assert(!speech->IsInWorld());}delete speech;
    }
    // The actual pre-existing shared LOS/combat bodies are byte-certified by
    // the wrapper. Exercise their native IDs through this controlled host too.
    for(uint32 entry:{108056u,108107u,108149u,108054u,108059u,108058u,108177u}){Fixture f;f.clay.entry=entry;f.rogue.gm=true;f.Start();assert(f.ai.introDone);f.ai.UpdateAI(1);assert(!f.ai.gateOrderRetry);}
    {Fixture f;f.clay.canAttack=true;f.rogue.x=4;f.Start();assert(f.clay.attacks==1&&f.clay.victim==&f.rogue);f.ai.UpdateAI(1);assert(f.clay.melees==1);}
    std::cout<<"PASS "<<Fixture::cases<<" complete shared AI/gate controller ASAN contexts: admission, exact source/full GUID/publication, receipt/once, bounded retry/reset/reentry, pending cancellation and retained shared LOS/combat. Native services modeled; native/client proofs separate\n";
}
