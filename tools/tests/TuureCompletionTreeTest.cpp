#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;
#include "CriteriaTypes.inc"
#include "TreeOperators.inc"
struct CriteriaTreeEntry{uint32 ID=0,Parent=0,Amount=0,CriteriaID=0,Flags=0,Operator=0;};
struct CriteriaEntry{uint32 Type=92;};struct Criteria{CriteriaEntry const* Entry=nullptr;};
struct CriteriaTree{std::vector<CriteriaTree const*> Children;CriteriaTreeEntry const* Entry=nullptr;struct Criteria const* Criteria=nullptr;uint32 ID=0;};
struct CriteriaProgress{uint64 Counter=0;bool completed=false;};
struct AchievementGlobalMgr{std::map<uint32,CriteriaTree> trees;CriteriaTree const* GetCriteriaTree(uint32 id){auto i=trees.find(id);return i==trees.end()?nullptr:&i->second;}bool IsGroupCriteriaType(CriteriaTypes)const;template<typename Func>static void WalkCriteriaTree(CriteriaTree const*,Func const&);};
AchievementGlobalMgr registry;auto sAchievementMgr=&registry;
template<class Owner>struct AchievementMgr{std::map<uint32,CriteriaProgress> storedProgress;CriteriaProgress* GetCriteriaProgress(uint32 id){auto i=storedProgress.find(id);return i==storedProgress.end()?nullptr:&i->second;}
 bool IsCompletedCriteria(CriteriaTree const*,uint64);bool IsCompletedCriteriaTree(CriteriaTree const*);bool IsCompletedScenarioTree(CriteriaTreeEntry const*);};
#include "TreeCompletion.inc"
struct ScenarioStepEntry{uint32 ID=0,Criteriatreeid=0,Flags=0;uint8 OrderIndex=0;bool IsBonusObjective()const{return Flags&1;}};
struct TreeStore{std::map<uint32,CriteriaTreeEntry> entries;CriteriaTreeEntry const* LookupEntry(uint32 id){auto i=entries.find(id);return i==entries.end()?nullptr:&i->second;}} sCriteriaTreeStore;
struct InstanceScript{void setScenarioStep(uint8){}void onScenarionNextStep(uint8){}void UpdatePhasing(){}};
struct InstanceMap{InstanceScript* GetInstanceScript(){return nullptr;}void UpdatePhasing(){}};
struct Map{InstanceMap* ToInstanceMap(){return nullptr;}};
struct OutdoorPvP{void SetData(uint32,uint32){}};
enum{SCENARIO_STEP_IN_PROGRESS,SCENARIO_STEP_DONE};using ScenarioSteps=std::vector<ScenarioStepEntry const*>;
struct Scenario{uint8 currentStep=6;uint32 currentTree=47678,rewards=0,updates=0;bool hasbonus=false;ScenarioSteps steps;std::vector<uint32> ActiveSteps;AchievementMgr<int> achievements;
 AchievementMgr<int>& GetAchievementMgr(){return achievements;}OutdoorPvP* GetOutdoorPvP(){return nullptr;}uint32 GetOutdoorPvPZone(){return 0;}Map* GetMap(){return nullptr;}
 void SendStepUpdate(){++updates;}void SetStepState(ScenarioStepEntry const*,uint32){}void Reward(bool bonus,uint32 old){assert(!bonus&&old==6);++rewards;}
 void UpdateCurrentStep(bool);uint32 GetScenarioCriteriaByStep(uint8);bool IsCompleted(bool)const;uint8 GetStepCount(bool)const;};
#include "ScenarioCompletion.inc"
int main(int argc,char** argv){
 assert(argc==2);std::ifstream input(argv[1]);assert(input);std::map<uint32,ScenarioStepEntry> steps;std::string line;
 while(std::getline(input,line)){std::istringstream fields(line);std::string kind;uint32 step=0;fields>>kind>>step;
  if(kind=="TREE"){CriteriaTreeEntry e;fields>>e.ID>>e.Parent>>e.Amount>>e.Operator>>e.Flags>>e.CriteriaID;assert(fields);sCriteriaTreeStore.entries[e.ID]=e;}
  if(kind=="STEP"){auto& e=steps[step];e.OrderIndex=uint8(step);fields>>e.ID>>e.Criteriatreeid;assert(fields);}
  if(kind=="STEP_META"){uint32 reward;fields>>steps[step].Flags>>reward;assert(fields);}
 }
 assert(!registry.IsGroupCriteriaType(CRITERIA_TYPE_SCRIPT_EVENT_2));
 assert(steps.size()==7&&sCriteriaTreeStore.entries.at(47678).Operator==CRITERIA_TREE_OPERATOR_ALL);
 CriteriaEntry entry;Criteria criterion{&entry};
 for(auto const& pair:sCriteriaTreeStore.entries){auto& t=registry.trees[pair.first];t.ID=pair.first;t.Entry=&pair.second;if(pair.second.CriteriaID)t.Criteria=&criterion;}
 for(auto const& pair:sCriteriaTreeStore.entries)if(pair.second.Parent)registry.trees.at(pair.second.Parent).Children.push_back(&registry.trees.at(pair.first));
 for(int acquired=-1;acquired<=1;++acquired)for(int departed=-1;departed<=1;++departed){Scenario scenario;for(auto const& pair:steps)scenario.steps.push_back(&pair.second);assert(scenario.GetStepCount(false)==7);
  if(acquired>=0)scenario.achievements.storedProgress[47679].Counter=acquired;if(departed>=0)scenario.achievements.storedProgress[47680].Counter=departed;
  bool complete=acquired==1&&departed==1;assert(scenario.achievements.IsCompletedScenarioTree(sCriteriaTreeStore.LookupEntry(47678))==complete);
  scenario.UpdateCurrentStep(false);assert(scenario.currentStep==(complete?7:6)&&scenario.rewards==(complete?1u:0u));scenario.UpdateCurrentStep(false);assert(scenario.rewards==(complete?1u:0u));
 }
 std::cout<<"PASS actual native IsCompletedCriteria zero->one normalization, ALL tree/ScenarioTree, UpdateCurrentStep/GetStepCount and reward dispatch over fresh26972 Tuure metadata: absent/zero/one50679 x50702 matrix requires both, then one native reward without explicit COMPLETE_SCENARIO. Reward delivery/storage/packets are adapters, not actual character or client verification.\n";
}
