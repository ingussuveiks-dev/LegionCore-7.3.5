// Whole selected Merlis death hook; actual native Player/Pet/Guardian, Kill,
// Scenario, Conversation payload/ownership and source scope. Earlier stage
// state and actor admission are seeded, not a complete client traversal.
#define ALTHEA_INTRO_NATIVE_SUPPORT_ONLY
#include "AltheaIntroNative.cpp"
#include "MapManager.h"
#include "MerlisHandoff.inc"
void AddSC_Kingslayers();
static Conversation* DeathStory(Map& map){Conversation* found=nullptr;for(auto story:*Access(StoriesTag{}))if(story&&story->GetMap()==&map&&story->GetEntry()==2448){Require(!found,"Duplicate Merlis death conversation");found=story;}return found;}
int main(int argc,char** argv){try{
 InitializeAltheaGuardsNative(argc,argv);Require(sObjectMgr->LoadTrinityStrings(),"Native strings missing");sSpellMgr->LoadSpellProcEvents();sSpellMgr->LoadSpellProcs();sSpellMgr->LoadSpellScene();sObjectMgr->LoadPageTexts();sObjectMgr->LoadGameObjectTemplate();
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());AddSC_Kingslayers();sScriptMgr->SwapScriptContext(true);
 uint32 cases=0,bindings=0;
 for(uint32 quest:{42504u,42627u})for(uint32 kind=0;kind<3;++kind){
  MapInstanced parent(1620,300);InstanceMap map(1620,300,9997000+cases,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);auto scenario=sScenarioMgr->AddScenario(&map,1123);scenario->SetCurrentStep(7);auto session=Session(),other=Session(),outsider=Session();
  {NativePlayer player(session,map,101,{-8315.93f,287.852f,156.916f,0});player.SetByteValue(UNIT_FIELD_BYTES_0,0,quest==42504?RACE_HUMAN:RACE_ORC);player.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_ROGUE);player.setFactionForRace(player.getRace());player.SetQuestStatus(quest,QUEST_STATUS_INCOMPLETE);player.SetQuestSlot(0,quest);
   NativePlayer companion(other,map,102,player),unrelated(outsider,map,103,player);std::unique_ptr<CreditParty> party;if(kind==2)party=std::make_unique<CreditParty>(&player,&companion);
   Actor boss(&map,107831,28201,{-8321.93f,287.852f,156.916f,.7101f}),old(&map,108222,28202,{-8312.8f,284.535f,160.054f,3.24861f});Require(map.AddToMap(static_cast<Creature*>(&boss))&&map.AddToMap(static_cast<Creature*>(&old)),"Native original actors missing");auto ai=new npc_merlis_handoff_probe::npc_merlis_malaganAI(&boss);Require(boss.AIM_Initialize(ai),"Complete Merlis AI missing");
   auto control=new Conversation;Require(control->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(),2448,&player,nullptr,player),"Living native2448 control failed");Require(control->GetDuration()==6650&&control->GetLines()[0]->id==5171,"Original death payload/lifetime changed");control->Remove();map.RemoveAllObjectsInRemoveList();
   std::unique_ptr<CreditPet> pet;std::unique_ptr<CreditGuardian> guardian;if(kind==1)pet=std::make_unique<CreditPet>(player,map);if(kind==2)guardian=std::make_unique<CreditGuardian>(player,map);Unit* killer=pet?static_cast<Unit*>(pet.get()):guardian?static_cast<Unit*>(guardian.get()):static_cast<Unit*>(&player);
   boss.SetLootRecipient(&player);boss.LowerPlayerDamageReq(uint32(boss.GetMaxHealth()));if(!kind)killer->DealDamage(&boss,UINT32_MAX-4096,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);else killer->Kill(&boss,false);
   Require(!boss.IsAlive()&&scenario->GetCurrentStep()==8&&scenario->GetAchievementMgr().GetCriteriaProgressMap()->at(48589).Counter==1,"Native owner51615 lost");
#ifdef MERLIS_DEATH_BASELINE
   Require(!DeathStory(map),"Pinned dead-source actor refusal no longer reproduces");
#else
   auto story=DeathStory(map);Require(story&&story->IsInWorld()&&story->GetDuration()==6650&&story->GetLines().size()==1&&story->GetLines()[0]->id==5171,"Actual complete death hook lost2448");Require(story->GetCreatures().size()==1&&story->GetCreatures()[0]->GuidLow==boss.GetGUID().GetLowPart()&&story->GetCreatures()[0]->GuidHight==boss.GetGUID().GetHighPart(),"Death speech rebound another actor");Require(story->GetQuestObjectScope().lock()==boss.GetQuestObjectScope().lock(),"Death conversation scope differs from original actor");
   Require(story->IsInPersonnalVisibilityList(player.GetGUID())&&story->IsInPersonnalVisibilityList(companion.GetGUID())==bool(party)&&!story->IsInPersonnalVisibilityList(unrelated.GetGUID()),"Native owner/legitimate group personal visibility changed");
   killer->Kill(&boss,false);ai->Reset();Require(DeathStory(map)==story&&scenario->GetAchievementMgr().GetCriteriaProgressMap()->at(48589).Counter==1,"Repeated native death/Reset replayed speech or credit");
#endif
   auto parents=std::list<Creature*>{};GetCreatureListWithEntryInGrid(parents,&boss,108222,100.f);uint32 accepted=0;for(auto actor:parents)if(actor->IsAlive()&&actor->ToTempSummon()&&actor->ToTempSummon()->GetSummonerGUID()==boss.GetGUID()&&actor->AI()&&actor->AI()->GetData(108222))++accepted;Require(accepted==1,"Native earned Garona handoff changed");Require(!player.HasItemCount(128870,1)&&player.GetQuestStatus(quest)==QUEST_STATUS_INCOMPLETE,"Death speech granted item/quest");CleanStories(map);
#ifndef MERLIS_DEATH_BASELINE
   auto allocations=objectCountInWorld[uint8(HighGuid::Conversation)];
   Actor duplicate(&map,107831,28203,player);Require(map.AddToMap(static_cast<Creature*>(&duplicate)),"Living duplicate control missing");
   auto bound=[&](ObjectGuid guid,bool expected){
    auto value=new npc_merlis_handoff_probe::npc_merlis_malaganAI::DeathConversation(guid);bool admitted=value->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(),2448,&player,nullptr,player);Require(admitted==expected,"Native exact death actor refusal differs");
    if(admitted){Require(value->IsInWorld()&&value->GetCreatures().size()==1&&value->GetCreatures()[0]->GuidLow==guid.GetLowPart()&&value->GetCreatures()[0]->GuidHight==guid.GetHighPart(),"Bound actor switched to living duplicate");value->Remove();map.RemoveAllObjectsInRemoveList();}
    else{Require(!value->IsInWorld()&&!value->IsInGrid(),"Refused native conversation retained ownership");delete value;}
    Require(objectCountInWorld[uint8(HighGuid::Conversation)]==allocations,"Refused/bound native conversation allocation leak");++bindings;
   };
   auto guid=boss.GetGUID(),otherHigh=guid,otherLow=guid,otherUpperLow=guid;otherHigh.SetRawValue(guid.GetHighPart()^1,guid.GetLowPart());otherLow.SetRawValue(guid.GetHighPart(),guid.GetLowPart()+100000);otherUpperLow.SetRawValue(guid.GetHighPart(),guid.GetLowPart()^(uint64(1)<<40));bound(guid,true);bound(otherHigh,false);bound(otherLow,false);bound(otherUpperLow,false);bound(player.GetGUID(),false);bound(old.GetGUID(),false);
   boss.SetPhaseMask(2,false);bound(guid,false);boss.SetPhaseMask(1,false);
   auto home=boss.GetPosition();boss.Relocate(home.GetPositionX()+2000,home.GetPositionY(),home.GetPositionZ(),home.GetOrientation());bound(guid,false);boss.Relocate(home);
   auto actors=const_cast<std::vector<ConversationCreature>*>(sConversationDataStore->GetConversationCreature(2448));Require(actors&&actors->size()==1,"Native death actor input differs");auto original=(*actors)[0];(*actors)[0].id=1;bound(guid,false);(*actors)[0]=original;(*actors)[0].creatureId=108222;bound(guid,false);(*actors)[0]=original;
   auto scope=std::make_shared<QuestObjectScope>();scope->Owner=unrelated.GetGUID();boss.BindQuestObjectScope(scope);bound(guid,false);scope->Owner=player.GetGUID();
   auto scoped=new npc_merlis_handoff_probe::npc_merlis_malaganAI::DeathConversation(guid);Require(scoped->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(),2448,&player,nullptr,player),"Native owned scope refused");auto storyGuid=scoped->GetGUID();auto cancelled=scope->Cancel();Require(std::find(cancelled.begin(),cancelled.end(),storyGuid)!=cancelled.end(),"Actual conversation GUID not registered in source attempt before publication");scoped->Update(0);map.RemoveAllObjectsInRemoveList();Require(!ObjectAccessor::GetConversation(boss,storyGuid)&&objectCountInWorld[uint8(HighGuid::Conversation)]==allocations,"Cancelled native conversation not removed");bound(guid,false);++bindings;
#endif
  }
  for(auto value:{session,other,outsider}){value->SetPlayer(nullptr);delete value;}sScenarioMgr->RemoveScenario(map.GetInstanceId());++cases;
 }
#ifdef MERLIS_DEATH_BASELINE
 std::cout<<"COUNTERFACTUAL "<<cases<<" full-source native player/pet/guardian kills: living2448 control succeeds, original dead-source speech absent while native51615/Garona succeeds\n";
#else
 std::cout<<"PASS "<<cases<<" full-source native player/pet/guardian one-hit/death contexts: actual dead full-GUID actor5171/6650, unchanged51615/accepted Garona/no item grant and no repeat; earlier state seeded, client playback separate\n";
 std::cout<<"PASS "<<bindings<<" native exact death actor binding/refusal/cancellation contexts: living duplicate/full GUID, entry/index/phase/range, foreign scope, real registered cancellation removal and no allocation/grid leaks; native owner/group personal visibility retained\n";
#endif
 CloseAltheaNative();return 0;
 }catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}
