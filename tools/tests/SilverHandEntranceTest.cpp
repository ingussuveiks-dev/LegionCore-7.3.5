#include <cassert>
#include <atomic>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <new>
#include <vector>
using uint32=uint32_t;
using uint64=uint64_t;
using FunctionList=std::multimap<uint64,std::function<void()>>;
#include "FunctionClass.inc"
#include "FunctionMethods.inc"
enum { CLASS_PALADIN=2, QUEST_STATUS_INCOMPLETE=3, TYPEID_PLAYER=4, UNIT_FIELD_NPC_FLAGS=1,
    UNIT_NPC_FLAG_GOSSIP=1, CRITERIA_TYPE_SCRIPT_EVENT_2=92, MOTION_SLOT_IDLE=0 };
constexpr float PET_FOLLOW_DIST=1.0f,PET_FOLLOW_ANGLE=1.57f;
struct ObjectGuid
{
    uint64 _low,_high;
    ObjectGuid();
    ObjectGuid(uint64 low,uint64 high):_low(low),_high(high){}
    void Clear();
    bool IsEmpty() const;
    bool operator<(ObjectGuid const& other) const {return _high<other._high || (_high==other._high && _low<other._low);}
    bool operator==(ObjectGuid const& other) const {return _high==other._high && _low==other._low;}
};
#include "GuidLifecycle.inc"
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
struct CreatureAI
{
    virtual ~CreatureAI()=default;
    virtual void Reset(){}
    virtual void UpdateLifecycle(uint32){}
};
struct Unit : WorldObject
{
    virtual Player* ToPlayer() {return nullptr;}
    virtual uint32 GetTypeId() const {return 0;}
};
struct Player : Unit
{
    ObjectGuid guid{1,7};
    bool contact=true,alive=true,teleport=false,phase=true,visible=true,los=true,rewarded=false;
    uint32 klass=2,status=3,instance=42,credits=0,arrivalCredits=0;
    float distance=1;
    std::function<void()> onCriteria;
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
    void UpdateAchievementCriteria(uint32 type,uint32 event) {assert(type==92 && event==50765);++arrivalCredits;if(onCriteria)onCriteria();}
};
struct Motion
{
    uint32 moves=0,follows=0;
    Creature* followed=nullptr;
    float followAngle=0;
    std::function<void()> onMove;
    void MovePoint(uint32 point,float positionX,float positionY,float positionZ)
    {assert(point==1 && positionX==2137.70f && positionY==2397.60f && positionZ==118.54f);++moves;if(onMove)onMove();}
    void MoveFollow(Creature* actor,float distance,float angle,uint32 slot)
    {assert(distance==PET_FOLLOW_DIST && slot==MOTION_SLOT_IDLE);++follows;followed=actor;followAngle=angle;}
};
struct Creature : WorldObject
{
    bool alive=true;
    bool m_AI_locked=false,IsAIEnabled=true;
    CreatureAI* i_AI=nullptr;
    CreatureAI* i_disabledAI=nullptr;
    uint32 entry=106429,mapId=1539,instance=42,flags=0x200;
    Player* nearestPlayer=nullptr;
    Creature* efrin=nullptr;
    Creature* duval=nullptr;
    Motion motion;
    std::vector<std::function<void()>> pending;
    bool IsAlive() const {return alive;}
    uint32 GetEntry() const {return entry;}
    uint32 GetMapId() const {return mapId;}
    uint32 GetInstanceId() const {return instance;}
    bool IsWithinDistInMap(Unit* who,float radius) const {return static_cast<Player*>(who)->distance<=radius;}
    Motion* GetMotionMaster() {return &motion;}
    Player* FindNearestPlayer(float radius) {assert(radius==100.0f);return nearestPlayer;}
    Creature* FindNearestCreature(uint32 identity,float radius,bool live)
    {assert(radius==50 && live && (identity==106371 || identity==106370));return identity==106371?efrin:duval;}
    void SetFlag(uint32 field,uint32 mask) {assert(field==UNIT_FIELD_NPC_FLAGS);flags|=mask;}
    void SetUInt32Value(uint32 field,uint32 value) {assert(field==UNIT_FIELD_NPC_FLAGS);flags=value;}
    void AddDelayedEvent(uint32 delay,std::function<void()> callback) {assert(delay==5000 || delay==2000);pending.push_back(std::move(callback));}
    CreatureAI* AI(){return i_AI;}
    bool AIM_Destroy();
    void Run(uint32 diff=5000)
    {
        auto callbacks=std::move(pending);pending.clear();for(auto& callback:callbacks)callback();
#include "LifecycleBridge.inc"
    }
};
uint32 debugMessages=0;
#define TC_LOG_DEBUG(...) ++debugMessages
#define ASSERT(value,...) assert(value)
#include "DestroyAI.inc"
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
struct EventMap {void Reset(){}};
namespace SilverHandCompanion {void Recover(Creature*,bool&,bool&) {}}
struct EntranceAI : CreatureAI
{
    Creature* me;
    bool _introDone=false,firstwp=false,firstfight=false;
    ObjectGuid _arrivalPlayerGuid;
    bool wait_heal=false,_pendingHealRecovery=false;
    FunctionProcessor _entryFunctions;
    uint64 _entryGeneration=0;
    bool _entryPending=false;
    EventMap events;
    std::vector<uint32> talks;
    std::function<void(uint32)> onTalk;
    explicit EntranceAI(Creature* actor):me(actor){actor->i_AI=this;}
    void Talk(uint32 line) {talks.push_back(line);if(onTalk)onTalk(line);}
    uint32 Pending(){return uint32(me->pending.size())+_entryFunctions.Size()+_entryFunctions.SizeQueue();}
#include "EntranceLos.inc"
#include "EntranceArrival.inc"
#include "EntranceReset.inc"
#include "EntranceLifecycle.inc"
#include "EntranceFight.inc"
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
#if defined(ARRIVAL_BASELINE)
    for(uint32 invalid:{4u,18u,19u,25u})
    {
        manager={};Player player;Creature actor;EntranceAI ai{&actor};actor.nearestPlayer=&player;
        Invalidate(invalid,player,actor);ai.CompleteEntranceArrival();
        assert(player.arrivalCredits==1 && ai.firstwp && actor.flags==1);
    }
    {manager={};Creature actor;EntranceAI ai{&actor};ai.CompleteEntranceArrival();assert(ai.firstwp && actor.flags==1);}
    std::cout<<"PASS pinnedc5eb41f3: actual old POINT1 publishes ready flags/firstwp without a valid original recipient and attempts50765 on its controlled nearest player in4 invalid contexts, even publishes without any player. No native persisted-counter/client proof.\n";
#elif defined(BASELINE)
    for(uint32 invalid:{0u,10u,18u,25u})
    {
        manager={};Player player;Creature actor;EntranceAI ai{&actor};ai.MoveInLineOfSight(&player);
        Invalidate(invalid,player,actor);actor.Run();assert(player.credits==1 && actor.motion.moves==1 && ai._introDone);
    }
    std::cout<<"PASS pinned4685321c: delayed Travard callback retains raw invoker and attempts credit/movement after logout/foreign map/abandon/completed scenario in controlled services. Not a native UAF crash or proof of persisted invalid quest credit.\n";
#elif defined(LIFETIME_BASELINE)
    manager={};Player player;Creature actor;
    HashMapHolder<Player>::objects={{player.guid,&player}};
    alignas(EntranceAI) unsigned char storage[sizeof(EntranceAI)];
    auto original=new(storage) EntranceAI(&actor);original->MoveInLineOfSight(&player);
    assert(actor.pending.size()==1);original->~EntranceAI();
    auto replacement=new(storage) EntranceAI(&actor);actor.Run();
    assert(!replacement->_introDone && replacement->_arrivalPlayerGuid==player.guid && player.credits==1 && actor.motion.moves==1 && replacement->talks==std::vector<uint32>{1});
    replacement->~EntranceAI();actor.i_AI=nullptr;
    original=new(storage) EntranceAI(&actor);original->ArmFirstFight();assert(actor.pending.size()==1);
    original->~EntranceAI();replacement=new(storage) EntranceAI(&actor);actor.Run(2000);
    assert(replacement->firstfight);replacement->~EntranceAI();actor.i_AI=nullptr;
    std::cout<<"PASS pinned1237db9f: Unit-owned5s and2s callbacks survive transparent same-address AI destruction/reconstruction: unsolicited arrival token/motion/106416 credit attempt and firstfight write affect replacement. Defined controlled replacement, not dereferencing freed storage or claiming a persisted-counter/client crash reproduction.\n";
#else
    for(uint32 invalid=0;invalid<26;++invalid)for(bool delayed:{false,true})
    {
        manager={};Player player;Creature actor;EntranceAI ai{&actor};HashMapHolder<Player>::objects={{player.guid,&player}};
        if(delayed)ai.MoveInLineOfSight(&player);
        Invalidate(invalid,player,actor);
        if(delayed)actor.Run();else ai.MoveInLineOfSight(&player);
        assert(!player.credits && !actor.motion.moves && !ai._introDone && ai.Pending()==0);
        assert(ai.talks==(delayed?std::vector<uint32>{0}:std::vector<uint32>{}));
    }
    for(uint32 step:{0u,1u})
    {
        manager={};manager.scenario.step=step;Player player;player.distance=20;Creature actor;EntranceAI ai{&actor};Unit pet,guardian;
        HashMapHolder<Player>::objects={{player.guid,&player}};
        ai.MoveInLineOfSight(nullptr);ai.MoveInLineOfSight(&pet);ai.MoveInLineOfSight(&guardian);assert(!ai._introDone);
        ai.MoveInLineOfSight(&player);ai.MoveInLineOfSight(&player);assert(ai.Pending()==1 && actor.pending.empty());
        actor.Run();ai.MoveInLineOfSight(&player);actor.Run();
        assert(player.credits==1 && actor.motion.moves==1 && ai.talks==std::vector<uint32>({0,1}));
    }
    for(bool preDelete:{false,true})
    {
        manager={};Player player,other;other.guid._low=2;Creature actor;EntranceAI ai{&actor};
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
    for(uint32 invalid=0;invalid<26;++invalid)
    {
        manager={};Player player,other;other.guid._low=2;Creature actor;EntranceAI ai{&actor};actor.nearestPlayer=&other;
        HashMapHolder<Player>::objects={{player.guid,&player},{other.guid,&other}};ai.MoveInLineOfSight(&player);actor.Run();
        Invalidate(invalid,player,actor);ai.CompleteEntranceArrival();ai.CompleteEntranceArrival();
        assert(!player.arrivalCredits && !other.arrivalCredits && !ai.firstwp && !ai._introDone && ai._arrivalPlayerGuid.IsEmpty());
        assert(actor.flags==0x200 && ai.talks==std::vector<uint32>({0,1}));
    }
    for(uint32 step:{0u,1u})
    {
        manager={};manager.scenario.step=step;Player player,other;other.guid._low=2;player.distance=20;
        Creature actor,efrin,duval;actor.efrin=&efrin;actor.duval=&duval;actor.nearestPlayer=&other;EntranceAI ai{&actor};
        HashMapHolder<Player>::objects={{player.guid,&player},{other.guid,&other}};
        ai.CompleteEntranceArrival();assert(!ai.firstwp && actor.flags==0x200);
        ai.MoveInLineOfSight(&player);actor.Run();assert(ai._arrivalPlayerGuid==player.guid);
        player.onCriteria=[&ai]{ai.CompleteEntranceArrival();};ai.CompleteEntranceArrival();ai.CompleteEntranceArrival();
        assert(player.arrivalCredits==1 && !other.arrivalCredits && ai.firstwp && ai._arrivalPlayerGuid.IsEmpty());
        assert(actor.flags==0x201 && ai.talks==std::vector<uint32>({0,1,2}));
        assert(efrin.motion.follows==1 && duval.motion.follows==1 && efrin.motion.followed==&actor && duval.motion.followed==&actor);
        assert(efrin.motion.followAngle==-PET_FOLLOW_ANGLE && duval.motion.followAngle==PET_FOLLOW_ANGLE);
    }
    for(bool preDelete:{false,true})
    {
        manager={};Player player,other;other.guid._low=2;Creature actor;actor.nearestPlayer=&other;EntranceAI ai{&actor};
        HashMapHolder<Player>::objects={{player.guid,&player},{other.guid,&other}};ai.MoveInLineOfSight(&player);actor.Run();
        if(preDelete)player.preDelete=true;else HashMapHolder<Player>::objects.erase(player.guid);
        ai.CompleteEntranceArrival();assert(!player.arrivalCredits && !other.arrivalCredits && !ai.firstwp && !ai._introDone);
        ai.MoveInLineOfSight(&other);actor.Run();ai.CompleteEntranceArrival();assert(other.arrivalCredits==1 && ai.firstwp);
    }
    {
        manager={};auto player=std::make_unique<Player>();Creature actor;EntranceAI ai{&actor};
        HashMapHolder<Player>::objects={{player->guid,player.get()}};ai.MoveInLineOfSight(player.get());actor.Run();
        HashMapHolder<Player>::objects.clear();player.reset();ai.CompleteEntranceArrival();assert(!ai.firstwp && !ai._introDone && actor.flags==0x200);
    }
    for(uint32 mode:{0u,1u,2u,3u})
    {
        manager={};Player player;Creature actor;EntranceAI ai{&actor};CreatureAI temporary;
        HashMapHolder<Player>::objects={{player.guid,&player}};ai.MoveInLineOfSight(&player);
        actor.Run(4999);assert(ai._entryPending && !actor.motion.moves && !player.credits && ai._arrivalPlayerGuid.IsEmpty());
        if(mode==1 || mode==3){actor.i_AI=mode==1?&temporary:nullptr;actor.i_disabledAI=&ai;}
        if(mode==2)actor.i_disabledAI=&ai;
        actor.motion.onMove=[&]{assert(actor.m_AI_locked);ai.Reset();};
        actor.Run(1);assert(!ai._entryPending && ai._introDone && ai._arrivalPlayerGuid==player.guid && player.credits==1 && actor.motion.moves==1);
        ai.CompleteEntranceArrival();actor.Run();assert(ai.firstwp && player.arrivalCredits==1 && player.credits==1);
        actor.i_disabledAI=nullptr;
    }
    {
        manager={};Player player,other;other.guid._high=8;Creature actor;EntranceAI ai{&actor};
        HashMapHolder<Player>::objects={{player.guid,&player},{other.guid,&other}};
        ai.MoveInLineOfSight(&player);actor.Run(4000);ai.Reset();assert(!ai._entryPending && !ai._introDone && ai._entryGeneration==1);
        ai.MoveInLineOfSight(&other);actor.Run(1000);assert(ai._entryPending && !player.credits && !other.credits && !actor.motion.moves);
        actor.Run(4000);assert(!ai._entryPending && ai._arrivalPlayerGuid==other.guid && other.credits==1 && !player.credits);
        ai.Reset();assert(ai._introDone && ai._entryGeneration==1 && ai._arrivalPlayerGuid==other.guid);
        ai.CompleteEntranceArrival();ai.MoveInLineOfSight(&player);actor.Run();
        assert(other.arrivalCredits==1 && !player.arrivalCredits && actor.motion.moves==1);
    }
    {
        manager={};Player player,other;other.guid._high=8;Creature actor;EntranceAI ai{&actor};bool resetOnce=false;
        HashMapHolder<Player>::objects={{player.guid,&player},{other.guid,&other}};
        ai.onTalk=[&](uint32 line){if(line==0 && !resetOnce){resetOnce=true;ai.Reset();ai.MoveInLineOfSight(&other);}};
        ai.MoveInLineOfSight(&player);assert(ai.Pending()==2 && ai._entryPending && ai._entryGeneration==1);
        actor.Run();assert(!player.credits && other.credits==1 && actor.motion.moves==1 && ai._arrivalPlayerGuid==other.guid);
    }
    {
        manager={};Player player;Creature actor;HashMapHolder<Player>::objects={{player.guid,&player}};
        auto original=new EntranceAI(&actor);original->MoveInLineOfSight(&player);
        assert(actor.pending.empty() && actor.AIM_Destroy() && !actor.i_AI);
        actor.Run();assert(!player.credits && !actor.motion.moves);
        auto replacement=std::make_unique<EntranceAI>(&actor);actor.Run();assert(!player.credits && replacement->_arrivalPlayerGuid.IsEmpty());
        replacement->MoveInLineOfSight(&player);
        actor.motion.onMove=[&]{auto before=debugMessages;assert(!actor.AIM_Destroy() && debugMessages==before+1);};
        actor.Run();assert(player.credits==1 && actor.motion.moves==1 && replacement->_arrivalPlayerGuid==player.guid);
        replacement->CompleteEntranceArrival();assert(player.arrivalCredits==1);
        actor.i_AI=nullptr;
    }
    {
        manager={};Creature actor;EntranceAI ai{&actor};ai.ArmFirstFight();ai.Reset();
        actor.Run(1999);assert(!ai.firstfight);actor.Run(1);assert(ai.firstfight && !actor.motion.moves);
    }
    {
        manager={};Creature actor;auto original=new EntranceAI(&actor);original->ArmFirstFight();
        assert(actor.pending.empty() && actor.AIM_Destroy());auto replacement=std::make_unique<EntranceAI>(&actor);
        actor.Run(2000);assert(!replacement->firstfight);replacement->ArmFirstFight();actor.Run(2000);assert(replacement->firstfight);
        actor.i_AI=nullptr;
    }
    std::cout<<"PASS: actual entrance LOS/reset/lifecycle/POINT1/firstfight timer and native FunctionProcessor/GUID/ObjectAccessor/Creature lock/AIM_Destroy;26 invalid5s/POINT1 contexts and prior original-recipient/20yd/0..1/duplicate/reentrant/follower checks retained. New4999+1ms entry/1999+1ms firstfight, pending Reset/fresh generation retry, accepted Reset retaining POINT1 token, synchronous Talk0 Reset/re-entry, primary/disabled/alias, both timer destruction/replacement and locked destruction rejection covered. Firstfight Reset behavior retained. Controlled world/motion/companion-heal stub, not persisted quest/scenario/group counters or full client traversal.\n";
#endif
}
