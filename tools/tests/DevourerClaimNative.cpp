// Whole production pickup class/native lookup/items/cast/criteria/1948.
// Earlier quest/stage/victory/positions and equipped pair are controlled inputs.
#define ALTHEA_INTRO_NATIVE_SUPPORT_ONLY
#include "AltheaIntroNative.cpp"
#include "GameObjectModel.h"
#include "Item.h"
#include "DevourerInstance.inc"
#include "ClaimAI.inc"
struct ClaimItemsTag{using Type=Item* (Player::*)[PLAYER_SLOTS_COUNT];friend Type Access(ClaimItemsTag);};
template struct FixtureAccess<ClaimItemsTag,&Player::m_items>;
struct ClaimGO:GameObject{~ClaimGO(){if(IsInWorld())GetMap()->RemoveFromMap(static_cast<GameObject*>(this),false);ResetMap();}};
struct ClaimInventory{
 Player& player;std::unique_ptr<Item> parent=std::make_unique<Item>(),child=std::make_unique<Item>();
 explicit ClaimInventory(Player& p,uint32 id):player(p){Require(parent->Create(10000+id*2,128476,nullptr)&&child->Create(10001+id*2,128479,nullptr),"Native item create failed");parent->SetOwnerGUID(p.GetGUID());child->SetOwnerGUID(p.GetGUID());parent->SetChildItem(child->GetGUID());parent->SetSlot(INVENTORY_SLOT_ITEM_START);(p.*Access(ClaimItemsTag{}))[INVENTORY_SLOT_ITEM_START]=parent.get();}
 void Pair(){auto& slots=player.*Access(ClaimItemsTag{});slots[INVENTORY_SLOT_ITEM_START]=nullptr;slots[EQUIPMENT_SLOT_MAINHAND]=parent.get();slots[EQUIPMENT_SLOT_OFFHAND]=child.get();parent->SetSlot(EQUIPMENT_SLOT_MAINHAND);child->SetSlot(EQUIPMENT_SLOT_OFFHAND);}
 ~ClaimInventory(){auto& slots=player.*Access(ClaimItemsTag{});slots[INVENTORY_SLOT_ITEM_START]=slots[EQUIPMENT_SLOT_MAINHAND]=slots[EQUIPMENT_SLOT_OFFHAND]=nullptr;}
};
static uint64 ClaimCredit(Scenario* scenario){auto p=scenario->GetAchievementMgr().GetCriteriaProgressMap();auto f=p->find(47371);return f==p->end()?0:f->second.Counter;}
int main(int argc,char** argv){try{
 InitializeAltheaGuardsNative(argc,argv);dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1620,{}},{1607,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1620,{}},{1607,{}}});sObjectMgr->LoadTrinityStrings();sObjectMgr->LoadPageTexts();sObjectMgr->LoadGameObjectTemplate();LoadGameObjectModelList(sWorld->GetDataPath());HashMapHolder<GameObject>::SetSize(20000);
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new instance_devourer;new go_devourer_claim_probe;sScriptMgr->SwapScriptContext(true);uint32 cases=0;
 for(uint32 race:{uint32(RACE_HUMAN),uint32(RACE_ORC)})for(uint32 mode=0;mode<8;++mode){
  MapInstanced parent(1607,300);InstanceMap map(1607,300,9927000+cases,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);auto scenario=sScenarioMgr->AddScenario(&map,1078);scenario->SetCurrentStep(6);(*const_cast<CriteriaProgressMap*>(scenario->GetAchievementMgr().GetCriteriaProgressMap()))[47369].Counter=1;auto session=Session(),memberSession=Session();
  {Position pose{1854.05f,1275.41f,91.7165f,1.52036f};NativePlayer player(session,map,101,pose),member(memberSession,map,102,pose);player.SetByteValue(UNIT_FIELD_BYTES_0,0,race);player.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_ROGUE);player.setFactionForRace(race);player.SetQuestStatus(41924,mode==7?QUEST_STATUS_COMPLETE:QUEST_STATUS_INCOMPLETE);player.SetQuestSlot(0,41924);auto exact=player.GetGUID();ClaimInventory items(player,cases);std::unique_ptr<CreditParty> party;if(mode==7){member.SetByteValue(UNIT_FIELD_BYTES_0,0,race);member.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_ROGUE);member.setFactionForRace(race);member.SetQuestStatus(41924,QUEST_STATUS_INCOMPLETE);member.SetQuestSlot(0,41924);party=std::make_unique<CreditParty>(&player,&member);}
   ClaimGO go;Require(go.Create(7000+cases,249347,&map,1,pose,G3D::Quat(0,0,std::sin(pose.GetOrientation()/2),std::cos(pose.GetOrientation()/2)),255,GO_STATE_READY)&&map.AddToMap(static_cast<GameObject*>(&go)),"Native249347 create/admission failed");Require(go.AI()&&DevourerClaimNativeScope::CanTake(&player,&go),"Native original pickup admission unavailable");auto request=exact;if(mode==1)request.SetRawValue(exact.GetHighPart()^1,exact.GetLowPart());if(mode==2)request.SetRawValue(exact.GetHighPart(),exact.GetLowPart()^(uint64(1)<<40));Require(ObjectAccessor::GetPlayer(go,request)==&player,"Native same-index GUID control absent");go.SetLootState(GO_READY);if(mode==0){CreditPet pet(player,map);CreditGuardian guardian(player,map);for(Unit* target:{static_cast<Unit*>(&pet),static_cast<Unit*>(&guardian)}){go.SetLootState(GO_JUST_DEACTIVATED,target);Require(go.AI()->GetGUID(0).IsEmpty()&&!ClaimCredit(scenario),"Nonplayer loot target started claim");go.SetLootState(GO_READY);}}if(mode==0||mode==7)go.SetLootState(GO_JUST_DEACTIVATED,&player);else go.AI()->SetGUID(request,0);bool pending=!go.AI()->GetGUID(0).IsEmpty();
#ifdef DEVOURER_CLAIM_BASELINE
   Require(pending,"Pinned valid claim not armed");
#else
   Require(pending==(mode!=1&&mode!=2),"Native fullGUID start guard differs");
#endif
   if(mode==3){player.SetQuestStatus(41924,QUEST_STATUS_NONE);player.SetQuestStatus(41924,QUEST_STATUS_INCOMPLETE);}if(mode==4){auto status=player.getQuestStatus(41924);Require(status,"Quest status unavailable");status->ScriptObjects=std::make_shared<QuestObjectScope>();status->ScriptObjects->Owner=player.GetGUID();}if(mode==5){auto other=exact;other.SetRawValue(exact.GetHighPart()^1,exact.GetLowPart());player.SetGuidValue(OBJECT_FIELD_GUID,other);}if(mode==6){player.SetQuestStatus(41924,QUEST_STATUS_NONE);}
   auto playbackCaster=player.GetGUID();std::cout<<"CASE race="<<race<<" mode="<<mode<<std::endl;items.Pair();go.AI()->UpdateAI(2000);
#ifdef DEVOURER_CLAIM_BASELINE
   bool credited=mode!=6;
#else
   bool credited=mode==0||mode==7;
#endif
   Require(ClaimCredit(scenario)==uint64(credited)&&scenario->GetCurrentStep()==(credited?7:6),"Native50301 receipt guard/progression differs");Require(go.AI()->GetData(0)==uint32(credited),"Native finished latch differs");player.SetGuidValue(OBJECT_FIELD_GUID,exact);Require(player.HasItemCount(128476,1)&&player.GetItemByEntry(128476)==items.parent.get(),"Canceled claim lost native owned artifact");
   if(!credited){Require(go.AI()->GetGUID(0).IsEmpty()&&go.getLootState()==GO_READY,"Canceled claim did not retain retry");player.SetQuestStatus(41924,QUEST_STATUS_INCOMPLETE);Require(go.AI()->GossipHello(&player,false)==false,"Retry admission blocked");go.AI()->SetGUID(exact,0);go.AI()->UpdateAI(1);Require(ClaimCredit(scenario)==1&&scenario->GetCurrentStep()==7,"Explicit fresh native claim retry failed");}
   go.AI()->UpdateAI(5000);Require(ClaimCredit(scenario)==1,"Native credit replayed");uint32 stories=0;for(auto story:*Access(StoriesTag{}))if(story&&story->GetMap()==&map&&story->GetEntry()==1948){++stories;Require(story->GetDuration()==11150&&story->GetCasterGUID()==(credited?playbackCaster:exact)&&story->GetActors().size()==1&&story->GetActors()[0]->actorId==52490&&story->GetActors()[0]->creatureId==105464&&story->GetActors()[0]->displayId==69095&&story->GetLines().size()==1&&story->GetLines()[0]->id==4096,"Original1948 virtual payload/lifetime changed");Require(story->IsInPersonnalVisibilityList(member.GetGUID())==bool(party),"Original native group1948 audience changed");}Require(stories==1,"Native outro1948 missing/repeated");CleanStories(map);player.RemoveAllAuras();player.m_Events.KillAllEvents(true);
  }session->SetPlayer(nullptr);memberSession->SetPlayer(nullptr);delete session;delete memberSession;sScenarioMgr->RemoveScenario(map.GetInstanceId());++cases;
 }
#ifdef DEVOURER_CLAIM_BASELINE
 std::cout<<"COUNTERFACTUAL "<<cases<<" native whole pickup contexts: wrong fullGUID, pending identity and canceled/replaced attempt can send50301. Earlier quest/stage/victory/owned equipped pair controlled; no client proof\n";
#else
 std::cout<<"PASS "<<cases<<" native whole pickup contexts: fullGUID before cast/delayed50301, real receipt cancel/reaccept/replacement, retained native items/fresh retry/once, active COMPLETE/actual group/pet-guardian loot rejection and original1948 virtual payload11150; earlier stage/victory/equipped pair seeded, actual equip tick/client separate\n";
#endif
 CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
