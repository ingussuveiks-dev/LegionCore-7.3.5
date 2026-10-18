#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <vector>
using uint32=uint32_t;using uint16=uint16_t;using uint8=uint8_t;using int16=int16_t;
#define TC_LOG_DEBUG(...) ((void)0)
static void Require(bool ok,char const* why){if(!ok)throw std::runtime_error(why);}
namespace GameTime {uint32 now=1000;uint32 GetGameTime(){return now;}}
#include "QuestInfo.inc"
#include "WorldQuestResetType.inc"
struct Quest {
 uint32 id,QuestSortID,QuestInfoID=135;
 uint32 GetQuestId()const{return id;}bool IsWorld()const{return true;}bool IsEmissary()const{return QuestInfoID==QUEST_INFO_EMISSARY_QUEST;}bool IsLegionInvasion()const{return false;}
};
struct WorldQuestTemplate {uint32 QuestInfoID=135,ZoneID=0;float Chance=100;};
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
struct QuestV2CliTaskEntry {uint32 WorldStateExpressionID=0;};
struct WorldStateExpressionEntry {void Eval(void*,std::set<WorldQuestState>*)const{throw std::runtime_error("Unexpected fixture expression");}};
template<class T>struct EmptyStore{T const* LookupEntry(uint32){return nullptr;}};
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
struct QuestDataStoreMgr {
 std::map<uint32,Quest> quests;
 std::map<uint32,std::vector<WorldQuestTemplate>> _worldQuestTemplate;
 std::map<uint32,std::map<uint32,std::vector<WorldQuestUpdate>>> _worldQuestUpdate;
 std::map<uint32,std::map<uint32,WorldQuest>> _worldQuest;
 uint32 WorldLegionInvasionZoneID=0,generated=0,saved=0,tasks=0;
 Quest const* GetQuestTemplate(uint32 id){return Trinity::Containers::MapGetValuePtr(quests,id);}
 WorldQuestTemplate const* GetWorldQuestTemplate(Quest const*);
 WorldQuestUpdate const* GetWorldQuestUpdate(uint32,uint32,uint32=0);
 WorldQuest const* GenerateNewWorldQuest(uint32,uint32=0);
 WorldQuest const* GenerateWorldQuest(WorldQuestUpdate const*,WorldQuestTemplate const*);
 bool CanBeActivate(WorldQuestTemplate const*,WorldQuestUpdate const*);
 void CalculateWorldQuestReward(WorldQuestTemplate const*,WorldQuest*){++generated;}
 void AddWorldQuestTask(Quest const*){++tasks;}void SaveWorldQuest(){++saved;}
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

int Run(int argc,char**argv){Require(argc==2,"Expected validated migration row");
 std::ifstream rows(argv[1]);WorldQuestUpdate restored;
 rows>>restored.QuestID>>restored.Timer>>restored.VariableID>>restored.VariableID1>>restored.Value>>restored.Value1>>restored.EventID;
 Require(rows&&restored.QuestID==44923&&restored.VariableID==12678&&restored.Value==3,"Bad migrated native state mapping");
 QuestDataStoreMgr lookup;lookup.quests[44923]={44923,7541};
 Require(!lookup.GenerateNewWorldQuest(44923),"Missing template must reject generation");
 lookup._worldQuestTemplate[135].push_back({135,7541});
 Require(!lookup.GenerateNewWorldQuest(44923)&&!lookup.generated,"Baseline missing candidate must reject generation");
 restored.quest=&lookup.quests[44923];restored.worldQuest=lookup.GetWorldQuestTemplate(restored.quest);
 lookup._worldQuestUpdate[135][44923].push_back(restored);
 auto wq=lookup.GenerateNewWorldQuest(44923);
 Require(wq&&lookup.generated==1&&lookup.tasks==1&&lookup.saved==1&&wq->State.count({12678,3})&&
  wq->ResetTime==GameTime::now+restored.Timer&&sWorldStateMgr.values[12678]==3,"Native restored generation did not publish Vrekt");
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
 QuestDataStoreMgr store;WorldQuestTemplate templ;Quest q{44923,7541};restored.quest=&q;restored.worldQuest=&templ;
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
 <<"one live Helarjar variant, exact-expiry release, restored native generation, unrelated/event/emissary boundaries.\n";
#endif
 return 0;
}
int main(int argc,char**argv){try{return Run(argc,argv);}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}
