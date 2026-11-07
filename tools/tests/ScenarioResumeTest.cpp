#include "ScenarioResume.h"
#include <cassert>
#include <iostream>
#include <map>
#include <limits>
using uint32=uint32_t;using uint16=uint16_t;using uint64=uint64_t;
#define TC_LOG_INFO(...) ((void)0)
constexpr int SCENARIO_TYPE_CHALLENGE_MODE=1;
constexpr int QUEST_STATUS_INCOMPLETE=3,ALLIANCE=0,CLASS_WARRIOR=1,CLASS_WARLOCK=9,CLASS_MONK=10;
struct SpellTargetPosition {uint32 target_mapId;float target_X,target_Y,target_Z,target_Orientation;};
struct SpellManager {std::map<uint32,SpellTargetPosition> targets;SpellTargetPosition const* GetSpellTargetPosition(uint32 id){auto i=targets.find(id);return i==targets.end()?nullptr:&i->second;}} spellManager;
auto sSpellMgr=&spellManager;
struct Position {
    float x=10,y=20,z=30,o=1;
    float GetPositionX()const{return x;}float GetPositionY()const{return y;}
    float GetPositionZ()const{return z;}float GetOrientation()const{return o;}
    void Relocate(float a,float b,float c,float d){x=a;y=b;z=c;o=d;}
};
struct MapEntry {
    uint32 ID=9000;bool scenario=true,pvp=false,garrison=false;
    bool IsScenario()const{return scenario;}bool IsBattlegroundOrArena()const{return pvp;}
    bool IsGarrison()const{return garrison;}
};
struct ScenarioEntry {int Type=0;};
enum Difficulty { DIFFICULTY_NORMAL_SCENARIO=12 };
struct MapDifficultyEntry {uint32 DifficultyID=12;};
struct DB2Manager {MapDifficultyEntry value;bool missing=false;MapDifficultyEntry const* GetDefaultMapDifficulty(uint32){return missing?nullptr:&value;}} sDB2Manager;
struct ScenarioStore {ScenarioEntry value;bool missing=false;ScenarioEntry const* LookupEntry(uint32 id){return missing||!id?nullptr:&value;}} sScenarioStore;
struct MapStore {MapEntry value;bool missing=false;MapEntry const* LookupEntry(uint32 id){return missing||value.ID!=id?nullptr:&value;}} sMapStore;
struct Map {
    MapEntry entry;
    uint32 GetId()const{return entry.ID;}bool IsScenario()const{return entry.IsScenario();}
    MapEntry const* GetEntry()const{return &entry;}
};
struct Scenario {
    Map* map;uint32 id=777,step=0;bool completed=false;
    Map* GetMap(){return map;}uint32 GetScenarioId(){return id;}
    bool IsCompleted(bool){return completed;}uint32 GetCurrentStep(){return step;}
};
struct ScenarioManager {
    std::map<uint32,Scenario*> entries;bool missingSteps=false;
    Scenario* GetScenario(uint32 id){auto i=entries.find(id);return i==entries.end()?nullptr:i->second;}
    int const* GetScenarioSteps(uint32){static int steps;return missingSteps?nullptr:&steps;}
} scenarios;
auto sScenarioMgr=&scenarios;
namespace lfg {struct LFGDungeonData {struct Dbc{uint32 ScenarioID=777;}entry;Dbc*dbc=&entry;float x=1,y=2,z=3,o=4;uint32 difficulty=12,id=908,map=9000;};}
struct LfgManager {
    lfg::LFGDungeonData data;
    bool missing=false,shared=false;
    lfg::LFGDungeonData const* GetLFGDungeon(uint16 scenario,uint16 map){return missing||shared||scenario!=data.dbc->ScenarioID||map!=data.map?nullptr:&data;}
    lfg::LFGDungeonData const* GetLFGDungeon(uint32 map,Difficulty,uint32){return missing||!shared||map!=data.map?nullptr:&data;}
} lfgManager;
auto sLFGMgr=&lfgManager;
struct ScenarioData {uint32 ScenarioID=777,Team=1,Class=1,DifficultyID=12,LfgDungeonID=908,MapID=9000;};
struct ObjectManager {ScenarioData data;bool missing=false;ScenarioData const* GetScenarioOnMap(uint32,uint32,uint32,uint32,uint32){return missing?nullptr:&data;}} objects;
auto sObjectMgr=&objects;
namespace GameTime {uint64 GetStartTime(){return 1000;}}
struct MapManager {
    static bool IsValidMapCoord(uint32,float x,float y,float z,float o){return std::isfinite(x)&&std::isfinite(y)&&std::isfinite(z)&&std::isfinite(o)&&std::abs(x)<17000&&std::abs(y)<17000&&std::abs(z)<17000;}
};
struct Player {
    ScenarioResume m_scenarioResume;
    uint32 m_scenarioLoginId=0,m_scenarioLoginInstance=0,selector=0,instance=42,cls=1,team=1;bool pendingSheilun=false;
    Map map;Position pos;bool inWorld=true,transport=false,vehicle=false,relocated=false;
    struct Session {bool loading=false;bool PlayerLoading(){return loading;}} session;
    Session* GetSession(){return &session;}
    Map* GetMap()const{return const_cast<Map*>(&map);}uint32 GetMapId()const{return map.GetId();}
    uint32 GetGUIDLow(){return 1;}uint32 GetInstanceId(){return instance;}
    uint32 GetTeam()const{return team;}uint32 getClass()const{return cls;}int GetQuestStatus(int){return 0;}
    // The real owned-item/native-objective predicate is independently compiled
    // and exercised by Test-SheilunLogin; this host controls only its admission result.
    bool HasPendingSheilunReturn()const{return pendingSheilun;}
    bool IsQuestRewarded(int){return false;}uint32 GetScenarioId(){return selector;}
    bool IsInWorld(){return inWorld;}bool GetTransport(){return transport;}bool GetVehicle(){return vehicle;}
    Position GetPosition(){return pos;}void SetScenarioId(uint16 id){selector=id;}
    void Relocate(float x,float y,float z,float o){pos.Relocate(x,y,z,o);relocated=true;}
    void PrepareScenarioLogin(uint32,uint32);bool CanResumeScenarioLogin(uint32)const;void RecordScenarioEntrance();
};
#include "../../src/server/game/Scenarios/BrokenShoreClassRoute.h"
#include "Resume.inc"
bool allowed(Player* player,uint32 mapid){
#include "Access.inc"
    return true;
}
ScenarioResume saved(){ScenarioResume r;r.map=9000;r.scenario=777;r.instance=42;r.boot=1000;r.x=100;r.y=200;r.z=300;r.o=2;return r;}
void SharedWarriorEntrances(){
    struct Route {uint32 map,scenario,spell,foreign,foreignClass;float x,y,z,o;};
    unsigned cases=0;
    for(auto route:{Route{1511,944,193335,973,3,3349.74f,1355.64f,182.79f,0.715585f},Route{1539,1037,203920,1092,2,1966.66f,2494.39f,133.152f,0.147302f}})for(unsigned team:{0u,1u}){
        auto setup=[&](Player& p,Scenario& live){
            p=Player{};p.team=team;p.map.entry.ID=route.map;live={&p.map};live.id=route.scenario;live.step=3;
            scenarios.entries.clear();scenarios.entries[p.instance]=&live;scenarios.missingSteps=false;sMapStore.value.ID=route.map;sMapStore.missing=sScenarioStore.missing=false;sDB2Manager.missing=false;
            objects.missing=false;objects.data={};objects.data.ScenarioID=route.scenario;objects.data.Team=0;objects.data.Class=CLASS_WARRIOR;objects.data.LfgDungeonID=908;
            lfgManager.missing=false;lfgManager.shared=true;lfgManager.data={};lfgManager.data.dbc=&lfgManager.data.entry;lfgManager.data.entry.ScenarioID=route.foreign;lfgManager.data.map=route.map;lfgManager.data.x=lfgManager.data.y=lfgManager.data.z=lfgManager.data.o=0;
            spellManager.targets.clear();spellManager.targets[route.spell]={route.map,route.x,route.y,route.z,route.o};
        };
        for(unsigned reason=0;reason<4;++reason){Player p;Scenario live{};setup(p,live);live.step=reason?0:3;p.session.loading=reason==1;p.transport=reason==2;p.vehicle=reason==3;p.RecordScenarioEntrance();
#ifdef WARRIOR_RESUME_BASELINE
            assert(!p.m_scenarioResume.scenario);
#else
            assert(p.m_scenarioResume.scenario==route.scenario&&p.m_scenarioResume.x==route.x&&p.m_scenarioResume.y==route.y&&p.m_scenarioResume.z==route.z&&p.m_scenarioResume.o==route.o);
#endif
            assert(p.pos.x==10&&!p.relocated&&live.step==(reason?0:3));++cases;
        }
        {Player p;Scenario live{};setup(p,live);lfgManager.data.x=11;lfgManager.data.y=22;lfgManager.data.z=33;p.RecordScenarioEntrance();
#ifdef WARRIOR_RESUME_BASELINE
            assert(p.m_scenarioResume.x==11);
#else
            assert(p.m_scenarioResume.x==route.x);
#endif
            ++cases;
        }
        for(unsigned fault=0;fault<6;++fault){Player p;Scenario live{};setup(p,live);lfgManager.data.x=11;lfgManager.data.y=22;lfgManager.data.z=33;auto& target=spellManager.targets[route.spell];
            if(fault==0)spellManager.targets.clear();if(fault==1)target.target_mapId=1;if(fault==2)target.target_X=target.target_Y=target.target_Z=0;if(fault==3)target.target_X=std::numeric_limits<float>::quiet_NaN();if(fault==4)target.target_Orientation=std::numeric_limits<float>::infinity();if(fault==5)target.target_X=20000;
            p.RecordScenarioEntrance();
#ifdef WARRIOR_RESUME_BASELINE
            assert(p.m_scenarioResume.scenario==route.scenario&&p.m_scenarioResume.x==11);
#else
            assert(!p.m_scenarioResume.scenario);
#endif
            assert(!p.relocated&&p.pos.x==10);++cases;
        }
        {Player p;Scenario live{};setup(p,live);p.m_scenarioResume={route.map,route.scenario,7,999,123,456,78,2};spellManager.targets.clear();p.RecordScenarioEntrance();assert(p.m_scenarioResume.x==123&&p.m_scenarioResume.instance==42&&p.m_scenarioResume.boot==1000);++cases;}
        {Player p;Scenario live{};setup(p,live);live.step=0;p.RecordScenarioEntrance();assert(p.m_scenarioResume.x==p.pos.x&&p.m_scenarioResume.z==p.pos.z);++cases;}
        {Player p;Scenario live{};setup(p,live);p.cls=route.foreignClass;objects.data.Class=route.foreignClass;objects.data.ScenarioID=live.id=route.foreign;lfgManager.data.x=11;lfgManager.data.y=22;lfgManager.data.z=33;p.RecordScenarioEntrance();assert(p.m_scenarioResume.scenario==route.foreign&&p.m_scenarioResume.x==11);++cases;}
        for(unsigned other=0;other<2;++other){Player p;Scenario live{};setup(p,live);if(!other){objects.data.ScenarioID=live.id=route.scenario+1;}else{objects.data.MapID=p.map.entry.ID=lfgManager.data.map=route.map==1511?1539:1511;sMapStore.value.ID=p.map.entry.ID;}lfgManager.data.x=11;lfgManager.data.y=22;lfgManager.data.z=33;p.RecordScenarioEntrance();assert(p.m_scenarioResume.scenario==live.id&&p.m_scenarioResume.x==11);++cases;}
    }
    assert(cases==64);
#ifdef WARRIOR_RESUME_BASELINE
    std::cout<<"COUNTERFACTUAL pinned2396b47f7 source64:44 expected-broken Warrior late/source/default-row cases,20 retained controls;native class/terrain/client checks separate\n";
#else
    std::cout<<"PASS actual RecordScenarioEntrance source64:Warrior944/1037/both teams/late-loading-transport-vehicle,valid foreign LFG priority,24 invalid native-source refusals,existing checkpoint/real first-entry preservation,foreign class/map/scenario controls;no movement or progress grant. Engine data controlled;native separate\n";
#endif
}
int main()
{
    ScenarioResume r,original=saved();assert(r.Read(original.Write())&&r.Write()==original.Write());
    for(auto value:{"","2 9000 777 42 1000 1 2 3 4","1 9000 0 42 1000 1 2 3 4","1 9000 70000 42 1000 1 2 3 4","1 9000 777 42 1000 nan 2 3 4","1 9000 777 42 1000 1 2 3 4 junk","1 9000 777 42 1000 1 2 3"})assert(!r.Read(value)&&r.Write().empty());
    Player p;p.m_scenarioResume=saved();p.PrepareScenarioLogin(9000,42);
    assert(p.selector==777&&p.relocated&&p.pos.x==100&&!p.m_scenarioLoginInstance);
    // Native constructor consumes the selector before the final login check.
    p.selector=0;assert(p.CanResumeScenarioLogin(777)&&!p.CanResumeScenarioLogin(778));
    p=Player{};Scenario live{&p.map};scenarios.entries[42]=&live;p.m_scenarioResume=saved();
    p.PrepareScenarioLogin(9000,42);assert(p.m_scenarioLoginInstance==42&&!p.relocated&&p.pos.x==10);
    p.RecordScenarioEntrance();assert(p.m_scenarioResume.x==100&&!p.selector);
    assert(allowed(&p,9000));p.m_scenarioLoginId=778;assert(!allowed(&p,9000));p.m_scenarioLoginId=777;
    p.m_scenarioResume.boot=999;p.m_scenarioLoginInstance=0;p.PrepareScenarioLogin(9000,42);assert(p.relocated&&!p.m_scenarioLoginInstance);
    p=Player{};live.map=&p.map;live.completed=true;p.m_scenarioResume=saved();p.PrepareScenarioLogin(9000,42);
    assert(!p.m_scenarioLoginId&&!p.selector&&!p.m_scenarioResume.scenario);
    live.completed=false;live.id=778;p=Player{};live.map=&p.map;p.m_scenarioResume=saved();p.PrepareScenarioLogin(9000,42);assert(p.relocated&&!p.m_scenarioLoginInstance);
    live.id=777;p=Player{};Map other;other.entry.ID=8000;live.map=&other;p.m_scenarioResume=saved();p.PrepareScenarioLogin(9000,42);assert(p.relocated&&!p.m_scenarioLoginInstance);
    scenarios.entries.clear();
    for(int invalid=0;invalid<8;++invalid){
        p=Player{};p.m_scenarioResume=saved();
        if(invalid==0)p.m_scenarioResume.map=8000;
        if(invalid==1)p.m_scenarioResume.x=20000;
        if(invalid==2)sMapStore.value.scenario=false;
        if(invalid==3)sMapStore.value.pvp=true;
        if(invalid==4)sMapStore.value.garrison=true;
        if(invalid==5)sScenarioStore.value.Type=1;
        if(invalid==6)lfgManager.missing=true;
        if(invalid==7)scenarios.missingSteps=true;
        p.PrepareScenarioLogin(9000,42);assert(!p.selector&&!p.m_scenarioResume.scenario);
        sMapStore.value=MapEntry{};sScenarioStore.value={};lfgManager.missing=false;scenarios.missingSteps=false;
    }
    p=Player{};live={&p.map};scenarios.entries[42]=&live;p.RecordScenarioEntrance();
    assert(p.m_scenarioResume.x==10&&p.m_scenarioResume.scenario==777&&p.m_scenarioResume.boot==1000);
    p.pos.x=900;live.step=4;p.RecordScenarioEntrance();assert(p.m_scenarioResume.x==10);
    p.m_scenarioResume={};p.RecordScenarioEntrance();assert(p.m_scenarioResume.x==1);
    p.m_scenarioResume={};lfgManager.data.x=lfgManager.data.y=lfgManager.data.z=0;p.RecordScenarioEntrance();assert(!p.m_scenarioResume.scenario);
    p.m_scenarioResume=saved();p.map.entry.scenario=false;p.RecordScenarioEntrance();assert(!p.m_scenarioResume.scenario);
    // Shared native LFG metadata must still select this exact faction/class
    // scenario, both for fresh restart and the legacy later-stage entrance.
    scenarios.entries.clear();lfgManager.data={};lfgManager.data.dbc=&lfgManager.data.entry;lfgManager.shared=true;
    p=Player{};p.m_scenarioResume=saved();p.PrepareScenarioLogin(9000,42);assert(p.selector==777&&p.relocated&&p.pos.x==100);
    p=Player{};live={&p.map};live.step=4;scenarios.entries[42]=&live;p.RecordScenarioEntrance();assert(p.m_scenarioResume.scenario==777&&p.m_scenarioResume.x==1);
    p=Player{};live.map=&p.map;p.m_scenarioResume=saved();p.PrepareScenarioLogin(9000,42);assert(p.m_scenarioLoginInstance==42&&!p.relocated);
    for(int invalid=0;invalid<8;++invalid){
        p=Player{};p.m_scenarioResume=saved();
        if(invalid==0)sDB2Manager.missing=true;
        if(invalid==1)lfgManager.missing=true;
        if(invalid==2)objects.missing=true;
        if(invalid==3)objects.data.ScenarioID=778;
        if(invalid==4)objects.data.Team=2;
        if(invalid==5)objects.data.Class=2;
        if(invalid==6)objects.data.DifficultyID=13;
        if(invalid==7)objects.data.LfgDungeonID=909;
        p.PrepareScenarioLogin(9000,42);assert(!p.selector&&!p.m_scenarioResume.scenario);
        p.m_scenarioResume={};live.map=&p.map;p.RecordScenarioEntrance();assert(!p.m_scenarioResume.scenario);
        sDB2Manager.missing=false;lfgManager.missing=false;objects.missing=false;objects.data={};
    }
    // Zero fields are native wildcards; an exact legacy LFG route retains
    // precedence even when unrelated fallback data is unavailable.
    objects.data.Team=objects.data.Class=objects.data.DifficultyID=objects.data.LfgDungeonID=0;
    p=Player{};p.m_scenarioResume=saved();p.PrepareScenarioLogin(9000,42);assert(p.m_scenarioLoginInstance==42);
    lfgManager.shared=false;objects.missing=true;p=Player{};p.m_scenarioResume=saved();p.PrepareScenarioLogin(9000,42);assert(p.selector==777);
    // Real Stromkar reset receipt: saving only the entrance position leaves
    // no login authorization. Preserve the native checkpoint through map
    // construction, which consumes the transient selector before admission.
    scenarios.entries.clear();objects.missing=false;lfgManager.shared=false;
    lfgManager.data.entry.ScenarioID=1037;lfgManager.data.dbc=&lfgManager.data.entry;lfgManager.data.map=1539;
    sMapStore.value.ID=1539;
    p=Player{};p.map.entry.ID=1539;p.instance=7;
    live={&p.map};live.id=1037;scenarios.entries[7]=&live;
    p.PrepareScenarioLogin(1539,0);assert(!p.selector&&!allowed(&p,1539));
    assert(p.m_scenarioResume.Read("1 1539 1037 0 0 1966.66 2494.39 133.152 0.147302"));
    p.PrepareScenarioLogin(1539,0);
    assert(p.selector==1037&&p.m_scenarioLoginId==1037&&p.relocated&&!p.m_scenarioLoginInstance);
    assert(std::abs(p.pos.x-1966.66f)<0.001f&&std::abs(p.pos.z-133.152f)<0.001f);
    p.selector=0;assert(allowed(&p,1539)&&p.CanResumeScenarioLogin(1037)&&!p.CanResumeScenarioLogin(991));
    live.id=991;assert(!allowed(&p,1539));live.id=1037;
    live.completed=true;assert(!allowed(&p,1539));live.completed=false;
    p.RecordScenarioEntrance();assert(p.m_scenarioResume.scenario==1037&&p.m_scenarioResume.instance==7&&p.m_scenarioResume.boot==1000);
    // Completion of Sheilun's pickup precedes its distinct return objective.
    // Prepare runs before inventory loads; admission/Record then require the
    // real owned-pickup predicate. Other completed scenarios remain refused.
    lfgManager.data.entry.ScenarioID=1007;lfgManager.data.map=1541;sMapStore.value.ID=1541;
    for(bool earned:{false,true}){
        p=Player{};p.cls=CLASS_MONK;p.map.entry.ID=1541;p.pendingSheilun=earned;
        assert(p.m_scenarioResume.Read("1 1541 1007 42 1000 -1073 -3143 27 0"));
        live={&p.map};live.id=1007;live.completed=true;scenarios.entries[42]=&live;
        p.PrepareScenarioLogin(1541,42);assert(p.m_scenarioLoginInstance==42&&p.selector==1007&&!p.relocated);
        assert(allowed(&p,1541)==earned);p.RecordScenarioEntrance();
        assert(bool(p.m_scenarioResume.scenario)==earned);
        if(earned){assert(!p.selector&&p.CanResumeScenarioLogin(1007));p.pendingSheilun=false;p.RecordScenarioEntrance();assert(!p.m_scenarioResume.scenario&&!p.m_scenarioLoginInstance);}
    }
    p=Player{};p.cls=CLASS_MONK;p.map.entry.ID=1541;p.pendingSheilun=true;
    assert(p.m_scenarioResume.Read("1 1541 1007 42 999 -1073 -3143 27 0"));
    live.map=&p.map;p.PrepareScenarioLogin(1541,42);assert(p.relocated&&p.selector==1007&&!p.m_scenarioLoginInstance&&p.pos.x==-1073);
    std::cout<<"PASS: production checkpoint parsing, fresh restart, live rejoin, consumed selector, ID reuse/boot separation, completion, map/type/routing gates, shared faction/class/difficulty/dungeon admission and legacy entrance retention/exit cleanup; actual Stromkar1539/1037 reset rejects empty checkpoint and admits the exact entrance receipt after selector consumption\n";
    SharedWarriorEntrances();
}
