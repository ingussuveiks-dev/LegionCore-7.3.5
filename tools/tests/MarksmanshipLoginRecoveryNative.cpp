// Actual quest/objective/Prepare/CanResume/Scenario components; accepted quest-row,
// saved-transport/position and selected-map endpoints controlled. No return grants.
#ifdef MARKSMAN_LOGIN_BASELINE
#include "PinnedAcquisitionRoute.inc"
#endif
#define main WarriorRecoveryUnusedMain
#include "WarriorLoginRecoveryNative.cpp"
#undef main
#include "PlayerDefines.h"
struct HunterInstanceTag {using Type=uint32 Player::*;friend Type Access(HunterInstanceTag);};
template struct FixtureAccess<HunterInstanceTag,&Player::m_scenarioLoginInstance>;
static void Objective(Player& p,int32 entry,int32 value){auto q=sQuestDataStore->GetQuestTemplate(40419);Require(q,"Actual Hunter quest missing");for(auto const& o:q->GetObjectives())if(o.ObjectID==entry){Require(o.StorageIndex>=0&&o.StorageIndex<=5&&o.Amount==1,"Actual receipt storage/amount changed");p.SetQuestObjectiveData(q,&o,value,true);return;}throw std::runtime_error("Actual Hunter receipt missing");}
struct MarksmanPlayer : RecoveryPlayer {
 using RecoveryPlayer::RecoveryPlayer;
 void Binding(Map* map,uint32 mapId){auto& m_scenarioResume=this->*Access(RecoveryResumeTag{});auto& m_scenarioLoginId=this->*Access(RecoveryIdTag{});auto& m_scenarioLoginInstance=this->*Access(HunterInstanceTag{});
#include "MarksmanshipBindingNative.inc"
 (void)map;(void)mapId;(void)m_scenarioResume;(void)m_scenarioLoginId;(void)m_scenarioLoginInstance;
 }
};
#ifndef MARKSMAN_LOGIN_SUPPORT_ONLY
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);sAreaTriggerDataStore->LoadAreaTriggerTeleports();sLFGMgr->LoadLFGDungeons();sSpellMgr->LoadSpellTargetPositions();
 auto quest=const_cast<Quest*>(sQuestDataStore->GetQuestTemplate(40419));Require(quest&&quest->GetObjectives().empty(),"Actual40419 bootstrap missing/already populated");auto objectives=WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged FROM quest_objectives WHERE QuestID=40419 ORDER BY StorageIndex");Require(bool(objectives),"Actual40419 objective rows missing");do{quest->LoadQuestObjective(objectives->Fetch());}while(objectives->NextRow());Require(quest->GetObjectives().size()==6,"Actual40419 objective count changed");auto& targets=sSpellMgr->*Access(RecoveryTargetsTag{});auto target=targets.at(201674);
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1489,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1489,{}}});
 unsigned early=0,fresh=0;
 for(unsigned race:{unsigned(RACE_HUMAN),unsigned(RACE_ORC)}){
  auto session=Session();unsigned instanceId=9998700+race;ScenarioResume prepared;Position late{62.6788f,1215.55f,-38.6585f,0};
  {
   MapInstanced parent(1489,300);InstanceMap map(1489,300,instanceId,DIFFICULTY_N_SCENARIO,&parent);auto progress=sScenarioMgr->AddScenario(&map,972);progress->SetCurrentStep(3);
   for(unsigned kind=0;kind<31;++kind){MarksmanPlayer p(session,map,801,late);p.acceptedQuest=40419;p.SetByteValue(UNIT_FIELD_BYTES_0,0,race);p.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_HUNTER);p.setFactionForRace(race);auto& receipt=p.*Access(RecoveryResumeTag{});bool existing=kind==5||kind==6||kind==7||kind==24||kind==25||kind==26||kind==27||kind==29;
    if(existing)receipt={1489,972,instanceId,uint64(GameTime::GetStartTime()),101,202,30,1};if(kind==1)Require(!receipt.Read("malformed"),"Malformed receipt accepted");if(kind==2)receipt={1489,944,instanceId,uint64(GameTime::GetStartTime()),1,2,3,0};if(kind==3)receipt={1539,972,instanceId,uint64(GameTime::GetStartTime()),1,2,3,0};if(kind==4)receipt={1489,972,instanceId,uint64(GameTime::GetStartTime()),std::numeric_limits<float>::quiet_NaN(),2,3,0};if(kind==6||kind==25)++receipt.boot;
    if(kind==7||kind==8)p.holder.status=QUEST_STATUS_COMPLETE;if(kind==9)p.holder.status=QUEST_STATUS_NONE;if(kind==10)p.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_WARLOCK);if(kind==13)p.validInput=false;if(kind==14)targets.erase(201674);if(kind==15)targets.at(201674).target_mapId=1495;if(kind==16){auto& t=targets.at(201674);t.target_X=t.target_Y=t.target_Z=0;}if(kind==17)targets.at(201674).target_X=std::numeric_limits<float>::quiet_NaN();if(kind==18)targets.at(201674).target_Orientation=std::numeric_limits<float>::infinity();if(kind==19)targets.at(201674).target_X=20000;if(kind==20)p.acceptedQuest=40043;if(kind==30){sScenarioMgr->RemoveScenario(instanceId);progress=nullptr;}
    bool loaded=p.Early(kind==11?1539:1489,instanceId,kind==12?AT_LOGIN_UNLOCK:0);p.Normal(loaded);Require(p.loads==1,"Native accepted status was loaded more than once");if(kind==21||kind==26)const_cast<RewardedQuestSet&>(p.getRewardedQuests()).insert(40419u);if(kind==22||kind==27)p.SetQuestStatus(40419,QUEST_STATUS_NONE);if(kind==23||kind==24||kind==25)Objective(p,103490,1);
    bool seeded=kind<=4||kind==21||kind==22||kind==23||kind==28||kind==30;
#ifndef MARKSMAN_LOGIN_BASELINE
    seeded=seeded||kind==8;
#endif
#ifdef MARKSMAN_LOGIN_BASELINE
    if(seeded)Require(!p.GetScenarioId()&&!loaded,"Pinned early loader unexpectedly recovered Hunter");bool allowed=existing&&kind!=26&&kind!=27;
#else
    if(seeded){Require(p.GetScenarioId()==972&&!p.GetScenarioLoginInstance()&&!receipt.instance&&!receipt.boot&&p.GetExactDist(target.target_X,target.target_Y,target.target_Z)<0.001f,"Actual early Hunter entrance-only recovery failed");if(!kind)prepared=receipt;}
    if(kind==28||kind==29||kind==30)p.Binding(&map,1489);if(kind==28)Require(!p.GetScenarioId()&&!receipt.scenario&&!p.GetScenarioLoginInstance(),"Empty receipt inherited an already live bound instance");if(kind==29)Require(p.GetScenarioLoginInstance()==instanceId,"Valid exact live receipt blocked by binding guard");
    bool allowed=(seeded||existing)&&kind!=8&&kind!=21&&kind!=22&&kind!=23&&kind!=26&&kind!=27&&kind!=28;
#endif
    Require(p.CanResumeScenarioLogin(972)==allowed,"Native late Hunter admission/claim guard differs");if(existing)Require(receipt.x==101,"Existing valid Hunter receipt overwritten");if(!seeded&&!existing)Require(!p.GetScenarioId(),"Refused native source unexpectedly selected a scenario");if(progress)Require(progress->GetCurrentStep()==3,"Recovery granted stage progress");Require(!p.HasItemCount(128826,1,true),"Login granted artifact");p.Relocate(late);targets.insert_or_assign(201674,target);++early;
   }
  }
#ifndef MARKSMAN_LOGIN_BASELINE
  for(unsigned dungeonId:{1162u}){MapInstanced parent(1489,300);InstanceMap map(1489,300,instanceId,DIFFICULTY_N_SCENARIO,&parent);{MarksmanPlayer p(session,map,801,late);p.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_HUNTER);p.SetQuestStatus(40419,QUEST_STATUS_INCOMPLETE);Require((p.*Access(RecoveryResumeTag{})).Read(prepared.Write()),"Prepared native seed roundtrip failed");p.PrepareScenarioLogin(1489,0);p.Binding(&map,1489);auto sc=sScenarioMgr->AddScenario(&map,sLFGMgr->GetLFGDungeon(dungeonId),&p,false);Require(sc&&sc->GetScenarioId()==972&&sc->GetCurrentStep()==0&&!p.GetScenarioId()&&p.CanResumeScenarioLogin(972),"Actual fresh constructor did not consume Hunter seed at step0");p.Relocate(late);++fresh;}sScenarioMgr->RemoveScenario(instanceId);}
#endif
  session->SetPlayer(nullptr);delete session;
 }
 Require(early==62,"Native Marksmanship early case count changed");
#ifdef MARKSMAN_LOGIN_BASELINE
 Require(!fresh,"Pinned route dependency unexpectedly seeded a new run");std::cout<<"COUNTERFACTUAL pinned387b8a385 route/early capsule62:empty Marksmanship checkpoints do not select native entrance;current native Prepare/CanResume support,not historical whole binary or ordinary success"<<std::endl;
#else
 Require(fresh==2,"Native Marksmanship fresh count changed");std::cout<<"PASS native Marksmanship login64:early62/fresh2,actual quest/objective rows and source201674,one accepted quest load,live/fresh/complete and recorded claimed receipt preservation,unrecorded claim/refused source/status/binding guards,destroyed map/recycled ID/native1162 constructor972 step0;no item/credit/return grant. Quest parser,saved transport/position and selected-map endpoint controlled;full saved-character/group/client/owned-return separate"<<std::endl;
#endif
 CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}
#endif
