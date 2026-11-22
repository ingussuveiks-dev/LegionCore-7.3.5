#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "CreatureTextMgr.h"
#include "GossipData.h"
#include "SmartAI.h"
#include "SmartScriptMgr.h"
#include "Vehicle.h"

void AddSC_the_barrens();
void AddSC_SmartScripts();
void AddSC_boss_the_lich_king();

namespace
{
constexpr uint32 Delivery = 13975;
constexpr uint32 Credit = 65892;
constexpr uint32 Sweeper = 46714;
constexpr uint32 RoccoEntry = 34578;
constexpr uint32 KodoEntry = 34577;
constexpr uint32 PackEntry = 34432;
constexpr uint32 RiderSeat = 0;

void Shadow(char const* table, char const* filter)
{
    auto create = Trinity::StringFormat("CREATE TEMPORARY TABLE caravan_probe_%s AS SELECT * FROM %s WHERE %s", table, table, filter);
    auto binding = Trinity::StringFormat("CREATE TEMPORARY TABLE %s AS SELECT * FROM caravan_probe_%s", table, table);
    WorldDatabase.DirectExecute(create.c_str());
    WorldDatabase.DirectExecute(binding.c_str());
    Require(bool(WorldDatabase.PQuery("SHOW CREATE TABLE %s", table)), "Could not scope the quest fixture table");
}

Quest const* LoadDelivery()
{
    auto row = WorldDatabase.Query(
#include "NativeCaravanDelivery.inc"
    );
    Require(bool(row), "Missing Crossroads Caravan Delivery template");
    auto quest = new Quest(row->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    quests[Delivery] = quest;
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(Delivery, quest);
    auto objectives = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=13975 ORDER BY ID");
    Require(objectives && objectives->GetRowCount() == 2, "Missing original credit and sweeper slots");
    for (auto entry : {Credit, Sweeper})
    {
        auto fields = objectives->Fetch();
        auto type = entry == Credit ? QUEST_OBJECTIVE_MONSTER : QUEST_OBJECTIVE_ITEM;
        Require(fields[2].GetUInt8() == type && fields[4].GetUInt32() == entry && fields[5].GetUInt32() == 1,
            "Caravan credit and quest item must have their native objective types");
        quest->LoadQuestObjective(fields);
        objectives->NextRow();
    }
    auto addon = WorldDatabase.Query(
        "SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,RewardMailTemplateID,RewardMailDelay,"
        "RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMinRepValue,RequiredMaxRepFaction,RequiredMaxRepValue,"
        "ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=13975");
    Require(bool(addon), "Missing caravan inventory and event flags");
    quest->LoadQuestTemplateAddon(addon->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST | QUEST_SPECIAL_FLAGS_DELIVER);
    Require(quest->SourceItemId == Sweeper && quest->SourceItemIdCount == 1 &&
        quest->HasSpecialFlag(QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT), "Caravan's original item and event metadata changed");
    Require(bool(WorldDatabase.Query("SELECT 1 FROM creature c JOIN creature_questender q ON q.id=c.id "
        "WHERE q.quest=13975 AND c.id=3429 AND c.map=1")), "Thork has no original receiver placement");
    return quest;
}

void LoadCaravanActors()
{
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadPlayerInfo();
    sCreatureTextMgr->LoadCreatureTexts();
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    AddSC_SmartScripts();
    AddSC_the_barrens();
    AddSC_boss_the_lich_king();
    sScriptMgr->SwapScriptContext(true);
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    Shadow("gossip_menu_option", "MenuID=34578 AND OptionID=0");
    Shadow("conditions", "(SourceTypeOrReferenceId=15 AND SourceGroup=34578 AND SourceEntry=0) OR "
        "(SourceTypeOrReferenceId=18 AND SourceGroup=34577 AND SourceEntry=46598)");
    Shadow("smart_scripts", "source_type=0 AND entryorguid IN(34431,34577,34578)");
    Shadow("npc_spellclick_spells", "npc_entry=34577");
    Shadow("waypoints", "entry=34577");
    Shadow("spell_target_position", "id=65662");
    Shadow("spell_script_names", "spell_id IN(68576,96791,96792)");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sGossipDataStore->LoadGossipMenuItems();
    sObjectMgr->LoadNPCSpellClickSpells();
    sSpellMgr->LoadSpellTargetPositions();
    sScriptDataStore->LoadSpellScriptNames();
    sScriptDataStore->ValidateSpellScripts();
    sSmartWaypointMgr->LoadFromDB();
    sSmartScriptMgr->LoadSmartAIFromDB();
    sConditionMgr->LoadConditions();
}

struct Rocco : Actor
{
    using Actor::Actor;

    void AddToWorld() override
    {
        sObjectAccessor->AddObject(static_cast<Creature*>(this));
        Unit::AddToWorld();
        auto ai = sScriptMgr->GetCreatureAI(this);
        Require(ai && AIM_Initialize(ai), "Rocco's linked SmartAI delegate is missing");
    }
};

Creature* KodoFor(LootPlayer& player)
{
    auto guids = player.GetSummonList(KodoEntry);
    Require(guids->size() <= 1, "Duplicate native caravan kodo");
    return guids->empty() ? nullptr : player.GetMap()->GetCreature(guids->front());
}

void RelocateKodo(Creature& kodo, Position const& point)
{
    kodo.GetMap()->LoadGrid(point.GetPositionX(), point.GetPositionY());
    kodo.GetMap()->CreatureRelocation(&kodo, point.GetPositionX(), point.GetPositionY(), point.GetPositionZ(), point.GetOrientation());
    kodo.GetMap()->MoveAllCreaturesInMoveList();
}

void CheckDelivery(Quest const* quest)
{
    auto spawn = WorldDatabase.Query("SELECT guid,position_x,position_y,position_z,orientation FROM creature WHERE id=34578 AND map=1");
    Require(spawn && spawn->GetRowCount() == 1, "Missing original Rocco");
    auto data = spawn->Fetch();
    Position start{data[1].GetFloat(),data[2].GetFloat(),data[3].GetFloat(),data[4].GetFloat()};
    auto path = WorldDatabase.Query("SELECT pointid,position_x,position_y,position_z FROM waypoints WHERE entry=34577 ORDER BY pointid");
    Require(path && path->GetRowCount() == 24, "Native 24-point route changed");
    std::vector<Position> points;
    do
    {
        auto row = path->Fetch();
        Require(row[0].GetUInt32() == points.size() + 1, "Native route point order changed");
        points.emplace_back(row[1].GetFloat(),row[2].GetFloat(),row[3].GetFloat(),0.0f);
    } while (path->NextRow());
    auto summon = sSpellMgr->GetSpellInfo(65662);
    Require(summon && summon->GetEffect(EFFECT_0)->MiscValue == KodoEntry &&
        summon->GetEffect(EFFECT_0)->MiscValueB == 64, "Missing native delivery summon properties");
    Map map(1, 300, 0, DIFFICULTY_NONE);
    LootPlayer owner(SupplySession(), map, 13975, start);
    LootPlayer other(SupplySession(), map, 13976, start);
    for (auto player : {&owner, &other})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_ORC);
        player->setFactionForRace(RACE_ORC);
        player->InitDisplayIds();
        player->SetLevel(19);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
    }
    Rocco rocco(&map, RoccoEntry, data[0].GetUInt32(), start);
    Require(map.AddToMap(static_cast<Creature*>(&rocco)), "Could not publish the DB Rocco");
    auto choose = [&](LootPlayer& player) { rocco.AI()->sGossipSelect(&player, RoccoEntry, 0); };
    choose(owner);
    Require(!KodoFor(owner) && !owner.GetItemCount(Sweeper), "No-quest player received a ride or gun");
    for (auto player : {&owner, &other})
    {
        player->AddQuestAndCheckCompletion(quest, nullptr);
        Require(player->GetItemCount(Sweeper) == 1, "Native accept did not grant the sweeper");
    }
    other.SetQuestStatus(Delivery, QUEST_STATUS_COMPLETE);
    Require(!other.CanRewardQuest(quest, false), "Legacy COMPLETE without the delivery marker was rewardable");
    sScriptMgr->OnPlayerLogin(&other);
    Require(other.GetQuestStatus(Delivery) == QUEST_STATUS_INCOMPLETE, "Legacy COMPLETE could not resume delivery");

    choose(owner);
    choose(owner);
    choose(other);
    auto first = KodoFor(owner);
    auto second = KodoFor(other);
    Require(first && second && first != second && first->IsInPersonnalVisibilityList(owner.GetGUID()) &&
        !first->IsInPersonnalVisibilityList(other.GetGUID()) && second->IsInPersonnalVisibilityList(other.GetGUID()),
        "Native summoning did not produce two private vehicles");
    Require(first->GetVehicleKit() && first->GetVehicleKit()->HasEmptySeat(RiderSeat), "The native player seat is unavailable");
    Require(first->GetSummonList(PackEntry)->size() == 1, "The native pack kodo is missing");
    auto pack = map.GetCreature(first->GetSummonList(PackEntry)->front());
    Require(pack && pack->GetQuestObjectScope().lock() == first->GetQuestObjectScope().lock(), "Pack kodo did not inherit the quest");
    SupplyMove(owner, first->GetPosition());
    SupplyMove(other, first->GetPosition());
    Require(!first->HandleSpellClick(&other) && !static_cast<SmartAI*>(first->AI())->HasEscortState(SMART_ESCORT_ESCORTING),
        "Another active quest player's click boarded or started the private vehicle");
    Require(first->CastSpell(&rocco, 96791, true) != SPELL_CAST_OK, "The vacant vehicle accepted a shot");
    Require(first->HandleSpellClick(&owner), "The owner could not use the native Ride Vehicle action");
    owner.Unit::Update(1);
    first->Unit::Update(100);
    Require(first->GetVehicleKit()->GetPassenger(RiderSeat) == &owner && owner.GetVehicleBase() == first &&
        static_cast<SmartAI*>(first->AI())->HasEscortState(SMART_ESCORT_ESCORTING), "Actual native boarding did not start the path");
    Require(first->CastSpell(&rocco, 96791, true) != SPELL_CAST_OK, "The real rider could shoot Rocco");
    first->AI()->MovementInform(POINT_MOTION_TYPE, 23);
    Require(!owner.GetQuestObjectiveData(Delivery, Credit), "A wrong-location waypoint notification gave credit");
    for (uint32 i = 1; i <= 23; ++i)
    {
        RelocateKodo(*first, points[i - 1]);
        first->AI()->MovementInform(POINT_MOTION_TYPE, i);
    }
    Require(owner.GetQuestObjectiveData(Delivery, Credit) == 1 && owner.CanRewardQuest(quest, false) &&
        !other.GetQuestObjectiveData(Delivery, Credit), "Arrival failed to credit just the actual rider");
    RelocateKodo(*first, points.back());
    first->AI()->MovementInform(POINT_MOTION_TYPE, 24);
    first->Unit::Update(100);
    Tick(owner, 100);
    Require(!owner.GetVehicle(), "The native final waypoint did not eject the rider");
    SupplyMove(other, second->GetPosition());
    Require(second->HandleSpellClick(&other), "The second player could not board their own kodo");
    Tick(other, 100);
    Require(other.GetVehicleBase() == second, "The remaining kodo did not carry its real owner");
    ObjectGuid completed = first->GetGUID(), abandoned = second->GetGUID();
    owner.RewardQuest(quest, 0, nullptr, false);
    AbandonSupply(other, Delivery);
    map.RemoveAllObjectsInRemoveList();
    Require(!owner.GetItemCount(Sweeper) && !other.GetItemCount(Sweeper) && owner.GetQuestRewardStatus(Delivery),
        "Quest reward or abandonment left the borrowed gun in a bag");
    Require(!map.GetCreature(completed) && !map.GetCreature(abandoned) && !other.GetVehicle(),
        "Finished or abandoned delivery retained a private vehicle or its rider");
    std::cout << "PASS native summons, two owners, actual seat boarding and ejection, scoped arrival, legacy repair and item cleanup\n";
}
}

int main(int argc, char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    SetUnhandledExceptionFilter(CrashTrace);
    try
    {
        InitializeNative(argc, argv);
        opcodeTable.Initialize();
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1, {}}});
        LoadGameObjectModelList(argv[2]);
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "Native delivery requires the isolated character schema");
        auto quest = LoadDelivery();
        LoadCaravanActors();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckDelivery(quest);
        CharacterDatabase.Close();
        LoginDatabase.Close();
        HotfixDatabase.Close();
        WorldDatabase.Close();
        return 0;
    }
    catch (std::exception const& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
