// Native instance factory/lifecycle/death receipts and actual complete consumer.
#define ALTHEA_INTRO_NATIVE_SUPPORT_ONLY
#include "AltheaIntroNative.cpp"
int main(int argc,char** argv){try{
 InitializeAltheaGuardsNative(argc,argv);uint32 cases=0;
 for(uint32 quest:{42504u,42627u})for(uint32 mode=0;mode<13;++mode){
  // Deliberately recycle the numeric instance ID after full destruction. State
  // belongs to the instance object, not a static map keyed by recycled IDs.
  MapInstanced parent(1620,300);InstanceMap map(1620,300,9993000,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);
  auto data=dynamic_cast<instance_kingslayers::instance_kingslayers_InstanceMapScript*>(map.GetInstanceScript());Require(data&&!data->GetData(KingslayersGuards::AllDefeated),"Fresh registered instance inherited old guard receipts");
  auto scenario=sScenarioMgr->AddScenario(&map,1123);scenario->SetCurrentStep(4);auto first=Session(),second=Session();
  {
   NativePlayer player(first,map,101,{-8630,410,103,0}),gm(second,map,102,player.GetPosition());for(auto p:{&player,&gm}){p->SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_ROGUE);p->SetQuestStatus(quest,QUEST_STATUS_INCOMPLETE);p->SetQuestSlot(0,quest);}gm.SetGameMaster(true);
   Actor source(&map,108218,19001,{-8607.59f,394.141f,110.255f,0}),garona(&map,108214,19002,{-8643.02f,421.559f,102.051f,0});
   Require(map.AddToMap(static_cast<Creature*>(&source))&&map.AddToMap(static_cast<Creature*>(&garona)),"Native original source/ally admission failed");
   Guard a(&map,108216,19003,{-8616.6f,409.425f,103.058f,0}),b(&map,108216,19004,{-8620.01f,406.714f,103.058f,0}),c(&map,108217,19005,{-8616.99f,397.823f,103.011f,0});a.Bind(360869);b.Bind(360872);c.Bind(360871);
   for(auto guard:{&a,&b,&c})Require(map.AddToMap(static_cast<Creature*>(guard)),"Native authored guard admission failed");
   for(auto guard:{&a,&b,&c})Require(data->GetGuidData(uint32(guard->GetDBTableGUIDLow()))==guard->GetGUID(),"Native full-GUID guard registration failed");
   auto ai=new npc_sister_althea_ebonlocke::npc_sister_althea_ebonlockeAI(&source);Require(source.AIM_Initialize(ai),"Actual Althea consumer AI failed");source.AddAura(214873,&source);Require(source.HasAura(214873),"Native release-aura control missing");
   auto kill=[&](Guard& guard,Unit* killer){Require(guard.IsInWorld()&&guard.GetMap()==&map,"Native guard must own its map before Kill");guard.SetLootRecipient(&player);guard.LowerPlayerDamageReq(uint32(guard.GetMaxHealth()));killer->Kill(&guard,false);killer->Kill(&guard,false);Require(!guard.IsAlive()&&data->GetData(uint32(guard.GetDBTableGUIDLow())),"Native Unit::Kill did not deliver exact instance guard receipt");};
   std::unique_ptr<Guard> replacement;std::unique_ptr<Actor> unrelated;Guard* last=&c;
   if(mode==6){unrelated=std::make_unique<Actor>(&map,108216,19007,source.GetPosition());unrelated.get()->*Access(AltheaGuardDbTag{})=555555;Require(map.AddToMap(static_cast<Creature*>(unrelated.get())),"Native unrelated duplicate missing");data->OnCreatureCreateForScript(unrelated.get());}
   if(mode==7){map.RemoveFromMap(static_cast<Creature*>(&c),false);ai->MoveInLineOfSight(&player);Require(ai->introDone&&!ai->introDone2&&!ai->checkdies,"Missing living guard consumed mandatory handoff");c.SetMap(&map);Require(map.AddToMap(static_cast<Creature*>(&c)),"Native missing guard recovery failed");}
   if(mode==8){kill(a,&player);kill(b,&player);kill(c,&player);for(auto guard:{&a,&b,&c})map.RemoveFromMap(static_cast<Creature*>(guard),false);}
   ai->MoveInLineOfSight(&player);Require(ai->introDone&&ai->introDone2&&ai->checkdies&&IntroCounter(scenario)==1,"Native intro/guard handoff prerequisites failed");
   if(mode==0){for(auto guard:{&a,&b,&c}){guard->NearTeleportTo(garona.GetPositionX(),garona.GetPositionY(),garona.GetPositionZ(),0);map.MoveAllCreaturesInMoveList();Require(guard->IsAlive()&&source.GetExactDist(guard)>40,"Live moved guard endpoint missing");}ai->UpdateIntro(1000);Require(ai->checkdies&&!ai->introReleased&&source.HasAura(214873),"Live outside40 guard incorrectly released boss");}
   if(mode==9){kill(a,&player);a.Unit::setDeathState(ALIVE);a.SetHealth(a.GetMaxHealth());Require(!data->GetData(360869),"Same-GUID living respawn reused old death");map.RemoveFromMap(static_cast<Creature*>(&a),false);Require(!data->GetData(360869),"Live removal restored old death");a.SetMap(&map);Require(map.AddToMap(static_cast<Creature*>(&a)),"Native same-GUID guard reentry failed");}
   if(mode==10){map.RemoveFromMap(static_cast<Creature*>(&c),false);replacement=std::make_unique<Guard>(&map,108217,19006,c.GetPosition());replacement->Bind(360871);Require(map.AddToMap(static_cast<Creature*>(replacement.get())),"Native replacement guard missing");last=replacement.get();c.SetMap(&map);c.Unit::setDeathState(CORPSE);last->Unit::setDeathState(CORPSE);data->CreatureDiesForScript(&c,&player);Require(!data->GetData(360871)&&data->GetGuidData(360871)==last->GetGUID(),"Stale full GUID delivered replacement death");last->Unit::setDeathState(ALIVE);last->SetHealth(last->GetMaxHealth());}
   std::unique_ptr<CreditPet> pet;std::unique_ptr<CreditGuardian> guardian;if(mode==1)pet=std::make_unique<CreditPet>(player,map);if(mode==2)guardian=std::make_unique<CreditGuardian>(player,map);
   UnitAI* retiredSource=nullptr;
   if(mode==4){retiredSource=source.GetAI();source.IsAIEnabled=false;source.SetAI(nullptr);}
   if(mode!=8){kill(a,&player);kill(b,&player);
    if(mode==1){last->NearTeleportTo(garona.GetPositionX(),garona.GetPositionY(),garona.GetPositionZ(),0);map.MoveAllCreaturesInMoveList();ai->UpdateIntro(1000);Require(last->IsAlive()&&ai->checkdies&&!ai->introReleased,"Last distant live guard released boss");}
    if(mode==2){map.RemoveFromMap(static_cast<Creature*>(last),false);ai->UpdateIntro(1000);Require(last->IsAlive()&&ai->checkdies&&!ai->introReleased,"Missing living guard counted as death");last->SetMap(&map);Require(map.AddToMap(static_cast<Creature*>(last)),"Native live missing-guard reentry failed");}
    UnitAI* retiredGuard=nullptr;if(mode==5){retiredGuard=last->GetAI();last->IsAIEnabled=false;last->SetAI(nullptr);}
    Unit* killer=pet?static_cast<Unit*>(pet.get()):guardian?static_cast<Unit*>(guardian.get()):mode==3?static_cast<Unit*>(&garona):static_cast<Unit*>(&player);kill(*last,killer);
    if(retiredGuard){last->SetAI(retiredGuard);last->IsAIEnabled=true;}
   }
   Require(data->GetData(KingslayersGuards::AllDefeated),"Three actual native deaths did not satisfy instance receipt");
   if(mode==3)for(auto guard:{&a,&b,last})map.RemoveFromMap(static_cast<Creature*>(guard),false);
   if(mode==4){source.SetAI(retiredSource);source.IsAIEnabled=true;CleanStories(map);ai=new npc_sister_althea_ebonlocke::npc_sister_althea_ebonlockeAI(&source);Require(source.AIM_Initialize(ai),"Native source AI recreation failed");ai->MoveInLineOfSight(&player);Require(ai->introDone2&&data->GetData(KingslayersGuards::AllDefeated),"AI recreation lost instance-owned death receipts");}
   if(mode==11){player.SetQuestStatus(quest,QUEST_STATUS_NONE);ai->UpdateIntro(1000);Require(ai->checkdies&&!ai->introReleased&&source.HasAura(214873),"No eligible owner incorrectly consumed valid death checkpoint");player.SetQuestStatus(quest,QUEST_STATUS_INCOMPLETE);}
   ai->UpdateIntro(1000);Require(ai->introReleased&&!ai->checkdies&&!source.HasAura(214873)&&source.getVictim()==&player,"Actual guard deaths failed qualified native release");
   if(unrelated)Require(unrelated->IsAlive(),"Unrelated copy must not be killed or required for original guard receipt");
   auto spline=source.movespline->GetId();ai->UpdateIntro(1000);Require(source.movespline->GetId()==spline&&IntroCounter(scenario)==1&&!Counter(scenario)&&scenario->GetCurrentStep()==5&&!scenario->GetAchievementMgr().GetCriteriaProgressMap()->count(48597),"Repeated guard checkpoint replayed movement/progress or granted victory/door");
   std::cout<<"CASE native exact guard receipts quest="<<quest<<" mode="<<mode<<" three actual deaths, qualified release once, no victory/door grant\n";CleanStories(map);
  }
  first->SetPlayer(nullptr);second->SetPlayer(nullptr);delete first;delete second;sScenarioMgr->RemoveScenario(map.GetInstanceId());++cases;
 }
 std::cout<<"PASS "<<cases<<" native registered instance/whole Althea guard cases: living moved/missing guards never release; actual three Unit::Kill receipts survive corpse cleanup and source/guard AI retirement, pet/guardian/Garona final blows, pre-intro kills, respawn/replacement/stale GUID, unrelated actor and recycled instance isolation. Registration endpoint/guard attack observer, relocation, respawn/stale-callback endpoints and clocks controlled; no full client/combat traversal claim\n";std::cout.flush();CloseAltheaNative();
}catch(std::exception const& error){std::cerr<<error.what()<<'\n';return 1;}}
