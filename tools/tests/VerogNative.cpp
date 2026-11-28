#define LOST_ARMY_NATIVE_SUPPORT_ONLY
#include "LostArmyLootNative.cpp"
#include "SmartScriptMgr.h"

namespace
{
constexpr uint32 VerogQuest = 851;
constexpr uint32 Verog = 3395;
constexpr uint32 Head = 5023;
constexpr uint32 WynethSpawn = 260742;
constexpr uint32 KolkarSpawns[] = {87702, 86939, 87649};

Quest const* LoadVerog()
{
    auto row = WorldDatabase.Query(
#include "NativeVerog.inc"
    );
    Require(row && row->GetRowCount() == 1, "Missing the native Verog quest");
    auto quest = new Quest(row->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    quests[VerogQuest] = quest;
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(VerogQuest, quest);
    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=851");
    Require(objective && objective->GetRowCount() == 1 && objective->Fetch()[2].GetUInt8() == QUEST_OBJECTIVE_ITEM &&
        objective->Fetch()[4].GetUInt32() == Head && objective->Fetch()[5].GetInt32() == 1, "Missing the native Verog head objective");
    quest->LoadQuestObjective(objective->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER);
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadPlayerInfo();
    sSpellMgr->LoadSpellBonusess();
    sSpellMgr->LoadSpellThreats();
    auto& table = LootTemplates_Creature.*Access(LootTableNameTag{});
    auto original = table;
    table += " WHERE Entry IN(3274,3275,3397,3395,34846)";
    Require((LootTemplates_Creature.*Access(LoadLootTableTag{}))() > 0, "Could not load original Kolkar loot data");
    table = original;

    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature WHERE guid IN(87702,86939,87649,260742)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE smart_scripts AS SELECT * FROM legion_world.smart_scripts "
        "WHERE source_type=0 AND entryorguid IN(3274,3275,3397,3395,34846)");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    auto role = WorldDatabase.Query("SELECT COUNT(*) FROM smart_scripts WHERE event_type=6 AND "
        "((entryorguid=3275 AND id=1 AND link=2) OR (action_type=12 AND action_param1=3395 AND entryorguid IN(3274,3397)))");
    Require(role && role->Fetch()[0].GetUInt32() == 0, "Legacy Kolkar death behavior still reaches Verog");
    sObjectMgr->LoadCreatures();
    sSmartScriptMgr->LoadSmartAIFromDB();
    return quest;
}

struct KolkarActor : Creature
{
    KolkarActor(Map& map, uint32 guid) : Creature(false)
    {
        auto spawn = sObjectMgr->GetCreatureData(guid);
        Require(spawn, "Missing the original Kolkar placement");
        map.LoadGrid(spawn->posX, spawn->posY);
        Require(LoadCreatureFromDB(guid, &map, true) && IsInWorld() && AI(), "Original Kolkar AI did not load");
        Require(IsAlive(), "Original Kolkar actor did not start alive");
    }

    ~KolkarActor()
    {
        CombatStop(true);
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<Creature*>(this), false);
        ResetMap();
    }
};

void FatalHit(Unit& killer, LootPlayer& player, Creature& victim)
{
    victim.LowerPlayerDamageReq(uint32(victim.GetMaxHealth()));
    victim.SetLootRecipient(&player);
    victim.GetSaveThreatList()->insert(player.GetGUID());
    killer.Kill(&victim, false);
    Require(victim.isDead(false), "A native fatal hit did not finish the creature");
    Tick(player, 100);
}

void MoveTo(LootPlayer& player, Position const& pos)
{
    player.GetMap()->PlayerRelocation(&player, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation());
}

bool CheckVerog(Quest const* quest, bool usePet)
{
    for (uint32 guid : {87702u, 86939u, 87649u, WynethSpawn})
        if (auto data = sObjectMgr->GetCreatureData(guid))
            sObjectMgr->RemoveCreatureFromGrid(guid, data);

    Map map(1, 300, 0, DIFFICULTY_NONE);
    Position spawnPosition{-1207.03f, -2753.84f, 100.44f, 0.f};
    map.LoadGrid(spawnPosition.GetPositionX(), spawnPosition.GetPositionY());
    auto session = new WorldSession(0, "Verog native probe", {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {});
    LootPlayer player(session, map, 851001, spawnPosition);
    player.SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_ORC);
    player.SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_HUNTER);
    player.setFactionForRace(RACE_ORC);
    player.InitDisplayIds();
    player.SetLevel(18);
    for (uint16 field : {UNIT_FIELD_MOD_TIME_RATE, UNIT_FIELD_MOD_CASTING_SPEED, UNIT_FIELD_MOD_SPELL_HASTE, UNIT_FIELD_MOD_HASTE, UNIT_FIELD_MOD_HASTE_REGEN})
        player.SetFloatValue(field, 1.0f);
    player.AddQuestAndCheckCompletion(quest, nullptr);

    for (uint32 guid : KolkarSpawns)
    {
        KolkarActor actor(map, guid);
        MoveTo(player, actor.GetNearPosition(1.f, 0.f));
        FatalHit(player, player, actor);
        Require(!actor.FindNearestCreature(Verog, 15.f, true), "An ordinary Kolkar death summoned Verog");
    }

    MoveTo(player, spawnPosition);
    LootPet pet(player, map);
    Unit& killer = usePet ? static_cast<Unit&>(pet) : static_cast<Unit&>(player);
    KolkarActor wyneth(map, WynethSpawn);
    MoveTo(player, wyneth.GetNearPosition(1.f, 0.f));
    FatalHit(killer, player, wyneth);
    Creature* verog = wyneth.FindNearestCreature(Verog, 45.f, true);
    Require(verog && verog->ToTempSummon() && verog->ToTempSummon()->GetSummonerGUID() == wyneth.GetGUID() &&
        verog->GetExactDist(&spawnPosition) < 1.f, "The native Wyneth death did not publish the authored Verog summon");
    MoveTo(player, verog->GetNearPosition(1.f, 0.f));
    FatalHit(killer, player, *verog);

    // In the outdoor personal loot path, quest items belong to the shared corpse.
    Loot& loot = verog->loot;
    WorldPackets::Loot::LootResponse packet;
    loot.BuildLootResponse(packet, &player, ALL_PERMISSION);
    bool storedHead = false;
    for (auto const& item : packet.Items)
        if (item.Loot.ItemID == Head)
        {
            player.StoreLootItem(item.LootListID - 1, &loot);
            storedHead = player.GetItemCount(Head) == 1;
            break;
        }
    verog->DespawnOrUnsummon();
    map.RemoveAllObjectsInRemoveList();
    if (storedHead)
        Require(player.GetQuestStatus(VerogQuest) == QUEST_STATUS_COMPLETE && player.CanRewardQuest(quest, false),
            "The real Verog kill and loot did not complete the quest");
    return storedHead;
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
        auto login = sConfigMgr->GetStringDefault("LoginDatabaseInfo", "");
        Require(login.substr(login.find_last_of(';') + 1) == "fixture_auth", "Expected a disposable realm");
        LoginDatabase.SetConnectionInfo(login, 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1, {}}});
        auto quest = LoadVerog();
        HashMapHolder<Player>::SetSize(32);
        GameTime::UpdateGameTimers();
        for (bool usePet : {false, true})
        {
            bool storedHead = false;
            for (unsigned i = 0; i < 12 && !storedHead; ++i)
                storedHead = CheckVerog(quest, usePet);
            Require(storedHead, "Native Verog kill did not provide the quest item in twelve trials");
        }
        std::cout << "PASS ordinary Kolkar deaths, real Wyneth summons, player and pet Verog kills and head inventory receipt\n";
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
