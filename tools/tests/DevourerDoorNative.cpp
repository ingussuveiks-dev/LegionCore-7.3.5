// Whole door AI with native GameObject.Use/criteria/items/Map. Earlier stage,
// quest, key storage and cage/cache receipts are explicitly seeded endpoints.
#define ALTHEA_INTRO_NATIVE_SUPPORT_ONLY
#include "AltheaIntroNative.cpp"
#include "GameObjectModel.h"
#include "Item.h"
#include "DevourerInstance.inc"
#include "DoorAI.inc"
struct DoorItemsTag{using Type=Item* (Player::*)[PLAYER_SLOTS_COUNT];friend Type Access(DoorItemsTag);};
template struct FixtureAccess<DoorItemsTag,&Player::m_items>;
struct DoorObject:GameObject{~DoorObject(){if(IsInWorld())GetMap()->RemoveFromMap(static_cast<GameObject*>(this),false);ResetMap();}};
struct DoorKey{
 Player& player;Item* item=new Item;
 explicit DoorKey(Player& p,uint32 id):player(p){Require(item->Create(12000+id,136802,nullptr),"Native Wardstone create failed");item->SetOwnerGUID(p.GetGUID());item->SetSlot(INVENTORY_SLOT_ITEM_START);(p.*Access(DoorItemsTag{}))[INVENTORY_SLOT_ITEM_START]=item;}
 ~DoorKey(){auto& slot=(player.*Access(DoorItemsTag{}))[INVENTORY_SLOT_ITEM_START];if(slot==item){slot=nullptr;delete item;}}
};
static uint64 DoorCredit(Scenario* scenario){auto p=scenario->GetAchievementMgr().GetCriteriaProgressMap();auto f=p->find(48004);return f==p->end()?0:f->second.Counter;}
static void InitializeDoorNative(int argc,char** argv){ InitializeAltheaGuardsNative(argc,argv);dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1620,{}},{1607,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1620,{}},{1607,{}}});sObjectMgr->LoadTrinityStrings();sObjectMgr->LoadPageTexts();sObjectMgr->LoadGameObjectTemplate();LoadGameObjectModelList(sWorld->GetDataPath());HashMapHolder<GameObject>::SetSize(20000);
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new instance_devourer;new go_devourer_door_probe;sScriptMgr->SwapScriptContext(true);}
#ifndef DEVOURER_DOOR_SUPPORT_ONLY
int main(int argc,char** argv){try{
 InitializeDoorNative(argc,argv);uint32 cases=0;
 for(uint32 race:{uint32(RACE_HUMAN),uint32(RACE_ORC)})for(uint32 mode=0;mode<
#ifdef DEVOURER_DOOR_BASELINE
8
#else
12
#endif
;++mode){
  MapInstanced parent(1607,300);InstanceMap map(1607,300,9921000+cases,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);auto scenario=sScenarioMgr->AddScenario(&map,1078);scenario->SetCurrentStep(mode==3?1:2);auto& progress=*const_cast<CriteriaProgressMap*>(scenario->GetAchievementMgr().GetCriteriaProgressMap());auto session=Session(),memberSession=Session();
  {Position pose{1854.51f,1296.27f,62.1306f,0};NativePlayer player(session,map,101,pose),member(memberSession,map,102,pose);player.SetByteValue(UNIT_FIELD_BYTES_0,0,race);player.SetByteValue(UNIT_FIELD_BYTES_0,1,mode==4?CLASS_WARRIOR:CLASS_ROGUE);player.setFactionForRace(race);if(mode!=5){player.SetQuestStatus(41924,mode==10?QUEST_STATUS_COMPLETE:QUEST_STATUS_INCOMPLETE);player.SetQuestSlot(0,41924);}DoorKey key(player,cases);if(mode!=1&&mode!=3)player.UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,50908);if(mode!=1&&mode!=2&&mode!=3)Require(player.CastSpell(&player,211687,true)==SPELL_CAST_OK,"Actual reclaim211687 cast failed");
   std::unique_ptr<CreditParty> group;if(mode==8){member.SetByteValue(UNIT_FIELD_BYTES_0,0,race);member.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_ROGUE);member.setFactionForRace(race);member.SetQuestStatus(41924,QUEST_STATUS_INCOMPLETE);member.SetQuestSlot(0,41924);group=std::make_unique<CreditParty>(&player,&member);}
   if(mode==9){player.UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,50536);Require(scenario->GetCurrentStep()==3&&DoorCredit(scenario)==1,"Native earned checkpoint not established");player.DestroyItemCount(136802,1,true);}if(mode==11)player.DestroyItemCount(136802,1,true);
   DoorObject go,barrier;Require(go.Create(7200+cases*2,249390,&map,1,pose,G3D::Quat(0,0,0,1),255,GO_STATE_READY)&&map.AddToMap(static_cast<GameObject*>(&go)),"Native control create failed");Position b{1855.38f,1292.18f,62.5271f,0};Require(barrier.Create(7201+cases*2,249391,&map,1,b,G3D::Quat(0,0,0,1),255,GO_STATE_READY)&&map.AddToMap(static_cast<GameObject*>(&barrier)),"Native barrier create failed");go.SetLootState(GO_READY);barrier.SetLootState(GO_READY);Require(go.FindNearestGameObject(249391,10)==&barrier,"Native paired actor lookup differs");
   if(!mode){CreditPet pet(player,map);CreditGuardian guardian(player,map);for(Unit* target:{static_cast<Unit*>(&pet),static_cast<Unit*>(&guardian)}){go.Use(target);Require(player.HasItemCount(136802,1)&&!DoorCredit(scenario)&&barrier.GetPhaseMask()==1,"Nonplayer use awarded progression");}go.Use(&player);}else if(mode==6){auto result=player.CastSpell(&go,3365,false);Require(player.CastSpell(&go,3365,true)==SPELL_FAILED_DONT_REPORT,"Native triggered refusal mapping changed");Require(result==SPELL_FAILED_BAD_TARGETS&&player.HasItemCount(136802,1)&&!DoorCredit(scenario)&&barrier.GetPhaseMask()==1,"Lock0 Opening must fail without consumption");std::cout<<"CONTROL lock0 Opening3365 expected BAD_TARGETS13; independent ordinary Use follows"<<std::endl;go.Use(&player);}else go.AI()->GossipHello(&player,mode!=7);
#ifdef DEVOURER_DOOR_BASELINE
   bool hidden=mode!=0&&mode!=6&&mode!=7;bool credited=hidden&&mode!=3;Require(go.GetPhaseMask()==(hidden?4:1)&&barrier.GetPhaseMask()==1&&player.HasItemCount(136802,1)==!hidden&&DoorCredit(scenario)==uint64(credited),"Pinned door counterfactual changed");
#else
   bool success=mode==0||mode==6||mode==8||mode==9||mode==10;Require(go.GetPhaseMask()==(success?4:1)&&barrier.GetPhaseMask()==(success?4:1)&&player.HasItemCount(136802,1)==(!success&&mode!=11)&&DoorCredit(scenario)==uint64(success),"Native door admission/acknowledgement differs");if(success){Require(scenario->GetCurrentStep()==3,"Real native AND-root did not advance");go.Use(&player);go.AI()->GossipHello(&player,true);Require(DoorCredit(scenario)==1&&!player.HasItemCount(136802,1),"Native door replayed key/credit");}
#endif
   std::cout<<"CASE race="<<race<<" mode="<<mode<<std::endl;
  }session->SetPlayer(nullptr);memberSession->SetPlayer(nullptr);delete session;delete memberSession;sScenarioMgr->RemoveScenario(map.GetInstanceId());++cases;
 }
#ifdef DEVOURER_DOOR_BASELINE
 std::cout<<"COUNTERFACTUAL "<<cases<<" native door contexts: real Use misses50536; premature/class/no-quest/rejected-stage Hello consumes Wardstone and hides the control while its phase change prevents the original same-phase barrier lookup. Explicit earlier quest/key/receipts seeded, client separate\n";
#else
 std::cout<<"PASS "<<cases<<" native door contexts: actual Use and Hello, real cage/cache prerequisites/native50536 acknowledgement before Wardstone/both actors, invalid class/quest/stage/report retained and no replay. Earlier quest/key/receipts seeded, client separate\n";
#endif
 CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}

#endif
