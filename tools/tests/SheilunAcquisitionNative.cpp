// Actual core objects, spell dispatch, loot packet/storage and scoped scripts.
// Only prerequisite quest/scenario/actor identities are seeded in memory.
// World/Hotfix/Login are read-only. CharacterDB's native artifact-XP SELECT
// reads an empty connection-local shadow, with both connections read-only;
// no real character rows are read and no permanent writes are permitted.
#define MASTER_LOOT_NATIVE_SUPPORT_ONLY
#include "MasterLootNative.cpp"
#include "InstanceScript.h"
#include "InstanceSaveMgr.h"
#include "CreatureTextMgr.h"
#include "GameObject.h"
#include "MapManager.h"
#include "MapInstanced.h"
#include "MovementPackets.h"
#include "LFGMgr.h"
#include "AreaTriggerData.h"
#include "ScenarioMgr.h"
#include "Scenario.h"
#include "ScriptedGossip.h"
#include "GossipData.h"
#include "../../src/server/scripts/Scenario/Artifacts/Monk/Shey-lun/sheilun_plan.h"
#include "../../src/server/scripts/Scenario/Artifacts/Monk/Shey-lun/sheilun_acquisition.h"
#include "../../src/server/scripts/Scenario/Artifacts/Monk/Shey-lun/sheilun_return.h"
#include "../../src/server/scripts/Scenario/Artifacts/Monk/Shey-lun/sheilun_acquisition_scripts.h"
#include "SheilunGo.inc"

struct SheilunDataTag { using Type = InstanceScript* InstanceMap::*; friend Type Access(SheilunDataTag); };
template struct FixtureAccess<SheilunDataTag, &InstanceMap::i_data>;
struct NativeBossGuidTag { using Type = uint64 Creature::*; friend Type Access(NativeBossGuidTag); };
template struct FixtureAccess<NativeBossGuidTag, &Creature::m_DBTableGuid>;
struct SpellBindingsTag { using Type = SpellScriptsContainer ScriptDataStoreMgr::*; friend Type Access(SpellBindingsTag); };
template struct FixtureAccess<SpellBindingsTag, &ScriptDataStoreMgr::_spellScriptsStore>;
struct CriteriaDataTag { using Type = AchievementCriteriaDataVector AchievementGlobalMgr::*; friend Type Access(CriteriaDataTag); };
template struct FixtureAccess<CriteriaDataTag, &AchievementGlobalMgr::m_criteriaDataVector>;
struct SceneInstancesTag { using Type = std::unordered_map<uint32,uint32> Player::*; friend Type Access(SceneInstancesTag); };
template struct FixtureAccess<SceneInstancesTag, &Player::m_sceneInstanceID>;
struct TextListTag { using Type = CreatureTextList CreatureTextMgr::*; friend Type Access(TextListTag); };
template struct FixtureAccess<TextListTag, &CreatureTextMgr::mTextList>;
struct LogoutFlagTag { using Type = bool WorldSession::*; friend Type Access(LogoutFlagTag); };
template struct FixtureAccess<LogoutFlagTag, &WorldSession::m_playerLogout>;
struct LoadingFlagTag { using Type = ObjectGuid WorldSession::*; friend Type Access(LoadingFlagTag); };
template struct FixtureAccess<LoadingFlagTag, &WorldSession::m_playerLoading>;
struct ResumeTag { using Type = ScenarioResume Player::*; friend Type Access(ResumeTag); };
template struct FixtureAccess<ResumeTag, &Player::m_scenarioResume>;
struct SceneStatusTag { using Type = std::unordered_map<uint32,SceneEventStatus> Player::*; friend Type Access(SceneStatusTag); };
template struct FixtureAccess<SceneStatusTag, &Player::m_sceneStatus>;

struct AcquisitionPlayer : LootPlayer
{
    using LootPlayer::LootPlayer;
    WorldLocation SavedPosition()
    {
        // Execute only the unchanged native SaveToDB position serializer into
        // an actual prepared statement's memory parameters. Never enqueue it.
        std::unique_ptr<CharacterDatabasePreparedStatement> memory(CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHARACTER));
        auto stmt=memory.get(); uint8 index=0;
        auto finiteAlways=[](float f){return std::isfinite(f)?f:0.f;};
        #include "NativeSheilunSavePosition.inc"
        Require(index==9,"Native saved position layout changed");
        auto const& fields=stmt->GetParameters();
        return WorldLocation(std::get<uint16>(fields[0].data),std::get<float>(fields[5].data),
            std::get<float>(fields[6].data),std::get<float>(fields[7].data),std::get<float>(fields[8].data));
    }
};
static unsigned cases,returnCases,resumeCases,saveCases;
static bool refuseEquipment = false, refuseRide = false;
static unsigned equipmentRefusals = 0, rideRefusals = 0;

// Deliberate independent-endpoint faults: a failed optional equip must not
// strand an earned pickup, and a rejected first ride must not award progress.
// These loaders exist only in this native probe, never in server registration.
class probe_sheilun_equipment_refusal : public SpellScript
{
    PrepareSpellScript(probe_sheilun_equipment_refusal);
    SpellCastResult Check() { if(refuseEquipment){++equipmentRefusals;return SPELL_FAILED_BAD_TARGETS;}return SPELL_CAST_OK; }
    void Register() override { OnCheckCast += SpellCheckCastFn(probe_sheilun_equipment_refusal::Check); }
};
class probe_sheilun_ride_refusal : public SpellScript
{
    PrepareSpellScript(probe_sheilun_ride_refusal);
    bool Load() override { return GetCaster() && GetCaster()->IsPlayer(); }
    SpellCastResult Check() { if(refuseRide){++rideRefusals;return SPELL_FAILED_BAD_TARGETS;}return SPELL_CAST_OK; }
    void Register() override { OnCheckCast += SpellCheckCastFn(probe_sheilun_ride_refusal::Check); }
};

struct ObservedInstance : SheilunAcquisition::Instance
{
    explicit ObservedInstance(InstanceMap* map) : Instance(map) { }
    unsigned pickupEvents=0, returnEvents=0, leaves=0;
    void ProcessEvent(WorldObject*,uint32 event) override
    {
        if(event==48646) ++pickupEvents;
        if(event==48658) ++returnEvents;
    }
    void OnPlayerLeave(Player* player) override { ++leaves; Instance::OnPlayerLeave(player); }
};

#ifdef SHEILUN_SOURCE_BASELINE
struct PinnedAutomaticDeath : Scripted_NoMovementAI
{
    explicit PinnedAutomaticDeath(Creature* actor) : Scripted_NoMovementAI(actor) { }
    #include "PinnedSheilunDeath.inc"
};
#endif

struct CreditGuardian : Guardian
{
    CreditGuardian(LootPlayer& owner, Map& map, uint32 guid) : Guardian(nullptr,&owner,false)
    {
        Require(Create(guid,&map,1,299,0,ALLIANCE,owner.GetPositionX(),owner.GetPositionY(),owner.GetPositionZ(),0.f),"Native guardian creation failed");
        // Real Minion initialization establishes creator/owner and owner-list
        // semantics. Combat stats are fixture inputs; no guardian AI/scaling
        // fidelity is claimed by this killer-credit regression.
        Minion::InitStats(60000);
        SetLevel(110); SetMaxHealth(1000000); SetHealth(1000000);
        Require(map.AddToMap(static_cast<Creature*>(this)),"Native guardian map admission failed");
    }
    void AddToWorld() override
    {
        sObjectAccessor->AddObject(static_cast<Creature*>(this)); Unit::AddToWorld();
        Require(AIM_Initialize(new NullCreatureAI(this)),"Native idle guardian AI failed");
    }
    ~CreditGuardian()
    {
        CombatStop(true); CastStop(); RemoveAllAuras(); m_Events.KillAllEvents(true);
        if(IsInWorld()) GetMap()->RemoveFromMap(static_cast<Creature*>(this),false);
        ResetMap();
    }
};

static void InitializeSheilun(int argc, char** argv)
{
    Require(argc == 3, "Private native config/runtime missing");
    std::string error; Require(sConfigMgr->LoadInitial(argv[1], {}, error), "Private config failed");
    sLog->Initialize(nullptr); MySQL::Library_Init();
    WorldDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("WorldDatabaseInfo", ""), 1, 1);
    HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo", ""), 1, 1);
    LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
    CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
    Require(!WorldDatabase.Open() && WorldDatabase.PrepareStatements(), "World pool failed");
    Require(!HotfixDatabase.Open() && HotfixDatabase.PrepareStatements(), "Hotfix pool failed");
    Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Login pool failed");
    Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Character schema preparation failed");
    // StoreNewItem's native artifact initialization asks for prior XP. Use
    // the real schema with zero rows, shadowed before either connection can
    // issue a write. Prepared SELECT execution resolves this temporary table.
    CharacterDatabase.DirectExecute("CREATE TEMPORARY TABLE sheilun_empty_artifact_schema LIKE item_instance_artifact");
    CharacterDatabase.DirectExecute("CREATE TEMPORARY TABLE item_instance_artifact LIKE sheilun_empty_artifact_schema");
    CharacterDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY"); CharacterDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto empty = CharacterDatabase.Query("SELECT COUNT(*) FROM item_instance_artifact");
    Require(empty && !empty->Fetch()[0].GetUInt32(), "Artifact XP shadow is not empty");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY"); WorldDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY"); HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY"); LoginDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto ro = WorldDatabase.Query("SELECT @@session.transaction_read_only"); Require(ro && ro->Fetch()[0].GetBool(), "World must be read-only");
    ro = HotfixDatabase.Query("SELECT @@session.transaction_read_only"); Require(ro && ro->Fetch()[0].GetBool(), "Hotfix must be read-only");
    ro = LoginDatabase.Query("SELECT @@session.transaction_read_only"); Require(ro && ro->Fetch()[0].GetBool(), "Login must be read-only");
    ro = CharacterDatabase.Query("SELECT @@session.transaction_read_only"); Require(ro && ro->Fetch()[0].GetBool(), "Character session must be read-only");
    sDB2Manager.LoadStores(argv[2], LOCALE_enUS); LoadGameTables(argv[2]);
    sSpellMgr->LoadSpellInfoStore(); sSpellMgr->LoadSpellRanks(); sSpellMgr->LoadTalentSpellInfo();
    sSpellMgr->LoadSpellTargetPositions();
    sSpellMgr->LoadSkillLineAbilityMap(); sSpellMgr->LoadSpellCustomAttr(); sSpellMgr->LoadSpellInfoSpellSpecificAndAuraState();
    sObjectMgr->LoadScriptNames();
    // Pending code-coupled SQL stays outside the live updater. Seed only the
    // declared names/bindings/template link in memory; separate SQL tests must
    // validate the actual migration before a later offline promotion.
    auto& names = const_cast<ObjectMgr::ScriptNameContainer&>(sObjectMgr->GetAllScriptNames());
    for (auto name : {"npc_sheilun_owned_staff", "npc_sheilun_return_guide", "npc_sheilun_return_kite",
        "spell_sheilun_manual_claim", "spell_sheilun_claim_scene", "spell_sheilun_return_boarding", "spell_sheilun_return_transfer",
        "probe_sheilun_equipment_refusal", "probe_sheilun_ride_refusal"}) names.emplace_back(name);
    std::sort(names.begin(), names.end()); names.erase(std::unique(names.begin(), names.end()), names.end());
    sObjectMgr->LoadInstanceTemplate();
    sAreaTriggerDataStore->LoadAreaTriggerTeleports(); sLFGMgr->LoadLFGDungeons(); sObjectMgr->LoadScenarioData();
    // Exact pending data delta in memory; native SQL temporary-shadow tests
    // independently validate its guards, readback and idempotence.
    auto dungeon=sLFGMgr->GetLFGDungeon(1168);
    auto route=sObjectMgr->GetScenarioOnMap(1541,12,ALLIANCE,CLASS_MONK,1168);
    auto entrance=sSpellMgr->GetSpellTargetPosition(202742);
    Require(dungeon && dungeon->map==1541 && dungeon->difficulty==12 && dungeon->dbc->ScenarioID==0 &&
        route && route->ScenarioID==1007 && route->Class==CLASS_MONK && !route->Team && !route->LfgDungeonID &&
        (route->DifficultyID==8 || route->DifficultyID==12) && entrance && entrance->target_mapId==1541,
        "Native1168/1007/202742 resume data no longer matches the pending migration");
    #ifdef SHEILUN_LIVE_DATA
    Require(route->DifficultyID==12,"Live1007 route was not migrated to native NormalScenario12");
    #else
    const_cast<ScenarioData*>(route)->DifficultyID=12;
    #endif
    Require((!dungeon->x && !dungeon->y && !dungeon->z) ||
        (std::fabs(dungeon->x-entrance->target_X)<.001f && std::fabs(dungeon->y-entrance->target_Y)<.001f &&
            std::fabs(dungeon->z-entrance->target_Z)<.001f),"Custom native resume entrance must not be overwritten in the fixture");
    #ifdef SHEILUN_LIVE_DATA
    Require(std::fabs(dungeon->x-entrance->target_X)<.001f && std::fabs(dungeon->y-entrance->target_Y)<.001f &&
        std::fabs(dungeon->z-entrance->target_Z)<.001f && std::fabs(dungeon->o-entrance->target_Orientation)<.001f,
        "Actual live native LFG loader did not read the migrated1168 entrance");
    #else
    auto patched=const_cast<lfg::LFGDungeonData*>(dungeon);
    patched->x=entrance->target_X;patched->y=entrance->target_Y;patched->z=entrance->target_Z;patched->o=entrance->target_Orientation;
    #endif
    sObjectMgr->LoadCreatureModelInfo(); sObjectMgr->LoadWDBCreatureTemplates();
    sObjectMgr->LoadCreatureTemplates(); sObjectMgr->LoadCreatureClassLevelStats();
    sObjectMgr->LoadCreatureScalingData(); sObjectMgr->LoadCreatureDifficultyStat();
    sObjectMgr->LoadPageTexts(); sObjectMgr->LoadGameObjectTemplate(); sObjectMgr->LoadNPCSpellClickSpells();
    sObjectMgr->LoadVehicleTemplateAccessories();
    for (auto pair : {std::make_pair(102479u,"npc_sheilun_owned_staff"), std::make_pair(101880u,"npc_sheilun_return_guide"),
        std::make_pair(102648u,"npc_sheilun_return_kite")})
    {
        #ifdef SHEILUN_LIVE_DATA
        Require(sObjectMgr->GetCreatureTemplate(pair.first)->ScriptID==sObjectMgr->GetScriptId(pair.second),"Actual live creature loader ScriptName mismatch");
        #else
        const_cast<CreatureTemplate*>(sObjectMgr->GetCreatureTemplate(pair.first))->ScriptID = sObjectMgr->GetScriptId(pair.second);
        #endif
    }
    #ifdef SHEILUN_LIVE_DATA
    Require(sObjectMgr->GetGameObjectTemplate(256913)->chest.chestLoot==256913,"Actual live GO loader did not read the real loot link");
    #else
    const_cast<GameObjectTemplate*>(sObjectMgr->GetGameObjectTemplate(256913))->chest.chestLoot = 256913;
    #endif
    sScriptDataStore->LoadSpellScriptNames();
    auto& bindings = sScriptDataStore->*Access(SpellBindingsTag{});
    for (auto pair : {std::make_pair(202435u,"spell_sheilun_manual_claim"), std::make_pair(202437u,"spell_sheilun_claim_scene"),
        std::make_pair(202828u,"spell_sheilun_return_boarding"), std::make_pair(202831u,"spell_sheilun_return_transfer"),
        std::make_pair(202438u,"probe_sheilun_equipment_refusal"), std::make_pair(46598u,"probe_sheilun_ride_refusal")})
    {
        uint32 id=sObjectMgr->GetScriptId(pair.second);
        auto range=bindings.equal_range(pair.first);
        auto count=std::count_if(range.first,range.second,[=](auto const& row){return row.second.first==id;});
        Require(count<=1,"Actual native binding is duplicated");
        #ifdef SHEILUN_LIVE_DATA
        if(pair.first==202435 || pair.first==202437 || pair.first==202828 || pair.first==202831)
            Require(count==1,"Actual live spell-script loader did not read the migrated binding");
        #endif
        if(!count) bindings.emplace(pair.first,std::make_pair(id,true));
    }
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    SheilunAcquisition::Register(); new go_art_sheylun();
    RegisterSpellScript(probe_sheilun_equipment_refusal);
    RegisterSpellScript(probe_sheilun_ride_refusal);
    sScriptMgr->SwapScriptContext(true);
    AIRegistry::Initialize(); UnitAI::FillAISpellInfo(); sWorld->LoadConfigSettings(); opcodeTable.Initialize();
    HashMapHolder<Player>::SetSize(32);
    // Manually seeded role GUIDs precede native summons. Initialize their
    // real indexed accessors just as the server/bootstrap does; Insert silently
    // refuses a GUID outside its initialized vector capacity.
    HashMapHolder<Creature>::SetSize(20000);
    HashMapHolder<Pet>::SetSize(20000);
    auto rows = WorldDatabase.Query(
#include "NativeSheilunQuest.inc"
    );
    Require(bool(rows), "Missing real quest41003");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{}); quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = 45089;
    auto quest = new Quest(rows->Fetch());
    Require(quest->GetQuestId() == 41003, "Wrong quest source");
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates())[41003] = quest; quests[41003] = quest;
    auto objectives = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged FROM quest_objectives WHERE QuestID=41003 ORDER BY StorageIndex");
    Require(bool(objectives), "Missing real quest objectives");
    do { quest->LoadQuestObjective(objectives->Fetch()); } while (objectives->NextRow());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
    Require(quest->GetObjectives().size() == 3, "Changed native quest41003");
    sObjectMgr->LoadItemTemplates(); sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadNPCText(); sQuestDataStore->LoadPointsOfInterest();
    sGossipDataStore->LoadGossipMenu(); sGossipDataStore->LoadGossipMenuItems();
    auto& table = LootTemplates_Gameobject.*Access(LootTableNameTag{});
    auto original = table; table += " WHERE Entry=256913";
    auto loaded = (LootTemplates_Gameobject.*Access(LoadLootTableTag{}))(); table = original;
    Require(loaded == 1, "Changed actual source loot table");
    sAchievementMgr->LoadCriteriaList();
    auto conditions = WorldDatabase.Query("SELECT COUNT(*) FROM achievement_criteria_data WHERE criteria_id IN(46662,46663,46717)");
    Require(conditions && !conditions->Fetch()[0].GetUInt32(), "Probe must load new Sheilun criteria conditions before proceeding");
    // Native LoadAchievementCriteriaData initializes this index before any
    // event. These three scoped native trees have no additional SQL rules.
    (sAchievementMgr->*Access(CriteriaDataTag{})).assign(MAX_CRITERIA_TREE,nullptr);
    sSpellMgr->LoadSpellScene();
    #ifdef SHEILUN_LIVE_DATA
    sCreatureTextMgr->LoadCreatureTexts();
    for(auto row:{std::make_tuple(101881u,13u,104773u),std::make_tuple(101881u,14u,104774u),
        std::make_tuple(101881u,15u,104775u),std::make_tuple(101881u,16u,104776u),std::make_tuple(101881u,17u,104777u),
        std::make_tuple(101881u,18u,104785u),std::make_tuple(101880u,13u,104958u)})
    {
        // GetTextGroup intentionally exposes only timed (MinTimer!=0)
        // groups. Ordinary SendChat reads these native untimed map rows.
        auto const& texts=sCreatureTextMgr->GetTextMap(); auto speaker=texts.find(std::get<0>(row));
        Require(speaker!=texts.end(),"Actual live text loader omitted the outro speaker");
        auto group=speaker->second.find(uint8(std::get<1>(row)));
        Require(group!=speaker->second.end() && group->second.size()==1 && group->second.front().BroadcastTextID==std::get<2>(row),
            "Actual live text loader did not read the native outro row");
    }
    #else
    // Pending text rows are represented in memory only. Read the actual
    // matching BroadcastText payload; the migration's complete SQL rows and
    // refusal/idempotence behavior are independently exercised by Data tests.
    auto& textMap=const_cast<CreatureTextMap&>(sCreatureTextMgr->GetTextMap());
    auto& textList=sCreatureTextMgr->*Access(TextListTag{}); textList.resize(101882,nullptr);
    for(auto row : {std::make_tuple(101881u,13u,104773u),std::make_tuple(101881u,14u,104774u),
        std::make_tuple(101881u,15u,104775u),std::make_tuple(101881u,16u,104776u),
        std::make_tuple(101881u,17u,104777u),std::make_tuple(101881u,18u,104785u),std::make_tuple(101880u,13u,104958u)})
    {
        auto native=sBroadcastTextStore.LookupEntry(std::get<2>(row)); Require(native,"Native outro BroadcastText missing");
        CreatureTextEntry text{}; text.creatureId=std::get<0>(row); text.group=uint8(std::get<1>(row));
        text.BroadcastTextID=native->ID; text.text=native->Text->Str[LOCALE_enUS];
        text.probability=100; text.type=CHAT_MSG_SAY; text.lang=LANG_UNIVERSAL;
        textMap[text.creatureId][text.group].push_back(text); textList[text.creatureId]=&textMap[text.creatureId];
    }
    #endif
    dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1541,{}},{1514,{}}});
    MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1541,{}},{1514,{}}});
}

static void AdvanceSource(SheilunAcquisition::Instance& controller, LootPlayer& player, uint32 duration)
{
    for (uint32 elapsed = 0; elapsed < duration; elapsed += 10)
    {
        if(!player.IsInWorld() || player.HasPendingTeleport()) return;
        player.Unit::Update(10); controller.Update(10);
        auto state = controller.State();
        for (auto guid : {state.staff,state.guide,state.kite})
            if (Creature* actor = player.GetMap()->GetCreature(guid)) actor->Update(10);
        // Native vehicle accessories use a null-properties summon and are
        // not in the owner's summon list. Run their actual join events too.
        if(auto kite=SheilunAcquisition::CreatureAt(player.GetMap(),state.kite))
        {
            std::list<Creature*> actors; kite->GetCreatureListWithEntryInGrid(actors,101880,50.f);
            for(auto actor:actors) if(auto summon=actor->ToTempSummon())
                if(summon->GetSummonerGUID()==kite->GetGUID()) actor->Update(10);
        }
        if (GameObject* proxy = player.GetMap()->GetGameObject(state.proxy)) proxy->Update(10);
        player.GetMap()->MoveAllCreaturesInMoveList();
        player.GetMap()->RemoveAllObjectsInRemoveList();
    }
}

static void AutoStoreGuid(LootPlayer& player, ObjectGuid loot, uint8 slot)
{
    WorldPacket wire(CMSG_LOOT_ITEM); wire << uint32(1) << loot << uint8(slot+1);
    WorldPackets::Loot::AutoStoreLootItem packet(std::move(wire)); packet.Read();
    Require(packet.Loot.size()==1 && packet.Loot[0].Object==loot, "Native autostore wire decode changed");
    player.GetSession()->HandleAutostoreLootItemOpcode(packet);
}
static void AutoStore(LootPlayer& player, Loot& loot, uint8 slot) { AutoStoreGuid(player,loot.GetGUID(),slot); }

struct NativeReturnSession
{
    LootPlayer& player;
    explicit NativeReturnSession(LootPlayer& owner) : player(owner)
    {
        // Stop/join only this probe's own freshly created map workers before
        // attaching a fixture player. Native transfer and ACK run synchronously.
        Map* base=sMapMgr->CreateBaseMap(1514); base->SetMapStop();
        auto thread=sMapMgr->_mapThreads[1514]; if(thread && thread->joinable()) thread->join();
        Map* destination=sMapMgr->CreateMap(1514,&player); destination->SetMapStop();
        if(base->CanCreatedZone()) for(auto& zone:static_cast<MapInstanced*>(base)->_zoneThreads)
            if(zone.second && zone.second->joinable()) zone.second->join();
        auto& sessions=const_cast<SessionMap&>(sWorld->GetAllSessions());
        Require(!sessions.count(player.GetSession()->GetAccountId()),"Native return fixture would replace another session");
        sessions[player.GetSession()->GetAccountId()]=WorldSessionPtr(player.GetSession(),[](WorldSession*){});
        player.GetSession()->SetMap(player.GetMap());
    }
    ~NativeReturnSession()
    {
        const_cast<SessionMap&>(sWorld->GetAllSessions()).erase(player.GetSession()->GetAccountId());
        player.GetSession()->SetMap(nullptr);
    }
};

static void CheckReturn(ObservedInstance& controller,AcquisitionPlayer& player,uint8 cancel=0,bool resumed=false)
{
    using namespace SheilunAcquisition;
    NativeReturnSession nativeSession(player);
    uint32 scene=0;
    for(auto const& entry:player.*Access(SceneInstancesTag{})) if(entry.second==1192) scene=std::max(scene,entry.first);
    Require((scene || resumed) && controller.pickupEvents==1,"Actual pickup scene/event identity missing");
    unsigned equipBefore=equipmentRefusals;
    if(!resumed)
    {
        refuseEquipment=true; player.SceneCompleted(scene); AdvanceSource(controller,player,110); refuseEquipment=false;
        Require(player.HasSceneStatus(1192,SCENE_COMPLETE) && equipmentRefusals==equipBefore+1,
            "Actual scene callback/declared optional equipment refusal did not execute");
    }
    else Require(!player.HasSceneStatus(1192,SCENE_COMPLETE),"Resume fabricated a scene-complete receipt");
    AdvanceSource(controller,player,35000);
    Creature* guide=CreatureAt(player.GetMap(),controller.State().guide);
    Require(guide && controller.State().guideReady && guide->HasQuestObjectScope(),"Native full outro did not create its owned guide");
    Require(sScriptMgr->OnGossipHello(&player,guide) && SheilunPlan::Menu(&player,19257,0,19257,0),
        "Actual owned guide did not offer native19257 return");
    guide->AI()->sGossipSelect(&player,19257,0);
    Require(controller.State().travel==Travel::Requested,"Actual return selection rejected earned pickup");
    unsigned rideBefore=rideRefusals;
    refuseRide=true; AdvanceSource(controller,player,100);
    ObjectGuid kiteGuid=controller.State().kite;
    Creature* kite=CreatureAt(player.GetMap(),kiteGuid);
    Quest const* quest=sQuestDataStore->GetQuestTemplate(41003);
    std::cout<<"RETURN_BOARD pending="<<player.waitOnSeat<<" kite="<<bool(kite)
        <<" refused="<<(rideRefusals-rideBefore)<<" event="<<controller.returnEvents
        <<" counter="<<player.GetQuestObjectiveData(quest,2)<<std::endl;
    Require(kite && rideRefusals>rideBefore && !player.GetVehicle() && !controller.returnEvents &&
        !player.GetQuestObjectiveData(quest,2),"Failed native ride awarded return progression");
    refuseRide=false; AdvanceSource(controller,player,1500);
    kite=CreatureAt(player.GetMap(),kiteGuid);
    Unit* driver=kite && kite->GetVehicleKit()?kite->GetVehicleKit()->GetPassenger(1):nullptr;
    std::cout<<"RETURN_JOIN same="<<(controller.State().kite==kiteGuid)<<" player="<<(player.GetVehicleBase()==kite)
        <<" driver="<<bool(driver)<<" scope="<<(driver&&driver->HasQuestObjectScope())
        <<" event="<<controller.returnEvents<<" counter="<<player.GetQuestObjectiveData(quest,2)
        <<" travel="<<unsigned(controller.State().travel)<<std::endl;
    Require(kite && controller.State().kite==kiteGuid && player.GetVehicleBase()==kite &&
        kite->GetVehicleKit()->GetPassenger(0)==&player && driver && driver->GetEntry()==101880,
        "Native same-actor retry/player seat0/Tak-Tak seat1 failed");
    Require(driver->HasQuestObjectScope() && driver->CanTargetQuestObject(&player) && kite->CanTargetQuestObject(driver),
        "Native driver did not inherit the kite's accepted quest scope before creation");
    Require(controller.returnEvents==1 && controller.State().returnEffects==6 && player.GetQuestObjectiveData(quest,2)==1,
        "Actual boarded native credit/event did not execute exactly once");
    player.CastSpell(&player,202828,true); AdvanceSource(controller,player,100);
    Require(controller.returnEvents==1 && controller.State().kite==kiteGuid,"Repeated native return cast duplicated progression/vehicle");
    for(uint32 elapsed=0;elapsed<60000 && player.IsInWorld() && !player.HasPendingTeleport();elapsed+=10)
        AdvanceSource(controller,player,10);
    Require(player.IsBeingTeleportedFar() && player.GetTeleportDest().GetMapId()==1514 && !player.GetVehicle() &&
        player.GetUnitBeingMoved()==&player,"Actual first flight/endpoint/exit did not reach native far transfer");
    Require(controller.pickupEvents==1 && controller.returnEvents==1,"Flight replayed pickup/return events");
    std::cout<<"CASE native return first leg+failed ride+same actor retry+actual spline+transfer PASS\n";
    Require(controller.leaves==(resumed?2u:1u),"Actual native transfer did not call source leave exactly once per removal");
    player.GetSession()->HandleWorldPortAck();
    Require(player.IsInWorld() && player.GetMapId()==1514 && !player.HasPendingTeleport(),"Actual native1514 far-map ACK failed");
    player.NotifyMapObjectArrivalLoaded(1514);
    Transit transit; ObjectGuid lastKite; bool destinationBoarded=false,destinationFlight=false,canceled=false;
    ObjectGuid retainedItem=InventoryArtifact(&player)->GetGUID(); uint32 inFlight=0;
    for(uint32 elapsed=0;elapsed<120000;elapsed+=10)
    {
        if(!ReadTransit(player.GetGUID(),transit)) break;
        player.Unit::Update(10); player.UpdateMapObjectArrival(10); sScriptMgr->OnUpdate(&player,10);
        ReadTransit(player.GetGUID(),transit);
        if(!transit.kite.IsEmpty()) lastKite=transit.kite;
        if(auto actor=CreatureAt(player.GetMap(),lastKite))
        {
            actor->Update(10);
            std::list<Creature*> drivers; actor->GetCreatureListWithEntryInGrid(drivers,101880,50.f);
            for(auto driver:drivers) if(auto summon=driver->ToTempSummon())
                if(summon->GetSummonerGUID()==actor->GetGUID()) driver->Update(10);
            if(player.GetVehicleBase()==actor && KiteDriver(actor)) destinationBoarded=true;
            if(transit.leg==Leg::Flight && player.GetUnitBeingMoved()==actor) destinationFlight=true;
        }
        if(cancel && !canceled && transit.leg==Leg::Flight && (inFlight+=10)>=2000)
        {
            canceled=true;
            if(cancel==1) player.ExitVehicle();
            else if(cancel==2) player.SetQuestStatus(41003,QUEST_STATUS_NONE); // In-memory native abandonment only.
            else
            {
                WorldLocation oldSave=player.SavedPosition();
                Require(oldSave.GetMapId()==1514 && oldSave.GetExactDist(Landing())>20.f,
                    "Pre-hook native save did not reproduce the old in-flight position");
                if(cancel==4) player.SetQuestStatus(41003,QUEST_STATUS_NONE);
                player.GetSession()->*Access(LogoutFlagTag{})=true;
                sScriptMgr->OnPlayerBeforeLogout(&player);
                WorldLocation repairedSave=player.SavedPosition();
                Require(repairedSave.GetMapId()==1514 && repairedSave.GetExactDist(Landing())<.1f &&
                    !ReadTransit(player.GetGUID(),transit),"Before-save dispatch did not preserve the native safe position/cleanup");
                player.GetSession()->*Access(LogoutFlagTag{})=false;
                ++saveCases;
                std::cout<<"CASE native save-position counterfactual air pose -> actual before-logout dispatch -> safe5710 memory parameters PASS; no character save executed\n";
            }
        }
        player.GetMap()->MoveAllCreaturesInMoveList(); player.GetMap()->RemoveAllObjectsInRemoveList();
    }
    if(cancel)
    {
        Require(canceled && !ReadTransit(player.GetGUID(),transit) &&
            (cancel>=3?player.IsBeingTeleportedFar():player.IsBeingTeleportedNear()) &&
            player.GetTeleportDest().GetMapId()==1514 && player.GetTeleportDest().GetExactDist(Landing())<.1f,
            "Interrupted flight did not exit then request its native safe landing");
        if(cancel>=3)
        {
            // The actual save proof above precedes this wait. A real new
            // Player has no old exit spline; finish this reused fixture's
            // native spline before driving an ACK for the safe saved pose.
            Tick(player,2000); player.GetSession()->HandleWorldPortAck();
        }
        else
        {
            WorldPacket wire(CMSG_MOVE_TELEPORT_ACK); wire<<player.GetGUID()<<int32(0)<<int32(0);
            WorldPackets::Movement::MoveTeleportAck ack(std::move(wire)); ack.Read(); player.GetSession()->HandleMoveTeleportAck(ack);
        }
        Require(!player.HasPendingTeleport() && InventoryArtifact(&player)->GetGUID()==retainedItem,
            "Interrupted flight's native landing ACK changed the earned item or left a pending teleport");
    }
    Position landing=Landing();
    if(player.GetDistance(landing)>=3.f || !player.movespline->Finalized())
        std::cout<<"DEST_FINAL cancel="<<unsigned(cancel)<<" distance="<<player.GetDistance(landing)
            <<" final="<<player.movespline->Finalized()<<" pending="<<player.HasPendingTeleport()<<std::endl;
    Require(!ReadTransit(player.GetGUID(),transit) && destinationBoarded && destinationFlight && !player.GetVehicle() &&
        player.GetUnitBeingMoved()==&player && player.movespline->Finalized() && player.GetDistance(landing)<3.f,
        "Actual destination boarding/hold/5619 spline/endpoint/exit/control/cleanup failed");
    Require(controller.pickupEvents==1 && controller.returnEvents==1 && (cancel==2 || cancel==4 || player.GetQuestObjectiveData(quest,2)==1),
        "Second leg duplicated native quest/event progression");
    for(uint32 elapsed=0;elapsed<2500;elapsed+=10)
    {
        player.Unit::Update(10);
        if(auto actor=CreatureAt(player.GetMap(),lastKite)) actor->Update(10);
        player.GetMap()->MoveAllCreaturesInMoveList(); player.GetMap()->RemoveAllObjectsInRemoveList();
    }
    Require(!CreatureAt(player.GetMap(),lastKite),"Destination vehicle did not retire after real exit spline");
    player.GetMap()->RemovePlayerFromMap(&player,false);
    ++returnCases;
    std::cout<<"CASE native1514 ACK+owned boarding+arrival hold+5619 flight+5710 landing+cleanup cancel="<<unsigned(cancel)<<" PASS\n";
}

static void CheckSourceRelog(ObservedInstance& controller,AcquisitionPlayer& player)
{
    using namespace SheilunAcquisition;
    auto map=static_cast<InstanceMap*>(player.GetMap());
    auto progress=controller.Progress(); ObjectGuid item=InventoryArtifact(&player)->GetGUID();
    Require(progress && progress->IsCompleted(false) && player.HasPendingSheilunReturn(),
        "Actual native completed pickup is missing its distinct pending return");
    player.RecordScenarioEntrance(); auto checkpoint=player.*Access(ResumeTag{});
    Require(checkpoint.map==1541 && checkpoint.scenario==1007 && checkpoint.instance==map->GetInstanceId(),
        "Completed pickup prematurely cleared its native entrance checkpoint");
    player.GetSession()->*Access(LogoutFlagTag{})=true; map->RemovePlayerFromMap(&player,false);
    Require(controller.State().logoutResume && controller.leaves==1,"Native source leave did not retain its accepted logout attempt");
    map->RemoveAllObjectsInRemoveList();
    // Model reconstruction of transient quest/scene objects only. Native
    // inventory/progress/Map removal/admission are real, no DB save/load or
    // authenticated reconnect is claimed by this controlled boundary.
    auto quest=player.getQuestStatus(41003); quest->ScriptLifetime.reset(); quest->ScriptObjects.reset();
    (player.*Access(SceneInstancesTag{})).clear(); (player.*Access(SceneStatusTag{})).clear();
    player.GetSession()->*Access(LogoutFlagTag{})=false;
    player.GetSession()->*Access(LoadingFlagTag{})=player.GetGUID();
    player.PrepareScenarioLogin(1541,map->GetInstanceId());
    Require(player.GetScenarioLoginInstance()==map->GetInstanceId(),"Exact native live completed outro rejoin was rejected");
    Require(map->AddPlayerToMap(&player) && player.CheckInstanceLoginValid(),"Native post-AddPlayer owned-outro admission failed");
    player.GetSession()->*Access(LoadingFlagTag{})=ObjectGuid::Empty;
    controller.Update(10);
    Require(controller.Lease(&player) && controller.State().claimed && controller.State().manualReceipt &&
        InventoryArtifact(&player)->GetGUID()==item && controller.State().dialogue==1 &&
        !player.HasSceneStatus(1192,SCENE_COMPLETE),"Completed outro did not rebind the native login attempt without resetting/awarding progress");
    std::cout<<"CASE native completed pickup+pending return checkpoint+logout removal+weak reconstruction+Prepare+actual map admission+controller resume PASS\n";
    ++resumeCases;
}

static void CheckSource(uint8 race, uint8 killerKind, bool fullBags, uint32 number,bool existing=false)
{
    using namespace SheilunAcquisition;
    MapInstanced parent(1541,300);
    InstanceMap map(1541,300,9999700+number,DIFFICULTY_N_SCENARIO,&parent);
    Require(sInstanceSaveMgr->AddInstanceSave(1541,map.GetInstanceId(),DIFFICULTY_N_SCENARIO,0,"",time(nullptr)+7200,true,true), "Missing in-memory map admission save");
    auto controller = new ObservedInstance(&map); map.*Access(SheilunDataTag{}) = controller;
    Scenario* progress = sScenarioMgr->AddScenario(&map,1007);
    Require(progress && progress->GetScenarioId()==1007, "Native scenario construction failed");
    auto session = new WorldSession(0,"Sheilun native fixture",{},SEC_PLAYER,6,0,"Win",LOCALE_enUS,0,false,AT_AUTH_FLAG_NONE,{});
    {
        AcquisitionPlayer player(session,map,700+number,{-1017.29f,-2772.8f,38.57111f,0.f});
        player.SetByteValue(UNIT_FIELD_BYTES_0,0,race); player.setFactionForRace(race);
        player.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_MONK);
        auto nativeRace=sChrRacesStore.LookupEntry(race); Require(nativeRace,"Native race model missing");
        Require(sCreatureDisplayInfoStore.LookupEntry(nativeRace->MaleDisplayID),"Native male race display missing");
        player.SetNativeDisplayId(nativeRace->MaleDisplayID); player.SetDisplayId(nativeRace->MaleDisplayID); player.SetObjectScale(1.f);
        player.SetQuestStatus(41003,QUEST_STATUS_INCOMPLETE); player.SetQuestSlot(0,41003);
        // The earlier outbound SmartAI LINK3 issues this native credit. Its
        // travel is outside this fixture; execute its real quest dispatch so
        // duplicate101880 objectives retain native first-unfilled semantics.
        Quest const* nativeQuest=sQuestDataStore->GetQuestTemplate(41003);
        player.KilledMonsterCredit(101880);
        Require(player.GetQuestObjectiveData(nativeQuest,0)==1 && !player.GetQuestObjectiveData(nativeQuest,2),
            "Native outbound credit did not leave its distinct return objective pending");
        ObjectGuid existingItem;
        if(existing)
        {
            // Persisted acquisition inputs for a fresh entrance restart. The
            // new scenario must still earn real victory and manual interaction.
            player.SetQuestObjectiveData(nativeQuest,&nativeQuest->GetObjectives()[1],1);
            ItemPosCountVec dest;
            Require(player.CanStoreNewItem(NULL_BAG,NULL_SLOT,dest,128937,1)==EQUIP_ERR_OK,"Existing artifact fixture inventory refused");
            Item* owned=player.StoreNewItem(dest,128937,true); Require(owned,"Existing native artifact fixture creation failed");
            existingItem=owned->GetGUID();
        }
        controller->OnPlayerEnter(&player); controller->Update(10);
        Require(controller->Lease(&player), "Native accepted attempt binding failed");
        // Seed only the earlier stage boundary, then execute all three real
        // tactic acknowledgements and the actual prepared native gossip offer.
        progress->SetCurrentStep(2);
        player.RecordScenarioEntrance();
        if((player.*Access(ResumeTag{})).scenario!=1007)
        {
            auto direct=sLFGMgr->GetLFGDungeon(uint16(1007),uint16(1541));
            auto difficulty=sDB2Manager.GetDefaultMapDifficulty(1541);
            auto fallback=difficulty?sLFGMgr->GetLFGDungeon(1541,Difficulty(difficulty->DifficultyID),player.GetTeam()):nullptr;
            auto route=fallback?sObjectMgr->GetScenarioOnMap(1541,fallback->difficulty,player.GetTeam(),CLASS_MONK,fallback->id):nullptr;
            std::cout<<"RESUME_ROUTE scenarioMap="<<map.IsScenario()<<" native="<<map.GetEntry()->IsScenario()
                <<" type="<<sScenarioStore.LookupEntry(1007)->Type<<" challenge="<<SCENARIO_TYPE_CHALLENGE_MODE
                <<" garrison="<<map.GetEntry()->IsGarrison()<<" direct="<<bool(direct)<<" fallback="<<bool(fallback)
                <<" route="<<(route?route->ScenarioID:0)<<" diff="<<(difficulty?difficulty->DifficultyID:0)
                <<" xyz="<<(direct?direct->x:0)<<','<<(direct?direct->y:0)<<','<<(direct?direct->z:0)
                <<" fallbackXYZ="<<(fallback?fallback->x:0)<<','<<(fallback?fallback->y:0)<<','<<(fallback?fallback->z:0)<<std::endl;
            for(auto native:sLfgDungeonsStore) if(native->MapID==1541)
                std::cout<<"NATIVE_LFG id="<<native->ID<<" scenario="<<native->ScenarioID<<" diff="<<unsigned(native->DifficultyID)
                    <<" type="<<unsigned(native->TypeID)<<" faction="<<int(native->Faction)<<" flags="<<native->Flags<<std::endl;
        }
        Require((player.*Access(ResumeTag{})).scenario==1007,"Actual native Sheilun entrance routing/checkpoint missing");
        for (uint32 event : {48446u,48447u,48448u}) player.UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,event);
        Require(progress->GetCurrentStep()==3 && SheilunPlan::Planned(progress),"Native plan prerequisite receipts failed");
        Actor taran(&map,101881,1100+number,player.GetPosition());
        taran.*Access(NativeBossGuidTag{}) = 362837;
        Require(map.AddToMap(static_cast<Creature*>(&taran)),"Native Taran role admission failed");
        controller->OnCreatureCreate(&taran); SheilunPlan::Refresh(&taran);
        Require(map.GetCreature(taran.GetGUID())==&taran,"Native controlled Taran GUID was not registered for post-criteria resolution");
        player.PrepareGossipMenu(&taran,19235,true);
        Require(SheilunPlan::Menu(&player,19235,0,19235,0),"Actual native plan offer missing");
        std::cout<<"PLAN_ADMISSION world="<<bool(SheilunPlan::World(&taran))<<" player="<<SheilunPlan::PlayerContext(&player,&taran)
            <<" gossip="<<taran.HasFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_GOSSIP)<<" phase="<<player.InSamePhase(&taran)
            <<" visible="<<player.canSeeOrDetect(&taran)<<" los="<<player.IsWithinLOSInMap(&taran)
            <<" range="<<player.IsWithinDistInMap(&taran,INTERACTION_DISTANCE)<<" rewarded="<<player.GetQuestRewardStatus(41003)
            <<" step="<<unsigned(progress->GetCurrentStep())<<" prior="<<Receipt(progress,46662)<<std::endl;
        SheilunPlan::Start(&player,&taran,19235,0);
        if (!controller->State().started) std::cout<<"PLAN_AFTER started="<<controller->State().started<<" receipt="<<Receipt(progress,46662)
            <<" context="<<SheilunPlan::PlayerContext(&player,&taran)<<" step="<<unsigned(progress->GetCurrentStep())
            <<" planned="<<SheilunPlan::Planned(progress)<<" gossip="<<taran.HasFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_GOSSIP)
            <<" lease="<<controller->Lease(&player)<<" present="<<controller->Present(&player)
            <<" controller="<<(Controller(&player)==controller)<<" db="<<taran.GetDBTableGUIDLow()
            <<" quest="<<!controller->State().quest.expired()<<" world="<<bool(SheilunPlan::World(&taran))
            <<" lookup="<<(map.GetCreature(taran.GetGUID())==&taran)<<" playerLookup="<<(ObjectAccessor::FindPlayer(&map,player.GetGUID())==&player)
            <<" entry="<<taran.GetEntry()<<std::endl;
        Require(Receipt(progress,46662) && controller->State().started,"Actual native plan->controller start acknowledgement failed");
        Actor boss(&map,101887,800+number,player.GetPosition());
        boss.*Access(NativeBossGuidTag{}) = 362866;
        Require(map.AddToMap(static_cast<Creature*>(&boss)), "Native boss admission failed");
        controller->OnCreatureCreate(&boss);
        boss.LowerPlayerDamageReq(uint32(boss.GetMaxHealth())); boss.SetLootRecipient(&player);
        Actor ally(&map,101882,900+number,player.GetPosition());
        Require(map.AddToMap(static_cast<Creature*>(&ally)), "Native killer actor admission failed");
        std::unique_ptr<LootPet> pet;
        std::unique_ptr<CreditGuardian> guardian;
        Unit* killer=&player;
        if(killerKind==1) { pet.reset(new LootPet(player,map)); killer=pet.get(); }
        if(killerKind==2) { guardian.reset(new CreditGuardian(player,map,1000+number)); killer=guardian.get(); }
        if(killerKind==3) killer=&ally;
        if(killerKind==3) controller->BattleAlly(&boss,&ally);
        bool ownerBefore = killer->GetCharmerOrOwnerPlayerOrPlayerItself() == &player;
        Require(killerKind==3 || ownerBefore,"Native player/pet/guardian fixture lost its actual owner");
        // Direct native Kill deliberately exercises death without EnterCombat;
        // damage scaling and actual client combat are separate retained checks.
        killer->Kill(&boss,false);
        if (!controller->State().victory)
            std::cout<<"DEATH_DIAGNOSTIC kind="<<unsigned(killerKind)<<" ownerBefore="<<ownerBefore
                <<" ownerAfter="<<(killer->GetCharmerOrOwnerPlayerOrPlayerItself()==&player)
                <<" started="<<controller->State().started<<" defeated="<<bool(controller->State().defeated)
                <<" lease="<<controller->Lease(&player)<<" present="<<controller->Present(&player,false)
                <<" range="<<player.IsAtGroupRewardDistance(&boss)<<" sameBoss="<<(controller->State().boss==boss.GetGUID())
                <<" alive="<<boss.IsAlive()<<" step="<<unsigned(progress->GetCurrentStep())
                <<" receipt="<<Receipt(progress,46663)<<std::endl;
        Require(controller->State().victory && Receipt(progress,46663), "Native death/owner/ally victory receipt missing");
        auto defeated = controller->State().defeated;
        controller->Defeated(&boss,&player);
        Require(controller->State().defeated==defeated &&
            (existing?(InventoryArtifact(&player) && InventoryArtifact(&player)->GetGUID()==existingItem):!InventoryArtifact(&player)) &&
            !Receipt(progress,46717), "Repeated death granted artifact/acquisition");
#ifdef SHEILUN_SOURCE_BASELINE
        {
            PinnedAutomaticDeath automatic(&boss);
            automatic.JustDied(killer);
            AdvanceSource(*controller,player,2010);
            Require(InventoryArtifact(&player) && Receipt(progress,46717) && !controller->State().manualOpen &&
                controller->State().staff.IsEmpty() && controller->State().proxy.IsEmpty(),"Pinned death body did not reproduce its pre-interaction artifact/48646 grant");
            controller->OnPlayerLeave(&player); map.RemoveAllObjectsInRemoveList();
            std::cout<<"CASE EXPECTED_BROKEN pinned ab99b2d2d native death body race="<<unsigned(race)<<" killer="<<unsigned(killerKind)<<" artifact+48646 before any manual source\n";
            ++cases;
            goto SourceCaseDone;
        }
#endif
        AdvanceSource(*controller,player,6000);
        auto staff=map.GetCreature(controller->State().staff);
        auto proxy=map.GetGameObject(controller->State().proxy);
        Require(staff && proxy && staff->GetDisplayId()==staff->GetCreatureTemplate()->GetFirstInvisibleModel(), "Native personal source/model creation failed");
        Require(staff->HasFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_SPELLCLICK) && !staff->HasFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NOT_SELECTABLE), "Invisible model lost native interaction");
        Require(staff->HasQuestObjectScope() && proxy->HasQuestObjectScope(), "Source weak quest scopes missing");
        std::cout<<"SOURCE_ADMISSION present="<<controller->Present(&player)<<" source="<<controller->Source(&player,staff,StaffEntry)
            <<" at="<<At(&player,staff)<<" phase="<<player.InSamePhase(staff)<<" visible="<<player.canSeeOrDetect(staff)
            <<" los="<<player.IsWithinLOSInMap(staff)<<" range="<<player.IsWithinDistInMap(staff,INTERACTION_DISTANCE)
            <<" scope="<<player.CanTargetQuestObject(staff)<<" castTime="<<sSpellMgr->GetSpellInfo(202435)->CalcCastTime()<<std::endl;
        Require(sScriptMgr->GetSpellScriptLoader(sObjectMgr->GetScriptId("spell_sheilun_manual_claim")),"Native claim script registration missing");
        ObjectGuid initialStaff=staff->GetGUID(),initialProxy=proxy->GetGUID(),item;
        // Exercise both real native entry points. NPC spellclick must suppress
        // every early progression effect; the GO delegate opens the same loot.
        if(killerKind==1 || existing) proxy->Use(&player); else staff->HandleSpellClick(&player);
        AdvanceSource(*controller,player,std::max(100u,uint32(sSpellMgr->GetSpellInfo(202435)->CalcCastTime())+20u));
        std::cout<<"SOURCE_OPEN manual="<<controller->State().manualOpen<<" stored="<<controller->State().stored
            <<" claimed="<<controller->State().claimed<<" item="<<bool(InventoryArtifact(&player))
            <<" criterion="<<Receipt(progress,46717)<<" scene="<<player.HasSceneStatus(1192,SCENE_LAUNCH)<<std::endl;
        if (!controller->State().manualOpen && !existing)
        {
            auto result=player.CastSpell(staff,202435,true);
            std::cout<<"SOURCE_DIAGNOSTIC triggeredCast="<<uint32(result)<<" proxyAt="<<At(&player,proxy)
                <<" directOpen="<<controller->Open(&player,staff)<<" expectedLoot="<<bool(controller->State().loot)<<std::endl;
            throw std::runtime_error("Native spellclick did not open its owned source; diagnostic direct invocation is not a passing substitute");
        }
        if(existing)
        {
            AdvanceSource(*controller,player,100);
            Require(controller->State().existingInventory && controller->State().claimed && controller->State().manualReceipt &&
                Receipt(progress,46717) && InventoryArtifact(&player)->GetGUID()==existingItem && player.GetItemCount(128937)==1 &&
                !map.GetCreature(initialStaff) && !map.GetGameObject(initialProxy),
                "Owned inventory recovery did not require manual source or duplicated the artifact");
            item=existingItem;
        }
        else
        {
        Require(controller->State().manualOpen,"Native spellclick/GO did not open its owned source");
        Require(!controller->State().stored && !controller->State().claimed && !InventoryArtifact(&player) &&
            !Receipt(progress,46717) && !player.HasSceneStatus(1192,SCENE_LAUNCH), "Opening source awarded acquisition");
        Loot* loot=proxy->IsPersonal()?player.GetPersonalLoot(proxy->GetGUID()):&proxy->loot;
        Require(loot && loot->GetGUID()==controller->State().loot, "Native loot witness missing");
        uint8 slot=255;
        for(uint8 index=0;index<loot->items.size();++index) if(loot->items[index].item.ItemID==128937) slot=index;
        Require(slot!=255,"Actual artifact loot missing");
        if(fullBags)
        {
            for(uint8 index=INVENTORY_SLOT_ITEM_START;index<player.GetInventoryEndSlot();++index)
                if(!player.GetItemByPos(INVENTORY_SLOT_BAG_0,index))
                {
                    ItemPosCountVec dest;
                    Require(player.CanStoreNewItem(INVENTORY_SLOT_BAG_0,index,dest,2589,200)==EQUIP_ERR_OK &&
                        player.StoreNewItem(dest,2589,true)!=nullptr,"Native full-bag prerequisite failed");
                }
            ItemPosCountVec dest;
            Require(player.CanStoreNewItem(NULL_BAG,NULL_SLOT,dest,128937,1)==EQUIP_ERR_INV_FULL,"Native artifact inventory is not full");
            AutoStore(player,*loot,slot);
            Require(!controller->State().stored && !loot->items[slot].is_looted && !InventoryArtifact(&player), "Full bags consumed source/claimed artifact");
            player.DestroyItem(INVENTORY_SLOT_BAG_0,INVENTORY_SLOT_ITEM_START,true);
        }
        AutoStore(player,*loot,slot);
        Require(loot->items[slot].is_looted && controller->State().stored && InventoryArtifact(&player) && !controller->State().claimed,
            "Native packet/storage callback did not acknowledge original source");
        ObjectGuid lootGuid=loot->GetGUID(),staffGuid=staff->GetGUID(),proxyGuid=proxy->GetGUID();
        AdvanceSource(*controller,player,100);
        Require(controller->State().claimed && controller->State().sceneLaunched && Receipt(progress,46717) && player.HasSceneStatus(1192,SCENE_LAUNCH),
            "Guarded native delayed claim/scene failed");
        item=InventoryArtifact(&player)->GetGUID();
        Require(controller->State().manualReceipt && !map.GetCreature(staffGuid) && !map.GetGameObject(proxyGuid),"Spent physical source was not retired after native scene launch");
        AutoStoreGuid(player,lootGuid,slot);
        AdvanceSource(*controller,player,100);
        Require(InventoryArtifact(&player)->GetGUID()==item && controller->State().claimed, "Repeated source changed actual artifact");
        }
        // These killer fixtures are uniquely owned by this scope. A real map
        // removal unsummons/deletes registered guardians; retire our fixtures
        // before travel rather than retaining a second deleting owner. Pet
        // persistence/portrait across travel is a separate client regression.
        guardian.reset(); pet.reset();
        bool resumed=race==RACE_HUMAN && killerKind==0 && !existing;
        if(resumed) CheckSourceRelog(*controller,player);
        if(killerKind==0 || killerKind==3 || killerKind==2)
            CheckReturn(*controller,player,killerKind==3?(race==RACE_HUMAN?1:2):(killerKind==2?(race==RACE_HUMAN?4:3):0),resumed);
        if(player.IsInWorld() && player.GetMap()==&map) controller->OnPlayerLeave(&player);
        map.RemoveAllObjectsInRemoveList();
        Require(!map.GetCreature(controller->State().staff) && !map.GetGameObject(controller->State().proxy), "Owned source cleanup failed");
        std::cout<<"CASE native manual source race="<<unsigned(race)<<" killer="<<unsigned(killerKind)<<" fullBags="<<fullBags
            <<" existing="<<existing<<" actual interaction+owned item+delayedScene+cleanup PASS\n";
        ++cases;
    }
#ifdef SHEILUN_SOURCE_BASELINE
SourceCaseDone:
#endif
    session->SetPlayer(nullptr); sScenarioMgr->RemoveScenario(map.GetInstanceId());
}

static int AcquisitionRun(int argc,char** argv)
{
    InitializeSheilun(argc,argv);
    uint32 number=0;
    for(uint8 race:{uint8(RACE_HUMAN),uint8(RACE_ORC)})
        for(uint8 killer=0;killer<4;++killer) CheckSource(race,killer,killer==0,++number);
#ifndef SHEILUN_SOURCE_BASELINE
    for(uint8 race:{uint8(RACE_HUMAN),uint8(RACE_ORC)}) CheckSource(race,0,false,++number,true);
#endif
    CharacterDatabase.Close(); LoginDatabase.Close(); HotfixDatabase.Close(); WorldDatabase.Close();
#ifdef SHEILUN_SOURCE_BASELINE
    std::cout<<"EXPECTED_BROKEN "<<cases<<" pinned actual automatic death-body cases on native current prerequisites/core; native current claim guards remain installed,14s teleport not executed. No ordinary passing-run claim.\n";
#else
    Require(cases==10 && returnCases==8 && resumeCases==1 && saveCases==2,"Native source/return/resume/save matrix was incomplete");
    std::cout<<"PASS "<<cases<<" native source cases, "<<returnCases<<" actual two-map return/boarding/spline/cancel cases, "<<resumeCases
        <<" completed-outro rejoin and "<<saveCases<<" before-logout memory-save cases; earlier stages/transient reload boundaries seeded, no character saves or full client/combat/camera/pet-portrait claim.\n";
#endif
    return 0;
}
int main(int argc,char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX); SetUnhandledExceptionFilter(CrashTrace);
    try{return AcquisitionRun(argc,argv);}catch(std::exception const& error){std::cerr<<error.what()<<'\n';return 1;}
}
