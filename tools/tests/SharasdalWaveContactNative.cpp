// Native geometric AT contact and asynchronous vehicle acknowledgement investigation.
#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
#include "AreaTriggerData.h"
#include "Vehicle.h"
#include <memory>
namespace SharasdalContactFixture {
#include "SharasdalWave.inc"
}
struct ContactActionsTag {friend auto ContactActions(ContactActionsTag);};
template<auto Member>struct ContactActionsAccess {friend auto ContactActions(ContactActionsTag){return Member;}};
template struct ContactActionsAccess<&AreaTrigger::_actionInfo>;
struct ContactPendingTag {friend auto ContactPending(ContactPendingTag);};
template<auto Member>struct ContactPendingAccess {friend auto ContactPending(ContactPendingTag){return Member;}};
template struct ContactPendingAccess<&Vehicle::_pendingJoinEvents>;
static unsigned Pending(Creature* wave){return wave->GetVehicleKit()?unsigned((wave->GetVehicleKit()->*ContactPending(ContactPendingTag{})).size()):0;}
static unsigned Hits(AreaTrigger* at){return (at->*ContactActions(ContactActionsTag{})).at(1).hitCount;}
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);Require(sObjectMgr->LoadTrinityStrings(),"Native strings absent");
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");Shadow("spell_proc_event","1");
 for(auto name:{"areatrigger_template","areatrigger_data","areatrigger_actions"})Shadow(name,"entry=6547 OR customEntry=11160");
 for(auto name:{"areatrigger_template_circle","areatrigger_template_sequence","areatrigger_polygon","areatrigger_scripts"})Shadow(name,"entry IN(6547,11160)");
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");sSpellMgr->LoadSpellProcEvents();sObjectAccessor->SetGuidSize(HighGuid::AreaTrigger,100000);sAreaTriggerDataStore->LoadAreaTriggerScripts();sAreaTriggerDataStore->LoadAreaTriggerActionsAndData();
 auto data=sAreaTriggerDataStore->GetAreaTriggerInfo(6547);Require(data&&data->spellId==209132&&data->customEntry==11160&&data->HasAttached&&data->actions.size()==1,"Actual attached6547 data changed");auto const& action=data->actions.front();Require(action.id==1&&action.spellId==46598&&action.targetFlags==1056840&&action.hitMaxCount==1&&action.moment==AT_ACTION_MOMENT_ENTER,"Actual wave ride action changed");
 Require(sAreaTriggerDataStore->GetAreaTriggerScriptId(11160)==0,"Load actual existing wave AI before testing");
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1600,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1600,{}}});sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new SharasdalContactFixture::npc_sharasdal_waterwall;sScriptMgr->SwapScriptContext(true);
 unsigned cases=0;
 for(unsigned race:{unsigned(RACE_DRAENEI),unsigned(RACE_ORC)})for(unsigned mode=0;mode<16;++mode){
  auto parent=std::make_unique<MapInstanced>(1600,300);auto mapOwner=std::make_unique<InstanceMap>(1600,300,9999900+race*10+mode,DIFFICULTY_N_SCENARIO,parent.get());auto& map=*mapOwner;auto scenario=sScenarioMgr->AddScenario(&map,mode==9?1123:1066);scenario->SetCurrentStep(3);auto session=Session();auto secondSession=mode==12?Session():nullptr;
  {
   Position pose{-44.86f,802.57f,797.15f,0};Position outside=pose;outside.m_positionX+=25;auto p=std::make_unique<NativePlayer>(session,map,991,outside);p->SetByteValue(UNIT_FIELD_BYTES_0,0,race);p->SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_SHAMAN);p->setFactionForRace(race);p->SetPhaseId({6120},false);p->SetQuestStatus(40341,QUEST_STATUS_INCOMPLETE);p->SetQuestSlot(0,40341);auto raceInfo=sChrRacesStore.LookupEntry(race);Require(raceInfo&&sCreatureDisplayInfoStore.LookupEntry(raceInfo->MaleDisplayID),"Native race model absent");p->SetNativeDisplayId(raceInfo->MaleDisplayID);p->SetDisplayId(raceInfo->MaleDisplayID);Require(p->GetCollisionHeight()>0,"Native model collision absent");
   auto savedData=*data;if(mode==11)const_cast<AreaTriggerInfo*>(data)->actions.front().hitMaxCount=2;
   auto wave=map.SummonCreature(mode==10?105298:105251,pose,nullptr,0);Require(wave&&wave->GetVehicleKit(),"Native wave missing");wave->SetPhaseId({6120},false);Require(wave->CastSpell(wave,209132)==SPELL_CAST_OK,"Ordinary authored209132 rejected");auto at=wave->GetAreaObject(209132);Require(at&&at->IsInWorld()&&at->GetRealEntry()==6547&&at->GetCustomEntry()==11160&&at->GetTargetGuid()==wave->GetGUID(),"Real aura395 did not create attached wave AT");
   bool scoped=mode!=9&&mode!=10&&mode!=11;
   Require(scoped?(at->GetAreaTriggerInfo().updateDelay==1000&&at->GetAreaTriggerInfo().actions.front().moment==(AT_ACTION_MOMENT_ENTER|AT_ACTION_MOMENT_UPDATE_TARGET)):(at->GetAreaTriggerInfo().updateDelay==0&&at->GetAreaTriggerInfo().actions.front().moment==AT_ACTION_MOMENT_ENTER),"Scoped native periodic retry/original foreign metadata differs");
   std::unique_ptr<CreditPet> pet;std::unique_ptr<CreditGuardian> guardian;
   if(mode==13){pet=std::make_unique<CreditPet>(*p,map);pet->SetPhaseId({6120},false);map.CreatureRelocation(pet.get(),pose.GetPositionX()+2,pose.GetPositionY(),pose.GetPositionZ(),0);}
   if(mode==14){guardian=std::make_unique<CreditGuardian>(*p,map);guardian->SetPhaseId({6120},false);map.CreatureRelocation(guardian.get(),pose.GetPositionX()+2,pose.GetPositionY(),pose.GetPositionZ(),0);}
   if(pet||guardian){at->UpdateAffectedList(1,AT_ACTION_MOMENT_ENTER);Require(!Hits(at)&&!Pending(wave)&&!at->IsUnitAffected((pet?static_cast<Unit*>(pet.get()):static_cast<Unit*>(guardian.get()))->GetGUID()),"Pet/guardian consumed Player-only contact");}
   at->UpdateAffectedList(1,AT_ACTION_MOMENT_ENTER);Require(!Hits(at)&&!p->GetVehicleBase(),"Outside player consumed contact");
   bool missingKit=mode==1||(mode>=5&&mode<=8);if(missingKit)wave->RemoveVehicleKit();
   auto contact=[&](bool enter){map.PlayerRelocation(p.get(),pose.GetPositionX()+(enter?2.f:25.f),pose.GetPositionY(),pose.GetPositionZ(),0);at->UpdateAffectedList(1,AT_ACTION_MOMENT_ENTER);};contact(true);
   std::cout<<"CONTACT race="<<race<<" mode="<<mode<<" affected="<<at->IsUnitAffected(p->GetGUID())<<" hit="<<Hits(at)<<" base="<<(p->GetVehicleBase()==wave)<<std::endl;
   Require(at->IsUnitAffected(p->GetGUID())&&Hits(at)==(scoped?0u:1u),"Contact/cast attempt consumed native ride budget before seat acknowledgement");
   std::unique_ptr<NativePlayer> other;
   if(mode==12){other=std::make_unique<NativePlayer>(secondSession,map,992,outside);other->SetPhaseId({6120},false);other->SetNativeDisplayId(raceInfo->MaleDisplayID);other->SetDisplayId(raceInfo->MaleDisplayID);map.PlayerRelocation(other.get(),pose.GetPositionX()+3,pose.GetPositionY(),pose.GetPositionZ(),0);at->UpdateAffectedList(1,AT_ACTION_MOMENT_ENTER);Require(Pending(wave)==1,"Another contact replaced/duplicated pending join");}
   if(!missingKit&&mode!=2){Require(Pending(wave)==1,"Native join request missing");at->ActionOnUpdate(1000);Require(Pending(wave)==1,"Pending join was recast");}
   if(mode==2)wave->RemoveVehicleKit();
   UnitAI* retired=nullptr;if(mode==3){retired=wave->GetAI();wave->IsAIEnabled=false;wave->SetAI(nullptr);}if(mode==4)Require(wave->AIM_Initialize(),"Native AI replacement failed");
   p->m_Events.Update(1);if(retired){wave->SetAI(retired);wave->IsAIEnabled=true;}
   if(mode==4&&!p->GetVehicleBase()){Require(!Hits(at),"AI reset consumed failed join");at->Update(1000);p->m_Events.Update(1);}
   if(missingKit||mode==2){
    Require(!p->GetVehicleBase()&&!Hits(at),"Failed ride consumed budget or boarded");Require(wave->CreateVehicleKit(4717,105251),"Native vehicle restoration failed");
    if(mode>=5&&mode<=8){
     if(mode==5)p->SetPhaseMask(2,false);if(mode==6)p->Unit::setDeathState(CORPSE);if(mode==7)contact(false);if(mode==8)p->SetSemaphoreTeleportNear(true);
     at->Update(1000);p->m_Events.Update(1);Require(!Hits(at)&&!Pending(wave)&&!p->GetVehicleBase(),"Ineligible late contact retried");
     if(mode==5)p->SetPhaseMask(1,false);if(mode==6){p->Unit::setDeathState(ALIVE);p->SetHealth(p->GetMaxHealth());}if(mode==7)contact(true);if(mode==8)p->SetSemaphoreTeleportNear(false);
     at->Update(1000);
    }else{at->Update(999);p->m_Events.Update(1);Require(!Hits(at)&&!p->GetVehicleBase(),"Retry fired before native periodic deadline");at->Update(1);}
    Require(!Hits(at),"Retry cast consumed budget before native seat event");p->m_Events.Update(1);
   }
   Require(wave->GetVehicleKit()->GetPassenger(0)==p.get()&&p->GetVehicleBase()==wave&&Hits(at)==1,"Native seat acknowledgement/recovery missing");
   if(other){other->m_Events.Update(1);Require(!other->GetVehicleBase(),"Pending visitor stole actual seat");}
   p->ExitVehicle();at->ActionOnUpdate(1000);p->m_Events.Update(1);Require(Hits(at)==1&&!p->GetVehicleBase(),"Completed wave replayed ride after dismount");
   if(mode==15){auto previous=at->GetGUID();wave->RemoveAura(209132);at->Update(1);map.RemoveAllObjectsInRemoveList();Require(wave->CastSpell(wave,209132)==SPELL_CAST_OK,"Fresh wave aura rejected");at=wave->GetAreaObject(209132);Require(at&&at->GetGUID()!=previous&&!Hits(at),"New AT inherited prior budget");contact(true);p->m_Events.Update(1);Require(Hits(at)==1&&p->GetVehicleBase()==wave,"Fresh AT did not accept actual new contact");p->ExitVehicle();}
   Require(scenario->GetCurrentStep()==3&&p->GetQuestStatus(40341)==QUEST_STATUS_INCOMPLETE&&!p->HasItemCount(128911,1,true),"Contact changed progression/item");
   wave->RemoveAura(209132);at->Update(1);map.RemoveAllObjectsInRemoveList();wave->UnSummon();map.RemoveAllObjectsInRemoveList();p->m_Events.Update(1);*const_cast<AreaTriggerInfo*>(data)=savedData;++cases;
  }
  sScenarioMgr->RemoveScenario(map.GetInstanceId());session->SetPlayer(nullptr);delete session;if(secondSession){secondSession->SetPlayer(nullptr);delete secondSession;}
 }
 Require(cases==32,"Contact investigation count changed");std::cout<<"PASS native wave contact32:geometric6547/46598 rides/real seat ack,missing-kit/aborted-join stationary recovery at999+1ms,phase/death/exit/teleport deferral,AI retirement/replacement,pet/guardian/visitor controls,new AT lifetime,foreign scenario/actor/custom budget semantics retained,no replay. Physical moving-path/client separate"<<std::endl;CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}
