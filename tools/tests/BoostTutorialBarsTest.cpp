#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <functional>
#include <map>
#include <set>
#include <vector>
#include <iostream>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;
#define TC_LOG_INFO(...) ((void)0)
enum{MAX_ACTION_BUTTONS=132,ACTIONBUTTON_DELETED=3,CRITERIA_TYPE_CAST_SPELL=29};
struct Entry{uint32 Type,Asset;};struct Criterion{struct Entry* Entry;};
struct CriteriaTree{Criterion* Criteria=nullptr;std::vector<CriteriaTree*> Children;};
struct Achievements{std::map<uint32,CriteriaTree*> trees;CriteriaTree* GetCriteriaTree(uint32 id){return trees[id];}} achievements,*sAchievementMgr=&achievements;
struct Scenario{uint32 step=0;uint32 GetCurrentStep(){return step;}uint8 GetStepCount(bool){return 5;}uint32 GetScenarioCriteriaByStep(uint8 id){return id;}} scenario;
struct Scenarios{bool exists=true;Scenario* GetScenario(uint32){return exists?&scenario:nullptr;}} scenarios,*sScenarioMgr=&scenarios;
namespace WorldPackets{namespace Spells{struct UpdateActionButtons{uint64 ActionButtons[MAX_ACTION_BUTTONS]={};uint32 Reason=0;UpdateActionButtons* Write(){return this;}};}}
struct Action{uint64 packedData;unsigned uState=0;};using ActionButtonList=std::map<uint8,Action>;
struct Player{
 uint32 map=1557;bool inWorld=true,loading=false,hold=true;void* transport=nullptr;
 ActionButtonList m_actionButtons;std::set<uint32> known{100,1464};std::array<uint64,MAX_ACTION_BUTTONS> visible{};unsigned sends=0;
 uint32 GetMapId(){return map;}uint32 GetInstanceId(){return 1;}uint32 GetGUIDLow(){return 0;}unsigned GetActiveTalentGroup(){return 0;}
 bool HasActiveSpell(uint32 id){return known.count(id);}
 bool IsInWorld(){return inWorld;}bool isBeingLoaded(){return loading;}bool IsMapObjectArrivalProtected(){return hold;}void* GetTransport(){return transport;}
 void SendDirectMessage(WorldPackets::Spells::UpdateActionButtons* p){std::copy(std::begin(p->ActionButtons),std::end(p->ActionButtons),visible.begin());++sends;}
 void SendActionButtons(uint32);
};
#include "Send.inc"
struct Instance{uint32 GetInstanceId(){return 1;}unsigned GetId(){return 1557;}} instance;
struct Controller{
 bool _arrivalBarsPending=true,_setupComplete=false;Player* player=nullptr;void* ship=nullptr;Instance* instance=::std::addressof(::instance);
 Player* GetPlayer(){return player;}void* GetGunship(){return ship;}
#include "Refresh.inc"
};
int main(){
 Entry first{CRITERIA_TYPE_CAST_SPELL,100},again{CRITERIA_TYPE_CAST_SPELL,100},next{CRITERIA_TYPE_CAST_SPELL,1464},unlearned{CRITERIA_TYPE_CAST_SPELL,999};
 Criterion a{&first},b{&again},c{&next},d{&unlearned};CriteriaTree one{&a},two{&b},three{&c},four{&d},parent{nullptr,{&two,&three,&four}};
 achievements.trees={{1,&one},{2,&parent}};
 for(auto map:{1554u,1557u}){
  Player p;p.map=map;int ship;p.transport=&ship;Controller controller;controller.player=&p;controller.ship=&ship;
  p.m_actionButtons={{0,{12294}},{1,{1464}},{60,{0x4000000100000001ULL}},{73,{34428}},{131,{100}}};auto saved=p.m_actionButtons;
  scenario.step=0;p.SendActionButtons(0);assert(std::all_of(p.visible.begin(),p.visible.end(),[](uint64 action){return !action;}));
  p.visible[0]=12294;controller.RefreshTutorialActionBar();assert(p.visible[0]==12294); // Setup not complete.
  controller._setupComplete=true;controller.RefreshTutorialActionBar();assert(p.visible[0]==12294); // Wait for native arrival hold.
  p.hold=false;p.loading=true;controller.RefreshTutorialActionBar();assert(p.visible[0]==12294);
  p.loading=false;scenarios.exists=false;controller.RefreshTutorialActionBar();assert(p.visible[0]==12294);scenarios.exists=true;
  controller.RefreshTutorialActionBar();assert(!controller._arrivalBarsPending&&p.visible[0]==0);
  auto sent=p.sends;controller.RefreshTutorialActionBar();assert(p.sends==sent); // No repeated rewrite.
  scenario.step=1;p.SendActionButtons(1);assert(p.visible[0]==100&&p.visible[72]==100&&p.visible[84]==100&&p.visible[1]==0);
  scenario.step=2;p.SendActionButtons(1);assert(p.visible[0]==100&&p.visible[1]==1464&&p.visible[2]==0&&p.visible[73]==1464);
  controller._arrivalBarsPending=true;controller.RefreshTutorialActionBar();assert(p.visible[1]==1464); // Re-entry keeps current lessons.
  for(auto const& row:saved)assert(p.m_actionButtons.at(row.first).packedData==row.second.packedData); // Packet overlay never edits saves/macros/forms.
  p.map=1460;p.SendActionButtons(1);assert(p.visible[0]==12294&&p.visible[60]==saved.at(60).packedData&&p.visible[73]==34428);
 }
 std::cout<<"PASS full production packet overlay and late arrival refresh: first-load client-overwrite counterfactual, two ship maps, intro empty/current learned lessons, forms, duplicates/unlearned filtering, missing scenario/retry and once-only refresh; ordinary island layout/macros remain untouched. Actual client scheduling is a controlled boundary.\n";
}
