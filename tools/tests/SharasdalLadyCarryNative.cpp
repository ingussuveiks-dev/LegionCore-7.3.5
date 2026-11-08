// Actual native209457 selection/vehicle regression;no encounter threshold/grants.
#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
#include "Vehicle.h"
#include "ConditionMgr.h"
#include <memory>
#include <sstream>
static void ApplyCarryMigration(bool guardOnly=false){
 std::string migration=
#include "SharasdalCarryMigration.inc"
 ;std::istringstream lines(migration);std::string line,sql;
 while(std::getline(lines,line)){auto first=line.find_first_not_of(" \t\r");if(first==std::string::npos||line.compare(first,2,"--")==0)continue;sql+=line+'\n';}
 if(guardOnly){sql.resize(sql.find("SET @sharasdal_carry_guard"));sql+="COMMIT;";}
 std::string statement;bool quote=false;for(char c:sql){if(c=='\'')quote=!quote;if(c==';'&&!quote){WorldDatabase.DirectExecute(statement.c_str());statement.clear();}else statement+=c;}
 auto check=WorldDatabase.Query("SELECT COALESCE(@sharasdal_carry_safe,0),(SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=13 AND SourceEntry=209457)");Require(check&&check->Fetch()[0].GetBool()!=guardOnly&&(guardOnly||check->Fetch()[1].GetUInt32()==6),"Native TEMP carry migration guard/cardinality differs");
}
static void CarryCase(unsigned race,unsigned mode,bool repaired,unsigned instance){
 auto parent=std::make_unique<MapInstanced>(1600,300);auto mapOwner=std::make_unique<InstanceMap>(1600,300,instance,DIFFICULTY_N_SCENARIO,parent.get());auto& map=*mapOwner;auto scenario=sScenarioMgr->AddScenario(&map,mode==7?1123:1066);scenario->SetCurrentStep(mode==8?3:4);auto session=Session();
 {
  auto p=std::make_unique<NativePlayer>(session,map,994,Position{40,803,805,0});p->SetByteValue(UNIT_FIELD_BYTES_0,0,race);p->SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_SHAMAN);p->setFactionForRace(race);p->SetPhaseId({6120},false);p->SetQuestStatus(40341,QUEST_STATUS_INCOMPLETE);p->SetQuestSlot(0,40341);
  auto lady=map.SummonCreature(mode==6?102826:104856,Position{50,803,805,0},nullptr,0);auto grash=map.SummonCreature(mode==12?105251:105298,Position{mode==11?250.f:55.f,803,805,0},nullptr,0);Require(lady&&grash&&grash->GetVehicleKit(),"Actual carry actors/native kit missing");lady->SetPhaseId({6120},false);grash->SetPhaseId({6120},false);
  TempSummon* other=nullptr;TempSummon* occupant=nullptr;TempSummon* second=nullptr;
  if(mode>=1&&mode<=3){other=map.SummonCreature(mode==1?102826:(mode==2?105251:105298),Position{52,803,805,0},nullptr,0);Require(other,"Nearby control missing");other->SetPhaseId({6120},false);}
  if(mode==4||mode==13){occupant=map.SummonCreature(104856,Position{54,803,805,0},nullptr,0);Require(occupant,"Occupied-seat control missing");occupant->SetPhaseId({6120},false);occupant->EnterVehicle(grash,0);occupant->m_Events.Update(1);Require(grash->GetVehicleKit()->GetPassenger(0)==occupant,"Native seat0 occupation failed");}
  if(mode==13){second=map.SummonCreature(104856,Position{54,804,805,0},nullptr,0);Require(second,"Second occupied-seat control missing");second->SetPhaseId({6120},false);second->EnterVehicle(grash,1);second->m_Events.Update(1);Require(grash->GetVehicleKit()->GetPassenger(0)==occupant&&grash->GetVehicleKit()->GetPassenger(1)==second&&occupant->GetVehicleBase()==grash&&occupant->GetGUID()!=second->GetGUID(),"Actual native full-seat setup failed");}
  if(mode==5)grash->RemoveVehicleKit();if(mode==9)grash->Unit::setDeathState(CORPSE);if(mode==10)grash->SetPhaseMask(2,false);
  auto count=scenario->GetAchievementMgr().GetCriteriaProgressMap()->size();auto stage=scenario->GetCurrentStep();auto hp=lady->GetHealth();auto result=lady->CastSpell(grash,209457);Require(!lady->GetVehicleBase(),"Cast attempt boarded before asynchronous event");lady->m_Events.Update(1);auto carrier=lady->GetVehicleBase();int seat=-1;if(carrier&&carrier->GetVehicleKit())for(int i=0;i<2;++i)if(carrier->GetVehicleKit()->GetPassenger(i)==lady)seat=i;
  std::cout<<(repaired?"CARRY":"COUNTERFACTUAL")<<" race="<<race<<" mode="<<mode<<" result="<<unsigned(result)<<" carrier="<<(carrier?carrier->GetEntry():0)<<" intended="<<(carrier==grash)<<" seat="<<seat<<" selfControl="<<lady->HasAuraEffect(209457,EFFECT_0)<<std::endl;
  if(!repaired)Require(!carrier,"Pinned missing-condition carry no longer reproduces");
  else{
   bool eligible=mode<=4||mode>=14;
   if(eligible){Require(carrier==(mode==3?other:grash)&&seat==(mode==4?1:0),"Native carry target/seat differs");Require(lady->GetTransGUID()==carrier->GetGUID(),"Native carry transport acknowledgement missing");}
   else Require(!carrier,"Invalid carry target/caster/scenario/phase/range/kit/seats accepted");
   if(mode==14){lady->CastSpell(grash,209457);lady->m_Events.Update(1);Require(lady->GetVehicleBase()==grash&&grash->GetVehicleKit()->GetPassenger(0)==lady,"Repeated cast lost/replaced real carry");}
   if(mode==15){lady->ExitVehicle();lady->m_Events.Update(1);Require(!lady->GetVehicleBase()&&!grash->HasAura(209457),"Native exit failed own aura cleanup");Require(lady->CastSpell(grash,209457)==SPELL_CAST_OK,"Native retry cast rejected");lady->m_Events.Update(1);Require(lady->GetVehicleBase()==grash&&grash->GetVehicleKit()->GetPassenger(0)==lady,"Native carry retry after exit failed");}
  }
  if(carrier)lady->ExitVehicle();if(lady->GetVehicleBase())lady->ExitVehicle();if(occupant&&occupant->GetVehicleBase())occupant->ExitVehicle();if(second&&second->GetVehicleBase())second->ExitVehicle();grash->RemoveAura(209457);if(other)other->RemoveAura(209457);lady->RemoveAura(209457);lady->m_Events.Update(1);
  Require(lady->GetHealth()==hp&&scenario->GetCurrentStep()==stage&&scenario->GetAchievementMgr().GetCriteriaProgressMap()->size()==count&&p->GetQuestStatus(40341)==QUEST_STATUS_INCOMPLETE&&!p->HasItemCount(128911,1,true),"Carry changed health/progression/item");
  for(TempSummon* actor:{second,occupant,other,lady,grash})if(actor)actor->UnSummon();map.RemoveAllObjectsInRemoveList();
 }
 sScenarioMgr->RemoveScenario(map.GetInstanceId());session->SetPlayer(nullptr);delete session;
}
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);Require(sObjectMgr->LoadTrinityStrings(),"Native strings missing");WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");Shadow("spell_proc_event","1");Shadow("conditions","SourceTypeOrReferenceId=13 AND SourceEntry=209457");
#ifndef SHARASDAL_CARRY_LIVE
 WorldDatabase.DirectExecute("DELETE FROM conditions");
#endif
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");sSpellMgr->LoadSpellProcEvents();
 auto spell=sSpellMgr->GetSpellInfo(209457);Require(spell&&spell->GetEffect(EFFECT_0)->ApplyAuraName==SPELL_AURA_CONTROL_VEHICLE&&spell->GetEffect(EFFECT_0)->TargetA.GetTarget()==TARGET_UNIT_NEARBY_ENTRY&&!spell->GetEffect(EFFECT_0)->ImplicitTargetConditions,"Actual original carry spell differs");
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1600,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1600,{}}});
 unsigned instance=9999000;
#ifndef SHARASDAL_CARRY_LIVE
 for(unsigned mode=0;mode<6;++mode)CarryCase(RACE_DRAENEI,mode,false,instance++);
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");ApplyCarryMigration();ApplyCarryMigration();
 WorldDatabase.DirectExecute("INSERT INTO conditions VALUES(13,2,209457,0,0,31,0,3,105298,0,0,0,'','Counterfactual guard sentinel')");ApplyCarryMigration(true);WorldDatabase.DirectExecute("DELETE FROM conditions WHERE SourceGroup=2");
 WorldDatabase.DirectExecute("UPDATE conditions SET ConditionValue2=102826 WHERE ConditionTypeOrReference=31 AND ConditionTarget=1");ApplyCarryMigration(true);WorldDatabase.DirectExecute("UPDATE conditions SET ConditionValue2=104856 WHERE ConditionTypeOrReference=31 AND ConditionTarget=1");ApplyCarryMigration();WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
#else
 auto live=WorldDatabase.Query("SELECT COUNT(*) FROM conditions");Require(live&&live->Fetch()[0].GetUInt32()==6,"Live six-row carry scope missing");
#endif
 sConditionMgr->LoadConditions();Require(spell->GetEffect(EFFECT_0)->ImplicitTargetConditions&&spell->GetEffect(EFFECT_0)->ImplicitTargetConditions->size()==6&&!spell->GetEffect(EFFECT_1)->ImplicitTargetConditions,"Native condition loader did not bind only effect0");
 for(unsigned race:{unsigned(RACE_DRAENEI),unsigned(RACE_ORC)})for(unsigned mode=0;mode<16;++mode)CarryCase(race,mode,true,instance++);
#ifdef SHARASDAL_CARRY_LIVE
 std::cout<<"PASS live native Lady carry32:selectively copied actual six World rows/native source13 loader/ordinary209457/native seat/negative and retry controls;no persistent edits. Full living-Lady producer/threshold/dialogue/finisher/client remain OPEN"<<std::endl;
#else
 std::cout<<"PASS native Lady carry32 with paired original6 missing-target counterfactual:real source13 loader/TEMP migration twice/extra-row+changed-caster guard refusal/native209457/nearby Grash/queued seat/other ally+vehicle controls/caster+stage+scenario+life+phase+range/missing kit/occupied seats/repeat/exit-retry;no health/item/progress changes. Native nearest matching Grash/free seat preserved;full living-Lady producer/threshold/dialogue/finisher/client remain OPEN"<<std::endl;
#endif
 CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}
