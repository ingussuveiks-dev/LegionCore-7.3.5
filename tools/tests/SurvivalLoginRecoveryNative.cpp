// Native quest/item/Prepare/CanResume/Scenario components; accepted quest-row,
// saved-transport/position, selected-map, zone and teleport endpoints controlled.
#define main WarriorRecoveryUnusedMain
#include "WarriorLoginRecoveryNative.cpp"
#undef main
#include "Bag.h"
#include "PlayerDefines.h"
struct HunterInstanceTag {using Type=uint32 Player::*;friend Type Access(HunterInstanceTag);};
template struct FixtureAccess<HunterInstanceTag,&Player::m_scenarioLoginInstance>;
struct HunterItemsTag {using Type=Item* (Player::*)[PLAYER_SLOTS_COUNT];friend Type Access(HunterItemsTag);};
template struct FixtureAccess<HunterItemsTag,&Player::m_items>;
static void Objective(Player& p,int32 entry,int32 value){auto q=sQuestDataStore->GetQuestTemplate(40385);Require(q,"Actual Hunter quest missing");for(auto const& o:q->GetObjectives())if(o.ObjectID==entry){Require(o.StorageIndex>=0&&o.StorageIndex<=2&&o.Amount==1,"Actual receipt storage/amount changed");p.SetQuestObjectiveData(q,&o,value,true);return;}throw std::runtime_error("Actual Hunter receipt missing");}
struct HunterPlayer : RecoveryPlayer {
 using RecoveryPlayer::RecoveryPlayer;
 struct ZoneEndpoint {bool valid=true;uint32 GetZoneId(uint32 map,float,float,float){Require(map==1220,"Wrong native exit map");return valid?7502:7541;}} zone;
 bool transferAccepted=true;unsigned transfers=0,screenRemovals=0;
 bool TeleportTo(uint32 map,float x,float y,float z,float o){++transfers;if(!transferAccepted)return false;GetTeleportDest().WorldRelocate(map,x,y,z,o);SetSemaphoreTeleportFar(true);return true;}
 void RemoveAurasDueToSpell(uint32 id){Require(id==205589,"Login exit removed unrelated aura");++screenRemovals;Unit::RemoveAurasDueToSpell(id);}
 void Binding(Map* map,uint32 mapId){auto& m_scenarioResume=this->*Access(RecoveryResumeTag{});auto& m_scenarioLoginId=this->*Access(RecoveryIdTag{});auto& m_scenarioLoginInstance=this->*Access(HunterInstanceTag{});
#include "SurvivalBindingNative.inc"
 (void)map;(void)mapId;(void)m_scenarioResume;(void)m_scenarioLoginId;(void)m_scenarioLoginInstance;
 }
 bool ExitCompletedTalonclawLogin(){
#ifdef SURVIVAL_LOGIN_BASELINE
 return false; // The pinned handler has no such call or implementation.
#else
 auto& m_scenarioResume=this->*Access(RecoveryResumeTag{});auto& m_scenarioLoginId=this->*Access(RecoveryIdTag{});auto& m_scenarioLoginInstance=this->*Access(HunterInstanceTag{});
#pragma push_macro("sMapMgr")
#undef sMapMgr
#define sMapMgr (&zone)
#include "TalonclawLoginExitNative.inc"
#pragma pop_macro("sMapMgr")
#endif
 }
 bool Rejected(bool validLoginMap){auto player=this;
#include "HunterLoginCaller.inc"
 }
};
struct HeldSpear {
 HunterPlayer& p;Item* spear=nullptr;Bag* bag=nullptr;uint8 slot=EQUIPMENT_SLOT_MAINHAND;
 HeldSpear(HunterPlayer& owner,unsigned mode):p(owner){
  if(mode==11)return;spear=Item::CreateItem(128808,1,&p);Require(spear&&!spear->IsInWorld(),"Native loading spear creation failed");
  auto& slots=p.*Access(HunterItemsTag{});
  if(mode==1||mode==2){auto item=Item::CreateItem(21841,1,&p);bag=item?item->ToBag():nullptr;Require(bag&&bag->GetBagSize()&&!bag->IsInWorld(),"Native loading bag missing");slot=mode==1?INVENTORY_SLOT_BAG_START:BANK_SLOT_BAG_START;Require(!slots[slot],"Fixture bag slot occupied");slots[slot]=bag;bag->SetSlot(slot);bag->StoreItem(0,spear,false);}
  else{Require(!slots[slot],"Fixture spear slot occupied");slots[slot]=spear;spear->SetSlot(slot);}
  if(mode==12)spear->SetGuidValue(ITEM_FIELD_OWNER,ObjectGuid::Create<HighGuid::Player>(999888));if(mode==13)spear->SetCount(0);
 }
 ~HeldSpear(){if(bag)bag->RemoveItem(0,false);(p.*Access(HunterItemsTag{}))[slot]=nullptr;delete bag;delete spear;}
};
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);sAreaTriggerDataStore->LoadAreaTriggerTeleports();sLFGMgr->LoadLFGDungeons();sSpellMgr->LoadSpellTargetPositions();
 auto quest=const_cast<Quest*>(sQuestDataStore->GetQuestTemplate(40385));Require(quest&&quest->GetObjectives().empty(),"Actual40385 bootstrap missing/already populated");auto objectives=WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged FROM quest_objectives WHERE QuestID=40385 ORDER BY StorageIndex");Require(bool(objectives),"Actual40385 objective rows missing");do{quest->LoadQuestObjective(objectives->Fetch());}while(objectives->NextRow());Require(quest->GetObjectives().size()==3,"Actual40385 objective count changed");auto& targets=sSpellMgr->*Access(RecoveryTargetsTag{});auto target=targets.at(198950);auto exit=targets.at(190598);auto portal=sSpellMgr->GetSpellInfo(190598);Require(portal&&portal->HasEffect(SPELL_EFFECT_TELEPORT_L)&&exit.target_mapId==1220,"Actual native Dalaran exit changed");
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1511,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1511,{}}});
 unsigned early=0,fresh=0,owners=0,returned=0;
 for(unsigned race:{unsigned(RACE_HUMAN),unsigned(RACE_ORC)}){
  auto session=Session();unsigned instanceId=9998700+race;ScenarioResume prepared;Position late{3601.95f,1782.54f,1.86404f,0};
  {
   MapInstanced parent(1511,300);InstanceMap map(1511,300,instanceId,DIFFICULTY_N_SCENARIO,&parent);auto progress=sScenarioMgr->AddScenario(&map,973);progress->SetCurrentStep(3);
   for(unsigned kind=0;kind<31;++kind){HunterPlayer p(session,map,801,late);p.acceptedQuest=40385;p.SetByteValue(UNIT_FIELD_BYTES_0,0,race);p.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_HUNTER);p.setFactionForRace(race);auto& receipt=p.*Access(RecoveryResumeTag{});bool existing=kind==5||kind==6||kind==7||kind==24||kind==25||kind==26||kind==27||kind==29;
    if(existing)receipt={1511,973,instanceId,uint64(GameTime::GetStartTime()),101,202,30,1};if(kind==1)Require(!receipt.Read("malformed"),"Malformed receipt accepted");if(kind==2)receipt={1511,944,instanceId,uint64(GameTime::GetStartTime()),1,2,3,0};if(kind==3)receipt={1539,973,instanceId,uint64(GameTime::GetStartTime()),1,2,3,0};if(kind==4)receipt={1511,973,instanceId,uint64(GameTime::GetStartTime()),std::numeric_limits<float>::quiet_NaN(),2,3,0};if(kind==6||kind==25)++receipt.boot;
    if(kind==7||kind==8)p.holder.status=QUEST_STATUS_COMPLETE;if(kind==9)p.holder.status=QUEST_STATUS_NONE;if(kind==10)p.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_WARLOCK);if(kind==13)p.validInput=false;if(kind==14)targets.erase(198950);if(kind==15)targets.at(198950).target_mapId=1495;if(kind==16){auto& t=targets.at(198950);t.target_X=t.target_Y=t.target_Z=0;}if(kind==17)targets.at(198950).target_X=std::numeric_limits<float>::quiet_NaN();if(kind==18)targets.at(198950).target_Orientation=std::numeric_limits<float>::infinity();if(kind==19)targets.at(198950).target_X=20000;if(kind==20)p.acceptedQuest=40043;if(kind==30){sScenarioMgr->RemoveScenario(instanceId);progress=nullptr;}
    bool loaded=p.Early(kind==11?1539:1511,instanceId,kind==12?AT_LOGIN_UNLOCK:0);p.Normal(loaded);Require(p.loads==1,"Native accepted status was loaded more than once");if(kind==21||kind==26)const_cast<RewardedQuestSet&>(p.getRewardedQuests()).insert(40385u);if(kind==22||kind==27)p.SetQuestStatus(40385,QUEST_STATUS_NONE);if(kind==23||kind==24||kind==25)Objective(p,100112,1);
    bool seeded=kind<=4||kind==21||kind==22||kind==23||kind==28||kind==30;
#ifdef SURVIVAL_LOGIN_BASELINE
    if(seeded)Require(!p.GetScenarioId()&&!loaded,"Pinned early loader unexpectedly recovered Hunter");bool allowed=existing&&kind!=25&&kind!=26&&kind!=27;
#else
    if(seeded){Require(p.GetScenarioId()==973&&!p.GetScenarioLoginInstance()&&!receipt.instance&&!receipt.boot&&p.GetExactDist(target.target_X,target.target_Y,target.target_Z)<0.001f,"Actual early Hunter entrance-only recovery failed");if(!kind)prepared=receipt;}
    if(kind==28||kind==29||kind==30)p.Binding(&map,1511);if(kind==28)Require(!p.GetScenarioId()&&!receipt.scenario&&!p.GetScenarioLoginInstance(),"Empty receipt inherited an already live bound instance");if(kind==29)Require(p.GetScenarioLoginInstance()==instanceId,"Valid exact live receipt blocked by binding guard");
    bool allowed=(seeded||existing)&&kind!=21&&kind!=22&&kind!=23&&kind!=25&&kind!=26&&kind!=27&&kind!=28;
#endif
    Require(p.CanResumeScenarioLogin(973)==allowed,"Native late Hunter admission/claim guard differs");if(existing)Require(receipt.x==101,"Existing valid Hunter receipt overwritten");if(!seeded&&!existing)Require(!p.GetScenarioId(),"Refused native source unexpectedly selected a scenario");if(progress)Require(progress->GetCurrentStep()==3,"Recovery granted stage progress");Require(!p.HasItemCount(128808,1,true),"Login granted artifact");p.Relocate(late);targets.insert_or_assign(198950,target);++early;
   }
  }
#ifndef SURVIVAL_LOGIN_BASELINE
  for(unsigned dungeonId:{1178u,1384u}){MapInstanced parent(1511,300);InstanceMap map(1511,300,instanceId,DIFFICULTY_N_SCENARIO,&parent);{HunterPlayer p(session,map,801,late);p.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_HUNTER);p.SetQuestStatus(40385,QUEST_STATUS_INCOMPLETE);Require((p.*Access(RecoveryResumeTag{})).Read(prepared.Write()),"Prepared native seed roundtrip failed");p.PrepareScenarioLogin(1511,0);p.Binding(&map,1511);auto sc=sScenarioMgr->AddScenario(&map,sLFGMgr->GetLFGDungeon(dungeonId),&p,false);Require(sc&&sc->GetScenarioId()==973&&sc->GetCurrentStep()==0&&!p.GetScenarioId()&&p.CanResumeScenarioLogin(973),"Actual fresh constructor did not consume Hunter seed at step0");p.Relocate(late);++fresh;}sScenarioMgr->RemoveScenario(instanceId);}
#endif
  {
   MapInstanced parent(1511,300);InstanceMap map(1511,300,instanceId+20,DIFFICULTY_N_SCENARIO,&parent);
   for(unsigned mode=0;mode<27;++mode){auto sc=sScenarioMgr->AddScenario(&map,mode==15?944:973);HunterPlayer p(session,map,802,late);p.SetByteValue(UNIT_FIELD_BYTES_0,0,race);p.SetByteValue(UNIT_FIELD_BYTES_0,1,mode==26?CLASS_WARLOCK:CLASS_HUNTER);p.setFactionForRace(race);p.SetQuestStatus(40385,mode==3?QUEST_STATUS_COMPLETE:QUEST_STATUS_INCOMPLETE);Objective(p,100111,mode==8?0:1);Objective(p,100112,mode==9?0:1);Objective(p,103798,mode==10?1:0);if(mode==5)p.SetQuestStatus(40385,QUEST_STATUS_NONE);if(mode==6)const_cast<RewardedQuestSet&>(p.getRewardedQuests()).insert(40385u);if(mode==7)p.SetGameMaster(true);if(mode==14)(p.*Access(RecoveryIdTag{}))=944;
    if(mode==23){sc->SetCurrentStep(12);(p.*Access(RecoveryResumeTag{}))={1511,973,map.GetInstanceId(),uint64(GameTime::GetStartTime()),target.target_X,target.target_Y,target.target_Z,target.target_Orientation};(p.*Access(RecoveryIdTag{}))=973;(p.*Access(HunterInstanceTag{}))=map.GetInstanceId();Require(p.CanResumeScenarioLogin(973),"Existing live claimed owner lost native admission");}
    HeldSpear held(p,mode);if(mode==4){p.SetHealth(0);p.setDeathState(CORPSE);p.CastSpell(&p,8326,true);p.SetHealth(1);}if(mode==16)targets.erase(190598);if(mode==17)targets.at(190598).target_mapId=1511;if(mode==18)targets.at(190598).target_X=std::numeric_limits<float>::quiet_NaN();if(mode==19)targets.at(190598).target_Orientation=std::numeric_limits<float>::infinity();if(mode==20)targets.at(190598).target_X=20000;if(mode==21)p.zone.valid=false;if(mode==22)p.transferAccepted=false;if(mode==24||mode==25){p.GetTeleportDest().WorldRelocate(mode==24?1220:1221,exit.target_X,exit.target_Y,exit.target_Z,exit.target_Orientation);p.SetSemaphoreTeleportFar(true);}
    bool rejected=p.Rejected(mode==23);bool eligible=mode<=4||mode==24;
#ifdef SURVIVAL_LOGIN_BASELINE
    Require(rejected==(mode!=23)&&!p.transfers&&!p.screenRemovals,"Pinned handler unexpectedly contains claimed-owner exit");
#else
    Require(rejected==(!eligible&&mode!=23),"Actual login fallback condition failed valid-map or owned-exit gate");Require(p.transfers==unsigned(mode<=4||mode==22)&&p.screenRemovals==unsigned(mode<=4),"Owned exit repeated/refused an unexpected transfer or cleanup");if(mode<=4){Require(p.GetTeleportDest().GetMapId()==1220&&p.GetTeleportDest().GetExactDist(exit.target_X,exit.target_Y,exit.target_Z)<0.001f&&!p.GetScenarioId()&&!(p.*Access(RecoveryResumeTag{})).scenario,"Accepted native-source exit did not clear its checkpoint");Require(!p.Rejected(false)&&p.transfers==1&&p.screenRemovals==1,"Repeated pending native exit was queued again");++returned;}
#endif
    if(held.spear)Require(p.GetQuestObjectiveData(40385,103798)==(mode==10?1u:0u)&&held.spear->GetEntry()==128808,"Login helper awarded return/changed artifact");if(mode==4)Require(!p.IsAlive()&&p.HasAura(8326),"Login exit resurrected a dead owner");Require(sc->GetCurrentStep()==(mode==23?12u:0u),"Owned exit granted scenario progress");targets.insert_or_assign(190598,exit);sScenarioMgr->RemoveScenario(map.GetInstanceId());++owners;
   }
  }
  session->SetPlayer(nullptr);delete session;
 }
 Require(early==62&&owners==54,"Native Hunter regression counts changed");
#ifdef SURVIVAL_LOGIN_BASELINE
 Require(!fresh&&!returned,"Pinned early/caller counterfactual granted recovery");std::cout<<"COUNTERFACTUAL pinned0af584162 early/caller capsules116:Hunter missing-entry selection absent and no claimed-owner login exit;current native support,not historical whole binary/ordinary success"<<std::endl;
#else
 Require(fresh==4&&returned==10,"Native fresh/owned recovery counts changed");std::cout<<"PASS native Hunter login120:early62/fresh4/owner54 with10 accepted native-source exits,one accepted quest load,source/status/claim/binding guards,real loading/equipment/inventory-bag/bank-bag items,pending/duplicate/failed transfer and valid-map short circuit;no credit/item/resurrection. Quest parser/transport/position,selected-map,zone/teleport and supplied statuses controlled;full character/client/group/arrival separate"<<std::endl;
#endif
 CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}
