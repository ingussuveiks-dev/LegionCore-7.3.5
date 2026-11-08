#define TUURE_ENTRANCE_SUPPORT_ONLY
#include "TuureEntranceNative.cpp"
#include <limits>
#include <memory>
// Native Player is large. Separate owned heap fixtures keep all four lexical
// cases from exhausting the default Windows main-thread stack before startup.
static ScenarioResume Legacy(unsigned instance){return {1489,1085,instance,uint64(GameTime::GetStartTime())-1,-828.85f,4372,738.64f,1.86768f};}
static void Prepare(Player& player,uint32 map,uint32 instance){
#ifdef TUURE_LEGACY_LOGIN_BASELINE
 PinnedTuureLogin::Prepare(player,map,instance);
#else
 player.PrepareScenarioLogin(map,instance);
#endif
}
static bool At(Position const& p,lfg::LFGDungeonData const* d){return p.GetExactDist(d->x,d->y,d->z)<0.001f&&std::fabs(p.GetOrientation()-d->o)<0.001f;}
struct RestoreSources{
 lfg::LFGDungeonData& data;lfg::LFGDungeonData old;LFGDungeonsEntry& dbc;LFGDungeonsEntry oldDbc;WorldSafeLocsEntry& safe;WorldSafeLocsEntry oldSafe;
 RestoreSources(lfg::LFGDungeonData& d,WorldSafeLocsEntry& s):data(d),old(d),dbc(*const_cast<LFGDungeonsEntry*>(d.dbc)),oldDbc(dbc),safe(s),oldSafe(s){}
 ~RestoreSources(){data=old;dbc=oldDbc;safe=oldSafe;}
};
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);sAreaTriggerDataStore->LoadAreaTriggerTeleports();sLFGMgr->LoadLFGDungeons();
 DB2Storage<CharBaseInfoEntry> pairs("CharBaseInfo.db2",CharBaseInfoLoadInfo::Instance());Require(pairs.Load(argv[1],0),"Matching race/class pairs absent");
 auto d=const_cast<lfg::LFGDungeonData*>(sLFGMgr->GetLFGDungeon(1243));auto safe=const_cast<WorldSafeLocsEntry*>(sWorldSafeLocsStore.LookupEntry(5480));Require(d&&d->dbc&&safe&&safe->MapID==1489&&sScenarioMgr->GetScenarioSteps(1085)->size()==7,"Native Tuure source absent");
 Require(std::fabs(d->x-safe->Loc.X)<0.01f&&std::fabs(d->y-safe->Loc.Y)<0.01f&&std::fabs(d->z-safe->Loc.Z)<0.01f,"Live1243 must contain the repaired5480 entrance");
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1489,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1489,{}}});
 unsigned cases=0;
 for(unsigned race:{unsigned(RACE_HUMAN),unsigned(RACE_UNDEAD_PLAYER)}){
  bool eligible=false;for(auto pair:pairs)if(pair->RaceID==race&&pair->ClassID==CLASS_PRIEST)eligible=true;Require(eligible,"Invalid Legion Priest fixture pair");auto session=Session();Position late{71.9653f,1218.51f,-74.2702f,0};
  for(auto status:{QUEST_STATUS_INCOMPLETE,QUEST_STATUS_COMPLETE})for(unsigned mode=0;mode<12;++mode){unsigned instance=9998200+cases;
   MapInstanced parent(1489,300);InstanceMap map(1489,300,instance,DIFFICULTY_N_SCENARIO,&parent);auto live=sScenarioMgr->AddScenario(&map,1085);live->SetCurrentStep(3);
   {auto pOwner=std::make_unique<NativePlayer>(session,map,811,late);auto& p=*pOwner;Priest(p,race);p.SetQuestStatus(41993,status);auto& saved=p.*Access(TuureEntryTag{});saved=Legacy(instance);
    if(mode==1||mode==3||mode==4||mode==5)saved.boot=GameTime::GetStartTime();if(mode==3||mode==4)p.Relocate(-828.85f,4372,mode==4?-100.0f:738.64f,0.2f);
    if(mode==9){saved.x=d->x;saved.y=d->y;saved.z=d->z;saved.o=d->o;}if(mode==10)saved.x+=0.1f;if(mode==11)saved.o+=0.1f;
    if(mode==5){live->SetCurrentStep(6);p.UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,50679);p.UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,50702);Require(live->IsCompleted(false),"Native completed control missing");}
    auto before=saved;Position pose=p.GetPosition();auto step=live->GetCurrentStep();auto count=live->GetAchievementMgr().GetCriteriaProgressMap()->size();Prepare(p,1489,instance+(mode==2));
    if(mode==5)Require(!saved.scenario&&!p.GetScenarioLoginInstance()&&!p.GetScenarioId()&&p.GetExactDist(pose)<0.001f,"Completed cleanup changed");
    else{
#ifdef TUURE_LEGACY_LOGIN_BASELINE
     Require(saved.Write()==before.Write(),"Pinned Prepare unexpectedly rewrote the receipt");if(mode==1||mode==3||mode==4)Require(p.GetExactDist(pose)<0.001f&&p.GetScenarioLoginInstance()==instance,"Pinned live control changed");else Require(p.GetExactDist(before.x,before.y,before.z)<0.001f,"Pinned bad restart not reproduced");
#else
     bool repair=mode<9;Require(saved.map==before.map&&saved.scenario==before.scenario&&saved.instance==before.instance&&saved.boot==before.boot,"Repair changed receipt ownership/boot");if(repair)Require(std::fabs(saved.x-d->x)<0.001f&&std::fabs(saved.y-d->y)<0.001f&&std::fabs(saved.z-d->z)<0.001f&&std::fabs(saved.o-d->o)<0.001f,"Actual stored legacy receipt not normalized");else Require(saved.Write()==before.Write(),"Valid/custom entrance rewritten");
     if(mode==1)Require(p.GetExactDist(pose)<0.001f&&p.GetScenarioLoginInstance()==instance,"Valid live pose lost");else if(mode==3||mode==4)Require(At(p,d)&&p.GetScenarioLoginInstance()==instance,"Live unsafe/falling legacy pose not recovered");else if(repair||mode==9)Require(At(p,d)&&!p.GetScenarioLoginInstance(),"Fresh recovery lost the real entry");else Require(p.GetExactDist(before.x,before.y,before.z)<0.001f,"Custom entry changed");
     if(mode==6){auto once=saved.Write();Prepare(p,1489,instance);Require(saved.Write()==once&&At(p,d),"Repeated normalization changed receipt");}
     if(mode==7){p.RecordScenarioEntrance();Require(std::fabs(saved.x-d->x)<0.001f&&saved.instance==instance&&saved.boot==uint64(GameTime::GetStartTime()),"Native later Record lost the repair");}
     if(mode==8){ScenarioResume read;Require(read.Read(saved.Write())&&read.Write()==saved.Write(),"Native checkpoint serialization lost repaired floats");}
#endif
    }
    Require(p.GetQuestStatus(41993)==status&&!p.HasItemCount(128825,1,true)&&live->GetCurrentStep()==step&&live->GetAchievementMgr().GetCriteriaProgressMap()->size()==count,"Prepare awarded/reset quest/item/criteria");++cases;
   }sScenarioMgr->RemoveScenario(instance);
  }
#ifndef TUURE_LEGACY_LOGIN_BASELINE
  for(unsigned fault=0;fault<22;++fault){unsigned instance=9998200+cases;MapInstanced parent(1489,300);InstanceMap map(1489,300,instance,DIFFICULTY_N_SCENARIO,&parent);auto live=sScenarioMgr->AddScenario(&map,1085);live->SetCurrentStep(3);
   {RestoreSources restore(*d,*safe);auto pOwner=std::make_unique<NativePlayer>(session,map,812,late);auto& p=*pOwner;Priest(p,race);auto& saved=p.*Access(TuureEntryTag{});saved=Legacy(instance);auto dbc=const_cast<LFGDungeonsEntry*>(d->dbc);
    if(fault==0)saved.instance=0;if(fault==1)saved.boot=0;if(fault==2)d->id=1059;if(fault==3)d->map=1511;if(fault==4)d->difficulty=DIFFICULTY_NORMAL;if(fault==5)dbc->ID=1059;if(fault==6)dbc->MapID=1511;if(fault==7)dbc->ScenarioID=940;if(fault==8)dbc->DifficultyID=1;if(fault==9)dbc->Flags&=~LFG_FLAG_USER_TELEPORT_NOT_ALLOWED;
    if(fault==10)safe->ID=5487;if(fault==11)safe->MapID=1220;if(fault==12)safe->Loc.X+=1;if(fault==13)safe->Loc.Y+=1;if(fault==14)safe->Loc.Z+=1;if(fault==15)safe->Loc.O+=1;if(fault==16)d->x+=1;if(fault==17)d->y+=1;if(fault==18)d->z+=1;if(fault==19)d->o+=1;if(fault==20)d->x=std::numeric_limits<float>::quiet_NaN();if(fault==21)d->o=std::numeric_limits<float>::quiet_NaN();
    Prepare(p,1489,instance);Require(!saved.scenario&&!p.GetScenarioLoginInstance()&&!p.GetScenarioId()&&p.GetExactDist(late)<0.001f&&p.GetQuestStatus(41993)==QUEST_STATUS_INCOMPLETE&&!p.HasItemCount(128825,1,true)&&live->GetCurrentStep()==3,"Invalid provenance/source did not fail closed");++cases;
   }sScenarioMgr->RemoveScenario(instance);
  }
#endif
  // Whole old map is gone; reuse its numeric ID with the original legacy
  // receipt and native1243 constructor, rather than pre-seeding a new stage.
  unsigned oldInstance=9998190+race;ScenarioResume gone=Legacy(oldInstance);gone.boot=GameTime::GetStartTime();
  {MapInstanced oldParent(1489,300);InstanceMap oldMap(1489,300,oldInstance,DIFFICULTY_N_SCENARIO,&oldParent);auto oldScenario=sScenarioMgr->AddScenario(&oldMap,1085);oldScenario->SetCurrentStep(3);{auto originalOwner=std::make_unique<NativePlayer>(session,oldMap,813,late);auto& original=*originalOwner;Priest(original,race);(original.*Access(TuureEntryTag{}))=gone;Require((original.*Access(TuureEntryTag{})).Read(gone.Write()),"Original legacy receipt serialization failed");gone=original.*Access(TuureEntryTag{});}sScenarioMgr->RemoveScenario(oldInstance);}
  Require(!sScenarioMgr->GetScenario(oldInstance),"Original native map lifetime survived");
  {unsigned instance=oldInstance;MapInstanced parent(1489,300);InstanceMap map(1489,300,instance,DIFFICULTY_N_SCENARIO,&parent);{auto pOwner=std::make_unique<NativePlayer>(session,map,813,late);auto& p=*pOwner;Priest(p,race);auto& saved=p.*Access(TuureEntryTag{});Require(saved.Read(gone.Write()),"Destroyed-map legacy receipt roundtrip failed");Prepare(p,1489,instance);Require(!p.GetScenarioLoginInstance()&&p.GetScenarioId()==1085,"Destroyed-map selection adopted recycled ID");auto live=sScenarioMgr->AddScenario(&map,d,&p,false);Require(live&&live->GetScenarioId()==1085&&!live->GetCurrentStep()&&!p.GetScenarioId()&&p.CanResumeScenarioLogin(1085)&&!p.HasItemCount(128825,1,true),"Native fresh constructor failed");
#ifdef TUURE_LEGACY_LOGIN_BASELINE
   Require(std::fabs(p.GetPositionX()+828.85f)<0.001f,"Pinned bad constructor position not reproduced");
#else
   Require(At(p,d),"Native constructor retained old foreign-world position");
#endif
   p.Relocate(late);++cases;}sScenarioMgr->RemoveScenario(instance);}
  session->SetPlayer(nullptr);delete session;
 }
#ifdef TUURE_LEGACY_LOGIN_BASELINE
 Require(cases==50,"Native counterfactual count changed");std::cout<<"COUNTERFACTUAL pinnedfdb24afb8 Tuure Prepare native50:legacy fresh/live/falling pose remains wrong,valid/custom/completed and recycled-ID native constructor controls;old source capsule with current native support,not a historical whole binary"<<std::endl;
#else
 Require(cases==94,"Native repaired count changed");std::cout<<"PASS native Tuure legacy login94:Human+Undead actual26972 pairs/incomplete+complete,real Player/Prepare/LFG/WorldSafe5480/Scenario/serialization,live safe pose retained/live unsafe XY+fall recovered,old boot/numeric mismatch,44 source/provenance refusals,completed cleanup,whole old-map lifetime+native fresh constructor;no character DB mutation or quest/item/criteria grant. Earlier quest/pose and source fault endpoints controlled;full LoadFromDB/group/client separate"<<std::endl;
#endif
 CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}
