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
#include "ClassHalls/ring_reforged.h"
#include <filesystem>
#include <fstream>
#include <cstdlib>
struct RingSpawnTag{using Type=uint64 Creature::*;friend Type Access(RingSpawnTag);};
template struct FixtureAccess<RingSpawnTag,&Creature::m_DBTableGuid>;
static unsigned cases;
static void Check(bool ok,char const* message){Require(ok,message);++cases;}
static void InitializeRing(char const* config,char const* runtime,char const* setup)
{
 std::string error;Require(sConfigMgr->LoadInitial(config,{},error),"Private native config failed");sLog->Initialize(nullptr);MySQL::Library_Init();
 auto worldInfo=sConfigMgr->GetStringDefault("WorldDatabaseInfo","");auto separator=worldInfo.find_last_of(';');
 Require(separator!=std::string::npos&&worldInfo.substr(separator+1).find("codex_ring_")==0,"Native World must be the disposable ring clone");
 WorldDatabase.SetConnectionInfo(worldInfo,1,1);
 HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
 Require(!WorldDatabase.Open()&&WorldDatabase.PrepareStatements(),"Isolated native World preparation failed");Require(!HotfixDatabase.Open()&&HotfixDatabase.PrepareStatements(),"Hotfix pool failed");
 // The random role can write only its disposable clone. Production World is
 // SELECT-only; all native prepared statements and checks remain unchanged.
 std::ifstream sql(setup);Require(bool(sql),"Native shadow setup missing");std::string line;
 while(std::getline(sql,line))if(!line.empty())WorldDatabase.DirectExecute(line.c_str());
 auto checks=std::getenv("LEGION_RING_CHECKS");Require(checks,"Native expected-data checks missing");std::ifstream verify(checks);
 while(std::getline(verify,line))if(!line.empty()){
  auto tab=line.find('\t');Require(tab==64,"Malformed expected hash check");auto result=WorldDatabase.Query(line.substr(tab+1).c_str());
  Require(result&&result->Fetch()[0].GetString()==line.substr(0,tab),"Native isolated context differs from full independent18-table expected fixture");
 }
 // Isolate the real shared96528 graph after checking the full original40-row
 // scope. Unrelated SmartAI scripts need their own quests/instance bootstrap.
 WorldDatabase.DirectExecute("DELETE FROM smart_scripts WHERE entryorguid<>96528 OR source_type<>0");
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");WorldDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
 HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
 auto readonly=WorldDatabase.Query("SELECT @@session.transaction_read_only");Require(readonly&&readonly->Fetch()[0].GetBool(),"World session not read-only");
 auto count=WorldDatabase.Query("SELECT COUNT(*) FROM quest_objectives WHERE QuestID=40225 AND ID IN(280438,280439) AND Amount=1");Require(count&&count->Fetch()[0].GetUInt32()==2,"Expected native Ring counts not shadowed");
 sDB2Manager.LoadStores(runtime,LOCALE_enUS);LoadGameTables(runtime);sSpellMgr->LoadSpellInfoStore();sSpellMgr->LoadSpellRanks();sSpellMgr->LoadTalentSpellInfo();
 sSpellMgr->LoadSkillLineAbilityMap();sSpellMgr->LoadSpellCustomAttr();sSpellMgr->LoadSpellInfoSpellSpecificAndAuraState();
 sObjectMgr->LoadScriptNames();sObjectMgr->LoadCreatureModelInfo();sObjectMgr->LoadWDBCreatureTemplates();sObjectMgr->LoadCreatureTemplates();
 sObjectMgr->LoadCreatureClassLevelStats();sObjectMgr->LoadCreatureScalingData();sObjectMgr->LoadCreatureDifficultyStat();sObjectMgr->LoadPageTexts();sObjectMgr->LoadGameObjectTemplate();
 sCreatureTextMgr->LoadCreatureTexts();sQuestDataStore->LoadPointsOfInterest();sObjectMgr->LoadNPCText();sGossipDataStore->LoadGossipMenu();sGossipDataStore->LoadGossipMenuItems();sWaypointMgr->Load();sSmartWaypointMgr->LoadFromDB();
 sScriptDataStore->LoadSpellScriptNames();
 auto rows=WorldDatabase.Query(
#include "NativeRingQuest.inc"
 );Require(bool(rows),"Native quest rows missing");auto& vector=sQuestDataStore->*Access(QuestVectorTag{});vector.resize(65536);sQuestDataStore->*Access(QuestMaxTag{})=65536;
 do{auto quest=new Quest(rows->Fetch());const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates())[quest->GetQuestId()]=quest;vector[quest->GetQuestId()]=quest;}while(rows->NextRow());
 auto objectives=WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged FROM quest_objectives WHERE QuestID IN(39746,40225) ORDER BY StorageIndex ASC");
 Require(bool(objectives),"Native objectives missing");do{auto f=objectives->Fetch();auto quest=const_cast<Quest*>(sQuestDataStore->GetQuestTemplate(f[1].GetUInt32()));Require(quest,"Scoped objective quest missing");quest->LoadQuestObjective(f);
  if(f[2].GetUInt8()==QUEST_OBJECTIVE_MONSTER)quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL|QUEST_SPECIAL_FLAGS_CAST); // matching loader's computed flags
 }while(objectives->NextRow());
 Require(sQuestDataStore->GetQuestTemplate(40225)->GetObjectives().size()==2,"Native objective count differs");
 sSmartScriptMgr->LoadSmartAIFromDB();
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new player_ring_reforged();new npc_ring_reforged_nobundo();new go_ring_reforged_rock();
 RegisterSpellScript(spell_ring_reforged_assume);RegisterSpellScript(spell_ring_reforged_oath);RegisterAuraScript(aura_ring_reforged_oath);RegisterSpellScript(spell_ring_reforged_rock);
 sScriptMgr->SwapScriptContext(true);AIRegistry::Initialize();UnitAI::FillAISpellInfo();sWorld->LoadConfigSettings();LoadGameObjectModelList(sWorld->GetDataPath());
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1469,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1469,{}}});
 HashMapHolder<Creature>::SetSize(200000);HashMapHolder<GameObject>::SetSize(200000);HashMapHolder<Player>::SetSize(1024);opcodeTable.Initialize();
 LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo",""),1,1);Require(!LoginDatabase.Open()&&LoginDatabase.PrepareStatements(),"Login pool failed");
 LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");LoginDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
}
struct RingPlayer:Player
{
 RingPlayer(Map& map,uint64 guid):Player(new WorldSession(0,std::string("Ring native fixture"),{},SEC_PLAYER,6,0,"Win",LOCALE_enUS,0,false,AT_AUTH_FLAG_NONE,{})){
  _Create(ObjectGuid::Create<HighGuid::Player>(guid));SetByteValue(UNIT_FIELD_BYTES_0,0,RACE_DRAENEI);SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_SHAMAN);SetLevel(110);setFaction(1);SetMaxHealth(10000);SetHealth(10000);
  Relocate(824.283f,1091.97f,49.4f,4.7f);SetMap(&map);GetSession()->SetPlayer(this);sWorld->IncreasePlayerCount();sObjectAccessor->AddObject(static_cast<Player*>(this));map.LoadGrid(GetPositionX(),GetPositionY());
  map.AddToGrid(static_cast<Player*>(this),Cell(Trinity::ComputeCellCoord(GetPositionX(),GetPositionY())));Unit::AddToWorld();SetQuestSlot(0,40225);SetQuestStatus(40225,QUEST_STATUS_INCOMPLETE);
 }
 ~RingPlayer(){CancelQuestObjects(40225);GetMap()->RemoveAllObjectsInRemoveList();CombatStop(true);CastStop();RemoveAllAuras();m_Events.KillAllEvents(true);Unit::RemoveFromWorld();RemoveFromGrid();sObjectAccessor->RemoveObject(static_cast<Player*>(this));ResetMap();}
};
struct RingSource:Creature
{
 explicit RingSource(Map& map){Require(Create(180001,&map,1,96528,0,ALLIANCE,824.283f,1090.97f,49.4341f,4.70418f),"Native source actor create failed");this->*Access(RingSpawnTag{})=11377878;SetHomePosition(*this);}
 ~RingSource(){if(IsInWorld())GetMap()->RemoveFromMap(static_cast<Creature*>(this),false);ResetMap();}
};
static void Advance(RingPlayer& p,uint32 duration)
{
 auto map=p.GetMap();for(uint32 elapsed=0;elapsed<duration;elapsed+=10){p.Unit::Update(10);
  GuidList actors=*p.GetSummonList(96528);for(auto id:actors)if(auto actor=map->GetCreature(id)){auto ai=dynamic_cast<RingReforged::AI*>(actor->AI());if(ai)if(auto object=ai->Door())object->Update(10);actor->Update(10);}
  map->MoveAllCreaturesInMoveList();map->RemoveAllObjectsInRemoveList();
 }
}
int main(int argc,char**argv){try{
 SetUnhandledExceptionFilter(CrashTrace);Require(argc==2,"Expected dbc directory");auto config=std::getenv("LEGION_DIALOGUE_CONFIG"),setup=std::getenv("LEGION_RING_SETUP");Require(config&&setup,"Private native inputs missing");
 auto dir=std::filesystem::path(argv[1]).lexically_normal();if(dir.filename().empty())dir=dir.parent_path();auto runtime=dir.parent_path().parent_path().string()+'/';InitializeRing(config,runtime.c_str(),setup);
 {Map map(1469,300,9996998,DIFFICULTY_NONE);RingPlayer p(map,100),other(map,101),partner(map,102),late(map,103),distant(map,104),wrong(map,105);RingSource source(map);Require(map.AddToMap(static_cast<Creature*>(&source)),"Native source publication failed");
  // Native in-memory GroupReference linkage; no Group::Create/AddMember or
  // CharacterDatabase rows. Native personal90 actions must not grant these members progress.
  Group group;p.SetGroup(&group,0);partner.SetGroup(&group,0);distant.SetGroup(&group,0);wrong.SetGroup(&group,0);
  partner.SetByteValue(UNIT_FIELD_BYTES_0,0,RACE_ORC);partner.setFaction(2);wrong.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_MAGE);
  map.PlayerRelocation(&distant,3000,3000,50,0);
  Check(p.GetQuestObjectiveData(40225,97047)==0&&p.GetQuestObjectiveData(40225,97048)==0,"Fixture received progress before real actions");
  p.SetQuestStatus(40225,QUEST_STATUS_COMPLETE);Check(!p.CanRewardQuest(sQuestDataStore->GetQuestTemplate(40225),false),"Legacy COMPLETE bypassed actual required counters");p.SetQuestStatus(40225,QUEST_STATUS_INCOMPLETE);
  Check(p.CastSpell(&p,195070,true)!=SPELL_CAST_OK&&p.CastSpell(&p,195359,true)!=SPELL_CAST_OK,"Unselected parent/waypoint cast admitted");
  auto actor=RingReforged::EnsureActor(&p);Check(actor&&actor->HasQuestObjectScope()&&actor->ToTempSummon()&&actor->ToTempSummon()->GetSummonerGUID()==p.GetGUID(),"Actual native prepublication owner binding failed");
  auto ai=RingReforged::Controller(&p);Check(ai&&RingReforged::EnsureActor(&p)==actor,"Native owned-actor lookup spawned a duplicate");
  Advance(p,2100);auto door=ai->Door();Check(door&&door->HasQuestObjectScope()&&door->GetGoState()==GO_STATE_READY&&door->m_model&&door->m_model->isCollisionEnabled(),"Actual native private closed rock missing");
  Check(!RingReforged::Current(door,&other)&&!door->CanCollideWith(&other)&&door->CanCollideWith(&p),"Foreign player sees/collides with another scope");
  Check(other.CastSpell(door,195132,false)!=SPELL_CAST_OK,"Foreign rock cast admitted");
  door->AI()->GossipHello(&p,true);Check(p.GetQuestObjectiveData(40225,97048)==0,"Raw native-hit bool granted progress without a spell receipt");
  door->Use(&p);Check(p.GetQuestObjectiveData(40225,97047)==0&&p.GetQuestObjectiveData(40225,97048)==0&&door->GetGoState()==GO_STATE_READY,"Premature use opened or credited the rock");
  Check(ai->Hello(&p)&&p.PlayerTalkClass->GetGossipMenu().GetMenuId()==18940&&p.PlayerTalkClass->GetGossipMenu().GetMenuItemCount()==1,"Actual native Farseer gossip option missing");
  ai->sGossipSelect(&p,18940,0);Check(p.GetQuestObjectiveData(40225,97047)==1&&p.GetQuestObjectiveData(40225,97048)==0,"Real195070 did not grant only its actual first objective");
  std::cout<<"GROUP first partner="<<partner.GetQuestObjectiveData(40225,97047)<<" distant="<<distant.GetQuestObjectiveData(40225,97047)<<" wrong="<<wrong.GetQuestObjectiveData(40225,97047)<<" range="<<sWorld->getFloatConfig(CONFIG_GROUP_XP_DISTANCE)<<'\n';
  Check(partner.GetQuestObjectiveData(40225,97047)==0&&distant.GetQuestObjectiveData(40225,97047)==0&&wrong.GetQuestObjectiveData(40225,97047)==0,"Native personal90 leadership credit leaked to a group member");
  late.SetGroup(&group,0); // Missed the first real action; second must not skip it.
  Advance(p,100);Check(p.HasAura(195359)&&ai->stage==RingReforged::Stage::Moving,"Native delayed195359/12s aura/movement request not acknowledged");
  Advance(p,3000);Check(ai->movement&&p.GetExactDist(&RingReforged::Circle)<2,"Actual player spline/473698 completion callback missing");
  Check(ai->stage==RingReforged::Stage::Moving&&p.GetQuestObjectiveData(40225,97048)==0,"Position/first credit advanced before native aura expiry");
  Advance(p,10000);Check(ai->expired&&ai->stage==RingReforged::Stage::Speech,"Actual12s aura expiry and movement did not unlock the next speech");
  Advance(p,14000);Advance(p,28000);
  std::cout<<"STATE stage="<<unsigned(ai->stage)<<" npc="<<actor->GetPositionX()<<','<<actor->GetPositionY()<<','<<actor->GetPositionZ()<<" movement="<<ai->movement<<" expired="<<ai->expired<<'\n';
  Check(ai->stage==RingReforged::Stage::Rock&&!door->HasFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_NOT_SELECTABLE),"Actual NPC approach did not unlock its rock");
  map.PlayerRelocation(&p,actor->GetPositionX(),actor->GetPositionY(),actor->GetPositionZ(),actor->GetOrientation());
  Check(p.CastSpell(&p,195134,true)==SPELL_CAST_OK,"Native failed-cast stun fixture failed");
  Check(p.CastSpell(door,195132,false)!=SPELL_CAST_OK&&!ai->openNonce&&p.GetQuestObjectiveData(40225,97048)==0,"Failed native stunned cast leaked nonce/opening/credit");p.RemoveAurasDueToSpell(195134);
  auto result=p.CastSpell(door,195132,false);
  std::cout<<"ROCK cast="<<unsigned(result)<<" in-range="<<door->IsWithinDistInMap(&p,door->GetInteractionDistance())<<" los-npc="<<p.IsWithinLOSInMap(actor)<<" nonce="<<ai->openNonce<<" state="<<unsigned(door->GetGoState())<<" flags="<<door->GetUInt32Value(GAMEOBJECT_FIELD_FLAGS)<<" area="<<p.GetAreaId()<<" mask="<<sSpellMgr->GetSpellInfo(195132)->GetExplicitTargetMask()<<'\n';
  Check(result==SPELL_CAST_OK,"Actual owned rock/nativeLock2454 cast rejected");
  Check(door->GetGoState()==GO_STATE_ACTIVE&&!door->m_model->isCollisionEnabled()&&p.GetQuestObjectiveData(40225,97048)==1,"Native OPEN_LOCK/personal endpoint acknowledgement did not complete the real rock objective");
  Check(p.GetQuestStatus(40225)==QUEST_STATUS_COMPLETE&&p.CanRewardQuest(sQuestDataStore->GetQuestTemplate(40225),false),"Actual native two-counter completion/reward admission failed");
  Check(other.GetQuestObjectiveData(40225,97047)==0&&other.GetQuestObjectiveData(40225,97048)==0,"Ungrouped nearby player received private progress");
  Check(partner.GetQuestObjectiveData(40225,97048)==0&&late.GetQuestObjectiveData(40225,97048)==0&&distant.GetQuestObjectiveData(40225,97048)==0&&wrong.GetQuestObjectiveData(40225,97048)==0,"Successful personal90 opening leaked to a group member");
  Advance(p,100);Check(p.HasAura(195134),"Native real opening did not retain its authored5s stun");
  Check(p.CastSpell(door,195132,false)!=SPELL_CAST_OK,"Repeated finished rock cast admitted");
  Advance(p,5100);Check(!ai->Door(),"Completed private rock survived cleanup/native9s re-close boundary");
  p.CancelQuestObjects(40225);map.RemoveAllObjectsInRemoveList();Check(!RingReforged::OwnedActor(&p),"Accepted scope cancellation left a current actor");
  for(auto player:{&p,&partner,&late,&distant,&wrong})player->SetGroup(nullptr);
 }
 {Map map(1469,300,9996996,DIFFICULTY_NONE);RingPlayer p(map,110);p.SetByteValue(UNIT_FIELD_BYTES_0,0,RACE_ORC);p.setFaction(2);RingSource source(map);Require(map.AddToMap(static_cast<Creature*>(&source)),"Retry source failed");
  auto actor=RingReforged::EnsureActor(&p);auto ai=RingReforged::Controller(&p);Check(actor&&ai,"Retry actor missing");Advance(p,2100);
  ai->Hello(&p);ai->sGossipSelect(&p,18940,0);Check(p.GetQuestObjectiveData(40225,97047)==1,"Retry did not start from real native first credit");
  // Abandon/re-entry lifetime cancellation before the native10ms child executes.
  auto receipt=p.GetQuestCancellationReceipt(40225);p.CancelQuestObjects(40225);map.RemoveAllObjectsInRemoveList();Advance(p,100);
  Check(receipt->load()&&!p.HasAura(195359)&&p.GetQuestObjectiveData(40225,97048)==0&&!RingReforged::OwnedActor(&p),"Canceled delayed child changed progress or retained its actor");
  auto resumed=RingReforged::EnsureActor(&p);ai=RingReforged::Controller(&p);Check(resumed&&ai&&ai->stage==RingReforged::Stage::Rock&&p.GetQuestCancellationReceipt(40225)!=receipt,"Persisted real first checkpoint failed to resume with a fresh accepted scope");
  Advance(p,2100);auto door=ai->Door();Check(door&&!door->HasFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_NOT_SELECTABLE),"Resumed native approach rock remained locked out");
  Check(!ai->BeginOpen(&p,door)&&!ai->openNonce,"Distant source position armed a resumed rock");
  auto oldDoor=door->GetGUID();door->Delete();map.RemoveAllObjectsInRemoveList();Advance(p,2200);door=ai->Door();
  Check(door&&door->GetGUID()!=oldDoor&&door->GetGoState()==GO_STATE_READY&&p.GetQuestObjectiveData(40225,97048)==0,"Missing private door did not recover without credit");
  p.SetQuestStatus(40225,QUEST_STATUS_NONE);Advance(p,20);Check(!RingReforged::OwnedActor(&p)&&!RingReforged::EnsureActor(&p),"Inactive quest retained/resummoned a private actor");
 }
 {Map map(1469,300,9996995,DIFFICULTY_NONE);RingPlayer p(map,120);RingSource source(map);Require(map.AddToMap(static_cast<Creature*>(&source)),"Interrupted oath source failed");
  RingReforged::EnsureActor(&p);auto ai=RingReforged::Controller(&p);Check(bool(ai),"Interrupted oath actor missing");Advance(p,2100);ai->Hello(&p);ai->sGossipSelect(&p,18940,0);Advance(p,3100);
  Check(ai->movement&&p.HasAura(195359),"Interrupted fixture missed real native movement/aura");
  p.RemoveAurasDueToSpell(195359);Advance(p,100);
  Check(!ai->expired&&ai->stage==RingReforged::Stage::Moving&&p.GetQuestObjectiveData(40225,97048)==0,"Non-expiry aura removal substituted for native oath completion");
  p.CancelQuestObjects(40225);map.RemoveAllObjectsInRemoveList();Check(!RingReforged::OwnedActor(&p),"Interrupted oath scope did not clean up");
 }
 {Map map(1469,300,9996994,DIFFICULTY_NONE);RingPlayer p(map,130);RingSource source(map);Require(map.AddToMap(static_cast<Creature*>(&source)),"Interrupted movement source failed");
  RingReforged::EnsureActor(&p);auto ai=RingReforged::Controller(&p);Check(bool(ai),"Interrupted movement actor missing");Advance(p,2100);ai->Hello(&p);ai->sGossipSelect(&p,18940,0);Advance(p,20);
  Check(p.HasAura(195359)&&!ai->movement,"Movement cancellation fixture missed the real pending path");p.GetMotionMaster()->Initialize();Advance(p,12500);
  Check(ai->expired&&!ai->movement&&ai->stage==RingReforged::Stage::Moving&&p.GetQuestObjectiveData(40225,97048)==0,"Aura expiry or canceled waypoint substituted for actual path completion");
  p.CancelQuestObjects(40225);map.RemoveAllObjectsInRemoveList();
 }
 std::cout<<"PASS "<<cases<<" actual native Ring gossip/parent-credit/delayed oath/player waypoint/12s aura/NPC movement/private rock/full-GO-target/open-lock/personal-dispatch/finish/cleanup and native group isolation cases; isolated expected World data, controlled acceptance/source publication, no client/audio/retail-cadence claim\n";std::cout.flush();LoginDatabase.Close();WorldDatabase.Close();HotfixDatabase.Close();MySQL::Library_End();
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
