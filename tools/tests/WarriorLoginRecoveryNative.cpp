// Real native resume/admission/constructor; accepted quest-row parsing and saved
// transport/position-valid endpoints controlled. No character DB writes.
#define ALTHEA_INTRO_NATIVE_SUPPORT_ONLY
#include "AltheaIntroNative.cpp"
#include "LFGMgr.h"
#include "AreaTriggerData.h"
#include "MapManager.h"
#include "WarriorScenarioRoute.h"
#include "AcquisitionScenarioRoute.h"
#include <limits>
struct RecoveryResumeTag {using Type=ScenarioResume Player::*;friend Type Access(RecoveryResumeTag);};
template struct FixtureAccess<RecoveryResumeTag,&Player::m_scenarioResume>;
struct RecoveryIdTag {using Type=uint32 Player::*;friend Type Access(RecoveryIdTag);};
template struct FixtureAccess<RecoveryIdTag,&Player::m_scenarioLoginId>;
struct RecoveryTargetsTag {using Type=SpellTargetPositionMap SpellMgr::*;friend Type Access(RecoveryTargetsTag);};
template struct FixtureAccess<RecoveryTargetsTag,&SpellMgr::mSpellTargetPositions>;
struct RecoveryPlayer : NativePlayer {
 using NativePlayer::NativePlayer;uint32 acceptedQuest=0;unsigned loads=0;bool validInput=true,transportCleared=false;
 struct Holder {int status=QUEST_STATUS_INCOMPLETE;int GetPreparedResult(unsigned index){Require(index==PLAYER_LOGIN_QUERY_LOADQUESTSTATUS,"Wrong actual quest prepared-result slot");return status;}} holder;
 bool IsPositionValid()const{return validInput;}
 void _LoadQuestStatus(int value){Require(acceptedQuest&&sQuestDataStore->GetQuestTemplate(acceptedQuest),"No actual native template for controlled accepted quest endpoint");SetQuestStatus(acceptedQuest,QuestStatus(value));++loads;}
 bool Early(uint32 mapId,uint32 instanceId,uint16 flags){
  auto& m_scenarioResume=this->*Access(RecoveryResumeTag{});auto& m_scenarioLoginId=this->*Access(RecoveryIdTag{});uint16 m_atLoginFlags=flags;auto mapEntry=sMapStore.LookupEntry(mapId);uint64 transLowGUID=99;
  struct {struct {bool reset=false;void Reset(){reset=true;}} transport;} m_movementInfo;
#include "WarriorEarlyNative.inc"
  transportCleared=m_movementInfo.transport.reset&&transLowGUID==0;Require(bool(m_scenarioLoginId)==transportCleared,"Saved transport reset did not follow actual selected recovery");return warriorQuestStatusLoaded;
 }
 void Normal(bool scenarioQuestStatusLoaded){
#include "WarriorNormalQuestNative.inc"
 }
};
int main(int argc,char** argv){try{
 InitializeAltheaGuardsNative(argc,argv);sAreaTriggerDataStore->LoadAreaTriggerTeleports();sLFGMgr->LoadLFGDungeons();sSpellMgr->LoadSpellTargetPositions();
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1495,{}},{1511,{}},{1539,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1495,{}},{1511,{}},{1539,{}}});
 unsigned cases=0;
 for(unsigned mapId:{1495u,1511u,1539u})for(unsigned race:{unsigned(RACE_HUMAN),unsigned(RACE_ORC)}){
  auto route=WarriorScenarioRoute::Find(mapId);Require(route,"Native route metadata missing");auto& targets=sSpellMgr->*Access(RecoveryTargetsTag{});auto target=targets.at(route->spell);auto spell=sSpellMgr->GetSpellInfo(route->spell);Require(target.target_mapId==mapId&&spell&&spell->GetEffect(EFFECT_1)->Effect==227,"Matching native entry changed");
  auto session=Session();ScenarioResume prepared;uint32 instanceId=9999900+cases;Position chamber{mapId==1495?4715.0f:mapId==1511?3390.0f:2200.0f,mapId==1495?346.0f:mapId==1511?1660.0f:2440.0f,10,0};
  {
   MapInstanced parent(mapId,300);InstanceMap map(mapId,300,instanceId,DIFFICULTY_N_SCENARIO,&parent);auto progress=sScenarioMgr->AddScenario(&map,route->scenario);progress->SetCurrentStep(3);
   for(unsigned kind=0;kind<22;++kind){
    RecoveryPlayer p(session,map,901,chamber);p.acceptedQuest=route->quest;p.SetQuestStatus(route->quest,QUEST_STATUS_NONE);p.SetByteValue(UNIT_FIELD_BYTES_0,0,race);p.setFactionForRace(race);auto& resume=p.*Access(RecoveryResumeTag{});
    if(kind==1)Require(!resume.Read("bad checkpoint"),"Malformed record accepted");
    if(kind==2)resume={mapId,mapId==1495?1068u:mapId==1511?973u:1092u,instanceId,uint64(GameTime::GetStartTime()),101,202,30,1};
    if(kind==3)resume={mapId+1,route->scenario,instanceId,uint64(GameTime::GetStartTime()),101,202,30,1};
    if(kind==4)resume={mapId,route->scenario,instanceId,uint64(GameTime::GetStartTime()),std::numeric_limits<float>::quiet_NaN(),202,30,1};
    if(kind==5||kind==6||kind==21)resume={mapId,route->scenario,instanceId,uint64(GameTime::GetStartTime())+(kind==6),101,202,30,1};
    if(kind==7||kind==21)p.holder.status=QUEST_STATUS_COMPLETE;if(kind==8)p.holder.status=QUEST_STATUS_NONE;if(kind==10)p.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_WARLOCK);
    p.validInput=kind!=13;
    if(kind==14)targets.erase(route->spell);else{auto& point=targets.at(route->spell);if(kind==15)point.target_mapId=1;if(kind==16)point.target_X=point.target_Y=point.target_Z=0;if(kind==17)point.target_X=std::numeric_limits<float>::quiet_NaN();if(kind==18)point.target_Orientation=std::numeric_limits<float>::infinity();if(kind==19)point.target_X=20000;}
    bool loaded=p.Early(kind==11?1:mapId,instanceId,kind==12?AT_LOGIN_UNLOCK:0);p.Normal(loaded);Require(p.loads==1,"Actual early/normal quest path loaded status twice");
    if(kind==9)const_cast<RewardedQuestSet&>(p.getRewardedQuests()).insert(route->quest);if(kind==20)p.SetQuestStatus(route->quest,QUEST_STATUS_NONE);
    bool existing=kind==5||kind==6||kind==21,recovered=kind<=4||kind==9||kind==20;
#ifdef WARRIOR_LOGIN_RECOVERY_BASELINE
    if(recovered)Require(p.GetScenarioId()!=route->scenario,"Pinned early block unexpectedly recovered missing record");
    if(!existing&&!recovered)Require(!p.GetScenarioId(),"Pinned control changed unrelated admission");
#else
    if(recovered){if(!(p.GetScenarioId()==route->scenario&&!p.GetScenarioLoginInstance()&&resume.instance==0&&resume.boot==0&&p.GetExactDist(target.target_X,target.target_Y,target.target_Z)<0.001f))std::cerr<<"CONTROL failed map="<<mapId<<" race="<<race<<" kind="<<kind<<" loaded="<<loaded<<" q="<<p.GetQuestStatus(route->quest)<<" selector="<<p.GetScenarioId()<<" loginInst="<<p.GetScenarioLoginInstance()<<" resume="<<resume.Write()<<" xyz="<<p.GetPositionX()<<','<<p.GetPositionY()<<','<<p.GetPositionZ()<<" expected="<<target.target_X<<','<<target.target_Y<<','<<target.target_Z<<std::endl;Require(p.GetScenarioId()==route->scenario&&!p.GetScenarioLoginInstance()&&resume.instance==0&&resume.boot==0&&p.GetExactDist(target.target_X,target.target_Y,target.target_Z)<0.001f,"Active missing receipt failed native entrance/new-instance recovery");if(!kind)prepared=resume;}
    if(!existing&&!recovered)Require(!p.GetScenarioId()&&p.GetExactDist(chamber)<0.001f,"Refused context seeded or relocated a player");
    if(existing)Require(p.GetScenarioId()==route->scenario&&resume.x==101&&((kind==6&&p.GetExactDist(101,202,30)<0.001f)||(kind!=6&&p.GetExactDist(chamber)<0.001f)),"Existing legitimate receipt lost live/entrance position");
    Require(p.CanResumeScenarioLogin(route->scenario)==((existing||recovered)&&kind!=9&&kind!=20),"Post-load active/rewarded admission mismatch");
#endif
    Require(progress->GetCurrentStep()==3&&!p.HasItemCount(128289,1,true)&&!p.HasItemCount(128908,1,true)&&!p.HasItemCount(128910,1,true),"Login granted progress or artifact");p.Relocate(chamber);targets.insert_or_assign(route->spell,target);++cases;
   }sScenarioMgr->RemoveScenario(instanceId);
  }
#ifndef WARRIOR_LOGIN_RECOVERY_BASELINE
  {
   MapInstanced parent(mapId,300);InstanceMap map(mapId,300,instanceId,DIFFICULTY_N_SCENARIO,&parent);
   {RecoveryPlayer p(session,map,901,{target.target_X,target.target_Y,target.target_Z,target.target_Orientation});p.SetByteValue(UNIT_FIELD_BYTES_0,0,race);p.setFactionForRace(race);p.SetQuestStatus(route->quest,QUEST_STATUS_INCOMPLETE);Require((p.*Access(RecoveryResumeTag{})).Read(prepared.Write()),"Native prepared entrance roundtrip failed");p.PrepareScenarioLogin(mapId,0);auto difficulty=sDB2Manager.GetDefaultMapDifficulty(mapId);auto dungeon=sLFGMgr->GetLFGDungeon(mapId,Difficulty(difficulty->DifficultyID),p.GetTeam());Require(dungeon,"Native LFG row missing");auto fresh=sScenarioMgr->AddScenario(&map,dungeon,&p,false);Require(fresh&&fresh->GetScenarioId()==route->scenario&&fresh->GetCurrentStep()==0&&!p.GetScenarioId()&&p.CanResumeScenarioLogin(route->scenario),"Native new map/constructor did not consume correct class selector at step0");++cases;}sScenarioMgr->RemoveScenario(instanceId);
  }
#endif
  session->SetPlayer(nullptr);delete session;
 }
#ifdef WARRIOR_LOGIN_RECOVERY_BASELINE
 Require(cases==132,"Pinned native case count changed");std::cout<<"COUNTERFACTUAL pinned5ec346c8e early capsule/current native support132:missing receipt cannot seed active native quest before transport/constructor;not a historical whole binary or successful gameplay"<<std::endl;
#else
 Require(cases==138,"Native recovery case count changed");std::cout<<"PASS actual early capsule/native resume138:all3 Warrior routes/Human+Orc,source/status/class/unlock guards,one controlled quest-row load,late reward/abandonment refusal,existing records,actual Map destruction/recycledID/native constructor0;no progress/item grant. Quest parser/transport/position-valid endpoints controlled;full character/client login separate"<<std::endl;
#endif
 CloseAltheaNative();return 0;
}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}
