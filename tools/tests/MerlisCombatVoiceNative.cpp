// Full production Merlis class with real native Conversation, Map and combat
// state. Prior quest/scenario state and helper clocks are controlled.
#define ALTHEA_INTRO_NATIVE_SUPPORT_ONLY
#include "AltheaIntroNative.cpp"
#include "MapManager.h"
#include "MerlisHandoff.inc"
static Conversation* CombatStory(Map& map,uint32 id){Conversation* found=nullptr;for(auto value:*Access(StoriesTag{}))if(value&&value->GetMap()==&map&&value->GetEntry()==id){Require(!found,"Duplicate combat story");found=value;}return found;}
int main(int argc,char** argv){try{
 InitializeAltheaGuardsNative(argc,argv);uint32 cases=0;
#ifdef MERLIS_COMBAT_FAULTS
 std::vector<uint32> modes{3};
#else
 std::vector<uint32> modes{0,1,2};
#endif
 for(uint32 quest:{42504u,42627u})for(uint32 mode:modes){
  MapInstanced parent(1620,300);InstanceMap map(1620,300,9998000+cases,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);auto scenario=sScenarioMgr->AddScenario(&map,1123);scenario->SetCurrentStep(7);auto session=Session();
  {NativePlayer player(session,map,101,{-8315.93f,287.852f,156.916f,0});player.SetByteValue(UNIT_FIELD_BYTES_0,0,quest==42504?RACE_HUMAN:RACE_ORC);player.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_ROGUE);player.setFactionForRace(player.getRace());player.SetQuestStatus(quest,QUEST_STATUS_INCOMPLETE);player.SetQuestSlot(0,quest);
   Actor boss(&map,107831,28101,{-8321.93f,287.852f,156.916f,.7101f});Require(map.AddToMap(static_cast<Creature*>(&boss)),"Native combat actor admission failed");
   auto ai=new npc_merlis_handoff_probe::npc_merlis_malaganAI(&boss);Require(boss.AIM_Initialize(ai),"Native full Merlis AI missing");boss.SetInCombatWith(&player);Require(boss.isInCombat(),"Native combat state missing");ai->EnterCombat(&player);
   if(mode==3){auto rows=const_cast<std::vector<ConversationData>*>(sConversationDataStore->GetConversationData(uint32(2442)));Require(rows&&!rows->empty(),"Native conversation input missing");auto original=*rows;rows->clear();ai->RequestCombatVoice();ai->UpdateCombatVoice(999);Require(ai->convid==2442&&!CombatStory(map,2442),"Refused native conversation consumed a line");*rows=original;ai->UpdateCombatVoice(1);Require(CombatStory(map,2442)&&ai->convid==2443,"Restored native conversation did not recover same line");}
   else{ai->RequestCombatVoice();Require(CombatStory(map,2442)&&ai->convid==2443,"Native first conversation not admitted");}
   auto first=CombatStory(map,2442);Require(first->GetDuration()==7150&&first->GetLines().size()==1&&first->GetLines()[0]->id==5165,"Original native line/lifetime changed");Require(first->GetCreatures().size()==1&&first->GetCreatures()[0]->GuidLow==boss.GetGUID().GetLowPart()&&first->GetCreatures()[0]->GuidHight==boss.GetGUID().GetHighPart(),"Wrong native speaker binding");
   if(mode==1){ai->RequestCombatVoice();ai->Reset();ai->UpdateCombatVoice(60000);Require(!CombatStory(map,2443)&&ai->convid==2443,"Reset retained pending dialogue");CleanStories(map);ai->EnterCombat(&player);ai->RequestCombatVoice();Require(ai->convid==2443&&CombatStory(map,2442),"Retry did not restart original first line");}
   if(mode==2){ai->RequestCombatVoice();boss.CombatStop(true);ai->UpdateCombatVoice(60000);Require(!CombatStory(map,2443)&&ai->convid==2443,"Out-of-combat dialogue dispatched");ai->Reset();}
   else{
    uint32 const lifetimes[]={7150,8400,9150,7850,7600,9200};
    for(uint32 id=2443;id<2448;++id){ai->RequestCombatVoice();ai->RequestCombatVoice();Require(!CombatStory(map,id),"Repeated same-tick request overlapped previous line");ai->UpdateCombatVoice(lifetimes[id-2443]-1);Require(!CombatStory(map,id),"Native published lifetime shortened");ai->UpdateCombatVoice(1);auto story=CombatStory(map,id);Require(story&&story->GetDuration()==int32(lifetimes[id-2442])&&story->GetLines()[0]->id==5165+id-2442&&ai->convid==id+1,"Actual native next line/lifetime receipt differs");}
    Require(ai->convid==2448,"Native six-line endpoint missing");ai->RequestCombatVoice();ai->UpdateCombatVoice(60000);Require(!CombatStory(map,2448),"Combat dialogue consumed separate death conversation");
   }
   Require(scenario->GetCurrentStep()==7&&scenario->GetAchievementMgr().GetCriteriaProgressMap()->find(48589)==scenario->GetAchievementMgr().GetCriteriaProgressMap()->end()&&!player.HasItemCount(128870,1)&&player.GetQuestStatus(quest)==QUEST_STATUS_INCOMPLETE,"Optional dialogue changed progression/inventory");CleanStories(map);
  }
  session->SetPlayer(nullptr);delete session;sScenarioMgr->RemoveScenario(map.GetInstanceId());++cases;
 }
 std::cout<<"PASS "<<cases<<" full-source native combat conversation contexts: original actors/payloads, published lifetime, same-line retry/reset/combat guard; clocks/earlier state controlled, no client playback claim\n";
 CharacterDatabase.Close();LoginDatabase.Close();HotfixDatabase.Close();WorldDatabase.Close();return 0;
 }catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}
