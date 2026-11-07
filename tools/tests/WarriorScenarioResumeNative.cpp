// Native class routing and entrance policy; earlier scenario step is controlled.
#define ALTHEA_INTRO_NATIVE_SUPPORT_ONLY
#include "AltheaIntroNative.cpp"
#include "LFGMgr.h"
#include "AreaTriggerData.h"
#include <limits>
struct WarriorResumeTag { using Type=ScenarioResume Player::*;friend Type Access(WarriorResumeTag); };
template struct FixtureAccess<WarriorResumeTag,&Player::m_scenarioResume>;
struct WarriorTargetsTag {using Type=SpellTargetPositionMap SpellMgr::*;friend Type Access(WarriorTargetsTag);};
template struct FixtureAccess<WarriorTargetsTag,&SpellMgr::mSpellTargetPositions>;
struct ResumeRoute { uint32 map,scenario,quest,spell,dungeon,foreign;Position chamber; };
int main(int argc,char** argv){try{
 InitializeAltheaGuardsNative(argc,argv);sAreaTriggerDataStore->LoadAreaTriggerTeleports();sLFGMgr->LoadLFGDungeons();sSpellMgr->LoadSpellTargetPositions();
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1511,{}},{1539,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1511,{}},{1539,{}}});
 unsigned cases=0;
 for(auto route:{ResumeRoute{1511,944,40043,193335,1178,973,{3390,1660,0,0}},ResumeRoute{1539,1037,41105,203920,1249,1092,{2200,2440,85,0}}})for(unsigned race:{unsigned(RACE_HUMAN),unsigned(RACE_ORC)}){
  auto dungeon=sLFGMgr->GetLFGDungeon(route.dungeon);auto target=sSpellMgr->GetSpellTargetPosition(route.spell);auto spell=sSpellMgr->GetSpellInfo(route.spell);
  Require(dungeon&&dungeon->dbc&&dungeon->map==route.map&&dungeon->dbc->ScenarioID==route.foreign&&target&&target->target_mapId==route.map&&spell&&spell->GetEffect(EFFECT_1)->Effect==227,"Native shared route/entry spell changed");
  auto session=Session();ScenarioResume saved;unsigned instanceId=9999800+cases;
  {
   MapInstanced parent(route.map,300);InstanceMap map(route.map,300,instanceId,DIFFICULTY_N_SCENARIO,&parent);auto scenario=sScenarioMgr->AddScenario(&map,route.scenario);scenario->SetCurrentStep(3);
   {
    NativePlayer player(session,map,901,route.chamber);player.SetByteValue(UNIT_FIELD_BYTES_0,0,race);player.setFactionForRace(race);player.SetQuestStatus(route.quest,QUEST_STATUS_INCOMPLETE);player.SetQuestSlot(0,route.quest);player.RecordScenarioEntrance();saved=player.*Access(WarriorResumeTag{});
    std::cout<<"CONTROL native Warrior route="<<route.scenario<<" race="<<race<<" sharedLFG="<<dungeon->id<<" foreign="<<dungeon->dbc->ScenarioID<<" LFGxyz="<<dungeon->x<<','<<dungeon->y<<','<<dungeon->z<<" nativeSpell="<<route.spell<<" name="<<spell->SpellName<<" target="<<target->target_X<<','<<target->target_Y<<','<<target->target_Z<<std::endl;
#ifdef WARRIOR_LOGIN_BASELINE
    Require(!saved.scenario&&!dungeon->x&&!dungeon->y&&!dungeon->z,"Pre-fix native missing checkpoint did not reproduce;baseline requires original game.lib");std::cout<<"COUNTERFACTUAL shared Warrior entrance="<<route.scenario<<" race="<<race<<": original RecordScenarioEntrance cannot record legacy late run;native LFG belongs to another class. Expected broken,not gameplay success"<<std::endl;
#else
    Require(saved.map==route.map&&saved.scenario==route.scenario&&fabs(saved.x-target->target_X)<0.001f&&fabs(saved.y-target->target_Y)<0.001f&&fabs(saved.z-target->target_Z)<0.001f,"Native class-specific Warrior entrance was not recorded");
#endif
   }
#ifndef WARRIOR_LOGIN_BASELINE
   for(unsigned mode=0;mode<3;++mode){NativePlayer incoming(session,map,901,route.chamber);incoming.SetByteValue(UNIT_FIELD_BYTES_0,0,race);incoming.setFactionForRace(race);incoming.SetQuestStatus(route.quest,QUEST_STATUS_INCOMPLETE);Require((incoming.*Access(WarriorResumeTag{})).Read(saved.Write()),"Native checkpoint serialization failed");if(mode==1)(incoming.*Access(WarriorResumeTag{})).boot=saved.boot+1;incoming.PrepareScenarioLogin(route.map,instanceId+(mode==2));if(!mode)Require(incoming.GetScenarioLoginInstance()==instanceId&&incoming.GetExactDist(route.chamber)<0.001f,"Native live instance lost real position");else Require(!incoming.GetScenarioLoginInstance()&&incoming.GetScenarioId()==route.scenario&&incoming.GetExactDist(saved.x,saved.y,saved.z)<0.001f,"Native fresh recovery lost class entry");Require(scenario->GetCurrentStep()==3,"Login changed old scenario progress");incoming.Relocate(route.chamber);++cases;}
#endif
   #ifndef WARRIOR_LOGIN_BASELINE
   auto shared=const_cast<lfg::LFGDungeonData*>(dungeon);float dx=shared->x,dy=shared->y,dz=shared->z,orientation=shared->o;shared->x=111;shared->y=222;shared->z=33;shared->o=1;
   auto& targets=sSpellMgr->*Access(WarriorTargetsTag{});auto originalTarget=targets.at(route.spell);
   {NativePlayer p(session,map,901,route.chamber);p.SetByteValue(UNIT_FIELD_BYTES_0,0,race);p.setFactionForRace(race);p.RecordScenarioEntrance();auto receipt=p.*Access(WarriorResumeTag{});Require(receipt.scenario==route.scenario&&receipt.x==originalTarget.target_X&&p.GetExactDist(route.chamber)<0.001f,"Valid foreign LFG overrode Warrior native entry or moved player");++cases;}
   for(unsigned fault=0;fault<6;++fault){
    if(fault==0)targets.erase(route.spell);else{auto& point=targets.at(route.spell);if(fault==1)point.target_mapId=1;if(fault==2)point.target_X=point.target_Y=point.target_Z=0;if(fault==3)point.target_X=std::numeric_limits<float>::quiet_NaN();if(fault==4)point.target_Orientation=std::numeric_limits<float>::infinity();if(fault==5)point.target_X=20000;}
    {NativePlayer p(session,map,901,route.chamber);p.SetByteValue(UNIT_FIELD_BYTES_0,0,race);p.setFactionForRace(race);p.RecordScenarioEntrance();Require(!(p.*Access(WarriorResumeTag{})).scenario&&p.GetExactDist(route.chamber)<0.001f&&scenario->GetCurrentStep()==3,"Invalid native entry fell back to foreign LFG,relocated or granted progress");++cases;}
    targets.insert_or_assign(route.spell,originalTarget);
   }
   {NativePlayer p(session,map,901,route.chamber);p.SetByteValue(UNIT_FIELD_BYTES_0,0,race);p.setFactionForRace(race);p.*Access(WarriorResumeTag{})=saved;(p.*Access(WarriorResumeTag{})).x=123;targets.erase(route.spell);p.RecordScenarioEntrance();Require((p.*Access(WarriorResumeTag{})).x==123,"Valid recorded entrance overwritten after native source loss");targets.insert_or_assign(route.spell,originalTarget);++cases;}
   {NativePlayer p(session,map,901,route.chamber);p.SetByteValue(UNIT_FIELD_BYTES_0,0,race);p.setFactionForRace(race);scenario->SetCurrentStep(0);p.RecordScenarioEntrance();Require((p.*Access(WarriorResumeTag{})).x==route.chamber.GetPositionX()&&p.GetExactDist(route.chamber)<0.001f,"Real ordinary first entry changed");scenario->SetCurrentStep(3);++cases;}
   shared->x=dx;shared->y=dy;shared->z=dz;shared->o=orientation;
   #endif
   sScenarioMgr->RemoveScenario(instanceId);
  }
#ifndef WARRIOR_LOGIN_BASELINE
  {
   MapInstanced parent(route.map,300);InstanceMap map(route.map,300,instanceId,DIFFICULTY_N_SCENARIO,&parent);
   {NativePlayer player(session,map,901,route.chamber);player.SetByteValue(UNIT_FIELD_BYTES_0,0,race);player.setFactionForRace(race);player.SetQuestStatus(route.quest,QUEST_STATUS_INCOMPLETE);Require((player.*Access(WarriorResumeTag{})).Read(saved.Write()),"Fresh native checkpoint failed");player.PrepareScenarioLogin(route.map,instanceId);Require(!player.GetScenarioLoginInstance()&&player.GetScenarioId()==route.scenario&&player.GetExactDist(saved.x,saved.y,saved.z)<0.001f,"Actual destroyed map kept stale pose");auto fresh=sScenarioMgr->AddScenario(&map,dungeon,&player,false);Require(fresh&&fresh->GetScenarioId()==route.scenario&&fresh->GetCurrentStep()==0&&!player.GetScenarioId()&&player.CanResumeScenarioLogin(route.scenario),"Native LFG construction used foreign class scenario or lost admission");player.Relocate(route.chamber);++cases;}sScenarioMgr->RemoveScenario(instanceId);
  }
  {
   auto shared=const_cast<lfg::LFGDungeonData*>(dungeon);float dx=shared->x,dy=shared->y,dz=shared->z,orientation=shared->o;shared->x=111;shared->y=222;shared->z=33;shared->o=1;
   MapInstanced parent(route.map,300);InstanceMap map(route.map,300,instanceId+1,DIFFICULTY_N_SCENARIO,&parent);auto foreign=sScenarioMgr->AddScenario(&map,route.foreign);foreign->SetCurrentStep(3);
   {NativePlayer p(session,map,901,route.chamber);auto foreignClass=route.map==1511?CLASS_HUNTER:CLASS_PALADIN;p.SetByteValue(UNIT_FIELD_BYTES_0,1,foreignClass);p.SetByteValue(UNIT_FIELD_BYTES_0,0,foreignClass==CLASS_PALADIN&&race==RACE_ORC?RACE_BLOODELF:race);p.setFactionForRace(p.getRace());p.RecordScenarioEntrance();Require((p.*Access(WarriorResumeTag{})).scenario==route.foreign&&(p.*Access(WarriorResumeTag{})).x==111&&p.GetExactDist(route.chamber)<0.001f,"Foreign class native LFG behavior changed");++cases;}sScenarioMgr->RemoveScenario(instanceId+1);shared->x=dx;shared->y=dy;shared->z=dz;shared->o=orientation;
  }
#else
  ++cases;
#endif
  session->SetPlayer(nullptr);delete session;
 }
 std::cout<<"PASS native shared Warrior login cases="<<cases<<std::endl;CloseAltheaNative();return 0;
}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}
