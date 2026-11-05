// Whole real Valzuun callbacks, native quest objectives/spells/scope and GO use.
// Admission/poses and update clocks controlled; far request is not client travel.
#define DEVOURER_ESCAPE_SUPPORT_ONLY
#include "DevourerEscapeNative.cpp"
#include "CreatureTextMgr.h"
#include "GameObjectPackets.h"
#include "LFGMgr.h"
#include "MapManager.h"
#include "CombatAI.h"
#include "AreaTriggerData.h"
#include "Portal.inc"
#ifdef DEVOURER_PORTAL_FAULTS
static bool refuseOpening=false;static uint32 refusalCalls=0;
class zzzz_devourer_portal_refusal_probe:public SpellScriptLoader{
public:zzzz_devourer_portal_refusal_probe():SpellScriptLoader("zzzz_devourer_portal_refusal_probe"){}
 class Hook:public SpellScript{PrepareSpellScript(Hook);SpellCastResult Reject(){if(refuseOpening){++refusalCalls;return SPELL_FAILED_NOT_READY;}return SPELL_CAST_OK;}void Register()override{OnCheckCast+=SpellCheckCastFn(Hook::Reject);}};
 SpellScript* GetSpellScript()const override{return new Hook;}
};
#endif
static std::vector<ObjectGuid> PortalActors(Creature& source){std::list<Creature*> found;GetCreatureListWithEntryInGrid(found,&source,105488,100);std::vector<ObjectGuid> guids;for(auto c:found)if(c->IsInWorld()&&!c->IsPreDelete())guids.push_back(c->GetGUID());return guids;}
static std::vector<ObjectGuid> PortalObjects(Creature& source){std::list<GameObject*> found;GetGameObjectListWithEntryInGrid(found,&source,248901,100);std::vector<ObjectGuid> guids;for(auto c:found)if(c->IsInWorld()&&!c->IsPreDelete())guids.push_back(c->GetGUID());return guids;}
static void PortalTick(NativePlayer& player,NativePlayer& member,Creature& source,uint32 ms){for(uint32 t=0;t<ms;t+=100){for(auto guid:PortalActors(source))if(auto actor=ObjectAccessor::GetCreature(source,guid))actor->Update(100);for(auto guid:PortalObjects(source))if(auto go=ObjectAccessor::GetGameObject(source,guid))go->Update(100);player.Unit::Update(100);member.Unit::Update(100);source.Unit::Update(100);source.GetMap()->RemoveAllObjectsInRemoveList();}}
struct PortalTravelSession
{
 Player& player;Map* destination;
 explicit PortalTravelSession(Player& p):player(p){
  // Private probe workers only. Keep real SafeTeleport/session lookup and map
  // creation; stop their asynchronous loops because this probe drives clocks.
  auto base=sMapMgr->CreateBaseMap(1607);base->SetMapStop();auto thread=sMapMgr->_mapThreads[1607];if(thread&&thread->joinable())thread->join();
  destination=sMapMgr->CreateMap(1607,&player);Require(destination,"Native portal destination creation failed");destination->SetMapStop();
  auto instances=static_cast<MapInstanced*>(base);if(base->CanCreatedZone())for(auto& zone:instances->_zoneThreads)if(zone.second&&zone.second->joinable())zone.second->join();
  auto& sessions=const_cast<SessionMap&>(sWorld->GetAllSessions());Require(!sessions.count(p.GetSession()->GetAccountId()),"Portal fixture would replace another session");sessions[p.GetSession()->GetAccountId()]=WorldSessionPtr(p.GetSession(),[](WorldSession*){});p.GetSession()->SetMap(p.GetMap());
 }
 ~PortalTravelSession(){WorldSessionPtr queued;while(destination->addSessQueue.next(queued))Require(queued.get()==player.GetSession(),"Unexpected foreign destination session");destination->m_sessions.erase(player.GetSession()->GetAccountId());const_cast<SessionMap&>(sWorld->GetAllSessions()).erase(player.GetSession()->GetAccountId());player.GetSession()->SetMap(nullptr);}
};
static void UsePortal(Player& player,ObjectGuid guid){PortalTravelSession travel(player);WorldPacket wire(CMSG_GAME_OBJ_USE);wire<<guid;WorldPackets::GameObject::GameObjectUse request(std::move(wire));request.Read();player.GetSession()->HandleGameObjectUse(request);}
static void Accept(Player& player,Creature& source,Quest const* quest){Require(sScriptMgr->OnQuestAccept(&player,&source,quest),"Native acceptance dispatch failed");}
static void Retake(Player& player){player.RemoveActiveQuest(41924);player.SetQuestSlot(0,0);player.SetQuestStatus(41924,QUEST_STATUS_INCOMPLETE);player.SetQuestSlot(0,41924);}
static Quest* InitializePortal(int argc,char**argv){
 InitializeEscapeNative(argc,argv);sMapMgr->InitInstanceIds();sAreaTriggerDataStore->LoadAreaTriggerTeleports();sLFGMgr->LoadLFGDungeons();sSpellMgr->LoadSpellProcEvents();sSpellMgr->LoadSpellProcs();sSpellMgr->LoadSpellLinked();sSpellMgr->LoadSpellTargetPositions();
#ifndef DEVOURER_PORTAL_BASELINE
#ifndef DEVOURER_PORTAL_LIVE
 // Shadow only the relevant tables on the private synchronous connection.
 // MariaDB forbids TEMP DDL in read-only transactions. Preparation alone uses
 // read/write; prove both shadows before applying SQL, then restore read-only.
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
 WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE portal_text_seed LIKE creature_text");WorldDatabase.DirectExecute("INSERT INTO portal_text_seed SELECT * FROM creature_text WHERE CreatureID=105488");WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_text LIKE portal_text_seed");WorldDatabase.DirectExecute("INSERT INTO creature_text SELECT * FROM portal_text_seed");
 WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE portal_go_seed LIKE gameobject_template");WorldDatabase.DirectExecute("INSERT INTO portal_go_seed SELECT * FROM gameobject_template WHERE entry=248901");WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE gameobject_template LIKE portal_go_seed");WorldDatabase.DirectExecute("INSERT INTO gameobject_template SELECT * FROM portal_go_seed");
 for(auto table:{"creature_text","gameobject_template"}){auto proof=WorldDatabase.Query((std::string("SHOW CREATE TABLE ")+table).c_str());Require(proof&&proof->Fetch()[1].GetString().find("CREATE TEMPORARY TABLE")!=std::string::npos,"Migration refuses a persistent table");}
 for(uint32 pass=0;pass<2;++pass)for(auto command:{
#include "PortalMigration.inc"
 })WorldDatabase.DirectExecute(command);
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");auto readonly=WorldDatabase.Query("SELECT @@session.transaction_read_only");Require(readonly&&readonly->Fetch()[0].GetBool(),"World probe runtime must be readonly");
 sObjectMgr->LoadGameObjectTemplate();
 std::cout<<"CONTROL private TEMP migration projection, persistent World untouched\n";
#endif
#endif
 sCreatureTextMgr->LoadCreatureTexts();sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new npc_valzuun_probe;sScriptMgr->SwapScriptContext(true);
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1620,{}},{1607,{}},{1502,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1620,{}},{1607,{}},{1502,{}}});
 auto quest=const_cast<Quest*>(sQuestDataStore->GetQuestTemplate(41924));Require(quest&&quest->GetObjectives().empty(),"Expected fresh native41924 bootstrap");auto rows=WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged FROM quest_objectives WHERE QuestID=41924 ORDER BY StorageIndex");Require(bool(rows),"Native41924 objectives missing");do{quest->LoadQuestObjective(rows->Fetch());}while(rows->NextRow());quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL|QUEST_SPECIAL_FLAGS_CAST);Require(quest->GetObjectives().size()==3&&quest->GetObjectives()[1].ObjectID==105435&&quest->GetObjectives()[2].ObjectID==248901,"Native opening/use objectives changed");return quest;
}
int main(int argc,char**argv){try{
 auto quest=InitializePortal(argc,argv);uint32 cases=0;
#ifdef DEVOURER_PORTAL_BASELINE
 // Explicit pre-migration counterfactual: the original248901 had no ScriptName.
 // Pin only this in-memory field so the historical control survives deployment.
 const_cast<GameObjectTemplate*>(sObjectMgr->GetGameObjectTemplate(248901))->ScriptId=0;
 std::vector<uint32> modes{0,2,3,9};
#else
#ifdef DEVOURER_PORTAL_FAULTS
 auto& names=const_cast<ObjectMgr::ScriptNameContainer&>(sObjectMgr->GetAllScriptNames());Require(names.back()<"zzzz_devourer_portal_refusal_probe","Refusal name must append without reindexing");names.emplace_back("zzzz_devourer_portal_refusal_probe");auto id=sObjectMgr->GetScriptId(names.back().c_str());sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new zzzz_devourer_portal_refusal_probe;sScriptMgr->SwapScriptContext(true);auto& bindings=sScriptDataStore->*Access(EscapeBindingsTag{});Require(!bindings.count(209056),"Refusal must not replace a live opening hook");bindings.emplace(209056,std::make_pair(id,true));std::vector<uint32> modes{21,22,23};
#else
 std::vector<uint32> modes{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,24};
#endif
 Require(sObjectMgr->GetGameObjectTemplate(248901)->ScriptId==sObjectMgr->GetScriptId("go_devourer_misc"),"Actual portal ScriptName not bound");for(uint8 group:{0,2,3})Require(sCreatureTextMgr->TextExist(105488,group),"Requested native text group missing");
#endif
 for(uint32 race:{uint32(RACE_HUMAN),uint32(RACE_ORC)})for(uint32 mode:modes){
  Map map(1502,300,9831000+cases,DIFFICULTY_NONE);auto first=Session(),second=Session();{
   NativePlayer player(first,map,101,{-850,4406,716.94f,0}),member(second,map,102,player.GetPosition());for(auto p:{&player,&member}){p->SetByteValue(UNIT_FIELD_BYTES_0,0,race);p->SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_ROGUE);p->setFactionForRace(race);p->SetQuestStatus(41924,QUEST_STATUS_INCOMPLETE);p->SetQuestSlot(0,41924);p->SetScenarioId(1078);}
   std::unique_ptr<CreditParty> party;if(mode==1)party=std::make_unique<CreditParty>(&player,&member);
   Actor source(&map,105464,18801+cases,{-851.507f,4403.77f,716.948f,5.52173f});Require(map.AddToMap(static_cast<Creature*>(&source)),"Native Valzuun source missing");
#ifdef DEVOURER_PORTAL_FAULTS
   refuseOpening=mode==23;
#endif
   Accept(player,source,quest);Require(PortalActors(source).size()==1&&player.GetScenarioId()==1078,"Native accepted actor/selector missing");
   if(mode==2){Accept(player,source,quest);sScriptMgr->OnGossipHello(&player,&source);}
   if(mode==3){player.RemoveActiveQuest(41924);player.SetQuestSlot(0,0);}
   if(mode==4||mode==5){PortalTick(player,member,source,5000);if(mode==4){player.RemoveActiveQuest(41924);player.SetQuestSlot(0,0);}else Retake(player);}
   if(mode==7){PortalTick(player,member,source,1000);for(auto guid:PortalActors(source))if(auto actor=ObjectAccessor::GetCreature(source,guid))actor->DespawnOrUnsummon();map.RemoveAllObjectsInRemoveList();Require(!sScriptMgr->OnGossipHello(&player,&source),"Recovery must preserve ordinary gossip");}
#ifdef DEVOURER_PORTAL_FAULTS
   if(mode==22){PortalTick(player,member,source,20000);Require(player.GetQuestObjectiveData(quest,1)==1&&PortalObjects(source).size()==1,"Prior actual native opening credit missing");for(auto guid:PortalObjects(source))ObjectAccessor::GetGameObject(source,guid)->Delete();map.RemoveAllObjectsInRemoveList();sScriptMgr->OnGossipHello(&player,&source);}
   PortalTick(player,member,source,5000);if(mode!=23)for(auto guid:PortalActors(source))ObjectAccessor::GetCreature(source,guid)->CastStop();PortalTick(player,member,source,15000);
   Require(PortalActors(source).empty()&&PortalObjects(source).empty()&&player.GetQuestObjectiveData(quest,1)==(mode==22?1:0)&&!player.HasPendingTeleport(),"Refused/interrupted opening published a portal from stale credit");refuseOpening=false;sScriptMgr->OnGossipHello(&player,&source);PortalTick(player,member,source,20000);
#else
#ifndef DEVOURER_PORTAL_BASELINE
   if(mode==0){PortalTick(player,member,source,5000);Require(!player.GetQuestObjectiveData(quest,1)&&PortalObjects(source).empty()&&ObjectAccessor::GetCreature(source,PortalActors(source).front())->IsNonMeleeSpellCast(false),"Opening bypassed native5s cast");PortalTick(player,member,source,5000);Require(player.GetQuestObjectiveData(quest,1)==1&&PortalObjects(source).empty(),"Credit/portal authored timing changed");PortalTick(player,member,source,2000);Require(PortalActors(source).size()==1&&PortalObjects(source).size()==1,"Authored11.5s portal missing");PortalTick(player,member,source,4000);Require(PortalActors(source).size()==1,"Closing3960ms voice actor retired early");PortalTick(player,member,source,4000);}else
#endif
   PortalTick(player,member,source,20000);
#endif
#ifdef DEVOURER_PORTAL_BASELINE
   Require(player.GetQuestObjectiveData(quest,1)==0&&PortalObjects(source).size()==(mode==2?2:1),"Pinned missing credit/stale portal counterfactual changed");for(auto guid:PortalObjects(source))Require(!ObjectAccessor::GetGameObject(source,guid)->HasQuestObjectScope(),"Pinned actor unexpectedly owns quest scope");
   if(mode==9){UsePortal(member,PortalObjects(source).front());std::cout<<"CONTROL baseline foreign pending="<<member.HasPendingTeleport()<<" counter="<<member.GetQuestObjectiveData(quest,2)<<" mover="<<(member.GetUnitBeingMoved()==&member)<<" contact="<<member.CanContact()<<std::endl;Require(member.HasPendingTeleport()&&member.GetQuestObjectiveData(quest,2)==1,"Pinned unrelated-player portal use counterfactual changed");}
#else
   if(mode==3||mode==4||mode==5){Require(PortalActors(source).empty()&&PortalObjects(source).empty()&&!player.GetQuestObjectiveData(quest,1)&&!player.HasPendingTeleport(),"Canceled attempt published portal/credit");if(mode==5){Require(!sScriptMgr->OnGossipHello(&player,&source),"Retry changed ordinary menu");PortalTick(player,member,source,20000);}else goto cleanup;}
   Require(PortalActors(source).empty()&&PortalObjects(source).size()==1&&player.GetQuestObjectiveData(quest,1)==1&&!player.GetQuestObjectiveData(quest,2)&&!player.HasPendingTeleport(),"Native209056 credit/finite actor/owned portal missing");Require(!member.GetQuestObjectiveData(quest,1),"Native effect90 personal credit leaked to group member");
   {auto guid=PortalObjects(source).front();auto go=ObjectAccessor::GetGameObject(source,guid);auto scope=go->GetQuestObjectScope().lock();Require(scope&&scope->Owner==player.GetGUID()&&go->GetRespawnTime()>GameTime::GetGameTime(),"Native quest scope/lifetime missing");
    // Native packet handling does not walk a synthetic player to the object.
    player.Relocate(go->GetPositionX()-2,go->GetPositionY()-2,716.94f,0);member.Relocate(player.GetPosition());
    if(mode==0){DynamicTreeCallback hit;float x,y,z;Require(!player.IsWithinLOSInMap(go)&&map.getObjectHitPos(player.GetPhases(),true,player.GetPositionX(),player.GetPositionY(),player.GetPositionZ()+2,go->GetPositionX(),go->GetPositionY(),go->GetPositionZ()+2,x,y,z,-0.1f,&hit)&&hit.go==go&&map.isInLineOfSight(player.GetPositionX(),player.GetPositionY(),player.GetPositionZ()+2,x,y,z,player.GetPhases(),VMAP::ModelIgnoreFlags::Nothing),"Native portal self-occlusion/visible surface control changed");}
    if(mode==24){DoorObject barrier;Position pose{player.GetPositionX()+0.7f,player.GetPositionY()+0.7f,player.GetPositionZ(),0};Require(barrier.Create(990001+cases,249391,&map,1,pose,G3D::Quat(0,0,0,1),255,GO_STATE_READY)&&map.AddToMap(static_cast<GameObject*>(&barrier)),"Native intervening barrier missing");DynamicTreeCallback hit;float x,y,z;Require(map.getObjectHitPos(player.GetPhases(),true,player.GetPositionX(),player.GetPositionY(),player.GetPositionZ()+2,go->GetPositionX(),go->GetPositionY(),go->GetPositionZ()+2,x,y,z,-0.1f,&hit)&&hit.go==&barrier,"Native barrier control does not intercept portal ray");UsePortal(player,guid);Require(!player.HasPendingTeleport()&&!player.GetQuestObjectiveData(quest,2),"Portal surface path ignored an intervening actor");}
    if(mode==6){Retake(player);UsePortal(player,guid);Require(!player.HasPendingTeleport()&&!player.GetQuestObjectiveData(quest,1)&&!player.GetQuestObjectiveData(quest,2),"Old portal admitted retaken quest");map.RemoveAllObjectsInRemoveList();sScriptMgr->OnGossipHello(&player,&source);PortalTick(player,member,source,20000);goto cleanup;}
    if(mode==8){go->Delete();map.RemoveAllObjectsInRemoveList();sScriptMgr->OnGossipHello(&player,&source);PortalTick(player,member,source,20000);Require(PortalObjects(source).size()==1&&PortalObjects(source).front()!=guid&&player.GetQuestObjectiveData(quest,1)==1,"Deleted portal gossip recovery failed");goto cleanup;}
    if(mode==9){UsePortal(member,guid);Require(!member.HasPendingTeleport()&&!member.GetQuestObjectiveData(quest,2)&&!go->HasFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_IN_USE),"Foreign player used private portal");}
    if(mode==10)player.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_WARRIOR);if(mode==11)player.SetGameMaster(true);if(mode==12)player.Unit::setDeathState(CORPSE);if(mode==13)player.SetPhaseMask(2,false);if(mode==14)player.Relocate(-820,4406,716.94f,0);if(mode==15){player.RemoveActiveQuest(41924);player.SetQuestSlot(0,0);}if(mode==16)const_cast<RewardedQuestSet&>(player.getRewardedQuests()).insert(41924);if(mode==17){auto status=player.getQuestStatus(41924);status->ScriptObjects=std::make_shared<QuestObjectScope>();status->ScriptObjects->Owner=player.GetGUID();}if(mode==18)go->SetPhaseMask(2,false);if(mode==19){auto high=player.GetPosition();high.m_positionZ+=30;player.Relocate(high);}if(mode==20){WorldPacket wire(CMSG_GAME_OBJ_REPORT_USE);wire<<guid;WorldPackets::GameObject::GameObjReportUse request(std::move(wire));request.Read();player.GetSession()->HandleGameobjectReportUse(request);Require(!player.HasPendingTeleport()&&!player.GetQuestObjectiveData(quest,2),"Report/Hello acted as portal Use");goto cleanup;}
    UsePortal(player,guid);
    if(mode>=10&&mode<=19)Require(!player.HasPendingTeleport()&&!player.GetQuestObjectiveData(quest,2),"Invalid portal use advanced native quest/transfer");
    else {std::cout<<"CONTROL native owner pending="<<player.HasPendingTeleport()<<" counter="<<player.GetQuestObjectiveData(quest,2)<<" selector="<<player.GetScenarioId()<<" visible="<<player.canSeeOrDetect(go)<<" los="<<player.IsWithinLOSInMap(go)<<" range="<<player.IsWithinDistInMap(go,go->GetInteractionDistance())<<std::endl;Require(player.HasPendingTeleport()&&player.GetTeleportDest().GetMapId()==1607&&player.GetQuestObjectiveData(quest,2)==1&&player.GetScenarioId()==1078,"Actual native GO use/209066 destination/selector failed");}
   }
#endif
cleanup:
   for(auto guid:PortalActors(source))if(auto actor=ObjectAccessor::GetCreature(source,guid))actor->DespawnOrUnsummon();for(auto guid:PortalObjects(source))if(auto go=ObjectAccessor::GetGameObject(source,guid))go->Delete();map.RemoveAllObjectsInRemoveList();player.CastStop();player.RemoveAllAuras();player.m_Events.KillAllEvents(true);std::cout<<"CASE race="<<race<<" mode="<<mode<<" real41924 accept/105488/209056/248901 lifecycle\n";
  }first->SetPlayer(nullptr);second->SetPlayer(nullptr);delete first;delete second;++cases;
 }
#ifdef DEVOURER_PORTAL_BASELINE
 std::cout<<"COUNTERFACTUAL "<<cases<<" pinned native opening contexts:209056 targetsNPC/no105435 credit, duplicate actors, abandoned quest still publishes unscoped portal, unrelated player can use it. Pair repaired run; not ordinary success\n";
#else
#ifdef DEVOURER_PORTAL_FAULTS
 Require(refusalCalls>=2,"Actual scripted opening did not reach native CheckCast refusal");std::cout<<"CONTROL explicit6 refusal/interruption contexts: no portal from failed current cast or earlier native credit; ordinary gossip retry succeeds. Expected6 warnings reviewed separately\n";
#else
 std::cout<<"PASS "<<cases<<" whole callback/native quest/actor/spell/GO scope contexts, both factions/group, actual105435 hit and native248901 use/209066 far request, cancel/retake/duplicate/recovery and invalid recipient/phase/floor guards. Quest admission/poses/clocks controlled; full far ACK/client traversal/audio remain separate\n";
#endif
#endif
 CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
