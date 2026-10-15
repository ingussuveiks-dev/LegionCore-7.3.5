#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
using uint32 = uint32_t;
using uint16 = uint16_t;
constexpr int CLASS_WARLOCK=9, QUEST_STATUS_INCOMPLETE=3, ALLIANCE=0;
constexpr int PLAYER_LOGIN_QUERY_LOADQUESTSTATUS=0;
#define TC_LOG_INFO(...) ((void)0)
#define TC_LOG_DEBUG(...) ((void)0)
struct Player;
struct InstanceMapFixture;
namespace lfg {struct LFGDungeonData {int map=1630,id=0;};}
struct Group {lfg::LFGDungeonData const* m_dungeon=nullptr;};
struct Scenario { int id; int step=0; int GetScenarioId(){return id;} void* GetMap(){return nullptr;} bool IsCompleted(bool){return false;} };
struct Manager {
    std::map<int,Scenario*> scenarios;
    Scenario created{0};
    Scenario* GetScenario(uint32 id){auto i=scenarios.find(id);return i==scenarios.end()?nullptr:i->second;}
    bool HasScenarioStep(lfg::LFGDungeonData const*,Player*){return true;}
    void AddScenario(InstanceMapFixture*,lfg::LFGDungeonData const*,Player*,bool=false);
} manager;
auto sScenarioMgr=&manager;
struct SpellTargetPosition {uint32 target_mapId=1630;float target_X=-854.89f,target_Y=1188.86f,target_Z=114.29f,target_Orientation=3.1683f;};
struct SpellManager {
    SpellTargetPosition destination; bool missing=false;
    SpellTargetPosition const* GetSpellTargetPosition(int id){assert(id==218595);return missing?nullptr:&destination;}
} spellManager;
auto sSpellMgr=&spellManager;
struct ScenarioData {int ScenarioID;};
struct ObjectManager {
    ScenarioData other{999};
    bool HasScenarioInMap(int){return false;}
    ScenarioData const* GetScenarioOnMap(int,int,int,int,int){return &other;}
} objectManager;
auto sObjectMgr=&objectManager;
struct LFGManager {
    lfg::LFGDungeonData data;
    lfg::LFGDungeonData const* GetLFGDungeon(int selector,uint16 map){assert(selector==1155 && map==1630);return &data;}
    lfg::LFGDungeonData const* GetLFGDungeon(int map,int,int){assert(map==1630);return &data;}
} lfgManager;
auto sLFGMgr=&lfgManager;
struct Player {
    int m_scenarioLoginId=0;
    void* GetMap(){return nullptr;} bool CanResumeScenarioLogin(int){return false;}
    int map=1630,cls=9,status=0,selector=0,instance=17,loads=0;
    bool rewarded=false,relocated=false; float x=-400,y=-4500,z=170,o=0;
    struct Holder {int status=3;int GetPreparedResult(int){return status;}} holder;
    int getClass(){return cls;} int GetTeam(){return 1;}
    int GetQuestStatus(int id){return id==43153?status:0;}
    bool IsQuestRewarded(int id){assert(id==43153);return rewarded;}
    int GetScenarioId(){return selector;} void SetScenarioId(int id){selector=id;}
    uint32 GetMapId(){return map;} int GetInstanceId(){return instance;}
    void _LoadQuestStatus(int saved){status=saved;++loads;}
    void Relocate(float nx,float ny,float nz,float no){x=nx;y=ny;z=nz;o=no;relocated=true;}
    bool early(){uint32 mapId=map,instanceId=instance;
#include "EarlyRestore.inc"
        return scenarioQuestStatusLoaded;
    }
    void normalLoad(bool scenarioQuestStatusLoaded){
#include "NormalQuestLoad.inc"
    }
    void restore(){
#include "ScepterRestore.inc"
    }
};
bool allowed(Player* player,uint32 mapid){
#include "Access.inc"
    return true;
}
// Execute the real constructor's selection/consumption branch before access.
struct ConstructScenario {
    struct Map {int GetDifficultyID(){return 12;}} mapValue;
    lfg::LFGDungeonData dungeonValue;
    int scenarioId=42;
    explicit ConstructScenario(Player* player,bool find=false){auto map=&mapValue;auto dungeonData=&dungeonValue;
#include "ConstructorSelector.inc"
    }
};
struct InstanceMapFixture {
    int instance;
    int GetId(){return 1630;} int GetDifficultyID(){return 12;}
    int GetInstanceId(){return instance;} bool IsScenario(){return true;}
    void add(Player* player){Group* group=nullptr;
#include "AddScenario.inc"
    }
};
void Manager::AddScenario(InstanceMapFixture* map,lfg::LFGDungeonData const*,Player* player,bool find){
    ConstructScenario construction(player,find);created={construction.scenarioId};scenarios[map->GetInstanceId()]=&created;
}
void fresh(int savedInstance,int actualInstance){
    manager.scenarios.clear();Player p;p.instance=savedInstance;
    assert(p.early() && p.loads==1 && p.selector==1155);
    // CreateMap can replace an expired saved bind with a different instance.
    p.instance=actualInstance;p.normalLoad(true);p.restore();
    assert(p.loads==1 && p.relocated && p.x==-854.89f && p.y==1188.86f && p.z==114.29f && p.o==3.1683f);
    InstanceMapFixture map{actualInstance};map.add(&p);
    assert(manager.GetScenario(actualInstance)->id==1155 && p.selector==0);
    assert(allowed(&p,1630));
    manager.scenarios.clear();
}
int main(){
    fresh(17,17);fresh(17,29);fresh(0,29);
    Scenario live{1155,7};manager.scenarios[17]=&live;Player p;
    assert(p.early());p.normalLoad(true);p.restore();
    assert(!p.relocated && p.x==-400 && live.step==7 && p.loads==1);
    InstanceMapFixture map{17};map.add(&p);
    assert(manager.GetScenario(17)==&live && live.step==7 && !p.relocated);
    p.selector=0;assert(allowed(&p,1630)); // live constructor already consumed it
    live.id=999;p=Player{};p.early();p.restore();
    assert(!p.selector && !p.relocated && !allowed(&p,1630));
    p.selector=1155;assert(!allowed(&p,1630)); // selector cannot override wrong live scenario
    manager.scenarios.clear();
    for(int status:{0,1,6}){p=Player{};p.holder.status=status;p.early();p.normalLoad(true);p.restore();assert(p.loads==1 && !p.selector && !p.relocated && !allowed(&p,1630));}
    p=Player{};p.holder.status=3;p.early();p.rewarded=true;p.restore();assert(!p.relocated && !allowed(&p,1630));
    p=Player{};p.cls=1;assert(!p.early());p.normalLoad(false);p.restore();assert(p.loads==1 && !p.selector && !p.relocated && !allowed(&p,1630));
    for(int mapId:{0,1,1522,1460}){p=Player{};p.map=mapId;bool loaded=p.early();p.normalLoad(loaded);p.restore();assert(p.loads==1 && !p.relocated && !p.selector && !allowed(&p,1630));}
    // The access function receives the requested map; the caller supplies the
    // player's actual map. A selector alone cannot admit an unrelated map.
    p=Player{};p.status=3;p.selector=1155;assert(!allowed(&p,1631));
    spellManager.missing=true;p.restore();assert(!p.relocated);
    spellManager.missing=false;spellManager.destination.target_mapId=1;p.restore();assert(!p.relocated);
    p=Player{};p.m_scenarioLoginId=998;assert(p.early()&&!p.selector);p.restore();assert(!p.relocated);
    std::cout<<"PASS: production Scepter login restores selector before map creation; constructor consumes it before access; fresh entrance, live stage/position, one quest load, map/class/status/wrong-scenario and destination gates\n";
}
