#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>
using uint32=uint32_t;
using uint64=uint64_t;
using ObjectGuid=uint64;
enum {EFFECT_0=0,CONVERSATION_FIELD_LAST_LINE_END_TIME=12};
#define TC_LOG_ERROR(...) (++errors)
uint32 errors=0,timeNow=0;
uint32 getMSTime(){return timeNow;}
struct Position {uint32 value=0;};
struct SpellEffect {uint32 MiscValue=2152;};
struct SpellInfo {uint32 Id=211824;SpellEffect effect;SpellEffect const* Effects[1]{&effect};};
struct Map {} activeMap,otherMap;
struct WorldObject {
    bool inWorld=true;
    uint32 phase=1;
    bool IsInWorld()const{return inWorld;}
    bool InSamePhase(WorldObject const* other)const{return other&&phase==other->phase;}
};
struct Creature:WorldObject {
    uint32 entry=106714,mapId=1609,instance=42,health=100;
    ObjectGuid guid=71;
    bool alive=true;
    Map* map=&activeMap;
    Creature* boss=nullptr;
    uint32 GetEntry()const{return entry;}
    uint32 GetMapId()const{return mapId;}
    uint32 GetInstanceId()const{return instance;}
    uint32 GetHealth()const{return health;}
    ObjectGuid GetGUID()const{return guid;}
    Map* GetMap()const{return map;}
    bool IsAlive()const{return alive;}
    Creature* FindNearestCreature(uint32 identity,float distance,bool aliveOnly){assert(identity==106730&&distance==100&&aliveOnly);return boss;}
};
struct Progress {uint64 Counter=0;};
struct Achievement {std::map<uint32,Progress> progress;auto GetCriteriaProgressMap(){return &progress;}};
struct Scenario {uint32 id=1099,step=2;Achievement achievement;uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;}auto& GetAchievementMgr(){return achievement;}} scenario;
struct ScenarioManager {bool exists=true;Scenario* GetScenario(uint32 identity){return exists&&identity==42?&scenario:nullptr;}} manager;
auto sScenarioMgr=&manager;
bool createOk=true;
std::vector<struct Conversation*> created;
struct Conversation:WorldObject {
    bool expired=false;
    uint32 duration=12000,casterGUID=3,deadline=12000,identity=0;
    Position position;
    std::map<uint32,uint32> playing;
    virtual ~Conversation()=default;
    virtual void Update(uint32);
    bool IsQuestObjectExpired()const{return expired;}
    uint32 GetDuration()const{return duration;}
    uint32 GetUInt32Value(uint32 field)const{assert(field==12);return deadline;}
    uint32 GetCasterGUID()const{return casterGUID;}
    void Remove(){inWorld=false;}
    bool CreateConversation(uint64,uint32 entry,Creature*,SpellInfo const*,Position const& target){
        if(!createOk)return false;
        identity=entry;position=target;created.push_back(this);return true;
    }
};
Creature* activeActor=nullptr;
struct ObjectAccessor {static Creature* GetCreature(WorldObject const&,uint32 identity){assert(identity==3);return activeActor;}};
enum class HighGuid {Conversation};
struct Generator {uint64 Generate(){return 1;}} generator;
struct ObjectManager {template<HighGuid>Generator* GetGenerator(){return &generator;}} objects;
auto sObjectMgr=&objects;
#include "BaseUpdate.inc"
#include "Combat.inc"
void Clear(){for(auto* story:created)delete story;created.clear();scenario={};manager.exists=true;createOk=true;errors=0;timeNow=0;}
int main()
{
    _set_abort_behavior(0,_CALL_REPORTFAULT);
    Creature actor,boss;
    boss.entry=106730;actor.boss=&boss;activeActor=&actor;
    assert(TitanstrikeCombatDialogue::Allowed(nullptr));
    actor.mapId=1;assert(TitanstrikeCombatDialogue::Allowed(&actor));actor.mapId=1609;
    actor.entry=1;assert(TitanstrikeCombatDialogue::Allowed(&actor));actor.entry=106714;
    assert(TitanstrikeCombatDialogue::Allowed(&actor));
    for(uint32 step:{0u,1u,3u,4u,5u}){scenario.step=step;assert(!TitanstrikeCombatDialogue::Allowed(&actor));}
    scenario.step=2;
    scenario.achievement.progress[48020]={0};assert(TitanstrikeCombatDialogue::Allowed(&actor));
    for(uint32 identity:{2152u,2153u,2154u}){
        Clear();SpellInfo info;info.effect.MiscValue=identity;
        TitanstrikeCombatDialogue::Create(&actor,&info,Position{71});
        assert(created.size()==1&&created[0]->identity==identity&&created[0]->position.value==71);
        created[0]->Update(1);assert(created[0]->IsInWorld());
        scenario.achievement.progress[48020]={1};created[0]->Update(1);assert(!created[0]->IsInWorld());
        created[0]->Update(1);assert(created.size()==1);
    }
    for(uint32 failure=0;failure<14;++failure){
        Clear();actor=Creature{};boss=Creature{};boss.entry=106730;actor.boss=&boss;activeActor=&actor;
        if(failure==0)actor.inWorld=false;if(failure==1)actor.alive=false;if(failure==2)manager.exists=false;
        if(failure==3)scenario.id=1;if(failure==4)scenario.step=3;if(failure==5)scenario.achievement.progress[48020]={1};
        if(failure==6)actor.boss=nullptr;if(failure==7)boss.inWorld=false;if(failure==8)boss.alive=false;
        if(failure==9)boss.health=1;if(failure==10)boss.health=0;if(failure==11)boss.map=&otherMap;if(failure==12)boss.phase=2;
        if(failure==13)actor.instance=7;
        assert(!TitanstrikeCombatDialogue::Allowed(&actor));SpellInfo info;
        TitanstrikeCombatDialogue::Create(&actor,&info,Position{});assert(created.empty());
    }
    for(uint32 failure=0;failure<5;++failure){
        Clear();actor=Creature{};boss=Creature{};boss.entry=106730;actor.boss=&boss;activeActor=&actor;SpellInfo info;
        TitanstrikeCombatDialogue::Create(&actor,&info,Position{});auto* story=created.back();
        if(failure==0)activeActor=nullptr;if(failure==1)actor.mapId=1;if(failure==2)actor.entry=1;if(failure==3)actor.phase=2;if(failure==4)story->expired=true;
        story->Update(1);assert(!story->IsInWorld());
    }
    Clear();actor=Creature{};boss=Creature{};boss.entry=106730;actor.boss=&boss;activeActor=&actor;SpellInfo info;
    createOk=false;TitanstrikeCombatDialogue::Create(&actor,&info,Position{});assert(created.empty()&&errors==1);
    createOk=true;TitanstrikeCombatDialogue::Create(&actor,&info,Position{});assert(created.size()==1);
    ++boss.guid;created[0]->Update(1);assert(!created[0]->IsInWorld());
    scenario.achievement.progress.clear();scenario.achievement.progress[30091]={1};assert(TitanstrikeCombatDialogue::Allowed(&actor));
    Clear();activeActor=&actor;
#ifdef BASELINE
    Conversation running;
#else
    TitanstrikeCombatDialogue::Story running(boss.GetGUID());
#endif
    scenario.achievement.progress[48020]={1};running.Update(1);assert(!running.IsInWorld());
    std::cout<<"PASS: native NPC-caster baseline retains combat dialogue after defeat; scoped production guard/story reject late creation and retire active dialogue on defeat/surrender/missing actor/map/phase/step/scope expiry, preserving other uses and native payload/position requests; controlled world/create sinks, not client audio.\n";
}
