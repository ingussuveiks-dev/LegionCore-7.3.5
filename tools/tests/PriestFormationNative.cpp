// Native Spell/SmartAI/Map/Player/GO execution against guarded expected isolated World data.
// Accepted player log and shared source publication are explicit fixtures; no login/save/client.
#define ROCKAWAY_NATIVE_SUPPORT_ONLY
#include "RockawayOffensiveNative.cpp"
#include "GossipData.h"
#include "CreatureTextMgr.h"
#include "WaypointManager.h"
#include "GameObject.h"
#include "GameObjectAI.h"
#include "SmartAI.h"
#include "Group.h"
#include "MoveSpline.h"
#include "ClassHalls/priest_formation.h"
#include "ClassHalls/priest_formation_follow.h"
#include "Item.h"
#include "MovementPackets.h"
#include <filesystem>
#include <fstream>
#include <cstdlib>
struct FormationSpawnTag{using Type=uint64 Creature::*;friend Type Access(FormationSpawnTag);};
template struct FixtureAccess<FormationSpawnTag,&Creature::m_DBTableGuid>;
static unsigned cases;
static void Check(bool ok,char const* message){Require(ok,message);++cases;}
static void InitializePriest(char const* config,char const* runtime,char const* setup)
{
 std::string error;Require(sConfigMgr->LoadInitial(config,{},error),"Private native config failed");sLog->Initialize(nullptr);MySQL::Library_Init();
 auto worldInfo=sConfigMgr->GetStringDefault("WorldDatabaseInfo","");auto separator=worldInfo.find_last_of(';');
 Require(separator!=std::string::npos&&worldInfo.substr(separator+1).find("codex_priest_")==0,"Native World must be the disposable Priest clone");
 WorldDatabase.SetConnectionInfo(worldInfo,1,1);
 HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
 Require(!WorldDatabase.Open()&&WorldDatabase.PrepareStatements(),"Isolated native World preparation failed");Require(!HotfixDatabase.Open()&&HotfixDatabase.PrepareStatements(),"Hotfix pool failed");
 // The random role can write only its disposable clone. Production World is
 // SELECT-only; all native prepared statements and checks remain unchanged.
 std::ifstream sql(setup);Require(bool(sql),"Native shadow setup missing");std::string line;
 while(std::getline(sql,line))if(!line.empty())WorldDatabase.DirectExecute(line.c_str());
 auto checks=std::getenv("LEGION_PRIEST_CHECKS");Require(checks,"Native expected-data checks missing");std::ifstream verify(checks);
 while(std::getline(verify,line))if(!line.empty()){
  auto tab=line.find('\t');Require(tab==64,"Malformed expected hash check");auto result=WorldDatabase.Query(line.substr(tab+1).c_str());
  Require(result&&result->Fetch()[0].GetString()==line.substr(0,tab),"Native isolated context differs from full independent25-table expected fixture");
 }
 // Keep the actual two source13 EventObject graphs after full context checks.
 // Unrelated SmartAI scripts need their own quest/instance bootstrap.
 WorldDatabase.DirectExecute("DELETE FROM smart_scripts WHERE source_type<>13 OR entryorguid NOT IN(119,120)");
 WorldDatabase.DirectExecute("DELETE FROM conditions WHERE NOT((SourceTypeOrReferenceId=22 AND SourceEntry IN(119,120) AND SourceId=13) OR (SourceTypeOrReferenceId=13 AND SourceEntry IN(202747,216409,216418,202982)))");
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");WorldDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
 HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
 auto readonly=WorldDatabase.Query("SELECT @@session.transaction_read_only");Require(readonly&&readonly->Fetch()[0].GetBool(),"World session not read-only");
 auto count=WorldDatabase.Query("SELECT COUNT(*) FROM quest_objectives WHERE QuestID=40938 AND ID=281419 AND Amount=1");Require(count&&count->Fetch()[0].GetUInt32()==1,"Expected native Priest counts not shadowed");
 sDB2Manager.LoadStores(runtime,LOCALE_enUS);LoadGameTables(runtime);sSpellMgr->LoadSpellInfoStore();sSpellMgr->LoadSpellRanks();sSpellMgr->LoadTalentSpellInfo();
 sSpellMgr->LoadSkillLineAbilityMap();sSpellMgr->LoadSpellCustomAttr();sSpellMgr->LoadSpellInfoSpellSpecificAndAuraState();
 sObjectMgr->LoadScriptNames();sObjectMgr->LoadCreatureModelInfo();sObjectMgr->LoadWDBCreatureTemplates();sObjectMgr->LoadCreatureTemplates();
 sObjectMgr->LoadCreatureClassLevelStats();sObjectMgr->LoadCreatureScalingData();sObjectMgr->LoadCreatureDifficultyStat();sObjectMgr->LoadPageTexts();sObjectMgr->LoadGameObjectTemplate();
 sCreatureTextMgr->LoadCreatureTexts();sQuestDataStore->LoadPointsOfInterest();sObjectMgr->LoadNPCText();sGossipDataStore->LoadGossipMenu();sGossipDataStore->LoadGossipMenuItems();sWaypointMgr->Load();sSmartWaypointMgr->LoadFromDB();
 sScriptDataStore->LoadSpellScriptNames();
 auto rows=WorldDatabase.Query(
#include "NativePriestQuest.inc"
 );Require(bool(rows),"Native quest rows missing");auto& vector=sQuestDataStore->*Access(QuestVectorTag{});vector.resize(65536);sQuestDataStore->*Access(QuestMaxTag{})=65536;
 do{auto quest=new Quest(rows->Fetch());const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates())[quest->GetQuestId()]=quest;vector[quest->GetQuestId()]=quest;}while(rows->NextRow());
 auto objectives=WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged FROM quest_objectives WHERE QuestID=40938 ORDER BY StorageIndex ASC");
 Require(bool(objectives),"Native objectives missing");do{auto f=objectives->Fetch();auto quest=const_cast<Quest*>(sQuestDataStore->GetQuestTemplate(f[1].GetUInt32()));Require(quest,"Scoped objective quest missing");quest->LoadQuestObjective(f);
  if(f[2].GetUInt8()==QUEST_OBJECTIVE_MONSTER)quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL|QUEST_SPECIAL_FLAGS_CAST); // matching loader's computed flags
 }while(objectives->NextRow());
 Require(sQuestDataStore->GetQuestTemplate(40938)->GetObjectives().size()==5,"Native objective count differs");
 sEventObjectDataStore->LoadEventObjectTemplates();sConditionMgr->LoadConditions();sSmartScriptMgr->LoadSmartAIFromDB();sSpellMgr->LoadSpellTargetPositions();sSpellMgr->LoadSpellLinked();sSpellMgr->LoadSpellScene();sObjectMgr->LoadItemTemplates();sObjectMgr->LoadItemTemplateAddon();
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new player_priest_formation();new npc_priest_formation_velen();new go_priest_formation_portal();new scene_priest_formation();new eventobject_priest_formation();
 RegisterSpellScript(spell_priest_formation_scene);RegisterAuraScript(aura_priest_formation_scene);RegisterSpellScript(spell_priest_formation_ready);RegisterSpellScript(spell_priest_formation_channel);RegisterAuraScript(aura_priest_formation_channel);RegisterSpellScript(spell_priest_formation_ritual);RegisterSpellScript(spell_priest_formation_finish);RegisterSpellScript(spell_priest_formation_entry);
 sScriptMgr->SwapScriptContext(true);AIRegistry::Initialize();UnitAI::FillAISpellInfo();sWorld->LoadConfigSettings();LoadGameObjectModelList(sWorld->GetDataPath());
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1220,{}},{1512,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1220,{}},{1512,{}}});
 HashMapHolder<EventObject>::SetSize(500000);HashMapHolder<Creature>::SetSize(200000);HashMapHolder<GameObject>::SetSize(200000);HashMapHolder<Player>::SetSize(1024);opcodeTable.Initialize();
 LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo",""),1,1);Require(!LoginDatabase.Open()&&LoginDatabase.PrepareStatements(),"Login pool failed");
 LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");LoginDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
 CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo",""),1,1);Require(!CharacterDatabase.Open()&&CharacterDatabase.PrepareStatements(),"Character native metadata pool failed");
 CharacterDatabase.DirectExecute("CREATE TEMPORARY TABLE priest_empty_artifact_schema LIKE item_instance_artifact");CharacterDatabase.DirectExecute("CREATE TEMPORARY TABLE item_instance_artifact LIKE priest_empty_artifact_schema");
 auto shadow=CharacterDatabase.Query("SHOW CREATE TABLE item_instance_artifact");Require(shadow&&shadow->Fetch()[1].GetString().find("CREATE TEMPORARY TABLE")!=std::string::npos,"Artifact readback is not TEMP");
 CharacterDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");CharacterDatabase.Execute("SET SESSION TRANSACTION READ ONLY");

}
struct FormationPlayer:Player
{
 FormationPlayer(Map& map,uint64 guid,Position const& pose=PriestFormation::Entry):Player(new WorldSession(0,std::string("Priest native fixture"),{},SEC_PLAYER,6,0,"Win",LOCALE_enUS,0,false,AT_AUTH_FLAG_NONE,{})){
  _Create(ObjectGuid::Create<HighGuid::Player>(guid));SetByteValue(UNIT_FIELD_BYTES_0,0,RACE_HUMAN);SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_PRIEST);SetLevel(110);setFaction(1);SetMaxHealth(10000);SetHealth(10000);SetMaxPower(POWER_MANA,10000);SetPower(POWER_MANA,10000);SetInventorySlotCount(16);
  Relocate(pose);SetMap(&map);map.GetZoneAndAreaId(m_zoneId,m_areaId,GetPositionX(),GetPositionY(),GetPositionZ());GetSession()->SetPlayer(this);sWorld->IncreasePlayerCount();sObjectAccessor->AddObject(static_cast<Player*>(this));map.LoadGrid(GetPositionX(),GetPositionY());
  map.AddToGrid(static_cast<Player*>(this),Cell(Trinity::ComputeCellCoord(GetPositionX(),GetPositionY())));Unit::AddToWorld();GetMotionMaster()->Initialize();SetQuestSlot(0,40938);SetQuestStatus(40938,QUEST_STATUS_INCOMPLETE);
 }
 void RestoreThrough(uint32 last){auto q=sQuestDataStore->GetQuestTemplate(40938);for(uint32 marker:{102466u,102465u,102652u,102653u,102654u}){for(auto const& o:q->GetObjectives())if(o.ObjectID==marker)SetQuestObjectiveData(q,&o,1,true);if(marker==last)break;}}
 void RestoreEntry(){auto q=sQuestDataStore->GetQuestTemplate(40938);for(auto const& o:q->GetObjectives())if(o.ObjectID==102466||o.ObjectID==102465)SetQuestObjectiveData(q,&o,1,true);}
 void Artifact(uint32 item){ItemPosCountVec where;Require(CanStoreNewItem(NULL_BAG,NULL_SLOT,where,item,1)==EQUIP_ERR_OK&&StoreNewItem(where,item,true),"Actual native artifact bag storage failed");Require(HasItemCount(item,1,false),"Actual native artifact bag ownership missing");}
 ~FormationPlayer(){PriestFormation::Cleanup(this);CancelQuestObjects(40938);GetMap()->RemoveAllObjectsInRemoveList();CombatStop(true);CastStop();RemoveAllAuras();m_Events.KillAllEvents(true);for(uint8 slot=PLAYER_SLOT_START;slot<PLAYER_SLOT_END;++slot)if(auto item=GetItemByPos(INVENTORY_SLOT_BAG_0,slot))if(item->IsInWorld())item->RemoveFromWorld();Unit::RemoveFromWorld();RemoveFromGrid();sObjectAccessor->RemoveObject(static_cast<Player*>(this));ResetMap();}
};
struct FormationGroup
{
 Group group;std::vector<Player*> members;
 FormationGroup(std::initializer_list<Player*> players):members(players){for(auto p:members)p->SetGroup(&group,0);}
 ~FormationGroup(){for(auto p:members)p->SetGroup(nullptr);}
};
struct FormationGoTag{using Type=uint64 GameObject::*;friend Type Access(FormationGoTag);};
template struct FixtureAccess<FormationGoTag,&GameObject::m_DBTableGuid>;
struct FormationSource:Creature
{
 explicit FormationSource(Map& map){auto pos=PriestFormation::VelenStart;Require(Create(180001,&map,1,101313,0,ALLIANCE,pos.GetPositionX(),pos.GetPositionY(),pos.GetPositionZ(),pos.GetOrientation()),"Native shared Velen create failed");this->*Access(FormationSpawnTag{})=371123;SetHomePosition(*this);Require(map.AddToMap(static_cast<Creature*>(this)),"Native shared Velen publication failed");}
 ~FormationSource(){if(IsInWorld())GetMap()->RemoveFromMap(static_cast<Creature*>(this),false);ResetMap();}
};
struct FormationPortal:GameObject
{
 explicit FormationPortal(Map& map){auto pos=PriestFormation::PortalPoint;Require(Create(180002,246663,&map,1,pos,G3D::Quat(0,0,std::sin(pos.GetOrientation()/2),std::cos(pos.GetOrientation()/2)),255,GO_STATE_READY),"Native actual portal creation failed");this->*Access(FormationGoTag{})=25164530;map.LoadGrid(pos.GetPositionX(),pos.GetPositionY());Require(map.AddToMap(static_cast<GameObject*>(this)),"Native actual portal publication failed");}
 ~FormationPortal(){if(IsInWorld())GetMap()->RemoveFromMap(static_cast<GameObject*>(this),false);ResetMap();}
};
struct FormationEvent:EventObject
{
 FormationEvent(Map& map,uint32 entry){float x=entry==119?1251.81f:1335.12f,y=entry==119?1344.14f:1344.29f,z=entry==119?185.0f:177.221f;Require(Create(entry==119?400319:400320,&map,1,entry,x,y,z,0,entry==119?5.f:10.f,0,0),"Actual native event creation failed");map.LoadGrid(x,y);map.AddToGrid(static_cast<EventObject*>(this),Cell(Trinity::ComputeCellCoord(x,y)));AddToWorld();}
 ~FormationEvent(){if(IsInWorld())RemoveFromWorld();RemoveFromGrid();ResetMap();}
};
static void Advance(FormationPlayer& p,uint32 duration)
{
 auto map=p.GetMap();for(uint32 elapsed=0;elapsed<duration;elapsed+=10){p.Unit::Update(10);PriestFormation::Tick(&p,10);
  auto state=PriestFormation::Load(&p);if(auto actor=PriestFormation::Anchor(&p,state))actor->Update(10);
  if(auto actor=PriestFormation::Leader(&p))actor->Update(10);
  map->MoveAllCreaturesInMoveList();map->RemoveAllObjectsInRemoveList();
 }
}
int main(int argc,char**argv){try{
 SetUnhandledExceptionFilter(CrashTrace);Require(argc==2,"Expected dbc directory");auto config=std::getenv("LEGION_DIALOGUE_CONFIG"),setup=std::getenv("LEGION_PRIEST_SETUP");Require(config&&setup,"Private native inputs missing");
 auto dir=std::filesystem::path(argv[1]).lexically_normal();if(dir.filename().empty())dir=dir.parent_path();auto runtime=dir.parent_path().parent_path().string()+'/';InitializePriest(config,runtime.c_str(),setup);

 {Map map(1220,300,9996899,DIFFICULTY_NONE);FormationPlayer p(map,90,PriestFormation::VelenStart);FormationSource source(map);FormationPortal portal(map);
  Check(p.CastSpell(&p,202324,true)!=SPELL_CAST_OK&&!p.GetQuestObjectiveData(40938,102465),"Unissued portal entry bypassed actual follow milestone");
  auto leader=PriestFormation::EnsureLeader(&p);auto ai=leader?dynamic_cast<PriestFormation::LeaderAI*>(leader->AI()):nullptr;
  Check(leader&&ai&&leader->HasQuestObjectScope()&&PriestFormation::EnsureLeader(&p)==leader,"Actual Velen private publication/owner list duplicated or failed");
  Advance(p,80000);std::cout<<"LEAD street step="<<unsigned(ai->step)<<" dist="<<leader->GetExactDist(&PriestFormation::Upper)<<'\n';
  Check(ai->step==PriestFormation::LeaderAI::Pad&&leader->GetExactDist(&PriestFormation::Upper)<3&&!p.GetQuestObjectiveData(40938,102466),"Actual native Velen street path failed or gave early credit");
  map.PlayerRelocation(&p,PriestFormation::Upper.GetPositionX(),PriestFormation::Upper.GetPositionY(),PriestFormation::Upper.GetPositionZ(),0);Advance(p,500);
  Check(ai->step==PriestFormation::LeaderAI::Transfer&&leader->GetExactDist(&PriestFormation::Lower)<8&&!p.GetQuestObjectiveData(40938,102466),"Native192293 Velen transfer did not reach lower hall");
  Check(p.CastSpell(&p,192293,true)==SPELL_CAST_OK&&p.IsBeingTeleportedNear(),"Actual player pad spell did not request lower hall transfer");
  WorldPackets::Movement::MoveTeleportAck ack{WorldPacket(CMSG_MOVE_TELEPORT_ACK)};ack.MoverGUID=p.GetGUID();p.GetSession()->HandleMoveTeleportAck(ack);Advance(p,500);
  Check(p.GetExactDist(&PriestFormation::Lower)<8&&ai->step==PriestFormation::LeaderAI::Hall,"Native pad acknowledgement failed to unlock lower route");
  Advance(p,80000);Check(ai->step==PriestFormation::LeaderAI::AtPortal&&!p.GetQuestObjectiveData(40938,102466),"Actual lower route failed or credited a distant owner");
  map.PlayerRelocation(&p,PriestFormation::PortalPoint.GetPositionX(),PriestFormation::PortalPoint.GetPositionY(),PriestFormation::PortalPoint.GetPositionZ(),0);Advance(p,100);
  Check(p.GetQuestObjectiveData(40938,102466)==1&&!p.GetQuestObjectiveData(40938,102465),"Real leader/portal arrival failed personal first milestone");
  {Player::ScopedTeleportDelay loading(p);portal.Use(&p);}Advance(p,50);std::cout<<"PORTAL pending="<<p.HasPendingTeleport()<<" far="<<p.IsBeingTeleportedFar()<<" map="<<p.GetTeleportDest().GetMapId()<<" distance="<<p.GetTeleportDest().GetExactDist(&PriestFormation::Entry)<<" counter="<<p.GetQuestObjectiveData(40938,102465)<<" issued="<<PriestFormation::Load(&p).entryPending<<" portal="<<bool(PriestFormation::Portal(&p))<<'\n';Check(p.IsBeingTeleportedFar()&&p.GetTeleportDest().GetMapId()==1512&&p.GetTeleportDest().GetExactDist(&PriestFormation::Entry)<.02f&&!p.GetQuestObjectiveData(40938,102465),"Native portal request prematurely credited entry or selected wrong destination");
  // Far-map client loading is a controlled boundary; no actual login/client claim.
  // Dispose the controlled loading-boundary fixture without a fake far ACK.
 }
 unsigned number=0;
 for(uint32 item:{128827u,128868u,128825u}){
  Map map(1512,300,9996900+number,DIFFICULTY_NONE);FormationPlayer p(map,100+number);FormationEvent entrance(map,119),circle(map,120);++number;
  Check(p.CastSpell(&p,202457,true)!=SPELL_CAST_OK,"Scene started before actual restored portal milestones");p.RestoreEntry();
  Check(p.CastSpell(&p,202457,true)!=SPELL_CAST_OK,"Scene started without a real inventory artifact");p.Artifact(item);
  entrance.Update(1);Advance(p,250);Check(p.HasAura(202457)&&p.HasSceneStatus(1191,SCENE_LAUNCH)&&PriestFormation::Load(&p).stage==PriestFormation::Stage::Intro,"Actual source13/89/85 scene publication/receipt failed");
  p.TrigerScene(999,"ReadyForSpell");p.TrigerScene(1,"ReadyForSpell");Check(!PriestFormation::Load(&p).ready&&!p.GetQuestObjectiveData(40938,102652),"Unissued/early scene event advanced readiness");
  Advance(p,36000);map.PlayerRelocation(&p,PriestFormation::Ritual.GetPositionX(),PriestFormation::Ritual.GetPositionY(),PriestFormation::Ritual.GetPositionZ(),0);
  circle.Update(1);Check(!p.GetQuestObjectiveData(40938,102652),"Circle location substituted for native ReadyForSpell acknowledgement");
  p.TrigerScene(1,"ReadyForSpell");circle.Update(1);Advance(p,250);auto state=PriestFormation::Load(&p);
  std::cout<<"READY item="<<item<<" stage="<<unsigned(state.stage)<<" prompt="<<state.ready<<" counter="<<p.GetQuestObjectiveData(40938,102652)<<" bar="<<PriestFormation::Bar(&p)<<" anchor="<<bool(PriestFormation::Anchor(&p,state))<<'\n';
  Check(p.GetQuestObjectiveData(40938,102652)==1&&state.stage==PriestFormation::Stage::Ready&&p.HasAura(PriestFormation::Bar(&p)),"Actual native readiness/selected override bar failed");
  Check(p.CastSpell(&p,202881,true)!=SPELL_CAST_OK&&p.CastSpell(&p,202945,true)!=SPELL_CAST_OK,"Unissued ritual tick/promotion admitted");
  FormationPlayer peer(map,310+number),late(map,410+number),wrong(map,510+number),distant(map,610+number),legacy(map,710+number);
  for(auto member:{&peer,&wrong,&distant,&legacy})member->RestoreThrough(102652);late.RestoreEntry();wrong.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_WARRIOR);
  auto quest=sQuestDataStore->GetQuestTemplate(40938);for(auto const& objective:quest->GetObjectives())if(objective.ObjectID==102466)legacy.SetQuestObjectiveData(quest,&objective,0,true);
  for(auto member:{&peer,&late,&wrong,&legacy})map.PlayerRelocation(member,PriestFormation::Ritual.GetPositionX(),PriestFormation::Ritual.GetPositionY()+2,PriestFormation::Ritual.GetPositionZ(),0);
  map.PlayerRelocation(&distant,PriestFormation::Ritual.GetPositionX()+500,PriestFormation::Ritual.GetPositionY(),PriestFormation::Ritual.GetPositionZ(),0);
  FormationGroup group{&p,&peer,&late,&wrong,&distant,&legacy};bool distantEligible=distant.IsAtGroupRewardDistance(&p);
  auto originalAnchor=PriestFormation::Anchor(&p,state);Check(originalAnchor&&PriestFormation::EnsureAnchor(&p)==originalAnchor,"Readiness spawned duplicate anchors");
  FormationPlayer visitor(map,200+number);visitor.RestoreEntry();visitor.Artifact(item);
  Check(!PriestFormation::Current(originalAnchor,&visitor)&&!originalAnchor->CanTargetQuestObject(&visitor),"Another player acquired a personal ritual target");
  uint32 interrupted=PriestFormation::Channel(PriestFormation::Bar(&p));Check(p.CastSpell(&p,interrupted,false)==SPELL_CAST_OK,"Native interrupted channel could not start");Advance(p,50);Advance(p,3000);p.InterruptNonMeleeSpells(true,interrupted);Advance(p,100);
  Check(!p.GetQuestObjectiveData(40938,102653)&&PriestFormation::Load(&p).stage==PriestFormation::Stage::Ready&&p.HasAura(202835),"Interrupted channel granted ritual credit or blocked retry");
  Check(p.CastSpell(&p,interrupted,false)==SPELL_CAST_OK,"Missing-anchor channel could not start");Advance(p,50);auto originalGuid=originalAnchor->GetGUID();originalAnchor->DespawnOrUnsummon();map.RemoveAllObjectsInRemoveList();Advance(p,100);state=PriestFormation::Load(&p);
  Check(state.stage==PriestFormation::Stage::Ready&&!p.GetQuestObjectiveData(40938,102653)&&PriestFormation::Anchor(&p,state)&&PriestFormation::Anchor(&p,state)->GetGUID()!=originalGuid,"Missing target failed to recover without ritual credit");

  uint32 channel=PriestFormation::Channel(PriestFormation::Bar(&p));auto result=p.CastSpell(&p,channel,false);Advance(p,50);state=PriestFormation::Load(&p);
  std::cout<<"CHANNEL spell="<<channel<<" result="<<unsigned(result)<<" stage="<<unsigned(state.stage)<<" self="<<p.HasAura(channel)<<" anchor="<<(PriestFormation::Anchor(&p,state)&&PriestFormation::Anchor(&p,state)->HasAura(channel,p.GetGUID()))<<'\n';
  Check(result==SPELL_CAST_OK&&state.stage==PriestFormation::Stage::Casting,"Actual native selected channel failed");Advance(p,13000);
  Check(!p.GetQuestObjectiveData(40938,102653),"Ritual credited before actual14s periodic receipt");Advance(p,1500);state=PriestFormation::Load(&p);
  std::cout<<"RITUAL stage="<<unsigned(state.stage)<<" counter="<<p.GetQuestObjectiveData(40938,102653)<<" tick="<<state.tick<<'\n';
  Check(state.stage==PriestFormation::Stage::RitualDone&&p.GetQuestObjectiveData(40938,102653)==1,"Actual14s channel/native202881/134 endpoint did not acknowledge ritual");
  Check(peer.GetQuestObjectiveData(40938,102653)==1&&!late.GetQuestObjectiveData(40938,102653)&&!wrong.GetQuestObjectiveData(40938,102653)&&!legacy.GetQuestObjectiveData(40938,102653)&&bool(distant.GetQuestObjectiveData(40938,102653))==distantEligible,"Native134 group event lost an eligible player or bypassed personal prerequisites/class/range");
  Check(p.CastSpell(&p,202881,true)!=SPELL_CAST_OK&&p.GetQuestObjectiveData(40938,102653)==1&&peer.GetQuestObjectiveData(40938,102653)==1,"Repeated ritual endpoint changed exactly-once group credit");
  Check(!p.HasAura(202835)&&!p.HasAura(202749)&&!p.HasAura(216416)&&!p.HasAura(216419),"Native202881 left ritual override auras");
  p.TrigerScene(1,"CeremonyComplete");p.TrigerScene(1,"PortPlayer");Check(!p.GetQuestObjectiveData(40938,102654),"Ceremony skipped actual grip/ordering");
  Advance(p,13000);p.TrigerScene(1,"LifeGrip");Advance(p,2000);state=PriestFormation::Load(&p);
  std::cout<<"GRIP issued="<<state.grip<<" arrived="<<state.gripArrived<<" distance="<<p.GetExactDist(&PriestFormation::Grip)<<'\n';
  Check(state.grip&&state.gripArrived,"Native202982 jump/actual arrival acknowledgement failed");Advance(p,30000);p.TrigerScene(1,"CeremonyComplete");
  Check(p.GetQuestObjectiveData(40938,102654)==1&&p.GetQuestStatus(40938)==QUEST_STATUS_COMPLETE&&p.CanRewardQuest(sQuestDataStore->GetQuestTemplate(40938),false),"Native ceremony/personal promotion/full reward admission failed");
  p.TrigerScene(1,"CeremonyComplete");Check(p.GetQuestObjectiveData(40938,102654)==1,"Duplicate ceremony changed exactly-once counter");
  Advance(p,4100);p.TrigerScene(1,"PortPlayer");p.SceneCompleted(1);map.RemoveAllObjectsInRemoveList();
  Check(!p.HasAura(202457)&&!PriestFormation::Anchor(&p,PriestFormation::Load(&p))&&p.HasItemCount(item,1,false),"Native scene completion/anchor cleanup mutated legitimate artifact ownership");
 }
 {Map map(1512,300,9996898,DIFFICULTY_NONE);FormationPlayer p(map,91);p.RestoreThrough(102466);
  p.Unit::RemoveFromWorld();sScriptMgr->OnPlayerLogin(&p);Check(PriestFormation::Load(&p).entryPending&&!p.GetQuestObjectiveData(40938,102465),"Prepublication login failed to retain real entry recovery or granted early credit");
  p.Unit::AddToWorld();PriestFormation::Tick(&p,0);Check(p.GetQuestObjectiveData(40938,102465)==1&&!PriestFormation::Load(&p).entryPending,"Actual restored native-entry receiver failed after publication");
  p.Artifact(128868);FormationEvent entrance(map,119),circle(map,120);entrance.Update(1);Advance(p,250);Check(p.HasAura(202457),"Recovery scene did not replay from the actual entry checkpoint");
  Advance(p,36000);map.PlayerRelocation(&p,PriestFormation::Ritual.GetPositionX(),PriestFormation::Ritual.GetPositionY(),PriestFormation::Ritual.GetPositionZ(),0);p.TrigerScene(1,"ReadyForSpell");circle.Update(1);Advance(p,250);auto old=PriestFormation::Load(&p);Check(PriestFormation::Anchor(&p,old),"Recovery ready anchor missing");
  p.CancelQuestObjects(40938);map.RemoveAllObjectsInRemoveList();PriestFormation::Tick(&p,10);Check(PriestFormation::Load(&p).stage==PriestFormation::Stage::None&&!p.HasAura(202457)&&!p.HasAura(202835)&&!PriestFormation::Anchor(&p,old)&&!p.GetQuestObjectiveData(40938,102653),"Actual accepted-attempt cancellation leaked anchor/aura/ritual credit");
  p.TrigerScene(1,"CeremonyComplete");Check(!p.GetQuestObjectiveData(40938,102654),"Canceled native scene callback granted promotion");
 }
 {Map map(1512,300,9996897,DIFFICULTY_NONE);FormationPlayer p(map,92);p.RestoreEntry();p.Artifact(128825);FormationEvent entrance(map,119),circle(map,120);
  entrance.Update(1);Advance(p,250);Advance(p,36000);map.PlayerRelocation(&p,PriestFormation::Ritual.GetPositionX(),PriestFormation::Ritual.GetPositionY(),PriestFormation::Ritual.GetPositionZ(),0);p.TrigerScene(1,"ReadyForSpell");circle.Update(1);Advance(p,250);auto previous=PriestFormation::Load(&p);Check(PriestFormation::Anchor(&p,previous)&&p.HasAura(202835),"Saved-aura recovery fixture failed readiness");
  p.Unit::RemoveFromWorld();sScriptMgr->OnPlayerLogin(&p);map.RemoveAllObjectsInRemoveList();Check(!p.HasAura(202457)&&!p.HasAura(202835)&&!p.HasAura(216419)&&!PriestFormation::Anchor(&p,previous)&&p.GetQuestObjectiveData(40938,102652)==1,"Login retained stale infinite scene/bar/actor or erased legitimate personal progress");
  p.Unit::AddToWorld();Advance(p,1500);Check(p.HasAura(202457)&&PriestFormation::Load(&p).stage==PriestFormation::Stage::Intro&&!p.GetQuestObjectiveData(40938,102653),"Real central checkpoint scene did not replay without free ritual credit");
  p.TrigerScene(1,"CeremonyComplete");Check(!p.GetQuestObjectiveData(40938,102654),"Old native scene instance survived login recovery");
  sScriptMgr->OnPlayerBeforeLogout(&p);map.RemoveAllObjectsInRemoveList();Check(!p.HasAura(202457)&&PriestFormation::Load(&p).stage==PriestFormation::Stage::None&&!p.GetQuestObjectiveData(40938,102653),"Actual prelogout hook retained formation state or granted ritual credit");
 }
 std::cout<<"PASS "<<cases<<" actual native Priest scene/event/3 inventory artifacts/readiness/14s channels/ritual/grip/ceremony/cleanup cases; restored entry counters and scene client callbacks explicitly controlled, no whole client claim\n";std::cout.flush();CharacterDatabase.Close();LoginDatabase.Close();WorldDatabase.Close();HotfixDatabase.Close();MySQL::Library_End();
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
