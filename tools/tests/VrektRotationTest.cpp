#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
using uint32=uint32_t;using uint16=uint16_t;using uint8=uint8_t;using int16=int16_t;
#define TC_LOG_DEBUG(...) ((void)0)
static void Require(bool ok,char const* why){if(!ok)throw std::runtime_error(why);}
namespace GameTime {uint32 now=1000;uint32 GetGameTime(){return now;}}
#include "QuestInfo.inc"
#include "WorldQuestResetType.inc"
#include "QuestObjectiveType.inc"
struct QuestObjective { uint32 Type, ObjectID; };
struct Quest {
 uint32 id,QuestSortID,QuestInfoID=135;
 uint32 AreaGroupID=0; std::vector<QuestObjective> objectives;
 std::vector<QuestObjective> const& GetObjectives()const{return objectives;}
 uint32 GetQuestId()const{return id;}bool IsWorld()const{return true;}bool IsEmissary()const{return QuestInfoID==QUEST_INFO_EMISSARY_QUEST;}bool IsLegionInvasion()const{return false;}
 bool IsAllowedInRaid(uint32)const{return false;}
};
struct WorldQuestTemplate {uint32 QuestInfoID=135,ZoneID=0;float Chance=100;uint32 PrimaryID=0;};
using WorldQuestTemplateMap=std::map<uint32,std::vector<WorldQuestTemplate>>;
#include "WorldQuestState.inc"
struct WorldQuestUpdate {
 std::vector<uint32> AreaIDs;WorldQuestTemplate const* worldQuest=nullptr;Quest const* quest=nullptr;
 uint32 QuestID=0,Timer=0,VariableID=0,VariableID1=0,Value=0,Value1=0,EventID=0;
};
struct WorldQuest {
 Quest const* quest=nullptr;WorldQuestTemplate const* worldQuest=nullptr;
 std::set<WorldQuestState> State;
 uint32 QuestID=0,VariableID=0,Value=0,Timer=0,StartTime=0,ResetTime=0;
};
struct QuestV2CliTaskEntry {
 uint32 WorldStateExpressionID=0,ConditionID=0,FiltMinSkillID=0,FiltMinSkillValue=0;
 int FiltMinLevel=110,FiltMaxLevel=-1;
 int64_t FiltRaces=-1;uint32 FiltClasses=65535,FiltCompletedQuest[3]={};
};
struct WorldStateExpressionEntry {void Eval(void*,std::set<WorldQuestState>*)const{throw std::runtime_error("Unexpected fixture expression");}};
template<class T>struct EmptyStore{std::map<uint32,T> rows;T const* LookupEntry(uint32 id){auto it=rows.find(id);return it==rows.end()?nullptr:&it->second;}};
EmptyStore<QuestV2CliTaskEntry> sQuestV2CliTaskStore;EmptyStore<WorldStateExpressionEntry> sWorldStateExpressionStore;
struct WorldStateMgr {
 std::map<uint32,uint32> values;
 void SetWorldState(uint32 variable,uint32,uint32 value){values[variable]=value;}
}sWorldStateMgr;
namespace Trinity {namespace Containers {
 template<class T,class K>auto MapGetValuePtr(T& map,K key)->typename T::mapped_type*{auto it=map.find(key);return it==map.end()?nullptr:&it->second;}
 template<class T>auto SelectRandomContainerElement(T const& set)->typename T::value_type{return *set.begin();}
}}
bool roll_chance_f(float){return true;}
// Database and area-group services are fixture leaves. Rockaway's actual
// objective marker has no static spawn, so the native fallback returns no rows.
struct Field {uint16 GetUInt16()const{return 0;}};
struct Result {Field operator[](unsigned)const{return {};}bool NextRow(){return false;}};
using QueryResult=std::shared_ptr<Result>;
struct Database {uint32 queries=0;QueryResult PQuery(char const*,uint32,uint32){++queries;return {};}} WorldDatabase;
struct DB2Manager {std::vector<uint32> GetAreasForGroup(uint32){return {};}} sDB2Manager;
struct QuestDataStoreMgr {
 std::map<uint32,Quest> quests;
 std::map<uint32,std::vector<WorldQuestTemplate>> _worldQuestTemplate;
 std::map<uint32,std::map<uint32,std::vector<WorldQuestUpdate>>> _worldQuestUpdate;
 std::map<uint32,std::map<uint32,WorldQuest>> _worldQuest;
 std::map<uint32,std::set<Quest const*>> _worldQuestAreaTaskStore;
 uint32 WorldLegionInvasionZoneID=0,generated=0,saved=0;
 Quest const* GetQuestTemplate(uint32 id){return Trinity::Containers::MapGetValuePtr(quests,id);}
 WorldQuestTemplate const* GetWorldQuestTemplate(Quest const*);
 WorldQuestUpdate const* GetWorldQuestUpdate(uint32,uint32,uint32=0);
 WorldQuest const* GenerateNewWorldQuest(uint32,uint32=0);
 WorldQuest const* GenerateWorldQuest(WorldQuestUpdate const*,WorldQuestTemplate const*);
 WorldQuest const* GetWorldQuest(Quest const*);
 std::set<Quest const*> const* GetWorldQuestTask(uint32 area){return Trinity::Containers::MapGetValuePtr(_worldQuestAreaTaskStore,area);}
 bool CanBeActivate(WorldQuestTemplate const*,WorldQuestUpdate const*);
 void CalculateWorldQuestReward(WorldQuestTemplate const*,WorldQuest*){++generated;}
 void AddWorldQuestTask(Quest const*);void SaveWorldQuest(){++saved;}
 void Candidate(WorldQuestUpdate const* update){
  std::set<WorldQuestUpdate const*> _wQS={update};auto wqTemplate=update->worldQuest;int16 countQuest=1,countMaxQuest=14;
  uint32 ZoneID=update->quest->QuestSortID;
#include "CandidateLoop.inc"
 }
};
#include "TemplateLookup.inc"
#include "UpdateLookup.inc"
#include "GenerateNew.inc"
#include "Generate.inc"
#include "CanActivate.inc"
#include "AddWorldQuestTask.inc"
#include "GetWorldQuest.inc"

constexpr uint32 MAX_QUEST_LOG_SIZE=25,QUEST_REQUIRED=3,QUEST_STATUS_REWARDED=6;
struct Group {bool raid=false;bool isRaidGroup()const{return raid;}};
struct Map {uint32 GetDifficultyID()const{return 0;}};
struct Player;
struct ConditionMgr {bool IsPlayerMeetingCondition(Player*,uint32); } conditionMgr;
auto sConditionMgr=&conditionMgr;
QuestDataStoreMgr* sQuestDataStore=nullptr;
struct Player {
 std::set<uint32> rewarded;uint32 slot=0,adds=0,objectiveUpdates=0,sourceRemovals=0,level=110;uint64_t raceMask=1;
 bool completed=false,canAdd=true;Group* group=nullptr;Map map;
 uint32 FindQuestSlot(uint32 id)const{return slot==id?0:MAX_QUEST_LOG_SIZE;}
 bool WorldQuestCompleted(uint32)const{return completed;}
 Group* GetGroup(){return group;}Map* GetMap(){return &map;}
 uint32 GetSkillValue(uint32)const{return 0;}uint32 getLevel()const{return level;}
 uint64_t getRaceMask()const{return raceMask;}uint32 getClassMask()const{return 1;}
 uint32 GetQuestStatus(uint32 id)const{return rewarded.count(id)?QUEST_STATUS_REWARDED:0;}
 bool CanAddQuest(Quest const*,bool)const{return canAdd;}
 void AddQuest(Quest const* q,void*){++adds;slot=q->GetQuestId();}
 void UpdateQuestObjectiveData(Quest const*){++objectiveUpdates;}
 uint32 GetQuestSlotQuestId(uint8 index)const{return index?0:slot;}
 void SetQuestSlot(uint8,uint32 id){slot=id;}
 void TakeQuestSourceItem(uint32,bool){++sourceRemovals;}
 void Arrive(uint32 newAreaId,uint32 oldAreaId=0){
  std::vector<uint32> questAdd;
#include "PlayerArrival.inc"
#include "PlayerDeparture.inc"
 }
};
// Condition evaluation is a fixture leaf; its required IDs and AND logic are
// separately checked by the real typed PlayerCondition catalog reader.
bool ConditionMgr::IsPlayerMeetingCondition(Player* p,uint32 id){
 Require(id==47902||id==45693,"Unexpected arrival condition");
 return p->rewarded.count(41694)&&p->rewarded.count(44721);
}

int Run(int argc,char**argv){Require(argc==3,"Expected validated migration row and quest ID");
 uint32 questId=std::stoul(argv[2]);bool rockaway=questId==44746;
 Require(rockaway||questId==44923,"Unexpected quest variant");
 uint32 zone=rockaway?7503:7541,value=rockaway?2:3,area=rockaway?7833:8362,restoredArea=0;
 std::ifstream rows(argv[1]);WorldQuestUpdate restored;
 rows>>restored.QuestID>>restored.Timer>>restored.VariableID>>restored.VariableID1>>restored.Value>>restored.Value1>>restored.EventID>>restoredArea;
 Require(rows&&restored.QuestID==questId&&restored.VariableID==12678&&restored.Value==value&&restoredArea==area,"Bad migrated native state/area mapping");
 QuestDataStoreMgr lookup;lookup.quests[questId]={questId,zone};
 if(rockaway)lookup.quests[questId].objectives.push_back({QUEST_OBJECTIVE_MONSTER,115168});
 Require(!lookup.GenerateNewWorldQuest(questId),"Missing template must reject generation");
 lookup._worldQuestTemplate[135].push_back({135,zone});
 Require(!lookup.GenerateNewWorldQuest(questId)&&!lookup.generated,"Baseline missing candidate must reject generation");
 auto beforeQueries=WorldDatabase.queries;
 lookup.AddWorldQuestTask(&lookup.quests[questId]);
 Require(lookup._worldQuestAreaTaskStore.empty()&&WorldDatabase.queries==beforeQueries+(rockaway?1:0),"Missing marker fallback must not invent an arrival area");
 restored.quest=&lookup.quests[questId];restored.worldQuest=lookup.GetWorldQuestTemplate(restored.quest);
 restored.AreaIDs.push_back(restoredArea);
 lookup._worldQuestUpdate[135][questId].push_back(restored);
 beforeQueries=WorldDatabase.queries;
 auto wq=lookup.GenerateNewWorldQuest(questId);
 Require(wq&&lookup.generated==1&&lookup.saved==1&&wq->State.count({12678,static_cast<int16>(value)})&&
  wq->ResetTime==GameTime::now+restored.Timer&&sWorldStateMgr.values[12678]==value,"Native restored generation did not publish landing");
 Require(lookup._worldQuestAreaTaskStore.size()==1&&lookup._worldQuestAreaTaskStore[area]==std::set<Quest const*>{restored.quest}&&
  WorldDatabase.queries==beforeQueries,"Native generation did not register exact arrival area independently of marker spawns");
 sQuestDataStore=&lookup;sQuestV2CliTaskStore.rows[questId].ConditionID=rockaway?47902:45693;
 auto eligible=[](){Player p;p.rewarded={41694,44721};return p;};
 for(uint32 race:{1u,512u}){
  auto p=eligible();p.raceMask=race;p.Arrive(area+1);Require(!p.adds,"Wrong area started task");
  p.Arrive(area);Require(p.adds==1&&p.objectiveUpdates==1&&p.slot==questId,"Native arrival did not add unlocked task");
  p.Arrive(area);Require(p.adds==1&&p.objectiveUpdates==1,"Repeated arrival duplicated task");
  p.Arrive(area+1,area);Require(!p.slot&&p.sourceRemovals==1,"Leaving coast did not clear task slot");
  p.Arrive(area,area+1);Require(p.adds==2&&p.slot==questId,"Re-entry did not restore live task");
 }
 for(uint32 missing:{41694u,44721u}){auto p=eligible();p.rewarded.erase(missing);p.Arrive(area);Require(!p.adds,"Missing unlock admitted task");}
 {auto p=eligible();p.level=109;p.Arrive(area);Require(!p.adds,"Under-level task admitted");}
 {auto p=eligible();p.completed=true;p.Arrive(area);Require(!p.adds,"Completed task admitted");}
 {auto p=eligible();p.canAdd=false;p.Arrive(area);Require(!p.adds,"Native CanAddQuest refusal ignored");}
 {auto p=eligible();Group raid{true};p.group=&raid;p.Arrive(area);Require(!p.adds,"Disallowed raid admitted");}
 auto& live=lookup._worldQuest[zone][questId];auto reset=live.ResetTime;live.ResetTime=GameTime::now;
 {auto p=eligible();p.Arrive(area);Require(!p.adds,"Expired task admitted");}live.ResetTime=reset;
 lookup.WorldLegionInvasionZoneID=zone;
 {auto p=eligible();p.Arrive(area);Require(!p.adds,"Invasion-suppressed task admitted");}
 lookup._worldQuest.clear();lookup.WorldLegionInvasionZoneID=0;
 {auto p=eligible();p.Arrive(area);Require(!p.adds,"Inactive task admitted");}
 sQuestV2CliTaskStore.rows.clear();
 // Every cross-zone ordering exercises the actual automatic candidate loop,
 // admission and generation, with fixture selection/reward/world services.
 uint32 order[]={0,1,2};unsigned permutations=0;
 do{QuestDataStoreMgr store;WorldQuestTemplate templ;uint32 ids[]={44737,44746,44923},zones[]={7334,7503,7541};WorldQuestUpdate updates[3];
  for(unsigned i=0;i<3;++i){store.quests[ids[i]]={ids[i],zones[i]};updates[i]=restored;updates[i].QuestID=ids[i];updates[i].Value=i+1;updates[i].quest=&store.quests[ids[i]];updates[i].worldQuest=&templ;}
  for(auto index:order)store.Candidate(&updates[index]);
#ifdef OLD_ADMISSION
  Require(store.generated==3&&sWorldStateMgr.values[12678]==order[2]+1,"Pinned baseline no longer reproduces world-state overwrite");
#else
  Require(store.generated==1&&sWorldStateMgr.values[12678]==order[0]+1,"Another zone overwrote an active landing");
  auto& active=store._worldQuest[zones[order[0]]][ids[order[0]]];
  active.ResetTime=GameTime::now;store.Candidate(&updates[order[1]]);
  Require(store.generated==2&&sWorldStateMgr.values[12678]==order[1]+1,"Exact expiry did not release the next landing");
  updates[order[2]].VariableID=9999;updates[order[2]].Value=7;store.Candidate(&updates[order[2]]);
  Require(store.generated==3&&sWorldStateMgr.values[9999]==7,"Unrelated world-state group affected");
#endif
  ++permutations;
 }while(std::next_permutation(order,order+3));
#ifndef OLD_ADMISSION
 QuestDataStoreMgr store;WorldQuestTemplate templ;Quest q{44923,7541};restored.quest=&q;restored.worldQuest=&templ;restored.Value=3;
 Require(!store.CanBeActivate(nullptr,&restored),"Null template admission changed");
 restored.EventID=12;Require(!store.CanBeActivate(&templ,&restored),"Rejected event gate bypassed");restored.EventID=0;
 WorldQuest active;active.ResetTime=GameTime::now+100;active.State={{12678,3}};store._worldQuest[7541][44923]=active;
 Require(store.CanBeActivate(&templ,&restored),"Same variant incorrectly blocked");
 store._worldQuest[7541][44923].State={{12678,0},{999,1}};
 Require(store.CanBeActivate(&templ,&restored),"Zero/unrelated state blocks a landing");
 // Native State also contains variables harvested from DB2 expressions.
 store._worldQuest[7541][44923].State={{555,1},{12678,1}};
 Require(!store.CanBeActivate(&templ,&restored),"Secondary/harvested shared state ignored");
 store._worldQuest.clear();templ.QuestInfoID=QUEST_INFO_EMISSARY_QUEST;Quest emissary{1,0,QUEST_INFO_EMISSARY_QUEST};
 active.quest=&emissary;active.ResetTime=GameTime::now+WORLD_QUEST_2_DAY;active.State.clear();store._worldQuest[0][1]=active;
 Require(!store.CanBeActivate(&templ,&restored),"Existing emissary gate changed");
#endif
 std::cout<<"PASS: "<<permutations<<" native automatic candidate orders; "
#ifdef OLD_ADMISSION
 <<"pinned baseline reproduces all three active variants and overwritten visibility state.\n";
#else
 <<"one live Helarjar variant, exact-expiry release, native area registration/player arrival, unlock/level/raid/expiry/re-entry and unrelated/event/emissary boundaries.\n";
#endif
 return 0;
}
int main(int argc,char**argv){try{return Run(argc,argv);}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}
