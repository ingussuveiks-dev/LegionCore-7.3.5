#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <vector>
using uint32=uint32_t;
using uint64=uint64_t;
using int32=int32_t;
using int64=int64_t;
enum {CLASS_PALADIN=2,QUEST_STATUS_INCOMPLETE=3,TYPEID_PLAYER=4};
#include "UnitState.inc"
#include "CriteriaTypes.inc"
struct AchievementGlobalMgr {bool IsGroupCriteriaType(CriteriaTypes type) const;} achievements;
auto* sAchievementMgr=&achievements;
#include "GroupTypes.inc"
using FunctionList=std::multimap<uint64,std::function<void()>>;
#include "FunctionClass.inc"
#include "FunctionMethods.inc"
struct Player;
struct Reference {Player* player;Player* getSource() const{return player;}};
struct Map
{
    std::vector<Reference> players;
    uint32 GetInstanceId() const{return 42;}
    auto const& GetPlayers() const{return players;}
} liveMap,otherMap;
struct ObjectGuid
{
    uint64 _low,_high;
    ObjectGuid();
    ObjectGuid(uint64 low,uint64 high):_low(low),_high(high){}
    void Clear();bool IsEmpty() const;
    bool IsPlayer() const{return _high==1;}
    bool operator<(ObjectGuid const& other) const{return _high<other._high || (_high==other._high && _low<other._low);}
};
#include "GuidMethods.inc"
struct WorldObject
{
    virtual ~WorldObject()=default;
    Map* contextMap=&liveMap;
    bool world=true,preDelete=false;
    ObjectGuid objectGuid{0,2};
    Map* GetMap() const{return contextMap;}
    bool IsInWorld() const{return world;}
    bool IsPreDelete() const{return preDelete;}
    ObjectGuid GetGUID() const{return objectGuid;}
};
struct Unit;
struct Creature;
struct InstanceScript
{
    bool CheckDamageManager() const{return false;}
    void UpdateDamageManager(ObjectGuid,int32,bool){}
};
struct UnitAI
{
    virtual ~UnitAI()=default;
    virtual void HealReceived(Unit*,uint32&){}
    virtual void HealDone(Unit*,uint32&){}
    virtual void Reset(){}
    virtual void UpdateLifecycle(uint32){}
};
using CreatureAI=UnitAI;
struct Unit : WorldObject
{
    Unit* owner=nullptr;
    Unit* charmer=nullptr;
    bool IsAIEnabled=false,alive=true;
    UnitAI* ai=nullptr;
    uint64 health=1,maximum=1000;
    FunctionProcessor functions;
    virtual Player* ToPlayer(){return nullptr;}
    virtual Player const* ToPlayer() const{return nullptr;}
    virtual Creature* ToCreature(){return nullptr;}
    ObjectGuid GetCharmerOrOwnerGUID() const{return charmer?charmer->GetGUID():owner?owner->GetGUID():ObjectGuid{};}
    Player* GetCharmerOrOwnerPlayerOrPlayerItself() const;
    UnitAI* GetAI(){return ai;}
    InstanceScript* GetInstanceScript(){return nullptr;}
    uint64 GetHealth(Unit* =nullptr) const{return health;}
    uint64 GetMaxHealth(Unit* =nullptr) const{return maximum;}
    bool IsAlive() const{return alive;}
    float GetHealthPct() const{return maximum?100.0f*float(health)/float(maximum):0;}
    void SetHealth(uint64 value,uint32){health=value;}
    void SetHealthScal(uint64 value,Unit*,uint32){health=value;}
    int64 ModifyHealth(int64 dVal,Unit* victim=nullptr,uint32 spellId=0);
    void AddDelayedEvent(uint64 offset,std::function<void()>&& function){assert(offset==0);functions.AddDelayedEvent(offset,std::move(function));}
    int32 DealHeal(Unit* victim,uint32 addhealth);
};
struct Scenario
{
    uint32 id=1092,step=7,first=0,second=0;
    uint32 GetScenarioId() const{return id;}
    uint32 GetCurrentStep() const{return step;}
    void UpdateAchievementCriteria(uint32 identity){assert(identity==50913 || identity==50914);if(identity==50913)++first;else ++second;}
};
struct ScenarioManager
{
    bool present=true;
    Scenario scenario;
    Scenario* GetScenario(uint32 instance){return present && instance==42?&scenario:nullptr;}
} scenarios;
auto* sScenarioMgr=&scenarios;
struct Player : Unit
{
    bool contact=true,gm=false,teleport=false,phase=true,rewarded=false;
    uint32 klass=2,status=3,instance=42,faction=0,credits=0;
    float distance=1;
    Player(){objectGuid={100,1};}
    Player* ToPlayer() override{return this;}
    Player const* ToPlayer() const override{return this;}
    bool CanContact() const{return contact;}
    bool isGameMaster() const{return gm;}
    bool IsBeingTeleported() const{return teleport;}
    uint32 getClass() const{return klass;}
    uint32 GetInstanceId() const{return instance;}
    bool InSamePhase(Creature*) const{return phase;}
    bool IsWithinDistInMap(Creature*,float radius) const{return distance<=radius;}
    bool GetQuestRewardStatus(uint32 quest) const{assert(quest==42120);return rewarded;}
    uint32 GetQuestStatus(uint32 quest) const{assert(quest==42120);return status;}
    bool InInstance() const{return contextMap==&liveMap;}
    void UpdateAchievementCriteria(CriteriaTypes type,uint32 identity)
    {
        assert(type==CRITERIA_TYPE_SCRIPT_EVENT_2 && (identity==50913 || identity==50914));++credits;
        bool ignoreGroup=false;
        uint32 referenceCache=identity;
        Map* map=GetMap();
#include "ScenarioBridge.inc"
    }
    template<class Packet>void SendDirectMessage(Packet const&){}
};
namespace WorldPackets::Combat {struct HealthUpdate {ObjectGuid Guid;uint64 Health=0;int Write(){return 0;}};}
#include "ModifyHealth.inc"
template<class Object>struct HashMapHolder
{
    static std::map<ObjectGuid,Object*> objects;
    static Object* Find(ObjectGuid guid){auto found=objects.find(guid);return found==objects.end()?nullptr:found->second;}
};
template<class Object>std::map<ObjectGuid,Object*> HashMapHolder<Object>::objects;
struct ObjectAccessor
{
#include "WorldPlayer.inc"
#include "MapPlayer.inc"
    static Player* GetPlayer(WorldObject const&,ObjectGuid);
};
#include "AccessorPlayer.inc"
#include "OwnerPlayer.inc"
struct Motion
{
    uint32 moves=0;
    std::function<void()> onMove;
    void MovePoint(uint32 point,float horizontal,float vertical,float height)
    {assert(point==0 && horizontal==1875.49f && vertical==2329.93f && height==49.64f);++moves;if(onMove)onMove();}
};
struct Creature : Unit
{
    Creature* ToCreature() override{return this;}
    // Existing rescue fixture isolates lifecycle behavior; dispatch is tested separately.
    void NotifyHealApplied(Unit*,uint32,int32){}
    uint32 entry=106933,mapId=1539,instance=42,states=UNIT_STATE_STUNNED,despawns=0;
    std::vector<uint32> removals;
    Motion motion;
    Player* nearest=nullptr;
    UnitAI* i_AI=nullptr;
    UnitAI* i_disabledAI=nullptr;
    bool m_AI_locked=false;
    Creature(){IsAIEnabled=true;}
    uint32 GetEntry() const{return entry;}
    uint32 GetMapId() const{return mapId;}
    uint32 GetInstanceId() const{return instance;}
    void ClearUnitState(uint32 state){assert(state==UNIT_STATE_STUNNED);states=0;}
    void RemoveAura(uint32 spell){assert(spell==211699 || spell==211698);removals.push_back(spell);}
    void DespawnOrUnsummon(uint32 delay){assert(delay==3000);++despawns;}
    Motion* GetMotionMaster(){return &motion;}
    Player* FindNearestPlayer(float radius){assert(radius==100);return nearest;}
    UnitAI* AI(){return i_AI;}
    void TickLifecycle(uint32 diff)
    {
#include "LifecycleBridge.inc"
    }
};
struct ScriptedAI : UnitAI {Creature* me;explicit ScriptedAI(Creature* creature):me(creature){creature->ai=creature->i_AI=this;}};
#include "RescuePolicy.inc"
#include "RescueAI.inc"
#include "DealHealPrefix.inc"
struct HealerAI : UnitAI
{
    bool cancel=false;
    void HealDone(Unit*,uint32& amount) override{if(cancel)amount=0;}
};
struct Fixture
{
    Player owner,neighbor,member;
    Creature actor;
    Unit pet,guardian,npc;
    HealerAI healerAI;
    npc_silverhand_eventsAI rescue{&actor};
    Fixture()
    {
        scenarios={};neighbor.objectGuid._low=101;member.objectGuid._low=102;neighbor.status=0;
        liveMap.players={{&neighbor},{nullptr},{&owner},{&member}};
        actor.nearest=&neighbor;pet.owner=guardian.owner=&owner;
        HashMapHolder<Player>::objects={{owner.objectGuid,&owner},{neighbor.objectGuid,&neighbor},{member.objectGuid,&member}};
    }
};
void Invalidate(uint32 invalid,Fixture& fixture)
{
    auto& owner=fixture.owner;auto& actor=fixture.actor;
    switch(invalid)
    {
        case 0:owner.world=false;break;case 1:owner.preDelete=true;break;case 2:owner.contact=false;break;
        case 3:owner.gm=true;break;case 4:owner.teleport=true;break;case 5:owner.klass=1;break;
        case 6:actor.world=false;break;case 7:actor.preDelete=true;break;case 8:actor.alive=false;break;
        case 9:actor.mapId=1220;break;case 10:actor.entry=106774;break;case 11:owner.contextMap=nullptr;break;
        case 12:owner.contextMap=&otherMap;break;case 13:owner.instance=43;break;case 14:owner.phase=false;break;
        case 15:owner.distance=100.01f;break;case 16:owner.rewarded=true;break;case 17:owner.status=0;break;
        case 18:owner.status=1;break;case 19:scenarios.present=false;break;case 20:scenarios.scenario.id=988;break;
        case 21:scenarios.scenario.step=6;break;case 22:scenarios.scenario.step=8;break;case 23:actor.contextMap=nullptr;break;
        default:assert(false);
    }
}
int main()
{
#ifdef BASELINE
    Fixture fixture;
    assert(fixture.owner.DealHeal(&fixture.actor,999)==999 && fixture.actor.health==1000);
    fixture.actor.TickLifecycle(0);
    assert(!fixture.rescue.wait_heal && fixture.owner.credits==0 && fixture.actor.despawns==0);
    fixture.owner.DealHeal(&fixture.actor,1);
    assert(fixture.rescue.wait_heal && fixture.neighbor.credits==1 && fixture.owner.credits==0);
    std::cout<<"PASS baseline: actual0a643f8a HealReceived precedes native ModifyHealth; one full heal leaves rescued actor latched false, another heal dispatches to ineligible closest neighbor. Not persisted credit/client proof.\n";
#else
    for(uint32 entry : {106933U,106721U,106722U})
        for(uint32 kind : {0U,1U,2U,3U})
            for(uint32 faction : {0U,1U})
            {
                Fixture fixture;fixture.actor.entry=entry;fixture.owner.faction=faction;
                Unit* healer=kind==0?static_cast<Unit*>(&fixture.owner):kind==1?&fixture.pet:kind==2?&fixture.guardian:&fixture.npc;
                assert(healer->DealHeal(&fixture.actor,899)==899);
                assert(fixture.actor.health==900 && !fixture.rescue.wait_heal && fixture.actor.despawns==0);
                fixture.actor.motion.onMove=[&]{assert(fixture.actor.m_AI_locked);uint32 heal=1;fixture.rescue.HealReceived(healer,heal);};
                fixture.actor.TickLifecycle(0);
                assert(fixture.rescue.wait_heal && fixture.owner.credits==1 && fixture.neighbor.credits==0 && fixture.member.credits==0);
                assert(fixture.actor.despawns==1 && fixture.actor.motion.moves==1 && fixture.actor.states==0);
                assert((fixture.actor.removals==std::vector<uint32>{211699,211698}));
                assert(scenarios.scenario.first==uint32(entry==106933) && scenarios.scenario.second==uint32(entry!=106933));
                fixture.rescue.Reset();healer->DealHeal(&fixture.actor,100);fixture.actor.TickLifecycle(0);
                assert(fixture.owner.credits==1 && fixture.rescue.wait_heal);
            }
    for(uint32 invalid=0;invalid<24;++invalid)
        for(bool delayed : {false,true})
        {
            Fixture fixture;fixture.member.status=0;
            if(!delayed)Invalidate(invalid,fixture);
            fixture.owner.DealHeal(&fixture.actor,999);
            if(delayed)Invalidate(invalid,fixture);
            fixture.actor.TickLifecycle(0);
            assert(!fixture.rescue.wait_heal && fixture.owner.credits==0 && fixture.neighbor.credits==0 && fixture.actor.despawns==0);
        }
    for(bool cancelled : {false,true})
    {
        Fixture fixture;fixture.guardian.IsAIEnabled=true;fixture.guardian.ai=&fixture.healerAI;fixture.healerAI.cancel=cancelled;
        fixture.guardian.DealHeal(&fixture.actor,898);fixture.actor.TickLifecycle(0);
        assert(!fixture.rescue.wait_heal && fixture.actor.despawns==0);
        fixture.healerAI.cancel=false;fixture.guardian.DealHeal(&fixture.actor,999);fixture.actor.TickLifecycle(0);
        assert(fixture.rescue.wait_heal && fixture.owner.credits==1);
    }
    {
        Fixture fixture;
        fixture.owner.DealHeal(&fixture.actor,999);fixture.rescue.Reset();fixture.actor.TickLifecycle(0);
        assert(!fixture.rescue.wait_heal && fixture.actor.despawns==0);
        fixture.owner.DealHeal(&fixture.actor,1);fixture.actor.TickLifecycle(0);
        assert(fixture.owner.credits==1);
    }
    {
        Fixture fixture;
        fixture.owner.DealHeal(&fixture.actor,500);fixture.owner.DealHeal(&fixture.actor,499);fixture.actor.TickLifecycle(0);
        assert(fixture.owner.credits==1 && fixture.actor.motion.moves==1);
    }
    {
        Fixture fixture;fixture.owner.DealHeal(&fixture.actor,999);
        fixture.owner.world=false;fixture.actor.TickLifecycle(0);
        assert(fixture.owner.credits==0 && fixture.member.credits==1);
    }
    {
        Fixture fixture;fixture.owner.DealHeal(&fixture.actor,999);
        fixture.actor.health=899;fixture.actor.TickLifecycle(0);
        assert(!fixture.rescue.wait_heal);fixture.owner.DealHeal(&fixture.actor,1);fixture.actor.TickLifecycle(0);
        assert(fixture.owner.credits==1);
    }
    {
        Fixture fixture;fixture.actor.health=1000;fixture.owner.DealHeal(&fixture.actor,0);fixture.actor.TickLifecycle(0);
        assert(!fixture.rescue.wait_heal);
        uint32 heal=1;fixture.rescue.HealReceived(nullptr,heal);fixture.actor.TickLifecycle(0);assert(!fixture.rescue.wait_heal);
    }
    {
        Fixture fixture;fixture.owner.alive=false;fixture.guardian.charmer=&fixture.member;
        fixture.guardian.DealHeal(&fixture.actor,999);fixture.actor.TickLifecycle(0);
        assert(fixture.owner.credits==0 && fixture.member.credits==1);
    }
    {
        Fixture fixture;fixture.owner.alive=false;fixture.pet.DealHeal(&fixture.actor,999);fixture.actor.TickLifecycle(0);
        assert(fixture.owner.credits==1);
    }
    {
        Fixture fixture;fixture.member.status=0;fixture.owner.DealHeal(&fixture.actor,999);fixture.owner.status=0;
        fixture.actor.TickLifecycle(0);assert(!fixture.rescue.wait_heal);
        fixture.owner.status=3;fixture.owner.DealHeal(&fixture.actor,1);fixture.actor.TickLifecycle(0);
        assert(fixture.owner.credits==1);
    }
    {
        Fixture fixture;
        auto temporary=std::make_unique<Player>();temporary->objectGuid={200,1};
        HashMapHolder<Player>::objects[temporary->objectGuid]=temporary.get();
        temporary->DealHeal(&fixture.actor,999);HashMapHolder<Player>::objects.erase(temporary->objectGuid);temporary.reset();
        fixture.owner.status=0;fixture.actor.TickLifecycle(0);assert(fixture.member.credits==1);
    }
    {
        Fixture fixture;fixture.owner.DealHeal(&fixture.actor,999);fixture.rescue.Reset();
        fixture.owner.DealHeal(&fixture.actor,1);fixture.actor.TickLifecycle(0);
        assert(fixture.owner.credits==1 && fixture.actor.despawns==1);
    }
    {
        Fixture fixture;std::vector<std::unique_ptr<Creature>> actors;
        std::vector<std::unique_ptr<npc_silverhand_eventsAI>> handlers;
        for(uint32 entry : {106933U,106933U,106721U,106722U})
        {
            auto actor=std::make_unique<Creature>();actor->entry=entry;
            auto handler=std::make_unique<npc_silverhand_eventsAI>(actor.get());
            fixture.guardian.DealHeal(actor.get(),999);actor->TickLifecycle(0);actor->TickLifecycle(0);
            actors.push_back(std::move(actor));handlers.push_back(std::move(handler));
        }
        assert(scenarios.scenario.first==2 && scenarios.scenario.second==2 && fixture.owner.credits==4);
    }
    for(uint32 mode : {0U,1U})
    {
        Fixture fixture;UnitAI temporary;
        fixture.owner.DealHeal(&fixture.actor,999);
        fixture.actor.i_disabledAI=&fixture.rescue;
        fixture.actor.i_AI=mode?static_cast<UnitAI*>(&fixture.rescue):&temporary;
        fixture.actor.TickLifecycle(0);
        assert(fixture.owner.credits==1 && fixture.actor.motion.moves==1 && !fixture.actor.m_AI_locked);
    }
    {
        Fixture fixture;
        auto previous=std::make_unique<npc_silverhand_eventsAI>(&fixture.actor);
        fixture.owner.DealHeal(&fixture.actor,999);previous.reset();
        auto replacement=std::make_unique<npc_silverhand_eventsAI>(&fixture.actor);
        fixture.actor.TickLifecycle(0);assert(fixture.owner.credits==0 && fixture.actor.despawns==0);
        fixture.owner.DealHeal(&fixture.actor,1);fixture.actor.TickLifecycle(0);assert(fixture.owner.credits==1);
    }
    std::cout<<"PASS: actual rescue AI/policy, native DealHeal prefix/ModifyHealth, locked primary/disabled/alias lifecycle bridge, AI-owned zero-delay FunctionProcessor and full-GUID owner/map/world/predelete lookup;24 initial/delayed invalid contexts, three entries, both faction labels, player/pet/guardian/NPC/charmer/dead owner,90pct crossing, cancelled/zero/subthreshold heals, reentrant/concurrent dedup, four-actor2+2, destroyed player/AI replacement, shared fallback, Reset generation/retained completed latch and retry. World/geometry/health getters/counter are controlled; not actual spell targeting/party counters or client traversal.\n";
#endif
}
