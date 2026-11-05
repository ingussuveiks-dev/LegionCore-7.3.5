// Actual final AI and native owner/Map/Conversation; earlier quest/stage/pose
// and combat flag seeded. No combat effects or client traversal are claimed.
#define ALTHEA_INTRO_NATIVE_SUPPORT_ONLY
#include "AltheaIntroNative.cpp"
#include "DevourerInstance.inc"
#include "Entrance.inc"
struct OpeningCreatedTag{using Type=uint32 Conversation::*;friend Type Access(OpeningCreatedTag);};
template struct FixtureAccess<OpeningCreatedTag,&Conversation::_createdAt>;
struct OpeningPlayingTag{using Type=std::map<ObjectGuid,uint32> Conversation::*;friend Type Access(OpeningPlayingTag);};
template struct FixtureAccess<OpeningPlayingTag,&Conversation::playing>;
static Conversation* Opening(Map& map){Conversation* found=nullptr;for(auto story:*Access(StoriesTag{}))if(story&&story->GetMap()==&map&&story->GetEntry()==1942){Require(!found,"Duplicate1942");found=story;}return found;}
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1620,{}},{1607,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1620,{}},{1607,{}}});
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new instance_devourer;sScriptMgr->SwapScriptContext(true);uint32 cases=0,bindings=0;
#if defined(DEVOURER_OPENING_BASELINE)||defined(DEVOURER_OPENING_FAULTS)
 uint32 modes=3;
#else
 uint32 modes=6;
#endif
 for(uint32 race:{uint32(RACE_HUMAN),uint32(RACE_ORC)})for(uint32 mode=0;mode<modes;++mode){
  MapInstanced parent(1607,300);InstanceMap map(1607,300,9929000+cases,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);auto scenario=sScenarioMgr->AddScenario(&map,1078);scenario->SetCurrentStep(5);auto first=Session(),second=Session(),third=Session();
  {Position pose{1854.15f,1275.45f,91.7998f,1.54631f};NativePlayer player(first,map,101,pose),member(second,map,102,pose),outsider(third,map,103,pose);
   player.SetByteValue(UNIT_FIELD_BYTES_0,0,race);player.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_ROGUE);player.setFactionForRace(race);player.SetQuestStatus(41924,QUEST_STATUS_INCOMPLETE);player.SetQuestSlot(0,41924);CreditParty party(&player,&member);
   auto sourcePose=pose;sourcePose.m_positionX+=10;Actor boss(&map,105660,35001,sourcePose),duplicate(&map,105660,35002,pose),wild(&map,105542,35003,pose);for(auto a:{&boss,&duplicate,&wild})Require(map.AddToMap(static_cast<Creature*>(a)),"Native original/duplicate actors missing");auto ai=new npc_akaari_last_probe::npc_akaari_shadowgore_lastAI(&boss);Require(boss.AIM_Initialize(ai),"Whole native opening AI missing");boss.SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_IN_COMBAT);
   std::unique_ptr<CreditPet> pet;std::unique_ptr<CreditGuardian> guardian;if(mode==1)pet=std::make_unique<CreditPet>(player,map);if(mode==2||mode==3)guardian=std::make_unique<CreditGuardian>(player,map);if(mode==3)guardian->SetCharmerGUID(player.GetGUID());Unit* caller=mode==5?nullptr:mode==4?static_cast<Unit*>(&wild):pet?static_cast<Unit*>(pet.get()):guardian?static_cast<Unit*>(guardian.get()):static_cast<Unit*>(&player);
#ifdef DEVOURER_OPENING_FAULTS
   auto data=const_cast<std::vector<ConversationData>*>(sConversationDataStore->GetConversationData(uint32(1942)));Require(data&&!data->empty(),"Original1942 data missing");auto original=*data;if(mode!=1)data->clear();auto originalPos=player.GetPosition();if(mode==1)player.Relocate(sourcePose.GetPositionX()+1000,sourcePose.GetPositionY(),sourcePose.GetPositionZ(),0);
   ai->EnterCombat(caller);Require(!Opening(map)&&ai->convers==1942,"Failed opening consumed cursor");ai->UpdateCombatVoice(999);Require(!Opening(map)&&ai->convers==1942,"Opening retried early");*data=original;player.Relocate(originalPos);
   if(mode==2){player.SetQuestStatus(41924,QUEST_STATUS_NONE);player.SetQuestStatus(41924,QUEST_STATUS_INCOMPLETE);}ai->UpdateCombatVoice(1);
   if(mode==2){Require(!Opening(map)&&ai->convers==1943&&!ai->combatVoicePending,"Abandoned opening resumed");ai->Reset();ai->EnterCombat(caller);}
   Require(Opening(map)&&ai->convers==1943,"Original owner/data restoration failed");
#else
   ai->EnterCombat(caller);
#endif
   auto story=Opening(map);Require(story&&story->IsInWorld()&&story->GetDuration()==7800&&story->GetLines().size()==1&&story->GetLines()[0]->id==4090,"Actual1942/4090/7800 missing");
#ifdef DEVOURER_OPENING_BASELINE
   Require(story->GetCasterGUID()==caller->GetGUID()&&story->GetCreatures()[0]->GuidLow==duplicate.GetGUID().GetLowPart(),"Pinned wrong-source/raw-caster counterfactual changed");Require(story->IsInPersonnalVisibilityList(player.GetGUID())==(mode==0)&&story->IsInPersonnalVisibilityList(member.GetGUID())==(mode==0),"Pinned personal/group audience counterfactual changed");
#else
   Require(story->GetCasterGUID()==(mode<4?player.GetGUID():boss.GetGUID()),"Opening did not resolve native owner/source fallback");Require(story->GetCreatures().size()==1&&story->GetCreatures()[0]->GuidLow==boss.GetGUID().GetLowPart()&&story->GetCreatures()[0]->GuidHight==boss.GetGUID().GetHighPart(),"Opening rebound living duplicate");Require(story->GetCreatures()[0]->unk1==0&&story->GetCreatures()[0]->unk2==0&&story->GetLines()[0]->textId==0&&story->GetLines()[0]->unk1==32683&&story->GetLines()[0]->unk2==0,"Original1942 opaque payload changed");Require(story->IsInPersonnalVisibilityList(player.GetGUID())==(mode<4)&&story->IsInPersonnalVisibilityList(member.GetGUID())==(mode<4)&&!story->IsInPersonnalVisibilityList(outsider.GetGUID()),"Opening native owner/group personal policy changed");
#endif
   Require(scenario->GetCurrentStep()==5&&player.GetQuestStatus(41924)==QUEST_STATUS_INCOMPLETE&&!player.HasItemCount(128476,1),"Opening narration granted progression/item");CleanStories(map);
#ifndef DEVOURER_OPENING_BASELINE
   if(mode==0){
    auto allocations=objectCountInWorld[uint8(HighGuid::Conversation)];auto receipt=std::make_shared<QuestObjectScope>();
    auto bound=[&](ObjectGuid guid,bool expected){auto value=new npc_akaari_last_probe::npc_akaari_shadowgore_lastAI::OpeningConversation(guid,receipt->CancellationReceipt());bool accepted=value->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(),1942,&player,nullptr,player);Require(accepted==expected,"Native exact opening binding/refusal differs");if(accepted){Require(value->GetCreatures()[0]->GuidLow==guid.GetLowPart()&&value->GetCreatures()[0]->GuidHight==guid.GetHighPart(),"Opening source changed to duplicate");value->Remove();map.RemoveAllObjectsInRemoveList();}else{Require(!value->IsInWorld()&&!value->IsInGrid(),"Refused opening retained grid");delete value;}Require(objectCountInWorld[uint8(HighGuid::Conversation)]==allocations,"Opening allocation leak");++bindings;};
    auto guid=boss.GetGUID(),high=guid,upper=guid,missing=guid;high.SetRawValue(guid.GetHighPart()^1,guid.GetLowPart());upper.SetRawValue(guid.GetHighPart(),guid.GetLowPart()^(uint64(1)<<40));missing.SetRawValue(guid.GetHighPart(),guid.GetLowPart()+100000);bound(guid,true);bound(high,false);bound(upper,false);bound(missing,false);bound(player.GetGUID(),false);bound(wild.GetGUID(),false);
    boss.Unit::setDeathState(JUST_DIED);bound(guid,false);boss.Unit::setDeathState(ALIVE);boss.SetHealth(boss.GetMaxHealth());boss.SetPhaseMask(2,false);bound(guid,false);boss.SetPhaseMask(1,false);
    auto home=boss.GetPosition();boss.Relocate(home.GetPositionX()+2000,home.GetPositionY(),home.GetPositionZ(),0);bound(guid,false);boss.Relocate(home);
    auto actors=const_cast<std::vector<ConversationCreature>*>(sConversationDataStore->GetConversationCreature(1942));auto original=(*actors)[0];(*actors)[0].id=1;bound(guid,false);(*actors)[0]=original;(*actors)[0].creatureId=105542;bound(guid,false);(*actors)[0]=original;
    auto live=new npc_akaari_last_probe::npc_akaari_shadowgore_lastAI::OpeningConversation(guid,receipt->CancellationReceipt());Require(live->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(),1942,&player,nullptr,player),"Live cancellable opening refused");auto storyGuid=live->GetGUID();receipt->Cancel();live->Update(0);map.RemoveAllObjectsInRemoveList();Require(!ObjectAccessor::GetConversation(boss,storyGuid)&&objectCountInWorld[uint8(HighGuid::Conversation)]==allocations,"Explicit opening receipt cancellation did not remove accepted story");++bindings;bound(guid,false);
    {auto control=new Conversation;Require(control->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(),1942,&player,nullptr,player),"Original playback control refused");control->*Access(OpeningCreatedTag{})=getMSTime()-7801;(control->*Access(OpeningPlayingTag{})).clear();control->Update(0);Require(control->IsInWorld(),"Original missing-playback key counterfactual changed");control->Remove();map.RemoveAllObjectsInRemoveList();std::cout<<"CONTROL original Player conversation waits beyond deadline with explicitly absent playback keys; scoped1942 cleanup paired below\n";}
    for(uint32 kind=0;kind<3;++kind){auto timed=new npc_akaari_last_probe::npc_akaari_shadowgore_lastAI::OpeningConversation(guid,{});Require(timed->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(),1942,&player,nullptr,player),"Native timed opening refused");auto timedGuid=timed->GetGUID();timed->*Access(OpeningCreatedTag{})=getMSTime()-7801;auto& viewers=timed->*Access(OpeningPlayingTag{});viewers.clear();if(kind)viewers[kind==1?member.GetGUID():player.GetGUID()]=getMSTime();timed->Update(0);if(kind){Require(timed->IsInWorld(),"Admitted native viewer truncated");viewers.begin()->second=getMSTime()-7801;timed->Update(0);}map.RemoveAllObjectsInRemoveList();Require(!ObjectAccessor::GetConversation(boss,timedGuid)&&objectCountInWorld[uint8(HighGuid::Conversation)]==allocations,"Native absent/finished playback opening leaked");++bindings;}
    receipt=std::make_shared<QuestObjectScope>();auto actorScope=std::make_shared<QuestObjectScope>();actorScope->Owner=outsider.GetGUID();boss.BindQuestObjectScope(actorScope);bound(guid,false);actorScope->Owner=player.GetGUID();auto scoped=new npc_akaari_last_probe::npc_akaari_shadowgore_lastAI::OpeningConversation(guid,receipt->CancellationReceipt());Require(scoped->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(),1942,&player,nullptr,player)&&scoped->GetQuestObjectScope().lock()==actorScope,"Native source scope not inherited");storyGuid=scoped->GetGUID();auto cancelled=actorScope->Cancel();Require(std::find(cancelled.begin(),cancelled.end(),storyGuid)!=cancelled.end(),"Own conversation GUID not registered before publication");scoped->Update(0);map.RemoveAllObjectsInRemoveList();Require(!ObjectAccessor::GetConversation(boss,storyGuid)&&objectCountInWorld[uint8(HighGuid::Conversation)]==allocations,"Source scope cancel failed to clean opening");++bindings;bound(guid,false);
   }
#endif
   if(guardian)guardian->SetCharmerGUID(ObjectGuid::Empty);boss.RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_IN_COMBAT);
  }for(auto s:{first,second,third}){s->SetPlayer(nullptr);delete s;}sScenarioMgr->RemoveScenario(map.GetInstanceId());++cases;
 }
#ifdef DEVOURER_OPENING_BASELINE
 std::cout<<"COUNTERFACTUAL "<<cases<<" native1942 contexts: closer duplicate speaks; pet/guardian raw NPC caster loses original Player/group personal audience. No client traversal\n";
#elif defined(DEVOURER_OPENING_FAULTS)
 std::cout<<"PASS "<<cases<<" actual native first-factory/owner-distance/explicit-abandon recovery contexts; original owner and real1942 payload preserved, no stale attempt or gameplay grant\n";
 std::cout<<"PASS "<<bindings<<" native exact actor/publication/receipt/source-scope cancellation controls\n";
#else
 std::cout<<"PASS "<<cases<<" actual native1942 player/pet/guardian/charmer/null/unowned contexts: exact original speaker4090/7800, native owner/group audience or source fallback, no progression grant; client separate\n";
 std::cout<<"PASS "<<bindings<<" native exact actor/publication/receipt/source-scope cancellation controls\n";
#endif
 CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
