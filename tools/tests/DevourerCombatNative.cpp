// Real native Conversation/Map/Players/EventMap; earlier quest/stage/pose,
// combat-victim admission, clock, spells and melee endpoints are controlled.
#define ALTHEA_INTRO_NATIVE_SUPPORT_ONLY
#include "AltheaIntroNative.cpp"
#include "DevourerInstance.inc"
static uint32 combatClock=0;
struct CombatProbeBase:ScriptedAI{
 using ScriptedAI::ScriptedAI;std::vector<std::pair<uint32,uint32>> casts;
 bool UpdateVictim(){return me->isInCombat();}
 void DoCast(uint32 id){casts.emplace_back(combatClock,id);} // Explicitly no native combat effects.
 void DoMeleeAttackIfReady(){}
};
#include "Entrance.inc"
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1620,{}},{1607,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1620,{}},{1607,{}}});
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new instance_devourer;sScriptMgr->SwapScriptContext(true);uint32 cases=0;
 for(uint32 race:{uint32(RACE_HUMAN),uint32(RACE_ORC)})for(uint32 mode=0;mode<3;++mode){
  MapInstanced parent(1607,300);InstanceMap map(1607,300,9928000+cases,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);auto scenario=sScenarioMgr->AddScenario(&map,1078);scenario->SetCurrentStep(5);auto first=Session(),second=Session();
  {Position pose{1854.15f,1275.45f,91.7998f,1.54631f};NativePlayer player(first,map,101,pose),other(second,map,102,pose);player.SetByteValue(UNIT_FIELD_BYTES_0,0,race);player.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_ROGUE);player.setFactionForRace(race);player.SetQuestStatus(41924,QUEST_STATUS_INCOMPLETE);player.SetQuestSlot(0,41924);
   Actor boss(&map,105660,34001,pose);Require(map.AddToMap(static_cast<Creature*>(&boss)),"Original Akaari absent");auto ai=new npc_akaari_last_probe::npc_akaari_shadowgore_lastAI(&boss);Require(boss.AIM_Initialize(ai),"Whole combat class missing");std::unique_ptr<CreditPet> pet;std::unique_ptr<CreditGuardian> guardian;if(mode==1)pet=std::make_unique<CreditPet>(player,map);if(mode==2)guardian=std::make_unique<CreditGuardian>(player,map);Unit* opener=pet?static_cast<Unit*>(pet.get()):guardian?static_cast<Unit*>(guardian.get()):static_cast<Unit*>(&player);
   boss.SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_IN_COMBAT);combatClock=0;ai->EnterCombat(opener);CleanStories(map);
   auto bad=const_cast<std::vector<ConversationData>*>(sConversationDataStore->GetConversationData(uint32(1943)));Require(bad&&!bad->empty(),"Original1943 data missing");auto original=*bad;
#ifdef DEVOURER_COMBAT_FAULTS
   bad->clear();
#endif
   std::vector<uint32> ids,times;uint32 last=0;
   for(combatClock=1;combatClock<=100000;++combatClock){
#ifdef DEVOURER_COMBAT_FAULTS
    if(combatClock==(mode==0?16000u:mode==1?32000u:64000u))*bad=original;
#endif
    auto previousCursor=ai->convers;auto previousCasts=ai->casts.size();ai->UpdateAI(1);
    if(previousCursor==ai->convers&&previousCasts==ai->casts.size()&&combatClock%1000)continue;
    std::vector<Conversation*> stories;for(auto story:*Access(StoriesTag{}))if(story&&story->GetMap()==&map)stories.push_back(story);
    for(auto story:stories){auto id=story->GetEntry();Require(id>=1943&&id<=1946,"Unexpected periodic conversation");auto actors=sConversationDataStore->GetConversationCreature(id);auto lines=sConversationDataStore->GetConversationData(id);Require(actors&&actors->size()==1&&lines&&lines->size()==1,"Original periodic inputs changed");
     Require(story->GetDuration()==int32((*actors)[0].duration+5000)&&story->GetLines().size()==1&&story->GetLines()[0]->id==(*lines)[0].id&&story->GetLines()[0]->textId==(*lines)[0].textId&&story->GetLines()[0]->unk1==(*lines)[0].unk1&&story->GetLines()[0]->unk2==(*lines)[0].unk2,"Original periodic line/duration changed");
     Require(story->GetCasterGUID()==boss.GetGUID()&&story->GetCreatures().size()==1&&story->GetCreatures()[0]->GuidLow==boss.GetGUID().GetLowPart()&&story->GetCreatures()[0]->GuidHight==boss.GetGUID().GetHighPart()&&story->GetCreatures()[0]->unk1==(*actors)[0].unk1&&story->GetCreatures()[0]->unk2==(*actors)[0].unk2,"Periodic native caster/speaker/opaque payload changed");
     if(last)Require(combatClock>=last,"Clock regressed");last=combatClock+story->GetDuration();ids.push_back(id);times.push_back(combatClock);story->Remove();map.RemoveAllObjectsInRemoveList();
    }
   }
   *bad=original;
#ifdef DEVOURER_COMBAT_BASELINE
   Require(ids==std::vector<uint32>({1944,1945,1946}),"Pinned skipped1943 counterfactual changed");
#else
   Require(ids==std::vector<uint32>({1943,1944,1945,1946})&&ai->convers==1947,"Periodic native four-line sequence lost/order/extra line");
#endif
   Require(scenario->GetCurrentStep()==5&&player.GetQuestStatus(41924)==QUEST_STATUS_INCOMPLETE&&!player.HasItemCount(128476,1),"Combat dialogue advanced/granted quest/item");Require(!ai->casts.empty(),"Original combat casts missing");
   std::cout<<"CASE race="<<race<<" opener="<<mode<<" lines=";for(auto id:ids)std::cout<<id<<',';std::cout<<" at=";for(auto t:times)std::cout<<t<<',';std::cout<<" casts="<<ai->casts.size()<<'\n';CleanStories(map);boss.RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_IN_COMBAT);
  }for(auto s:{first,second}){s->SetPlayer(nullptr);delete s;}sScenarioMgr->RemoveScenario(map.GetInstanceId());++cases;
 }
#ifdef DEVOURER_COMBAT_BASELINE
 std::cout<<"COUNTERFACTUAL "<<cases<<" native periodic actor/payload contexts: original cursor skips1943/4091. Victim/clock/spell endpoints controlled, not full combat/client\n";
#else
 std::cout<<"PASS "<<cases<<" native periodic four-line4091..4094/actual duration/opaque speaker payload contexts; no native quest/item grant. Controlled earlier state/victim/clock/spell endpoints, client separate\n";
#endif
 CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
