// Whole final Akaari AI, real native Kill/owner/Scenario/Conversation.
// Earlier quest/stage/pose and grid admission are seeded, not client traversal.
#define ALTHEA_INTRO_NATIVE_SUPPORT_ONLY
#include "AltheaIntroNative.cpp"
#include "DevourerInstance.inc"
#include "Entrance.inc"
static Conversation* DeathStory(Map& map){Conversation* found=nullptr;for(auto story:*Access(StoriesTag{}))if(story&&story->GetMap()==&map&&story->GetEntry()==1947){Require(!found,"Duplicate Akaari death conversation");found=story;}return found;}
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1620,{}},{1607,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1620,{}},{1607,{}}});
 sObjectMgr->LoadTrinityStrings();sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new instance_devourer;sScriptMgr->SwapScriptContext(true);
 auto info=sObjectMgr->GetCreatureTemplate(105660);Require(info,"Original Akaari template missing");
 if(info->lootid){auto& name=LootTemplates_Creature.*Access(LootNameTag{});auto old=name;name+=" WHERE Entry IN(108218,"+std::to_string(info->lootid)+") ORDER BY Entry,Item";auto loaded=(LootTemplates_Creature.*Access(LootLoadTag{}))();name=old;Require(loaded&&LootTemplates_Creature.HaveLootFor(info->lootid),"Original native Akaari loot missing");}
 uint32 cases=0,bindings=0;
#ifdef DEVOURER_DEATH_VOICE_BASELINE
 uint32 kinds=3;
#else
 uint32 kinds=4;
#endif
 for(uint32 race:{uint32(RACE_HUMAN),uint32(RACE_ORC)})for(uint32 kind=0;kind<kinds;++kind){
  MapInstanced parent(1607,300);InstanceMap map(1607,300,9927000+cases,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);auto scenario=sScenarioMgr->AddScenario(&map,1078);scenario->SetCurrentStep(5);auto first=Session(),second=Session(),third=Session();
  {Position pose{1854.15f,1275.45f,91.7998f,1.54631f};NativePlayer player(first,map,101,pose),companion(second,map,102,pose),outsider(third,map,103,pose);
   player.SetByteValue(UNIT_FIELD_BYTES_0,0,race);player.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_ROGUE);player.setFactionForRace(race);player.SetQuestStatus(41924,QUEST_STATUS_INCOMPLETE);player.SetQuestSlot(0,41924);
   Actor boss(&map,105660,33001,pose);Require(map.AddToMap(static_cast<Creature*>(&boss)),"Native final Akaari absent");auto ai=new npc_akaari_last_probe::npc_akaari_shadowgore_lastAI(&boss);Require(boss.AIM_Initialize(ai),"Whole final Akaari AI missing");
   auto control=new Conversation;Require(control->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(),1947,&player,nullptr,player),"Living1947 control failed");Require(control->GetDuration()==7350&&control->GetLines().size()==1&&control->GetLines()[0]->id==4095,"Original1947 payload changed");control->Remove();map.RemoveAllObjectsInRemoveList();
   std::unique_ptr<CreditParty> party;if(kind==2)party=std::make_unique<CreditParty>(&player,&companion);std::unique_ptr<CreditPet> pet;std::unique_ptr<CreditGuardian> guardian;if(kind==1)pet=std::make_unique<CreditPet>(player,map);if(kind==2)guardian=std::make_unique<CreditGuardian>(player,map);Unit* killer=pet?static_cast<Unit*>(pet.get()):guardian?static_cast<Unit*>(guardian.get()):static_cast<Unit*>(&player);
   auto actors=const_cast<std::vector<ConversationCreature>*>(sConversationDataStore->GetConversationCreature(1947));Require(actors&&actors->size()==1,"Original1947 actor input differs");auto original=(*actors)[0];if(kind==3)(*actors)[0].id=1;
   boss.SetLootRecipient(&player);boss.LowerPlayerDamageReq(uint32(boss.GetMaxHealth()));if(kind==0)killer->DealDamage(&boss,UINT32_MAX-4096,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);else killer->Kill(&boss,false);
   (*actors)[0]=original;
   Require(!boss.IsAlive()&&scenario->GetCurrentStep()==6&&scenario->GetAchievementMgr().GetCriteriaProgressMap()->at(47369).Counter==1,"Native owner50300 lost");
#ifdef DEVOURER_DEATH_VOICE_BASELINE
   Require(!DeathStory(map),"Pinned dead-source refusal no longer reproduces");
#else
   if(kind==3)Require(!DeathStory(map),"Explicit failed1947 factory unexpectedly succeeded");else{
   auto story=DeathStory(map);Require(story&&story->IsInWorld()&&story->GetCasterGUID()==player.GetGUID()&&story->GetDuration()==7350&&story->GetLines().size()==1&&story->GetLines()[0]->id==4095,"Native whole death hook lost1947");Require(story->GetCreatures().size()==1&&story->GetCreatures()[0]->GuidLow==boss.GetGUID().GetLowPart()&&story->GetCreatures()[0]->GuidHight==boss.GetGUID().GetHighPart(),"Native death speech rebound another actor");
   Require(story->GetCreatures()[0]->unk1==0&&story->GetCreatures()[0]->unk2==48&&story->GetLines()[0]->textId==0&&story->GetLines()[0]->unk1==1646290021&&story->GetLines()[0]->unk2==0,"Original1947 opaque actor/line payload changed");
   Require(story->IsInPersonnalVisibilityList(player.GetGUID())&&story->IsInPersonnalVisibilityList(companion.GetGUID())==bool(party)&&!story->IsInPersonnalVisibilityList(outsider.GetGUID()),"Owner/group personal audience changed");
   killer->Kill(&boss,false);ai->Reset();Require(DeathStory(map)==story&&scenario->GetAchievementMgr().GetCriteriaProgressMap()->at(47369).Counter==1,"Repeated kill/Reset replayed death");
   }
#endif
   Require(!player.HasItemCount(128476,1)&&!player.HasItemCount(128479,1)&&player.GetQuestStatus(41924)==QUEST_STATUS_INCOMPLETE,"Death speech granted artifact/quest");CleanStories(map);
#ifndef DEVOURER_DEATH_VOICE_BASELINE
   auto allocations=objectCountInWorld[uint8(HighGuid::Conversation)];
   Actor duplicate(&map,105660,33003,player);Require(map.AddToMap(static_cast<Creature*>(&duplicate)),"Living duplicate control missing");
   auto bound=[&](ObjectGuid guid,bool expected){
    auto value=new npc_akaari_last_probe::npc_akaari_shadowgore_lastAI::DeathConversation(guid);bool admitted=value->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(),1947,&player,nullptr,player);Require(admitted==expected,"Native exact death actor refusal differs");
    if(admitted){Require(value->IsInWorld()&&value->GetCreatures().size()==1&&value->GetCreatures()[0]->GuidLow==guid.GetLowPart()&&value->GetCreatures()[0]->GuidHight==guid.GetHighPart(),"Bound actor switched to living duplicate");value->Remove();map.RemoveAllObjectsInRemoveList();}
    else{Require(!value->IsInWorld()&&!value->IsInGrid(),"Refused native conversation retained ownership");delete value;}
    Require(objectCountInWorld[uint8(HighGuid::Conversation)]==allocations,"Refused/bound native conversation allocation leak");++bindings;
   };
   auto guid=boss.GetGUID(),otherHigh=guid,otherLow=guid,otherUpperLow=guid;otherHigh.SetRawValue(guid.GetHighPart()^1,guid.GetLowPart());otherLow.SetRawValue(guid.GetHighPart(),guid.GetLowPart()+100000);otherUpperLow.SetRawValue(guid.GetHighPart(),guid.GetLowPart()^(uint64(1)<<40));bound(guid,true);bound(otherHigh,false);bound(otherLow,false);bound(otherUpperLow,false);bound(player.GetGUID(),false);bound(outsider.GetGUID(),false);
   boss.SetPhaseMask(2,false);bound(guid,false);boss.SetPhaseMask(1,false);
   auto home=boss.GetPosition();boss.Relocate(home.GetPositionX()+2000,home.GetPositionY(),home.GetPositionZ(),home.GetOrientation());bound(guid,false);boss.Relocate(home);
   (*actors)[0].id=1;bound(guid,false);(*actors)[0]=original;(*actors)[0].creatureId=105542;bound(guid,false);(*actors)[0]=original;
   auto scope=std::make_shared<QuestObjectScope>();scope->Owner=outsider.GetGUID();boss.BindQuestObjectScope(scope);bound(guid,false);scope->Owner=player.GetGUID();
   auto scoped=new npc_akaari_last_probe::npc_akaari_shadowgore_lastAI::DeathConversation(guid);Require(scoped->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(),1947,&player,nullptr,player),"Native owned scope refused");auto storyGuid=scoped->GetGUID();auto cancelled=scope->Cancel();Require(std::find(cancelled.begin(),cancelled.end(),storyGuid)!=cancelled.end(),"Actual conversation GUID not registered in source attempt before publication");scoped->Update(0);map.RemoveAllObjectsInRemoveList();Require(!ObjectAccessor::GetConversation(boss,storyGuid)&&objectCountInWorld[uint8(HighGuid::Conversation)]==allocations,"Cancelled native conversation not removed");bound(guid,false);++bindings;
#endif
  }for(auto session:{first,second,third}){session->SetPlayer(nullptr);delete session;}sScenarioMgr->RemoveScenario(map.GetInstanceId());++cases;
 }
#ifdef DEVOURER_DEATH_VOICE_BASELINE
 std::cout<<"COUNTERFACTUAL "<<cases<<" whole-source native deaths: living1947/4095 control succeeds; actual dead Akaari speech missing while owner50300 reaches stage6. Expected broken baseline, client separate\n";
#else
 std::cout<<"PASS "<<cases<<" whole-source native player scaled one-hit/pet/guardian deaths: actual corpse fullGUID4095/7350, unchanged owner50300/stage6/no item or quest grant, legitimate group personal audience, repeated kill/Reset and failed-factory credit independence\n";
#endif
#ifndef DEVOURER_DEATH_VOICE_BASELINE
 std::cout<<"PASS "<<bindings<<" real-native bound-actor/fullGUID/duplicate/phase/range/slot/entry/scope/cancellation/refused-allocation contexts\n";
#endif
 CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
