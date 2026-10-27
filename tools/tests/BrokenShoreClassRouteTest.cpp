#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <vector>
using uint32=uint32_t;using uint16=uint16_t;
enum Difficulty{NORMAL=1,SCENARIO=12};
static uint32 errors=0,warnings=0;
#define TC_LOG_DEBUG(...) ((void)0)
#define TC_LOG_ERROR(...) (++errors)
#define TC_LOG_WARN(...) (++warnings)
struct Player{uint32 cls=1,team=67,selector=0;uint32 getClass()const{return cls;}uint32 GetTeam()const{return team;}uint16 GetScenarioId(){return uint16(selector);}void SetScenarioId(uint32 s){selector=s;}};
namespace lfg{struct LFGDungeonData{struct Dbc{uint32 ScenarioID=0;}native;Dbc*dbc=&native;uint32 id=0,map=1500;Difficulty difficulty=SCENARIO;};}
struct LfgManager{
 lfg::LFGDungeonData entries[3];bool missing=false;
 lfg::LFGDungeonData const*GetLFGDungeon(uint32 id,uint32){for(auto& d:entries)if(!missing&&d.id==id)return &d;return nullptr;}
 lfg::LFGDungeonData const*GetLFGDungeon(uint16 scenario,uint16 map){for(auto& d:entries)if(!missing&&d.map==map&&d.native.ScenarioID==scenario)return &d;return nullptr;}
 lfg::LFGDungeonData const*GetLFGDungeon(uint32 map,Difficulty diff,uint32){for(auto& d:entries)if(!missing&&d.map==map&&d.difficulty==diff)return &d;return nullptr;}
}lfgManager;auto*sLFGMgr=&lfgManager;
struct ScenarioData{uint32 ScenarioID,MapID,DifficultyID,Team,Class,LfgDungeonID;};
struct ObjectMgr{std::map<uint32,std::vector<ScenarioData*>>_scenarioDataList;ScenarioData const*GetScenarioOnMap(uint32,uint32,uint32,uint32,uint32)const;bool HasScenarioInMap(uint32){return false;}}objects;auto*sObjectMgr=&objects;
#include "Selector.inc"
#include "../../src/server/game/Scenarios/BrokenShoreClassRoute.h"
struct DifficultyEntry{uint32 DifficultyID=12;};struct DB2{DifficultyEntry value;DifficultyEntry*GetDefaultMapDifficulty(uint32){return &value;}}sDB2Manager;
#include "ResumeDungeon.inc"
struct Group{lfg::LFGDungeonData const*m_dungeon=nullptr;};
struct Scenario{uint32 scenarioId=0,dungeon=0;uint32 GetScenarioId(){return scenarioId;}};
struct Fixture;
struct ScenarioManager{Scenario result;bool existing=false;Scenario*GetScenario(uint32){return existing?&result:nullptr;}bool HasScenarioStep(lfg::LFGDungeonData const*,Player*){return true;}Scenario*AddScenario(Fixture*,lfg::LFGDungeonData const*,Player*,bool=false);}manager;auto*sScenarioMgr=&manager;
struct Fixture{uint32 GetId(){return 1500;}uint32 GetInstanceId(){return 42;}Difficulty GetDifficultyID(){return SCENARIO;}bool IsScenario(){return true;}bool Add(Player* player,Group*group=nullptr){
#include "Admission.inc"
 return true;}bool Baseline(Player*player,Group*group=nullptr){
#include "Baseline.inc"
 return true;}};
Scenario*ScenarioManager::AddScenario(Fixture*map,lfg::LFGDungeonData const*dungeonData,Player*player,bool find){uint32 scenarioId=dungeonData->dbc->ScenarioID;
#include "Constructor.inc"
 result={scenarioId,dungeonData->id};return &result;}
ScenarioData routes[3];
void Reset(bool original=false){
 errors=warnings=0;manager.existing=false;lfgManager.missing=false;objects._scenarioDataList.clear();
 uint32 scenario[3]={775,933,961},cls[3]={2,1,12},dungeon[3]={906,1176,1166};
 for(int i=0;i<3;++i){routes[i]={scenario[i],1500,original?0u:12u,0,original?0u:cls[i],original?0u:dungeon[i]};objects._scenarioDataList[1500].push_back(&routes[i]);auto&d=lfgManager.entries[i];d.id=dungeon[i];d.map=1500;d.difficulty=SCENARIO;d.dbc=&d.native;d.native.ScenarioID=i==1?0:scenario[i];}
}
int main(){
 Reset(true);Player player;Fixture map;assert(map.Baseline(&player)&&manager.result.scenarioId==961&&manager.result.dungeon==906);
 std::cout<<"COUNTERFACTUAL original unqualified native selector/map admission gives Warrior DH961 via arbitrary first LFG906.\n";
 uint32 checked=0;int order[3]={0,1,2};
 do{for(auto cls:{1u,2u,12u})for(auto team:{67u,469u})for(auto stale:{0u,775u,933u,961u})for(bool grouped:{false,true}){
  Reset();objects._scenarioDataList[1500]={&routes[order[0]],&routes[order[1]],&routes[order[2]]};player={cls,team,stale};Group wrong{&lfgManager.entries[0]};
  auto expected=cls==1?933u:cls==2?775u:961u;auto dungeon=cls==1?1176u:cls==2?906u:1166u;
  assert(map.Add(&player,grouped?&wrong:nullptr)&&manager.result.scenarioId==expected&&manager.result.dungeon==dungeon&&!player.selector&&!errors);
  assert(ResumeDungeon(&player,expected,1500)->id==dungeon);for(auto foreign:{775u,933u,961u})if(foreign!=expected)assert(!ResumeDungeon(&player,foreign,1500));++checked;
 }}while(std::next_permutation(order,order+3));
 for(int bad=0;bad<12;++bad){Reset();player={1,67,0};auto&r=routes[1];auto&d=lfgManager.entries[1];
  if(bad==0)r.Class=0;if(bad==1)r.ScenarioID=775;if(bad==2)r.MapID=1;if(bad==3)r.Team=469;if(bad==4)r.DifficultyID=1;if(bad==5)r.LfgDungeonID=906;
  if(bad==6)d.map=1;if(bad==7)d.difficulty=NORMAL;if(bad==8)d.native.ScenarioID=961;if(bad==9)d.dbc=nullptr;if(bad==10)lfgManager.missing=true;if(bad==11)objects._scenarioDataList.clear();
  assert(!map.Add(&player)&&errors==1&&!BrokenShoreClassRoute::Dungeon(&player,SCENARIO));
 }
 Reset();assert(!BrokenShoreClassRoute::Dungeon(nullptr,SCENARIO));player.cls=8;assert(!BrokenShoreClassRoute::Applies(1500,8)&&!BrokenShoreClassRoute::Dungeon(&player,SCENARIO));assert(!BrokenShoreClassRoute::Applies(1630,1));
 for(auto cls:{1u,2u,12u})for(auto live:{775u,933u,961u}){Reset();player={cls,67,0};manager.existing=true;manager.result.scenarioId=live;bool matches=live==BrokenShoreClassRoute::ScenarioId(cls);assert(map.Add(&player)==matches&&errors==(matches?0u:1u));}
 std::cout<<"PASS "<<checked<<" native selector/admission/constructor/shared-route resume cases: all row orders, both factions, three classes, foreign saved selectors, wrong group dungeon;12 metadata/missing-data refusals. No character progress or encounter completion simulated.\n";
}
