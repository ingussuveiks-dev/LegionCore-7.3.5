#define main SupplyChestProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

namespace
{
constexpr uint32 OakenscowlQuest = 2499;
constexpr uint32 Oakenscowl = 2166;
constexpr uint32 Tumor = 8136;
constexpr uint32 Balloon = 71034;

struct ControlledLootPet : LootPet
{
    ControlledLootPet(LootPlayer& owner, Map& map) : LootPet(owner, map)
    {
        m_ControlledByPlayer = true;
    }
};

void LoadInputs()
{
    auto rows = WorldDatabase.Query(
#include "NativeLowLevelWorldSpawns.inc"
    );
    Require(rows && rows->GetRowCount() == 4, "Missing native quest templates");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new Quest(rows->Fetch());
        quests[quest->Id] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->Id, quest);
    } while (rows->NextRow());

    auto objectives = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID IN(2499,29401,29412)");
    Require(objectives && objectives->GetRowCount() == 3, "Missing native quest objectives");
    do
    {
        auto row = objectives->Fetch();
        auto quest = quests[row[1].GetUInt32()];
        auto item = quest->Id == OakenscowlQuest ? Tumor : Balloon;
        auto amount = quest->Id == OakenscowlQuest ? 1 : 5;
        Require(row[2].GetUInt8() == QUEST_OBJECTIVE_ITEM && row[4].GetUInt32() == item && row[5].GetInt32() == amount &&
            quest->ItemDrop[0] == item && quest->ItemDropQuantity[0] == uint32(amount), "Wrong loot requirement or cleanup metadata");
        quest->LoadQuestObjective(row);
        quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER);
    } while (objectives->NextRow());
    auto addon = WorldDatabase.Query("SELECT PrevQuestID FROM quest_template_addon WHERE ID=2499");
    Require(addon && addon->Fetch()[0].GetUInt32() == 2498, "Wrong Oakenscowl predecessor");
    quests[OakenscowlQuest]->prevQuests.push_back(2498);

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    sObjectMgr->LoadEquipmentTemplates();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    for (auto pair : {std::pair{&LootTemplates_Creature, " WHERE Entry=2166"},
        std::pair{&LootTemplates_Gameobject, " WHERE Entry IN(39846,209242)"}})
    {
        auto& table = (*pair.first).*Access(LootTableNameTag{});
        auto original = table;
        table += pair.second;
        auto loaded = ((*pair.first).*Access(LoadLootTableTag{}))();
        table = original;
        Require(loaded > 0, "Native quest loot could not be loaded");
    }

    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature "
        "WHERE guid IN(78058,264754,266158,3101073)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter "
        "WHERE quest IN(2499,29401,29412)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender "
        "WHERE quest IN(2499,29401,29412)");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sObjectMgr->LoadCreatures();
}

Creature& LoadActor(Map& map, uint32 entry, uint64 guid)
{
    auto row = sObjectMgr->GetCreatureData(guid);
    Require(row && row->id == entry, "Native creature placement not found");
    map.LoadGrid(row->posX, row->posY);
    auto actor = map.GetCreature(ObjectGuid::Create<HighGuid::Creature>(map.GetId(), entry, guid));
    Require(actor && actor->IsAlive(), "The real creature did not appear in the loaded grid");
    return *actor;
}

void Setup(LootPlayer& player, uint8 race)
{
    player.SetByteValue(UNIT_FIELD_BYTES_0, 0, race);
    player.setFactionForRace(race);
    player.InitDisplayIds();
    player.SetLevel(15);
    player.SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
}

void Accept(LootPlayer& player, Creature const& giver, uint32 quest)
{
    SupplyMove(player, giver.GetPosition());
    WorldPackets::Quest::QuestGiverAcceptQuest packet{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    packet.QuestGiverGUID = giver.GetGUID();
    packet.QuestID = quest;
    player.GetSession()->HandleQuestGiverAcceptQuest(packet);
    Require(player.GetQuestStatus(quest) == QUEST_STATUS_INCOMPLETE, "The actual giver did not offer the requested quest");
}

void CheckOakenscowl()
{
    Map map(1, 501, 0, DIFFICULTY_NONE);
    auto& giver = LoadActor(map, 2080, 78058);
    auto& target = LoadActor(map, Oakenscowl, 3101073);
    auto height = map.GetHeight(target.GetPositionX(), target.GetPositionY(), target.GetPositionZ() + 2.0f);
    Require(target.GetZoneId() == 141 && abs(height - target.GetPositionZ()) < 4.0f, "Oakenscowl is off the native Teldrassil ground");
    LootPlayer owner(SupplySession(), map, 249901, giver.GetPosition());
    LootPlayer outsider(SupplySession(), map, 249902, giver.GetPosition());
    Setup(owner, RACE_NIGHTELF);
    Setup(outsider, RACE_NIGHTELF);
    owner.SetRewardedQuest(2498);
    auto quest = sQuestDataStore->GetQuestTemplate(OakenscowlQuest);
    for (bool abandon : {true, false})
    {
        Accept(owner, giver, OakenscowlQuest);
        SupplyMove(owner, target.GetPosition());
        std::unique_ptr<ControlledLootPet> pet;
        if (!abandon)
            pet = std::make_unique<ControlledLootPet>(owner, map);
        for (uint32 roll = 0; roll < 12 && !owner.GetItemCount(Tumor); ++roll)
        {
            if (!target.IsAlive())
            {
                target.Respawn(true, 0);
                target.Respawn(false, 0);
                Require(target.IsAlive(), "Native respawn did not restore Oakenscowl for a separate kill");
            }
            Unit& killer = pet ? static_cast<Unit&>(*pet) : static_cast<Unit&>(owner);
            Require(killer.DealDamage(&target, 1000000, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false) &&
                target.isDead(false), "Oakenscowl did not die through the native combat path");
            WorldPackets::Loot::LootResponse authorized, unauthorized;
            target.loot.BuildLootResponse(authorized, &owner, ALL_PERMISSION);
            target.loot.BuildLootResponse(unauthorized, &outsider, ALL_PERMISSION);
            Require(std::none_of(unauthorized.Items.begin(), unauthorized.Items.end(), [](auto const& item)
                { return item.Loot.ItemID == Tumor; }), "The tumor leaked to a player without the quest");
            for (auto const& item : authorized.Items)
                if (item.Loot.ItemID == Tumor)
                    owner.StoreLootItem(item.LootListID - 1, &target.loot);
        }
        Require(owner.GetItemCount(Tumor) == 1 && owner.CanRewardQuest(quest, false), "The real tumor did not allow turn-in");
        if (abandon)
        {
            AbandonSupply(owner, OakenscowlQuest);
            Require(!owner.GetItemCount(Tumor), "Abandonment did not clear the quest-only tumor");
        }
    }
    std::cout << "PASS real Oakenscowl placement and ground, Denalan acceptance, player and pet kills, native loot, abandon and retry\n";
}

void CheckBalloons(uint32 mapId, uint32 questId, uint32 entry, uint32 city, uint32 giverEntry, uint64 giverGuid, uint8 race, uint64 expected)
{
    auto rows = WorldDatabase.PQuery("SELECT guid,spawntimesecs,position_x,position_y,position_z "
        "FROM gameobject WHERE id=%u AND map=%u AND zoneId=%u ORDER BY guid", entry, mapId, city);
    Require(rows && rows->GetRowCount() == expected, "City is missing its archived balloon placements");
    Map map(mapId, 501, 0, DIFFICULTY_NONE);
    auto& giver = LoadActor(map, giverEntry, giverGuid);
    LootPlayer owner(SupplySession(), map, questId * 100 + 1, giver.GetPosition());
    LootPlayer outsider(SupplySession(), map, questId * 100 + 2, giver.GetPosition());
    Setup(owner, race);
    Setup(outsider, race);
    Accept(owner, giver, questId);
    bool checkedOutsider = false;
    do
    {
        auto row = rows->Fetch();
        Position point{row[2].GetFloat(), row[3].GetFloat(), row[4].GetFloat(), 0};
        map.LoadGrid(point.GetPositionX(), point.GetPositionY());
        Supply stock(map, {entry, row[0].GetUInt32(), row[1].GetUInt32(), point, G3D::Quat(0, 0, 0, 1)});
        Require(stock.GetZoneId() == city, "A balloon is outside its intended city");
        if (!checkedOutsider)
        {
            checkedOutsider = true;
            SupplyMove(outsider, point);
            Require(!stock.ActivateToQuest(&outsider), "The balloon marked for a player without the quest");
            OpenSupply(outsider, stock);
            Require(!outsider.GetLootGUID() && stock.getLootState() == GO_READY,
                "A player without the quest opened a shared balloon");
            uint32 otherRoute = questId == 29401 ? 29412 : 29401;
            outsider.AddQuestAndCheckCompletion(sQuestDataStore->GetQuestTemplate(otherRoute), nullptr);
            Require(outsider.HasQuestForItem(Balloon) && !stock.ActivateToQuest(&outsider),
                "The other city's quest marked the wrong balloon");
            OpenSupply(outsider, stock);
            Require(!outsider.GetLootGUID() && stock.getLootState() == GO_READY,
                "The other city's quest consumed a shared balloon");
            AbandonSupply(outsider, otherRoute);
        }
        if (owner.GetItemCount(Balloon) < 5)
        {
            SupplyMove(owner, point);
            Require(stock.ActivateToQuest(&owner), "The balloon was not a quest target");
            OpenSupply(owner, stock);
            TakeSupply(owner, stock, Balloon);
            ReleaseSupply(owner, stock);
        }
    } while (rows->NextRow());
    Require(owner.GetItemCount(Balloon) == 5 && owner.CanRewardQuest(sQuestDataStore->GetQuestTemplate(questId), false),
        "Native balloon collection did not allow turn-in");
    AbandonSupply(owner, questId);
    Require(!owner.GetItemCount(Balloon), "Abandonment did not clear the quest-only balloons");
    std::cout << "PASS quest " << questId << " native giver, " << expected << " city balloon placements, five native loot pickups and abandonment\n";
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{0, {}}, {1, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{0, {}}, {1, {}}});
        LoadGameObjectModelList(argv[2]);
        auto login = sConfigMgr->GetStringDefault("LoginDatabaseInfo", "");
        auto characters = sConfigMgr->GetStringDefault("CharacterDatabaseInfo", "");
        Require(login.substr(login.find_last_of(';') + 1) == "fixture_auth" &&
            characters.substr(characters.find_last_of(';') + 1) == "fixture_char", "The native quest probe requires disposable login and character schemas");
        LoginDatabase.SetConnectionInfo(login, 1, 1);
        CharacterDatabase.SetConnectionInfo(characters, 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        LoadInputs();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<Pet>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckOakenscowl();
        CheckBalloons(1, 29401, 209058, 1637, 54004, 264754, RACE_ORC, 26);
        CheckBalloons(0, 29412, 209242, 1519, 54117, 266158, RACE_HUMAN, 12);
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
