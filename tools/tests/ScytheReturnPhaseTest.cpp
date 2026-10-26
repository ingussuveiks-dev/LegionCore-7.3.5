#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <mutex>
#include <set>
#include <unordered_map>
#include <vector>
using uint8=uint8_t; using uint16=uint16_t; using uint32=uint32_t; using uint64=uint64_t;
using ConditionTypes=uint32; using QuestStatus=uint32;
enum { CONDITION_QUESTREWARDED=8, CONDITION_QUESTTAKEN=9, CONDITION_QUEST_NONE=14,
 CONDITION_QUEST_COMPLETE=28, CONDITION_QUEST_OBJECTIVE_DONE=41, CONDITION_SCENARION_STEP=44,
 QUEST_STATUS_INCOMPLETE=3, PHASEMASK_NORMAL=1, PHASEMASK_ANYWHERE=0xffffffff };
#define TC_LOG_DEBUG(...) ((void)0)
namespace sf { template<class... T> using contention_free_shared_mutex=std::mutex; }
struct Player;
struct Unit {
 virtual Player* ToPlayer(){return nullptr;}
 bool IsCreature(){return false;}
 Unit* ToUnit(){return this;}
 struct Threat {Unit* getHostilTarget(){return nullptr;}} threat;
 Threat& getThreatManager(){return threat;}
 bool isPet(){return false;} Unit* GetOwner(){return nullptr;}
};
struct Condition;
struct ConditionSourceInfo {Unit* mConditionTargets[1]; explicit ConditionSourceInfo(Unit* u):mConditionTargets{u}{};};
struct Condition {
 uint32 ElseGroup=0,ConditionType=0,ConditionTarget=0,ConditionValue1=0,ConditionValue2=0,ConditionValue3=0,ReferenceId=0;
 bool NegativeCondition=false;
 bool isLoaded()const{return true;}
 bool Meets(ConditionSourceInfo&);
};
using ConditionList=std::vector<Condition*>;
using ConditionReferenceContainer=std::map<uint32,ConditionList>;
using ConditionTypeContainer=std::map<uint32,ConditionList>;
using PhaseDefinitionConditionContainer=std::map<uint32,ConditionTypeContainer>;
struct ConditionMgr {
 ConditionReferenceContainer ConditionReferenceStore;
 PhaseDefinitionConditionContainer PhaseDefinitionsConditionStore;
 ConditionList GetConditionsForPhaseDefinition(uint32,uint32);
 bool IsObjectMeetToConditionList(ConditionSourceInfo&,ConditionList const&)const;
 bool IsObjectMeetToConditions(Unit* u,ConditionList const& rows){ConditionSourceInfo info(u);return rows.empty()||IsObjectMeetToConditionList(info,rows);}
} conditions;
auto* sConditionMgr=&conditions;
class AuraEffect;
#include "PhaseHeader.inc"
struct Player:Unit {
 uint32 status=0,zone=7334,mask=1; std::map<uint32,uint32> objectives; std::set<uint32> phases;
 bool NeedPhaseRecalculate=false,NeedPhaseUpdate=false; unsigned visibilityUpdates=0,packets=0;
 Player* ToPlayer()override{return this;}
 uint32 GetQuestStatus(uint32 id){assert(id==37660);return status;}
 uint32 GetQuestObjectiveData(uint32 quest,uint32 obj){assert(quest==37660);return objectives[obj];}
 uint32 GetCurrentZoneID(){return zone;}
 std::set<uint32> const& GetPhases()const{return phases;}
 void SetPhaseId(std::set<uint32> const& ids,bool){phases=ids;}
 bool isGameMaster()const{return false;}
 void SetPhaseMask(uint32 m,bool){mask=m;}
 uint32 GetPhaseMask(){return mask;}
 bool IsVisible(){return true;}
 void UpdateObjectVisibility(){++visibilityUpdates;}
};
bool Condition::Meets(ConditionSourceInfo& info){
 Unit* object=info.mConditionTargets[ConditionTarget];bool condMeets=false;
 switch(ConditionType){
#include "Conditions.inc"
 default:assert(false);
 }
 return NegativeCondition?!condMeets:condMeets;
}
struct Stores {
 PhaseDefinitionStore phases; SpellPhaseStore spells;
 auto const* GetPhaseDefinitionStore(){return &phases;}
 auto const* GetSpellPhaseStore(){return &spells;}
} stores;
auto* sObjectMgr=&stores;
#include "PhaseMethods.inc"
#include "ElseGroups.inc"
#include "PhaseLookup.inc"
// Network serialization is a boundary; server phase IDs/masks and native
// visibility update dispatch execute above, actual client rendering is pending.
void PhaseData::SendPhaseshiftToPlayer(){++player->packets;}
static void Refresh(PhaseMgr& mgr,Player& p){PhaseUpdateData update;update.AddQuestUpdate(37660);mgr.NotifyConditionChanged(update);assert(p.NeedPhaseRecalculate&&p.NeedPhaseUpdate);mgr.Recalculate();mgr.Update();}
int main(int argc,char** argv){
 assert(argc==3);std::ifstream input(argv[1]);assert(input);std::vector<Condition> rows;
 uint32 entry;Condition row;while(input>>entry>>row.ElseGroup>>row.ConditionType>>row.ConditionTarget>>row.ConditionValue1>>row.ConditionValue2>>row.ConditionValue3>>row.NegativeCondition){rows.push_back(row);rows.back().ReferenceId=entry;}
 for(auto& r:rows){entry=r.ReferenceId;r.ReferenceId=0;conditions.PhaseDefinitionsConditionStore[7334][entry].push_back(&r);}
 for(uint32 i=3;i<=7;++i){PhaseDefinition d{};d.zoneId=7334;d.entry=i;d.flags=16;d.phaseId.push_back(i==3?4265:i==7?4261:i+4258);stores.phases[7334].push_back(d);}
 bool baseline=std::string(argv[2])=="baseline";
 Player p;PhaseMgr mgr(&p);mgr.Recalculate();assert(p.phases==std::set<uint32>{4265});
 p.status=3;Refresh(mgr,p);assert(!p.phases.count(4265)&&p.phases.size()==4);
 p.objectives[237017]=1;Refresh(mgr,p);assert(!p.phases.count(4265));
 p.objectives[240012]=1;Refresh(mgr,p);assert(!p.phases.count(4265)); // optional exit never replaces victory
 p.objectives[89673]=1;Refresh(mgr,p);
 if(baseline){assert(!p.phases.count(4265));std::cout<<"PASS counterfactual: native original phase excludes existing Allari despite real victory and incomplete return.\n";return 0;}
 assert(p.phases.count(4265)&&p.phases.size()==5&&p.mask==1&&p.packets);
 // Victory update alone admits return; optional exit remains optional.
 p.objectives.erase(240012);p.objectives.erase(237017);Refresh(mgr,p);assert(p.phases.count(4265));
 // Screen mask remains an independent gate until native exit/cleanup removes it.
 PhaseData screen(&p);PhaseInfo info;info.phasemask=2;screen.AddAuraInfo(179183,info);screen.SendPhaseMaskToPlayer();assert(!(p.mask&1));
 screen.RemoveAuraInfo(179183);screen.SendPhaseMaskToPlayer();assert(p.mask==1);
 // Complete/rewarded/none/failed preserve the original negative-QUESTTAKEN branch.
 for(uint32 status:{1u,6u,0u,5u}){p.status=status;Refresh(mgr,p);assert(p.phases==std::set<uint32>{4265});}
 // Abandon/reaccept removes old victory; re-entry/relog derive visibility again.
 p.status=3;p.objectives.clear();Refresh(mgr,p);assert(!p.phases.count(4265));
 p.objectives[89673]=1;Player relog;relog.status=3;relog.objectives=p.objectives;PhaseMgr reloaded(&relog);reloaded.Recalculate();reloaded.Update();assert(relog.phases.count(4265));
 Player unrelated;unrelated.status=3;PhaseMgr other(&unrelated);other.Recalculate();assert(!unrelated.phases.count(4265)); // no shared/group progression invented
 std::cout<<"PASS native condition cases/ElseGroups/phase lookup, PhaseMgr recalculation/notifier and mask cleanup: victory admits Allari before return, early/unrelated do not; completion, abandon/retry and relog preserved. Network/rendering and actual player combat remain boundaries.\n";
}
