#include <atomic>
#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <vector>
using uint32=uint32_t;
using uint64=uint64_t;
using int32=int32_t;
#include "CriteriaTypes.inc"
struct AchievementGlobalMgr
{
    bool IsGroupCriteriaType(CriteriaTypes type) const;
} achievements;
auto* sAchievementMgr=&achievements;
#include "GroupTypes.inc"
using FunctionList=std::multimap<uint64,std::function<void()>>;
#include "FunctionClass.inc"
#include "FunctionMethods.inc"
enum {UNIT_STATE_STUNNED=1,UNIT_STATE_CASTING=2,WAYPOINT_MOTION_TYPE=2,POINT_MOTION_TYPE=8,
    UNIT_FIELD_FLAGS=0,UNIT_FLAG_NOT_SELECTABLE=1,
    UNIT_FLAG_IMMUNE_TO_PC=2,UNIT_FLAG_IMMUNE_TO_NPC=4,UNIT_FLAG_NOT_ATTACKABLE_1=8,EVENT_1=1,EVENT_2=2,
    CLASS_PALADIN=2,QUEST_STATUS_INCOMPLETE=3};
struct Creature;
struct Map;
extern Map liveMap;
struct CreatureAI
{
    virtual ~CreatureAI()=default;
    virtual void DoAction(int32)=0;
    virtual void Talk(uint32)=0;
};
struct Motion
{
    uint32 points=0;
    void MovePoint(uint32 identity,float positionX,float positionY,float positionZ)
    {assert(identity==1 && positionX==1914.39f && positionY==2335.43f && positionZ==75.56f);++points;}
};
struct Player
{
    bool contact=true;
    bool world=true,preDelete=false,gm=false,teleport=false,phase=true,rewarded=false,alive=true;
    uint32 klass=2,status=3,instanceId=42,faction=0;
    float distance=1;
    Map* map=&liveMap;
    Creature* third=nullptr;
    Creature* last=nullptr;
    Creature* boss=nullptr;
    uint32 lookups=0;
    std::vector<uint32> credits;
    bool CanContact() const{return contact;}
    bool IsInWorld() const{return world;}
    bool IsPreDelete() const{return preDelete;}
    bool isGameMaster() const{return gm;}
    bool IsBeingTeleported() const{return teleport;}
    uint32 getClass() const{return klass;}
    Map* GetMap() const{return map;}
    uint32 GetInstanceId() const{return instanceId;}
    bool InSamePhase(Creature*) const{return phase;}
    bool IsWithinDistInMap(Creature*,float radius) const{return distance<=radius;}
    bool GetQuestRewardStatus(uint32 quest) const{assert(quest==42120);return rewarded;}
    uint32 GetQuestStatus(uint32 quest) const{assert(quest==42120);return status;}
    Creature* FindNearestCreature(uint32 identity,float radius,bool living)
    {assert(radius==70 && living);++lookups;return identity==106676?third:identity==106777?last:boss;}
    std::function<void()> onCredit;
    bool InInstance() const{return map && instanceId==42;}
    void UpdateAchievementCriteria(CriteriaTypes type,uint32 event);
};
struct Reference
{
    Player* player;
    Player* getSource() const{return player;}
};
struct Map
{
    struct PlayerList : std::vector<Reference>
    {
        using std::vector<Reference>::operator=;
        bool isEmpty() const{return empty();}
    } players;
    PlayerList const& GetPlayers() const{return players;}
    uint32 GetInstanceId() const{return 42;}
} liveMap;
struct Creature
{
    CreatureAI* ai=nullptr;
    Motion motion;
    bool stunned=false,casting=false;
    bool world=true,preDelete=false,alive=true,mapPresent=true;
    uint32 entry=106676,mapId=1539,instanceId=42;
    uint32 despawn=0,flags=16;
    uint32 castResult=0;
    uint32 despawns=0;
    Map* map=&liveMap;
    Player* nearby=nullptr;
    std::vector<uint32> casts;
    CreatureAI* AI(){return ai;}
    bool IsInWorld() const{return world;}
    bool IsPreDelete() const{return preDelete;}
    bool IsAlive() const{return alive;}
    uint32 GetEntry() const{return entry;}
    uint32 GetMapId() const{return mapId;}
    uint32 GetInstanceId() const{return instanceId;}
    Map* GetMap(){return mapPresent?map:nullptr;}
    Motion* GetMotionMaster(){return &motion;}
    bool HasUnitState(uint32 state) const{return state==UNIT_STATE_STUNNED?stunned:casting;}
    float GetPositionX() const{return 1;}
    float GetPositionY() const{return 2;}
    float GetPositionZ() const{return 3;}
    float GetOrientation() const{return 4;}
    void SetHomePosition(float positionX,float positionY,float positionZ,float facing)
    {assert(positionX==1 && positionY==2 && positionZ==3 && facing==4);}
    Player* FindNearestPlayer(float radius){assert(radius==100);return nearby;}
    void DespawnOrUnsummon(uint32 delay){assert(delay==1000);despawn=delay;++despawns;}
    uint32 CastSpell(Creature* target,uint32 spell){assert(target==this && spell==215260);casts.push_back(spell);return castResult;}
    void SetFlag(uint32 field,uint32 mask){assert(field==UNIT_FIELD_FLAGS);flags|=mask;}
};
struct Scenario
{
    uint32 id=1092,step=8;
    uint32 GetScenarioId() const{return id;}
    uint32 GetCurrentStep() const{return step;}
    std::vector<uint32> events;
    void UpdateAchievementCriteria(uint32 event){events.push_back(event);}
};
struct ScenarioManager
{
    Scenario scenario;
    bool present=true;
    Scenario* GetScenario(uint32 instanceId){return present && instanceId==42?&scenario:nullptr;}
} scenarioManager;
auto* sScenarioMgr=&scenarioManager;
void Player::UpdateAchievementCriteria(CriteriaTypes type,uint32 event)
{
    assert(type==CRITERIA_TYPE_SCRIPT_EVENT_2);
    credits.push_back(event);
    if(onCredit)onCredit();
    bool ignoreGroup=false;
    uint32 referenceCache=event;
    Map* map=GetMap();
#include "ScenarioBridge.inc"
}
namespace SilverHandRitualEntry
{
#include "ActionPolicy.inc"
}
#include "ArrivalPolicy.inc"
uint32 warnings=0;
#define TC_LOG_WARN(...) ++warnings
namespace SilverHandCompanion {void Recover(Creature*,bool&,bool&) {}}
struct EventMap
{
    void Reset(){}
    void Update(uint32){}
    void RescheduleEvent(uint32,uint32){}
    uint32 ExecuteEvent() const{return 0;}
};
struct ThirdAI : CreatureAI
{
    Creature* me;
    FunctionProcessor _functions;
    FunctionProcessor _entryFunctions;
    EventMap events;
    bool _ritualStarted=false,_pendingHealRecovery=false,_entryPending=false,_introDone=false;
    bool _ritualMotionIssued=false,_ritualArrived=false,_ritualCompleted=false,wait_heal=false;
    uint64 _entryGeneration=0;
    std::vector<uint32> talks;
    std::function<void()> onTalk;
    explicit ThirdAI(Creature* actor):me(actor){actor->ai=this;}
    void Talk(uint32 line) override{talks.push_back(line);if(onTalk)onTalk();}
    bool UpdateVictim() const{return false;}
    void DoCast(uint32){assert(false);}
    void DoMeleeAttackIfReady(){assert(false);}
#include "Action.inc"
#include "Reset.inc"
#include "Update.inc"
#include "Movement.inc"
#include "Arrival.inc"
#include "Lifecycle.inc"
};
struct OtherAI : CreatureAI
{
    uint32 actions=0,talks=0;
    void DoAction(int32) override{++actions;}
    void Talk(uint32 line) override{assert(line==3);++talks;}
};
struct Instance
{
    Map* instance;
#include "Dispatch.inc"
};
int main()
{
#ifdef ARRIVAL_BASELINE
    {
        Creature actor;ThirdAI ai(&actor);Player outsider;outsider.klass=5;outsider.status=0;actor.nearby=&outsider;
        ai.MovementInform(POINT_MOTION_TYPE,1);ai.MovementInform(POINT_MOTION_TYPE,1);
        assert(actor.motion.points==0 && outsider.credits==std::vector<uint32>({50915,50915}) && actor.despawns==2);
        assert(scenarioManager.scenario.events==std::vector<uint32>({50915,50915}));
        std::cout<<"PASS pinned14992461: unissued POINT1 credits nearest ineligible Priest twice and despawns twice\n";
        return 0;
    }
#else
    {
        Creature actor;ThirdAI ai(&actor);
        ai.DoAction(0);ai.DoAction(1);ai.DoAction(1);
        ai.UpdateAI(1000);
#ifdef ACTION_BASELINE
        assert(actor.motion.points==3 && ai.talks==std::vector<uint32>({8,8,8}));
        std::cout<<"PASS action baseline: actual old DoAction accepts wrong action0 and repeated1, queues three overlapping ritual departures\n";
        return 0;
#else
        assert(actor.motion.points==1 && ai.talks==std::vector<uint32>{8});
        ai.DoAction(1);ai.Reset();ai.DoAction(1);ai.UpdateAI(1000);
        assert(actor.motion.points==1 && ai._ritualStarted);
#endif
    }
#ifndef ACTION_BASELINE
    assert(!SilverHandRitualEntry::CanAdvance(nullptr));
    {
        Creature actor;ThirdAI ai(&actor);
        for(int32 action:{-1,0,2,8})ai.DoAction(action);
        ai.UpdateAI(1000);assert(actor.motion.points==0 && !ai._ritualStarted);
        ai.DoAction(1);ai.Reset();ai.DoAction(1);
        ai.onTalk=[&ai]{ai.DoAction(1);};
        ai.UpdateAI(999);assert(actor.motion.points==0);
        ai.UpdateAI(1);ai.UpdateAI(1000);assert(actor.motion.points==1);
    }
    {
        Creature actor;
        {ThirdAI ai(&actor);ai.DoAction(1);}
        actor.ai=nullptr;ThirdAI replacement(&actor);replacement.UpdateAI(1000);
        assert(actor.motion.points==0 && !replacement._ritualStarted);
        replacement.DoAction(1);replacement.UpdateAI(1000);assert(actor.motion.points==1);
    }
    for(uint32 invalid=0;invalid<11;++invalid)
    {
        for(bool delayed:{false,true})
        {
            Creature actor;ThirdAI ai(&actor);
            scenarioManager=ScenarioManager{};
            if(delayed)ai.DoAction(1);
            switch(invalid)
            {
                case 0:actor.world=false;break;
                case 1:actor.preDelete=true;break;
                case 2:actor.alive=false;break;
                case 3:actor.entry=106777;break;
                case 4:actor.mapId=1220;break;
                case 5:actor.mapPresent=false;break;
                case 6:actor.instanceId=43;break;
                case 7:scenarioManager.present=false;break;
                case 8:scenarioManager.scenario.id=991;break;
                case 9:scenarioManager.scenario.id=1037;break;
                case 10:scenarioManager.scenario.step=7;break;
            }
            if(!delayed)ai.DoAction(1);
            ai.UpdateAI(1000);assert(actor.motion.points==0 && ai.talks.empty() && !ai._ritualStarted);
            actor.world=true;actor.preDelete=false;actor.alive=true;actor.entry=106676;actor.mapId=1539;actor.mapPresent=true;actor.instanceId=42;
            scenarioManager=ScenarioManager{};
            ai.DoAction(1);ai.UpdateAI(999);assert(actor.motion.points==0);
            ai.UpdateAI(1);assert(actor.motion.points==1);
        }
    }
    for(uint32 step=0;step<12;++step)
    {
        Creature actor;ThirdAI ai(&actor);scenarioManager=ScenarioManager{};
        scenarioManager.scenario.step=step;ai.DoAction(1);ai.UpdateAI(1000);
        assert(actor.motion.points==(step==8?1u:0u));
    }
    scenarioManager=ScenarioManager{};
#ifndef ARRIVAL_BASELINE
    for(uint32 faction:{0u,1u})
    {
        scenarioManager=ScenarioManager{};
        Creature actor;ThirdAI ai(&actor);Player outsider,eligible,duplicate;
        outsider.klass=5;eligible.faction=faction;eligible.alive=false;
        actor.nearby=&outsider;liveMap.players={{nullptr},{&outsider},{&eligible},{&duplicate}};
        ai.MovementInform(POINT_MOTION_TYPE,1);ai.UpdateLifecycle(1000);
        assert(actor.despawns==0 && eligible.credits.empty());
        ai.DoAction(1);
        ai.onTalk=[&ai]{ai.MovementInform(POINT_MOTION_TYPE,1);};
        ai.UpdateAI(1000);
        assert(!ai._ritualArrived && actor.motion.points==1 && eligible.credits.empty());
        ai.MovementInform(POINT_MOTION_TYPE,2);ai.MovementInform(7,1);
        eligible.onCredit=[&ai]{ai.MovementInform(POINT_MOTION_TYPE,1);ai.UpdateLifecycle(1);ai.Reset();};
        ai.MovementInform(POINT_MOTION_TYPE,1);
        ai.MovementInform(POINT_MOTION_TYPE,1);ai.UpdateLifecycle(1000);ai.DoAction(1);ai.UpdateAI(1000);
        assert(eligible.credits==std::vector<uint32>{50915} && outsider.credits.empty() && duplicate.credits.empty());
        assert(actor.despawns==1 && actor.motion.points==1 && ai._ritualCompleted);
        assert(scenarioManager.scenario.events==std::vector<uint32>{50915});
    }
    for(uint32 invalid=0;invalid<14;++invalid)
    {
        Creature actor;ThirdAI ai(&actor);Player player;Map otherMap;
        scenarioManager=ScenarioManager{};
        liveMap.players={{&player}};warnings=0;
        ai.DoAction(1);ai.UpdateAI(1000);
        switch(invalid)
        {
            case 0:player.world=false;break;
            case 1:player.preDelete=true;break;
            case 2:player.contact=false;break;
            case 3:player.gm=true;break;
            case 4:player.teleport=true;break;
            case 5:player.klass=1;break;
            case 6:player.map=&otherMap;break;
            case 7:player.map=nullptr;break;
            case 8:player.instanceId=43;break;
            case 9:player.phase=false;break;
            case 10:player.distance=100.01f;break;
            case 11:player.rewarded=true;break;
            case 12:player.status=0;break;
            case 13:player.status=1;break;
        }
        ai.MovementInform(POINT_MOTION_TYPE,1);ai.Reset();actor.stunned=true;
        ai.MovementInform(POINT_MOTION_TYPE,1);ai.UpdateLifecycle(5000);
        assert(player.credits.empty() && actor.despawns==0 && ai._ritualArrived && !ai._ritualCompleted && warnings==1);
        player.world=true;player.preDelete=false;player.contact=true;player.gm=false;player.teleport=false;
        player.klass=2;player.map=&liveMap;player.instanceId=42;player.phase=true;player.distance=100;player.rewarded=false;player.status=3;
        ai.UpdateLifecycle(1);ai.UpdateLifecycle(1);
        assert(player.credits==std::vector<uint32>{50915} && actor.despawns==1 && actor.motion.points==1);
        assert(scenarioManager.scenario.events==std::vector<uint32>{50915});
    }
    for(uint32 invalid=0;invalid<11;++invalid)
    {
        Creature actor;ThirdAI ai(&actor);Player player;
        liveMap.players={{&player}};scenarioManager=ScenarioManager{};
        ai.DoAction(1);ai.UpdateAI(1000);
        switch(invalid)
        {
            case 0:actor.world=false;break;
            case 1:actor.preDelete=true;break;
            case 2:actor.alive=false;break;
            case 3:actor.entry=106777;break;
            case 4:actor.mapId=1220;break;
            case 5:actor.mapPresent=false;break;
            case 6:actor.instanceId=43;break;
            case 7:scenarioManager.present=false;break;
            case 8:scenarioManager.scenario.id=991;break;
            case 9:scenarioManager.scenario.id=1037;break;
            case 10:scenarioManager.scenario.step=9;break;
        }
        ai.MovementInform(POINT_MOTION_TYPE,1);ai.UpdateLifecycle(1000);
        assert(player.credits.empty() && actor.despawns==0 && ai._ritualArrived);
        actor.world=true;actor.preDelete=false;actor.alive=true;actor.entry=106676;actor.mapId=1539;actor.mapPresent=true;actor.instanceId=42;
        scenarioManager=ScenarioManager{};ai.UpdateLifecycle(1);
        assert(player.credits==std::vector<uint32>{50915} && actor.despawns==1 && actor.motion.points==1);
    }
    {
        Creature actor;ThirdAI ai(&actor);Player late;liveMap.players.clear();warnings=0;
        scenarioManager=ScenarioManager{};
        ai.DoAction(1);ai.UpdateAI(1000);ai.MovementInform(POINT_MOTION_TYPE,1);
        ai.UpdateLifecycle(1000);assert(actor.despawns==0 && warnings==1);
        liveMap.players={{nullptr},{&late}};ai.Reset();ai.UpdateLifecycle(1);
        assert(late.credits==std::vector<uint32>{50915} && actor.despawns==1 && actor.motion.points==1);
        assert(scenarioManager.scenario.events==std::vector<uint32>{50915});
    }
    {
        Creature actor;Player player;liveMap.players.clear();warnings=0;
        {
            ThirdAI ai(&actor);ai.DoAction(1);ai.UpdateAI(1000);ai.MovementInform(POINT_MOTION_TYPE,1);
            assert(warnings==1 && actor.despawns==0);
        }
        ThirdAI replacement(&actor);liveMap.players={{&player}};
        replacement.UpdateLifecycle(1000);replacement.MovementInform(POINT_MOTION_TYPE,1);
        assert(player.credits.empty() && actor.despawns==0);
        replacement.DoAction(1);replacement.UpdateAI(1000);replacement.MovementInform(POINT_MOTION_TYPE,1);
        assert(player.credits==std::vector<uint32>{50915} && actor.despawns==1);
    }
    liveMap.players.clear();scenarioManager=ScenarioManager{};
    std::cout<<"PASS actual arrival: issued POINT1 only, both factions/dead eligible Player, one shared reference not nearest outsider, reentrant/repeated/reset exactly once;14 invalid Players and11 actor/scenario contexts defer without despawn, lifecycle retry even stunned, one warning, no movement replay; destroyed AI cannot complete through replacement\n";
#endif
    {
        Creature actor;
        ThirdAI ai(&actor);
        Player far,near;
        near.third=&actor;actor.nearby=&near;
        Map map;map.players={{&far},{&near}};actor.map=&map;far.map=&map;near.map=&map;far.distance=101;
        Instance script{&map};
        script.onScenarionNextStep(8);
        ai.UpdateAI(999);assert(actor.motion.points==0 && ai.talks.empty() && near.credits.empty());
        ai.UpdateAI(1);
#ifdef BASELINE
        assert(actor.motion.points==0 && near.lookups==0 && ai.talks.empty());
        std::cout<<"PASS baseline: actual step8 callback stops at first contactable participant lacking Travard; later participant's actor never receives native delayed action\n";
        return 0;
#else
        assert(actor.motion.points==1 && far.lookups==1 && near.lookups==1 && ai.talks==std::vector<uint32>{8});
        assert(near.credits.empty() && actor.despawn==0);
        ai.MovementInform(POINT_MOTION_TYPE,1);
        assert(near.credits==std::vector<uint32>{50915} && actor.despawn==1000);
#endif
    }
#ifndef BASELINE
    {
        Creature last,boss;
        OtherAI lastAI,bossAI;last.ai=&lastAI;boss.ai=&bossAI;
        Player first,second;second.last=&last;second.boss=&boss;
        Map map;map.players={{&first},{&second}};Instance script{&map};
        script.onScenarionNextStep(10);
#ifdef FINAL_BASELINE
        assert(lastAI.talks==0 && boss.casts.empty() && second.lookups==0);
        std::cout<<"PASS final baseline: step10 first contactable participant lacks both actors; original misses later Travard/Mordoth and exits without effects\n";
        return 0;
#else
        assert(lastAI.talks==1 && boss.casts==std::vector<uint32>{215260} && boss.flags==31);
        assert(first.lookups==2 && second.lookups==2);
#endif
    }
#ifndef FINAL_BASELINE
    for(bool bossFirst:{false,true})
    {
        Creature last,boss,otherLast,otherBoss;
        OtherAI lastAI,bossAI,otherLastAI,otherBossAI;
        last.ai=&lastAI;boss.ai=&bossAI;otherLast.ai=&otherLastAI;otherBoss.ai=&otherBossAI;
        Player first,second,duplicate;
        if(bossFirst){first.boss=&boss;second.last=&last;second.boss=&otherBoss;}
        else{first.last=&last;second.boss=&boss;second.last=&otherLast;}
        duplicate.last=&otherLast;duplicate.boss=&otherBoss;
        Map map;map.players={{&first},{&second},{&duplicate}};Instance script{&map};
        script.onScenarionNextStep(10);
        assert(first.lookups==2 && second.lookups==1 && duplicate.lookups==0);
        assert(lastAI.talks==1 && otherLastAI.talks==0 && boss.casts==std::vector<uint32>{215260} && otherBoss.casts.empty());
    }
    for(bool onlyBoss:{false,true})
    {
        Creature actor;OtherAI ai;actor.ai=&ai;
        Player first,second,offline;offline.contact=false;
        if(onlyBoss){first.boss=&actor;second.boss=&actor;}else{first.last=&actor;second.last=&actor;}
        Map map;map.players={{nullptr},{&offline},{&first},{&second}};Instance script{&map};
        script.onScenarionNextStep(10);
        assert(offline.lookups==0 && first.lookups==2 && second.lookups==1);
        assert(onlyBoss?(actor.casts==std::vector<uint32>{215260} && ai.talks==0):(ai.talks==1 && actor.casts.empty()));
    }
    {
        Player first,second;
        Map map;Instance script{&map};script.onScenarionNextStep(10);
        map.players={{&first},{&second}};script.onScenarionNextStep(10);
        assert(first.lookups==2 && second.lookups==2);
        Creature last,boss;OtherAI lastAI,bossAI;last.ai=&lastAI;boss.ai=&bossAI;
        second.last=&last;second.boss=&boss;boss.castResult=7;
        script.onScenarionNextStep(10);
        assert(lastAI.talks==1 && boss.casts==std::vector<uint32>{215260} && boss.flags==31);
    }
    for(uint32 prefix=0;prefix<4;++prefix)
    {
        Creature actor;
        ThirdAI ai(&actor);
        Player absent,offline,near,duplicate;
        offline.contact=false;near.third=&actor;duplicate.third=&actor;
        Map map;
        if(prefix&1)map.players.push_back({nullptr});
        if(prefix&2){map.players.push_back({&offline});map.players.push_back({&absent});}
        map.players.push_back({&near});map.players.push_back({&duplicate});
        Instance script{&map};script.onScenarionNextStep(8);
        ai.UpdateAI(1000);ai.UpdateAI(1000);
        assert(actor.motion.points==1 && ai.talks==std::vector<uint32>{8} && duplicate.lookups==0 && offline.lookups==0);
    }
    {
        Player first,second;
        Map map;Instance script{&map};
        script.onScenarionNextStep(8);
        map.players={{&first},{&second}};script.onScenarionNextStep(8);
        assert(first.lookups==1 && second.lookups==1);
        Creature actor;ThirdAI ai(&actor);second.third=&actor;
        script.onScenarionNextStep(8);ai.UpdateAI(1000);
        assert(actor.motion.points==1 && ai.talks==std::vector<uint32>{8});
    }
    {
        Creature actor;ThirdAI ai(&actor);
        Player player;player.third=&actor;
        Map map;map.players={{&player}};Instance script{&map};
        for(uint32 step:{0,1,5,7,9,11})script.onScenarionNextStep(step);
        ai.UpdateAI(5000);assert(actor.motion.points==0 && player.lookups==0);
        script.onScenarionNextStep(8);actor.stunned=true;ai.UpdateAI(1000);assert(actor.motion.points==0);
        actor.stunned=false;ai.UpdateAI(999);assert(actor.motion.points==0);
        ai.UpdateAI(1);assert(actor.motion.points==1);
    }
    {
        Creature third,last,boss;
        ThirdAI thirdAI(&third);OtherAI lastAI,bossAI;last.ai=&lastAI;boss.ai=&bossAI;
        Player first,second;first.last=&last;first.boss=&boss;second.third=&third;
        Map map;map.players={{&first},{&second}};Instance script{&map};
        script.onScenarionNextStep(10);thirdAI.UpdateAI(5000);
        assert(lastAI.talks==1 && boss.casts==std::vector<uint32>{215260} && boss.flags==31);
        assert(third.motion.points==0 && second.lookups==0 && thirdAI.talks.empty());
    }
    std::cout<<"PASS: full native instance callback and third handlers/native queue; step8 missing-first actor recovery/one action, step10 independent last-Travard/Mordoth searches/split participants/partial and absent actors/one effect per actor per call/uncontactable skips/failed-cast semantics preserved, original1000ms/stun/POINT1 and unrelated steps. Controlled world/motion/criteria services, not client traversal\n";
#endif
#endif
#endif
#endif
}
