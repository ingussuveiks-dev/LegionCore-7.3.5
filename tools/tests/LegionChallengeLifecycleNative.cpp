// Actual Challenge/Scenario factories, native Item storage, packets and scheduler.
// Roster publication is controlled; no sockets, persistent characters or combat run.
#define HARBARON_NATIVE_SUPPORT_ONLY
#include "HarbaronFragmentNative.cpp"
#include "ScenarioMgr.h"
#include "Item.h"
#include "ChallengeMgr.h"
#ifdef CHALLENGE_LIFECYCLE_BASELINE
#include "ChallengeActual.inc"
#endif

struct LifecyclePlayer:Player {
 LifecyclePlayer(InstanceMap& map,uint32 id,Position const& pose):Player(new WorldSession(0,"Isolated challenge fixture",{},SEC_PLAYER,6,0,"Win",LOCALE_enUS,0,false,AT_AUTH_FLAG_NONE,{})){
  _Create(ObjectGuid::Create<HighGuid::Player>(id));SetByteValue(UNIT_FIELD_BYTES_0,0,RACE_HUMAN);SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_HUNTER);SetLevel(110);setFactionForRace(RACE_HUMAN);auto race=sChrRacesStore.LookupEntry(RACE_HUMAN);SetDisplayId(race->MaleDisplayID);SetNativeDisplayId(race->MaleDisplayID);SetObjectScale(1.f);SetMaxHealth(1000000);SetHealth(1000000);
  SetMap(&map);Relocate(pose);GetZoneAndAreaId(m_zoneId,m_areaId);GetSession()->SetPlayer(this);sWorld->IncreasePlayerCount();sObjectAccessor->AddObject(static_cast<Player*>(this));map.LoadGrid(GetPositionX(),GetPositionY());map.AddToGrid(static_cast<Player*>(this),Cell(Trinity::ComputeCellCoord(GetPositionX(),GetPositionY())));Unit::AddToWorld();GetMotionMaster()->Initialize();GetMapRef().link(&map,this);
 }
 ~LifecyclePlayer(){SetGroup(nullptr);CombatStop(true);CastStop();RemoveAllAuras();m_Events.KillAllEvents(true);for(uint8 slot=PLAYER_SLOT_START;slot<PLAYER_SLOT_END;++slot)if(auto item=GetItemByPos(INVENTORY_SLOT_BAG_0,slot))if(item->IsInWorld())item->RemoveFromWorld();GetMapRef().unlink();Unit::RemoveFromWorld();RemoveFromGrid();sObjectAccessor->RemoveObject(static_cast<Player*>(this));GetSession()->SetPlayer(nullptr);ResetMap();}
 Item* Key(MapChallengeModeEntry const& route){ItemPosCountVec where;Require(CanStoreNewItem(NULL_BAG,NULL_SLOT,where,138019,1)==EQUIP_ERR_OK,"Native keystone inventory admission failed");auto item=StoreNewItem(where,138019,true);Require(item,"Native keystone storage failed");item->SetModifier(ITEM_MODIFIER_CHALLENGE_ID,route.ID);item->SetModifier(ITEM_MODIFIER_CHALLENGE_KEYSTONE_LEVEL,10);UpdateChallengeKey(item);m_challengeKeyInfo.Affix=uint32(Affixes::Quaking);m_challengeKeyInfo.Affix1=0;m_challengeKeyInfo.Affix2=0;m_challengeKeyInfo.challengeEntry=&route;return item;}
};
struct LifecycleGroup {
 Group group;LifecyclePlayer& owner;LifecyclePlayer* member;
 LifecycleGroup(LifecyclePlayer& p,LifecyclePlayer* other,MapChallengeModeEntry const& route,Item& key):owner(p),member(other){if(member){group.m_challengeOwner=p.GetGUID();group.m_challengeItem=key.GetGUID();group.m_challengeEntry=&route;group.m_challengeLevel=10;group.m_affixes={uint32(Affixes::Quaking),0,0};owner.SetGroup(&group,0);member->SetGroup(&group,0);}}
 ~LifecycleGroup(){owner.SetGroup(nullptr);if(member)member->SetGroup(nullptr);}
};

#ifndef CHALLENGE_LIFECYCLE_SUPPORT_ONLY
int main(int argc,char**argv){try{
 InitializeDungeon(argc,argv,true);sObjectMgr->LoadItemTemplates();sObjectMgr->LoadItemTemplateAddon();sObjectMgr->LoadScenarioData();
 std::cout<<"CATALOG quake="<<bool(sSpellMgr->GetSpellInfo(240447))<<" antikick="<<bool(sSpellMgr->GetSpellInfo(305284))<<" nativeKey="<<bool(sObjectMgr->GetItemTemplate(138019))<<'\n';
 unsigned routes=0;
 for(auto route:sMapChallengeModeStore)if(route->MapID==1456||route->MapID==1458||route->MapID==1466||route->MapID==1477||route->MapID==1492||route->MapID==1493||route->MapID==1501||route->MapID==1516||route->MapID==1571||route->MapID==1651||route->MapID==1677||route->MapID==1753)for(bool grouped:{false,true}){
  MapInstanced parent(route->MapID,300);InstanceMap map(route->MapID,300,9966000+routes,DIFFICULTY_MYTHIC_KEYSTONE,&parent);map.CreateInstanceData(nullptr);Position initial{2934.49f,711.757f,554.286f,1.55255f};LifecyclePlayer owner(map,201,initial);auto key=owner.Key(*route);float x=0,y=0,z=0,o=0;Require(ChallengeMgr::GetStartPosition(map.GetId(),x,y,z,o,owner.GetGUID()),"Native route start missing");
  auto member=grouped?std::make_unique<LifecyclePlayer>(map,202,initial):nullptr;LifecycleGroup group(owner,member.get(),*route,*key);
  Scenario scenario(&map,1175);scenario.CreateChallenge(&owner);auto challenge=scenario.GetChallenge();Check(challenge&&challenge->_canRun&&challenge->GetInstanceScript()==map.GetInstanceScript(),"Actual challenge/scenario attachment failed");Check(challenge->CanStart(),"Native accepted roster not ready");
#ifndef CHALLENGE_LIFECYCLE_BASELINE
  {Challenge invalid(&map,nullptr,map.GetInstanceId(),nullptr);invalid.Update(100);invalid.OnPlayerDiesForScript(&owner);invalid.Complete();Check(!invalid.CanStart(),"Invalid native challenge accepted a start");}
  challenge->Complete();Check(!challenge->_complete,"Challenge completed before a timed run started");
#endif
  challenge->Update(100);Check(!challenge->_checkStart&&!challenge->_run&&challenge->_challengeTimer==0,"Native countdown not scheduled");
  challenge->Update(9000);
#ifndef CHALLENGE_LIFECYCLE_BASELINE
  auto functions=challenge->m_Functions.Size()+challenge->m_Functions.SizeQueue();challenge->Start();Check(challenge->m_Functions.Size()+challenge->m_Functions.SizeQueue()==functions,"Repeated start queued a second countdown");
#endif
#ifdef CHALLENGE_LIFECYCLE_BASELINE
  Check(challenge->_challengeTimer==9000&&challenge->_affixQuakingTimer==11000,"Expected original countdown timer/affix contamination changed");
#else
  Check(challenge->_challengeTimer==0&&challenge->_affixQuakingTimer==20000,"Countdown advanced actual run or affix time");
#endif
  challenge->OnPlayerDiesForScript(&owner);
#ifdef CHALLENGE_LIFECYCLE_BASELINE
  Check(challenge->_challengeTimer==14000,"Expected original pre-run death penalty changed");
#else
  Check(challenge->_challengeTimer==0,"Death before running consumed a timed-run penalty");
#endif
  challenge->Update(1000);Check(challenge->_run,"Actual ten-second callback failed to start");
#ifdef CHALLENGE_LIFECYCLE_BASELINE
  Check(challenge->_challengeTimer==1000&&challenge->_affixQuakingTimer==10000,"Expected original callback tick charge changed");
#else
  Check(challenge->_challengeTimer==0&&challenge->_affixQuakingTimer==20000,"Start callback charged countdown tick to active run");
#endif
  challenge->Update(1000);auto active=challenge->_challengeTimer;challenge->OnPlayerDiesForScript(&owner);Check(challenge->_challengeTimer==active+5000,"Real running death lost native five-second penalty");
  // A different player cannot stand in for an expected missing challenger.
  Position pose{x,y,z,o};if(member)member->GetMapRef().unlink();{LifecyclePlayer outsider(map,203,pose);challenge->_challengers.insert(ObjectGuid::Create<HighGuid::Player>(202));challenge->_run=false;
#ifdef CHALLENGE_LIFECYCLE_BASELINE
   Check(challenge->CanStart(),"Expected original count-only roster substitution changed");
#else
   Check(!challenge->CanStart(),"Unregistered outsider replaced an expected missing challenger");
#endif
   if(!member)challenge->_challengers.erase(ObjectGuid::Create<HighGuid::Player>(202));challenge->_run=true;
#ifndef CHALLENGE_LIFECYCLE_BASELINE
   auto before=challenge->_challengeTimer;challenge->OnPlayerDiesForScript(&outsider);challenge->OnPlayerDiesForScript(nullptr);Check(challenge->_challengeTimer==before,"Unregistered/null death changed run time");
#endif
  }
  if(member)member->GetMapRef().link(&map,member.get());
#ifndef CHALLENGE_LIFECYCLE_BASELINE
  challenge->Update(9000);Check(challenge->_challengeTimer==15000&&challenge->_affixQuakingTimer==10000,"Native running clock/affix clock did not remain independent of death penalty");
  challenge->Update(9999);Check(challenge->_affixQuakingTimer==1,"Quaking fired before20 actual running seconds");challenge->Update(1);Check(challenge->_affixQuakingTimer==20000,"Native Quaking did not rearm after20 running seconds");
#endif
  challenge->_complete=true;auto finished=challenge->_challengeTimer;challenge->OnPlayerDiesForScript(&owner);
#ifdef CHALLENGE_LIFECYCLE_BASELINE
  Check(challenge->_challengeTimer==finished+5000,"Expected original post-completion death penalty changed");
#else
  Check(challenge->_challengeTimer==finished,"Death changed an already completed run");
#endif
  std::cout<<"ROUTE map="<<route->MapID<<" id="<<route->ID<<" group="<<grouped<<" scenario="<<scenario.GetScenarioId()<<" start="<<x<<','<<y<<','<<z<<" key="<<key->GetModifier(ITEM_MODIFIER_CHALLENGE_ID)<<'\n';++routes;
 }
 Check(routes==26,"Native challenge route scope changed");
#ifdef CHALLENGE_LIFECYCLE_BASELINE
 std::cout<<"COUNTERFACTUAL "<<checked<<" native checks reproduce countdown/affix timer contamination,callback-tick charge,count-only missing-player substitution and pre/post-run death penalties across13 routes;not ordinary successful runs\n";
#else
 std::cout<<"PASS "<<checked<<" actual native challenge lifecycle checks across13 routes;ordinary registered instances/native scenario attachment,stored keys,real scheduler and scoped roster publication. Completion flag is a controlled checkpoint;no earned full dungeon/client claim\n";
#endif
 CharacterDatabase.Close();LoginDatabase.Close();WorldDatabase.Close();HotfixDatabase.Close();MySQL::Library_End();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
#endif
