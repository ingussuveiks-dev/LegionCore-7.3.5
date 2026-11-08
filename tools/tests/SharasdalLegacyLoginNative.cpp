#include "SharasdalEntranceNative.h"
#include <limits>
#include <memory>
// Native Player is large. Separate owned heap fixtures keep all four lexical
// cases from exhausting the default Windows main-thread stack before startup.
static ScenarioResume Legacy(unsigned instance){return {1600,1066,instance,uint64(GameTime::GetStartTime())-1,836.16f,1035.4f,48.61f,1.7935f};}
static void Prepare(Player& player,uint32 map,uint32 instance){
#ifdef SHARASDAL_LEGACY_LOGIN_BASELINE
 PinnedSharasdalLogin::Prepare(player,map,instance);
#else
 player.PrepareScenarioLogin(map,instance);
#endif
}
static bool At(Position const& p,lfg::LFGDungeonData const* d){return p.GetExactDist(d->x,d->y,d->z)<0.001f&&std::fabs(p.GetOrientation()-d->o)<0.001f;}
struct SharasdalTargetsTag{using Type=SpellTargetPositionMap SpellMgr::*;friend Type Access(SharasdalTargetsTag);};
template struct FixtureAccess<SharasdalTargetsTag,&SpellMgr::mSpellTargetPositions>;
struct SharasdalStepsTag{using Type=ScenarioStepsByScenarioVector ScenarioMgr::*;friend Type Access(SharasdalStepsTag);};
template struct FixtureAccess<SharasdalStepsTag,&ScenarioMgr::m_stepVector>;
struct RestoreSources{
 lfg::LFGDungeonData& data;lfg::LFGDungeonData old;LFGDungeonsEntry& dbc;LFGDungeonsEntry oldDbc;SpellTargetPosition target;ScenarioSteps* steps;
 RestoreSources(lfg::LFGDungeonData& d,SpellTargetPosition const& p):data(d),old(d),dbc(*const_cast<LFGDungeonsEntry*>(d.dbc)),oldDbc(dbc),target(p),steps((sScenarioMgr->*Access(SharasdalStepsTag{}))[1066]){}
 ~RestoreSources(){data=old;dbc=oldDbc;(sSpellMgr->*Access(SharasdalTargetsTag{}))[207683]=target;(sScenarioMgr->*Access(SharasdalStepsTag{}))[1066]=steps;}
};
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);LoadSharasdalEntranceData();sAreaTriggerDataStore->LoadAreaTriggerTeleports();sLFGMgr->LoadLFGDungeons();
 DB2Storage<CharBaseInfoEntry> pairs("CharBaseInfo.db2",CharBaseInfoLoadInfo::Instance());Require(pairs.Load(argv[1],0),"Matching race/class pairs absent");
 auto d=const_cast<lfg::LFGDungeonData*>(sLFGMgr->GetLFGDungeon(1229));auto portal=const_cast<SpellTargetPosition*>(sSpellMgr->GetSpellTargetPosition(207683));Require(d&&d->dbc&&portal&&portal->target_mapId==1600&&sScenarioMgr->GetScenarioSteps(1066)->size()==6,"Native Sharasdal route absent");
#ifndef SHARASDAL_LEGACY_LOGIN_BASELINE
 Require(std::fabs(d->x-portal->target_X)<.01f&&std::fabs(d->y-portal->target_Y)<.01f&&std::fabs(d->z-portal->target_Z)<.01f,"Loaded1229 differs from actual207683");
#endif
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1600,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1600,{}}});
 unsigned cases=0;
 for(unsigned race:{unsigned(RACE_DRAENEI),unsigned(RACE_ORC)}){
  bool eligible=false;for(auto pair:pairs)if(pair->RaceID==race&&pair->ClassID==CLASS_SHAMAN)eligible=true;Require(eligible,"Invalid Legion Shaman fixture pair");auto session=Session();Position late{-184.494f,801.724f,796.705f,0};
  {unsigned instance=9998100+race;auto parent=std::make_unique<MapInstanced>(1600,300);auto mapOwner=std::make_unique<InstanceMap>(1600,300,instance,DIFFICULTY_N_SCENARIO,parent.get());auto& map=*mapOwner;auto scenario=sScenarioMgr->AddScenario(&map,1066);
   for(unsigned step=1;step<6;++step){scenario->SetCurrentStep(step);auto owner=std::make_unique<NativePlayer>(session,map,815,late);auto& p=*owner;Shaman(p,race);p.SetPhaseId({6120},false);auto floor=map.GetHeight(p.GetPhases(),portal->target_X,portal->target_Y,portal->target_Z+5);Require(floor<=portal->target_Z+.5f&&portal->target_Z-floor<=5,"Authored207683 arrival floor unsupported");auto count=scenario->GetAchievementMgr().GetCriteriaProgressMap()->size();p.RecordScenarioEntrance();auto saved=p.*Access(SharasdalEntryTag{});Require(saved.scenario==1066&&saved.map==1600&&saved.instance==instance&&saved.boot,"Native late record missing");
#ifdef SHARASDAL_LEGACY_LOGIN_BASELINE
    Require(std::fabs(saved.x-836.16f)<.001f&&std::fabs(saved.z-48.61f)<.001f,"Original1229 recorder mismatch");
#else
    Require(std::fabs(saved.x-portal->target_X)<.001f&&std::fabs(saved.y-portal->target_Y)<.001f&&std::fabs(saved.z-portal->target_Z)<.001f&&std::fabs(saved.o-portal->target_Orientation)<.001f,"Recorder lost actual207683 arrival");
#endif
    Require(p.GetExactDist(late)<.001f&&scenario->GetCurrentStep()==step&&scenario->GetAchievementMgr().GetCriteriaProgressMap()->size()==count&&!p.HasItemCount(128911,1,true),"Recorder granted motion/item/progress");++cases;
   }sScenarioMgr->RemoveScenario(instance);
  }
  for(auto status:{QUEST_STATUS_INCOMPLETE,QUEST_STATUS_COMPLETE})for(unsigned mode=0;mode<12;++mode){unsigned instance=9998200+cases;
   auto parent=std::make_unique<MapInstanced>(1600,300);auto mapOwner=std::make_unique<InstanceMap>(1600,300,instance,DIFFICULTY_N_SCENARIO,parent.get());auto& map=*mapOwner;auto live=sScenarioMgr->AddScenario(&map,1066);live->SetCurrentStep(3);
   {auto pOwner=std::make_unique<NativePlayer>(session,map,811,late);auto& p=*pOwner;Shaman(p,race);p.SetQuestStatus(40341,status);auto& saved=p.*Access(SharasdalEntryTag{});saved=Legacy(instance);
    if(mode==1||mode==3||mode==4||mode==5)saved.boot=GameTime::GetStartTime();if(mode==3||mode==4)p.Relocate(836.16f,1035.4f,mode==4?-100.0f:48.61f,0.2f);
    if(mode==9){saved.x=d->x;saved.y=d->y;saved.z=d->z;saved.o=d->o;}if(mode==10)saved.x+=0.1f;if(mode==11)saved.o+=0.1f;
    if(mode==5){live->SetCurrentStep(5);p.UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,49820);Require(live->IsCompleted(false),"Native completed control missing");}
    auto before=saved;Position pose=p.GetPosition();auto step=live->GetCurrentStep();auto count=live->GetAchievementMgr().GetCriteriaProgressMap()->size();Prepare(p,1600,instance+(mode==2));
    if(mode==5)Require(!saved.scenario&&!p.GetScenarioLoginInstance()&&!p.GetScenarioId()&&p.GetExactDist(pose)<0.001f,"Completed cleanup changed");
    else{
#ifdef SHARASDAL_LEGACY_LOGIN_BASELINE
     Require(saved.Write()==before.Write(),"Pinned Prepare unexpectedly rewrote the receipt");if(mode==1||mode==3||mode==4)Require(p.GetExactDist(pose)<0.001f&&p.GetScenarioLoginInstance()==instance,"Pinned live control changed");else Require(p.GetExactDist(before.x,before.y,before.z)<0.001f,"Pinned bad restart not reproduced");
#else
     bool repair=mode<9;Require(saved.map==before.map&&saved.scenario==before.scenario&&saved.instance==before.instance&&saved.boot==before.boot,"Repair changed receipt ownership/boot");if(repair)Require(std::fabs(saved.x-d->x)<0.001f&&std::fabs(saved.y-d->y)<0.001f&&std::fabs(saved.z-d->z)<0.001f&&std::fabs(saved.o-d->o)<0.001f,"Actual stored legacy receipt not normalized");else Require(saved.Write()==before.Write(),"Valid/custom entrance rewritten");
     if(mode==1)Require(p.GetExactDist(pose)<0.001f&&p.GetScenarioLoginInstance()==instance,"Valid live pose lost");else if(mode==3||mode==4)Require(At(p,d)&&p.GetScenarioLoginInstance()==instance,"Live unsafe/falling legacy pose not recovered");else if(repair||mode==9)Require(At(p,d)&&!p.GetScenarioLoginInstance(),"Fresh recovery lost the real entry");else Require(p.GetExactDist(before.x,before.y,before.z)<0.001f,"Custom entry changed");
     if(mode==6){auto once=saved.Write();Prepare(p,1600,instance);Require(saved.Write()==once&&At(p,d),"Repeated normalization changed receipt");}
     if(mode==7){p.RecordScenarioEntrance();Require(std::fabs(saved.x-d->x)<0.001f&&saved.instance==instance&&saved.boot==uint64(GameTime::GetStartTime()),"Native later Record lost the repair");}
     if(mode==8){ScenarioResume read;Require(read.Read(saved.Write())&&read.Write()==saved.Write(),"Native checkpoint serialization lost repaired floats");}
#endif
    }
    Require(p.GetQuestStatus(40341)==status&&!p.HasItemCount(128911,1,true)&&live->GetCurrentStep()==step&&live->GetAchievementMgr().GetCriteriaProgressMap()->size()==count,"Prepare awarded/reset quest/item/criteria");++cases;
   }sScenarioMgr->RemoveScenario(instance);
  }
#ifndef SHARASDAL_LEGACY_LOGIN_BASELINE
  for(unsigned fault=0;fault<26;++fault){unsigned instance=9998200+cases;auto parent=std::make_unique<MapInstanced>(1600,300);auto mapOwner=std::make_unique<InstanceMap>(1600,300,instance,DIFFICULTY_N_SCENARIO,parent.get());auto& map=*mapOwner;auto live=sScenarioMgr->AddScenario(&map,1066);live->SetCurrentStep(3);
   {portal=const_cast<SpellTargetPosition*>(sSpellMgr->GetSpellTargetPosition(207683));RestoreSources restore(*d,*portal);auto pOwner=std::make_unique<NativePlayer>(session,map,812,late);auto& p=*pOwner;Shaman(p,race);auto& saved=p.*Access(SharasdalEntryTag{});saved=Legacy(instance);auto dbc=const_cast<LFGDungeonsEntry*>(d->dbc);
    if(fault==0)(sSpellMgr->*Access(SharasdalTargetsTag{})).erase(207683);if(fault==1)portal->target_mapId=1220;if(fault==2)portal->target_X+=.1f;if(fault==3)portal->target_Y+=.1f;if(fault==4)portal->target_Z+=.1f;if(fault==5)portal->target_Orientation+=1;
    if(fault==6)d->id=0;if(fault==7)dbc->ScenarioID=972;if(fault==8)d->id=1059;if(fault==9)d->map=1511;if(fault==10)d->difficulty=DIFFICULTY_NORMAL;if(fault==11)dbc->ID=1059;if(fault==12)dbc->MapID=1511;if(fault==13)dbc->Flags&=~LFG_FLAG_USER_TELEPORT_NOT_ALLOWED;if(fault==14)d->x+=1;if(fault==15)d->o+=1;if(fault==16)d->x=std::numeric_limits<float>::quiet_NaN();if(fault==17)d->o=std::numeric_limits<float>::quiet_NaN();if(fault==18)saved.instance=0;if(fault==19)saved.boot=0;if(fault==20)(sScenarioMgr->*Access(SharasdalStepsTag{}))[1066]=nullptr;if(fault==21)dbc->DifficultyID=1;if(fault==22)d->z=0;if(fault==23)portal->target_X=std::numeric_limits<float>::quiet_NaN();if(fault==24)portal->target_Orientation=std::numeric_limits<float>::infinity();if(fault==25)portal->target_X=portal->target_Y=portal->target_Z=0;
    Prepare(p,1600,instance);Require(!saved.scenario&&!p.GetScenarioLoginInstance()&&!p.GetScenarioId()&&p.GetExactDist(late)<0.001f&&p.GetQuestStatus(40341)==QUEST_STATUS_INCOMPLETE&&!p.HasItemCount(128911,1,true)&&live->GetCurrentStep()==3,"Invalid provenance/source did not fail closed");++cases;
   }sScenarioMgr->RemoveScenario(instance);
  }
#endif
  // Whole old map is gone; reuse its numeric ID with the original legacy
  // receipt and native1229 constructor, rather than pre-seeding a new stage.
  unsigned oldInstance=9998190+race;ScenarioResume gone=Legacy(oldInstance);gone.boot=GameTime::GetStartTime();
  {auto oldParent=std::make_unique<MapInstanced>(1600,300);auto oldMapOwner=std::make_unique<InstanceMap>(1600,300,oldInstance,DIFFICULTY_N_SCENARIO,oldParent.get());auto& oldMap=*oldMapOwner;auto oldScenario=sScenarioMgr->AddScenario(&oldMap,1066);oldScenario->SetCurrentStep(3);{auto originalOwner=std::make_unique<NativePlayer>(session,oldMap,813,late);auto& original=*originalOwner;Shaman(original,race);(original.*Access(SharasdalEntryTag{}))=gone;Require((original.*Access(SharasdalEntryTag{})).Read(gone.Write()),"Original legacy receipt serialization failed");gone=original.*Access(SharasdalEntryTag{});}sScenarioMgr->RemoveScenario(oldInstance);}
  Require(!sScenarioMgr->GetScenario(oldInstance),"Original native map lifetime survived");
  {unsigned instance=oldInstance;auto parent=std::make_unique<MapInstanced>(1600,300);auto mapOwner=std::make_unique<InstanceMap>(1600,300,instance,DIFFICULTY_N_SCENARIO,parent.get());auto& map=*mapOwner;{auto pOwner=std::make_unique<NativePlayer>(session,map,813,late);auto& p=*pOwner;Shaman(p,race);auto& saved=p.*Access(SharasdalEntryTag{});Require(saved.Read(gone.Write()),"Destroyed-map legacy receipt roundtrip failed");Prepare(p,1600,instance);Require(!p.GetScenarioLoginInstance()&&p.GetScenarioId()==1066,"Destroyed-map selection adopted recycled ID");auto live=sScenarioMgr->AddScenario(&map,d,&p,false);Require(live&&live->GetScenarioId()==1066&&!live->GetCurrentStep()&&!p.GetScenarioId()&&p.CanResumeScenarioLogin(1066)&&!p.HasItemCount(128911,1,true),"Native fresh constructor failed");
#ifdef SHARASDAL_LEGACY_LOGIN_BASELINE
   Require(std::fabs(p.GetPositionX()-836.16f)<0.001f,"Pinned bad constructor position not reproduced");
#else
   Require(At(p,d),"Native constructor retained old foreign-world position");
#endif
   p.Relocate(late);++cases;}sScenarioMgr->RemoveScenario(instance);}
  session->SetPlayer(nullptr);delete session;
 }
#ifdef SHARASDAL_LEGACY_LOGIN_BASELINE
 Require(cases==60,"Native counterfactual count changed");std::cout<<"COUNTERFACTUAL pinned677430a07 Sharasdal Prepare native60:10 original bad late LFG receipts and legacy fresh/live/falling pose remain wrong;valid/custom/completed and recycled-ID native constructor controls;old source capsule with current native support,not a historical whole binary"<<std::endl;
#else
 Require(cases==112,"Native repaired count changed");std::cout<<"PASS native Sharasdal legacy login112:Draenei+Orc actual26972 pairs/incomplete+complete,10 native late Recorder receipts from207683 plus real Player/Prepare/LFG/actual207683/Scenario/serialization,live safe pose retained/live unsafe XY+fall recovered,old boot/numeric mismatch,52 source/provenance refusals,completed cleanup,whole old-map lifetime+native fresh constructor;no character DB mutation or quest/item/criteria grant. Earlier quest/pose and source fault endpoints controlled;full LoadFromDB/group/client separate"<<std::endl;
#endif
 CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}
