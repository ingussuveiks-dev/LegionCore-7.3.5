// Whole registered native1539 instance; downstream ritual AI action endpoint is observed.
#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
#include "MapManager.h"
#include "../../src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/silver_hand_first_fight.h"
#include "../../src/server/scripts/Scenario/Artifacts/Priest/slaghammer_handoff.h"
#include "SilverHandInstance.inc"
struct ScopeObserver:NullCreatureAI{using NullCreatureAI::NullCreatureAI;uint32 actions=0;void DoAction(int32 action)override{Require(action==1,"Unexpected native ritual action");++actions;}};
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);sCreatureTextMgr->LoadCreatureTexts();
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new instance_silver_hand;sScriptMgr->SwapScriptContext(true);
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1539,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1539,{}}});uint32 cases=0;
#ifdef SILVER_HAND_INSTANCE_BASELINE
 std::vector<uint32> scenarios{991,1037,1092};uint32 styles=1;
#else
 std::vector<uint32> scenarios{991,1037,1092,0};uint32 styles=3;
#endif
 for(uint32 race:{uint32(RACE_HUMAN),uint32(RACE_BLOODELF)})for(uint32 id:scenarios)for(uint32 step:{8u,10u})for(uint32 style=0;style<styles;++style){
  if(id==1092&&style)continue; // Genuine missing-actor warnings are retained/tested by the ritual regression.
  MapInstanced parent(1539,300);InstanceMap map(1539,300,9954000,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);auto data=dynamic_cast<instance_silver_hand::instance_silver_hand_InstanceMapScript*>(map.GetInstanceScript());Require(data,"Actual1539 instance factory/binding missing");auto scenario=id?sScenarioMgr->AddScenario(&map,id):nullptr;auto session=Session();
  {Position pose{1920,2335,76,0};std::unique_ptr<NativePlayer> player;if(style!=2){player=std::make_unique<NativePlayer>(session,map,101,pose);player->SetByteValue(UNIT_FIELD_BYTES_0,0,race);player->SetByteValue(UNIT_FIELD_BYTES_0,1,id==991?CLASS_PRIEST:id==1037?CLASS_WARRIOR:CLASS_PALADIN);player->setFactionForRace(race);}
   std::unique_ptr<Actor> third,last,boss;ScopeObserver* observer=nullptr;
   if(style!=1){third=std::make_unique<Actor>(&map,106676,33001,pose);last=std::make_unique<Actor>(&map,106777,33002,pose);boss=std::make_unique<Actor>(&map,106719,33003,pose);
    for(auto actor:{third.get(),last.get(),boss.get()})Require(map.AddToMap(static_cast<Creature*>(actor)),"Native ritual actor grid admission failed");observer=new ScopeObserver(third.get());Require(third->AIM_Initialize(observer),"Native action observer failed");boss->SetUInt32Value(UNIT_FIELD_FLAGS,0);
   }
   // Exercise real Scenario::SetCurrentStep dispatch where that native step exists.
   if(scenario && (step==8||id==1092))scenario->SetCurrentStep(uint8(step));else data->onScenarionNextStep(step);
#ifdef SILVER_HAND_INSTANCE_BASELINE
   bool expected=true;
#else
   bool expected=id==1092;
#endif
   if(step==8){Require((observer?observer->actions:0)==(expected?1u:0u),"Wrong shared-map step8 dispatch");Require(data->_finaleActorsStarted.empty()&&!data->_finaleDiscoveryPending,"Step8 modified finale state");}
   else {Require(data->_finaleActorsStarted.size()==(expected?2u:0u)&&!data->_finaleDiscoveryPending,"Wrong shared-map step10 admission/discovery");if(boss)Require((boss->GetUInt32Value(UNIT_FIELD_FLAGS)!=0)==expected,"Wrong shared-map boss flags");}
   std::cout<<"CASE instance race="<<race<<" scenario="<<id<<" step="<<step<<" style="<<style<<" dispatched="<<expected<<'\n';
  }session->SetPlayer(nullptr);delete session;if(scenario)sScenarioMgr->RemoveScenario(map.GetInstanceId());++cases;
 }
#ifdef SILVER_HAND_INSTANCE_BASELINE
 std::cout<<"COUNTERFACTUAL "<<cases<<" native pinned instance contexts:Priest/Warrior step8 reaches Paladin actor,unguarded step10 starts Paladin finale;1092controls retained. Actor actions observed,native cast/flags and Scenario step8 dispatch real;not full gameplay\n";
#else
 std::cout<<"PASS "<<cases<<" native registered shared1539 contexts:only1092dispatches step8/10;991/1037/absent scenario keep ritual actors/flags/discovery untouched,including absent actors/players. Native full-class factory and step8 transition used;downstream action endpoint observed,not full client traversal\n";
#endif
 CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
