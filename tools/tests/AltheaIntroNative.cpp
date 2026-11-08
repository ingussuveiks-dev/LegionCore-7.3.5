// Real native Players/Scenario/Conversation and whole production Althea class.
// Earlier quest state, LOS invocation, guard attack endpoint and clock are controlled.
#define ALTHEA_CREDIT_NATIVE_SUPPORT_ONLY
#include "AltheaCreditNative.cpp"
#include "MoveSpline.h"
#include "MapManager.h"
#include "../../src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/KingslayersGuardProgress.h"
#include "KingslayersInstance.inc"
#include "AltheaIntro.inc"
static uint64 IntroCounter(Scenario* scenario){auto rows=scenario->GetAchievementMgr().GetCriteriaProgressMap();auto found=rows->find(48577);return found==rows->end()?0:found->second.Counter;}
struct IntroGuardAI:NullCreatureAI{using NullCreatureAI::NullCreatureAI;uint32 attacks=0;ObjectGuid target;void AttackStart(Unit* who)override{++attacks;target=who->GetGUID();}};
static uint32 Stories(Map& map,uint32 id){uint32 count=0;for(auto story:*Access(StoriesTag{}))if(story&&story->GetMap()==&map&&story->GetEntry()==id)++count;return count;}
struct AltheaGuardDbTag{using Type=uint64 Creature::*;friend Type Access(AltheaGuardDbTag);};
template struct FixtureAccess<AltheaGuardDbTag,&Creature::m_DBTableGuid>;
struct Guard:Actor{using Actor::Actor;void Bind(uint32 spawn){this->*Access(AltheaGuardDbTag{})=spawn;}void AddToWorld()override{Actor::AddToWorld();Require(AIM_Initialize(new IntroGuardAI(this)),"Native observed guard AI missing");if(auto script=GetInstanceScript())script->OnCreatureCreateForScript(this);}void RemoveFromWorld()override{if(auto script=GetInstanceScript())script->OnCreatureRemoveForScript(this);Actor::RemoveFromWorld();}IntroGuardAI* Observed(){return static_cast<IntroGuardAI*>(AI());}};
static void InitializeAltheaGuardsNative(int argc,char** argv){
 InitializeAltheaNative(argc,argv);
 // Use the actual native loot links when Unit::Kill dispatches these guards.
 std::string loot="108218";for(uint32 id:{108216u,108217u}){auto info=sObjectMgr->GetCreatureTemplate(id);Require(info,"Native guard template missing");if(info->lootid)loot+=","+std::to_string(info->lootid);}
 // LoadLootTable clears its store: reload the UNION, retaining Althea's loot.
 auto& lootName=LootTemplates_Creature.*Access(LootNameTag{});auto originalName=lootName;lootName+=" WHERE Entry IN("+loot+") ORDER BY Entry,Item";auto loaded=(LootTemplates_Creature.*Access(LootLoadTag{}))();lootName=originalName;Require(loaded&&LootTemplates_Creature.HaveLootFor(108218),"Actual encounter loot union failed");
 for(uint32 id:{108216u,108217u}){auto info=sObjectMgr->GetCreatureTemplate(id);Require(!info->lootid||LootTemplates_Creature.HaveLootFor(info->lootid),"Actual native guard loot absent");}
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new instance_kingslayers;sScriptMgr->SwapScriptContext(true);
}
#ifndef ALTHEA_INTRO_NATIVE_SUPPORT_ONLY
int main(int argc,char** argv){try{
 InitializeAltheaGuardsNative(argc,argv);
 uint32 cases=0;
#ifdef ALTHEA_INTRO_FAULTS
 std::vector<uint32> modes={30,31,32,33};
#else
 std::vector<uint32> modes={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21};
#endif
 for(uint32 quest:{42504u,42627u})for(uint32 mode:modes){
  MapInstanced parent(1620,300);InstanceMap map(1620,300,9995000+cases++,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);Require(dynamic_cast<instance_kingslayers::instance_kingslayers_InstanceMapScript*>(map.GetInstanceScript()),"Actual registered Kingslayers instance factory missing");auto scenario=sScenarioMgr->AddScenario(&map,1123);scenario->SetCurrentStep(4);auto first=Session(),second=Session();
  {
   Position position{-8607.59f,394.141f,110.255f,0};Position visitor=position;visitor.m_positionX+=mode<12?50:30;
   NativePlayer player(first,map,101,visitor),gm(second,map,102,visitor);for(auto p:{&player,&gm}){p->SetByteValue(UNIT_FIELD_BYTES_0,0,quest==42504?RACE_HUMAN:RACE_ORC);p->SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_ROGUE);p->setFactionForRace(p->getRace());p->SetQuestStatus(quest,QUEST_STATUS_INCOMPLETE);p->SetQuestSlot(0,quest);}gm.SetGameMaster(true);
   Actor source(&map,108218,17001,position);std::unique_ptr<Actor> garona;
   auto addGarona=[&]{garona=std::make_unique<Actor>(&map,108214,17002,Position{-8643.02f,421.559f,102.051f,0});Require(map.AddToMap(static_cast<Creature*>(garona.get())),"Original Garona admission failed");};
   Require(map.AddToMap(static_cast<Creature*>(&source)),"Original Althea admission failed");if(mode!=30)addGarona();
   Guard a(&map,108216,17003,{-8616.6f,409.425f,103.058f,0}),b(&map,108216,17004,{-8620.01f,406.714f,103.058f,0}),c(&map,108217,17005,{-8616.99f,397.823f,103.011f,0});
   a.Bind(360869);b.Bind(360872);c.Bind(360871);
   for(auto g:{&a,&b,&c})Require(map.AddToMap(static_cast<Creature*>(g)),"Native original guard admission failed");
   auto ai=new npc_sister_althea_ebonlocke::npc_sister_althea_ebonlockeAI(&source);Require(source.AIM_Initialize(ai),"Complete native Althea AI missing");
   source.AddAura(214873,&source);Require(source.HasAura(214873),"Native release aura control missing");
   std::unique_ptr<CreditParty> party;if(mode==9){gm.SetGameMaster(false);party=std::make_unique<CreditParty>(&player,&gm);}
   if(mode==0){ai->MoveInLineOfSight(&gm);Require(!ai->introDone&&!ai->introDone2&&!IntroCounter(scenario),"GM consumed native intro");}
   if(mode==1){CreditPet pet(player,map);ai->MoveInLineOfSight(&pet);ai->MoveInLineOfSight(garona.get());ai->MoveInLineOfSight(nullptr);Require(!ai->introDone&&!ai->introDone2,"Non-Player consumed intro");}
   if(mode==2){player.SetQuestStatus(quest,QUEST_STATUS_NONE);ai->MoveInLineOfSight(&player);Require(!ai->introDone,"No-quest visitor consumed intro");player.SetQuestStatus(quest,QUEST_STATUS_INCOMPLETE);}
   if(mode==3){player.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_WARRIOR);ai->MoveInLineOfSight(&player);Require(!ai->introDone,"Wrong class consumed intro");player.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_ROGUE);}
   if(mode==4){player.Unit::setDeathState(CORPSE);ai->MoveInLineOfSight(&player);Require(!ai->introDone,"Dead visitor consumed intro");player.Unit::setDeathState(ALIVE);player.SetHealth(player.GetMaxHealth());}
   if(mode==5){player.SetPhaseMask(2,false);ai->MoveInLineOfSight(&player);Require(!ai->introDone,"Wrong phase consumed intro");player.SetPhaseMask(1,false);}
   if(mode==6){scenario->SetCurrentStep(3);ai->MoveInLineOfSight(&player);Require(!ai->introDone,"Wrong native stage consumed intro");scenario->SetCurrentStep(4);}
   if(mode==7){player.Relocate(position.GetPositionX()+56,position.GetPositionY(),position.GetPositionZ(),0);ai->MoveInLineOfSight(&player);Require(!ai->introDone,"Outside55 visitor consumed intro");player.Relocate(visitor);}
   if(mode==8)player.SetQuestStatus(quest,QUEST_STATUS_COMPLETE);
   if(mode==10)player.UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,51549);
   std::unique_ptr<CreditPet> pet;std::unique_ptr<CreditGuardian> guardian;
   if(mode==18)pet=std::make_unique<CreditPet>(player,map);if(mode==19)guardian=std::make_unique<CreditGuardian>(player,map);
   auto lines=mode==31||mode==32?const_cast<std::vector<ConversationData>*>(sConversationDataStore->GetConversationData(uint32(mode==31?2424:2425))):nullptr;std::vector<ConversationData> saved;
   if(lines){Require(!lines->empty(),"Native fault source data missing");saved=*lines;lines->clear();}
   auto criterion=mode==33?const_cast<CriteriaEntry*>(sCriteriaStore.LookupEntry(30469)):nullptr;uint32 asset=criterion?criterion->Asset:0;if(criterion)criterion->Asset=UINT32_MAX;
   ai->MoveInLineOfSight(&player);ai->MoveInLineOfSight(&player);
   if(mode==33){Require(!ai->introDone&&!ai->introDone2&&!IntroCounter(scenario)&&ai->introCreditRetry,"Rejected native criterion consumed intro");criterion->Asset=asset;ai->UpdateIntro(1000);ai->MoveInLineOfSight(&player);}
   Require(ai->introDone&&IntroCounter(scenario)==1&&scenario->GetCurrentStep()==5&&!Counter(scenario),"Native first intro receipt/independent victory differs");
   if(mode==11){ai->Reset();ai->MoveInLineOfSight(&player);Require(ai->introDone&&IntroCounter(scenario)==1&&Stories(map,2422)==1,"Native Reset replayed receipt/first conversation");}
   if(mode==30){Require(!ai->introDone2&&!ai->introVoices[0]&&ai->introRetries[0],"Missing mandatory companion consumed handoff or failed voice retry");addGarona();ai->UpdateIntro(1000);ai->MoveInLineOfSight(&player);}
   Require(mode==31||ai->introVoices[0],"Native original2422 not accepted");
   if(mode<12){Require(!ai->introDone2&&Stories(map,2422)==1,"50-yard control consumed35-yard stage or duplicated scene");player.Relocate(position.GetPositionX()+30,position.GetPositionY(),position.GetPositionZ(),0);}
   ai->MoveInLineOfSight(&player);if(mode==9)ai->MoveInLineOfSight(&gm);
   Require(ai->introDone2&&ai->checkdies&&a.Observed()->attacks==1&&b.Observed()->attacks==1&&c.Observed()->attacks==1&&a.Observed()->target==garona->GetGUID(),"Original native guard attack requests missing/replayed");
   if(mode==31){Require(!ai->introVoices[1]&&ai->introRetries[1],"Failed2424 lost presentation retry");*lines=saved;ai->UpdateIntro(1000);ai->MoveInLineOfSight(&player);}
   Require(ai->introVoices[1]&&Stories(map,2424)==1,"Native2424 accepted scene missing/replayed");
   ai->UpdateIntro(1000);Require(ai->checkdies&&!ai->introReleased&&source.HasAura(214873),"Live guards released Althea");
   Guard* guards[3]={&a,&b,&c};uint32 order[6][3]={{0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0}};auto sequence=order[mode>=12&&mode<18?mode-12:0];
   for(uint32 at=0;at<3;++at){Guard* guard=guards[sequence[at]];Unit* killer=&player;if(at==2){if(pet)killer=pet.get();if(guardian)killer=guardian.get();if(mode==20)killer=garona.get();}
    guard->SetLootRecipient(&player);guard->LowerPlayerDamageReq(uint32(guard->GetMaxHealth()));
    if(mode==17&&at==2){killer->DealDamage(guard,UINT32_MAX-4096,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);Require(!guard->IsAlive(),"Native one-hit guard damage did not reach death");}else killer->Kill(guard,false);
    killer->Kill(guard,false);Require(!guard->IsAlive(),"Native guard Kill failed");
    if(at==2&&mode==21)player.SetQuestStatus(quest,QUEST_STATUS_NONE);
    ai->UpdateIntro(1000);if(at<2||mode==21)Require(ai->checkdies&&!ai->introReleased&&source.getVictim()==nullptr,"Early/foreign-only native guard release");
   }
   if(mode==21){player.SetQuestStatus(quest,QUEST_STATUS_INCOMPLETE);ai->UpdateIntro(1000);}
   Require(ai->introReleased&&!ai->checkdies&&!source.HasAura(214873)&&(source.getVictim()==&player||(mode==9&&source.getVictim()==&gm)),"Native release did not select actual eligible Rogue over GM");auto spline=source.movespline->GetId();
   if(mode==32){Require(!ai->introVoices[2]&&ai->introRetries[2],"Failed2425 blocked combat or lost retry");*lines=saved;ai->UpdateIntro(1000);Require(source.movespline->GetId()==spline,"Optional voice retry restarted jump");}
   Require(ai->introVoices[2]&&Stories(map,2425)==1,"Native2425 missing/replayed");ai->MoveInLineOfSight(&player);ai->UpdateIntro(1000);Require(IntroCounter(scenario)==1&&Stories(map,2422)==1&&Stories(map,2424)==1&&Stories(map,2425)==1,"Repeated full intro replayed progress/scenes");
   if(mode==18||mode==19||mode==20){source.SetLootRecipient(&player);source.LowerPlayerDamageReq(uint32(source.GetMaxHealth()));Unit* killer=pet?static_cast<Unit*>(pet.get()):guardian?static_cast<Unit*>(guardian.get()):static_cast<Unit*>(garona.get());killer->Kill(&source,false);Require(Counter(scenario)==1&&IntroCounter(scenario)==1&&scenario->GetCurrentStep()==5&&!scenario->GetAchievementMgr().GetCriteriaProgressMap()->count(48597),"Real Althea owner/companion death lost independent victory or fabricated door credit");}
   std::cout<<"CASE native intro quest="<<quest<<" mode="<<mode<<" 51549="<<IntroCounter(scenario)<<" qualified release and native2422/2424/2425 accepted"<<std::endl;
   party.reset();CleanStories(map);
  }
  first->SetPlayer(nullptr);second->SetPlayer(nullptr);delete first;delete second;sScenarioMgr->RemoveScenario(map.GetInstanceId());
 }
 #ifdef ALTHEA_INTRO_FAULTS
 std::cout<<"PASS "<<cases<<" explicit native fault/recovery cases for real51549 and2422/2424/2425 endpoints; diagnostic WARNs expected and individually reviewed, progression/guard release independent of presentation. Not a clean ordinary run or full combat/client traversal\n";
 #else
 std::cout<<"PASS "<<cases<<" native complete-source Althea intro cases; actual criteria/Conversation, real guard deaths/all6 orders/owner-pet-guardian-NPC final blows, one-hit damage, Reset and qualified release; original movement/native scripts retained. Earlier quest state, direct LOS, guard attack observer and clock controlled; no client/full combat/scaling/traversal claim\n";
 #endif
 std::cout.flush();CloseAltheaNative();
}catch(std::exception const& error){std::cerr<<error.what()<<'\n';return 1;}}
#endif
