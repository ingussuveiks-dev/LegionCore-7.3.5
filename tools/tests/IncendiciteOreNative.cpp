#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "GameObjectPackets.h"

namespace
{
constexpr uint32 OreQuest = 25735;
constexpr uint32 FenbushBerries = 25725;
constexpr uint32 Vein = 203188;
constexpr uint32 Pick = 55240;
constexpr uint32 Ore = 55241;
constexpr uint32 Mine = 77020;

Quest const* LoadIncendiciteInputs()
{
    auto rows = WorldDatabase.Query(
#include "NativeIncendiciteOre.inc"
    );
    Require(rows && rows->GetRowCount() == 2, "The actual Ore quest and its prerequisite are missing");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new Quest(rows->Fetch());
        quests[quest->GetQuestId()] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->GetQuestId(), quest);
    } while (rows->NextRow());
    auto quest = quests[OreQuest];
    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=25735");
    Require(objective && objective->GetRowCount() == 1 && objective->Fetch()[2].GetUInt8() == QUEST_OBJECTIVE_ITEM &&
        objective->Fetch()[4].GetUInt32() == Ore && objective->Fetch()[5].GetUInt32() == 8, "Wrong actual Incendicite Ore objective");
    quest->LoadQuestObjective(objective->Fetch());
    auto addon = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,"
        "RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,"
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=25735");
    Require(addon && addon->Fetch()[4].GetUInt32() == FenbushBerries && !addon->Fetch()[9].GetUInt32() && quest->SourceItemId == Pick,
        "The original prerequisite, Mining admission or provided Pick differs");
    quest->LoadQuestTemplateAddon(addon->Fetch());
    quest->prevQuests.push_back(quest->PrevQuestID);
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER);
    Require(quest->PackageID == 2400 && quest->MinLevel == 18, "The original reward package or entry level changed");

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    auto item = sObjectMgr->GetItemTemplate(Pick);
    Require(item && std::any_of(item->Effects.begin(), item->Effects.end(), [](ItemEffectEntry const* effect)
        { return effect && effect->SpellID == Mine && !effect->Charges; }), "Expected the real reusable Pick spell");
    auto lock = sLockStore.LookupEntry(1896);
    Require(lock && lock->Type[0] == LOCK_KEY_ITEM && lock->Index[0] == Pick && lock->Type[1] == LOCK_KEY_SPELL && lock->Index[1] == Mine,
        "Wrong native Incendicite lock metadata");
    auto& table = LootTemplates_Gameobject.*Access(LootTableNameTag{});
    auto original = table;
    table += " WHERE Entry=203188";
    auto rowsLoaded = (LootTemplates_Gameobject.*Access(LoadLootTableTag{}))();
    table = original;
    Require(rowsLoaded == 1, "Expected the exact native ore loot table");
    sQuestDataStore->LoadGameObjectForQuests();

    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest=25735");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest=25735");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    return quest;
}

Position ReadPosition(Field* row)
{
    return {row[0].GetFloat(), row[1].GetFloat(), row[2].GetFloat(), row[3].GetFloat()};
}

void AcceptOre(LootPlayer& player, Creature& dunlor)
{
    SupplyMove(player, dunlor.GetPosition());
    WorldPackets::Quest::QuestGiverAcceptQuest accept{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    accept.QuestID = OreQuest;
    accept.QuestGiverGUID = dunlor.GetGUID();
    player.GetSession()->HandleQuestGiverAcceptQuest(accept);
}

void ClickVein(LootPlayer& player, GameObject const& vein)
{
    WorldPacket wire(CMSG_GAME_OBJ_USE);
    wire << vein.GetGUID();
    WorldPackets::GameObject::GameObjectUse packet(std::move(wire));
    packet.Read();
    player.GetSession()->HandleGameObjectUse(packet);
    Tick(player, 6500);
}

void CheckIncendicite(Quest const* quest)
{
    auto npc = WorldDatabase.Query("SELECT position_x,position_y,position_z,orientation FROM creature WHERE id=41128 AND guid=119136 AND map=0 AND zoneId=11");
    auto rows = WorldDatabase.Query("SELECT guid,spawntimesecs,position_x,position_y,position_z,orientation,rotation0,rotation1,rotation2,rotation3 "
        "FROM gameobject WHERE id=203188 AND map=0 AND zoneId=11 ORDER BY guid");
    Require(npc && rows && rows->GetRowCount() == 12, "The actual Dunlor or 12 Thelgen Ore placements are missing");
    std::vector<Placement> positions;
    do
    {
        auto row = rows->Fetch();
        positions.push_back({Vein, row[0].GetUInt32(), row[1].GetUInt32(), ReadPosition(row + 2),
            {row[6].GetFloat(), row[7].GetFloat(), row[8].GetFloat(), row[9].GetFloat()}});
    } while (rows->NextRow());

    Map map(0, 591, 0, DIFFICULTY_NONE);
    auto origin = ReadPosition(npc->Fetch());
    map.LoadGrid(origin.GetPositionX(), origin.GetPositionY());
    Actor dunlor(&map, 41128, 119136, origin);
    Require(map.AddToMap(static_cast<Creature*>(&dunlor)), "Could not publish the actual Dunlor");
    LootPlayer owner(SupplySession(), map, 2573501, origin), other(SupplySession(), map, 2573502, origin);
    for (auto player : {&owner, &other})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_HUMAN);
        player->setFactionForRace(RACE_HUMAN);
        player->InitDisplayIds();
        player->SetLevel(20);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
    }
    AcceptOre(owner, dunlor);
    Require(owner.GetQuestStatus(OreQuest) == QUEST_STATUS_NONE && !owner.GetItemCount(Pick), "Dunlor bypassed the required Fenbush Berries");
    owner.SetRewardedQuest(FenbushBerries);
    AcceptOre(owner, dunlor);
    Require(owner.GetQuestStatus(OreQuest) == QUEST_STATUS_INCOMPLETE && owner.GetItemCount(Pick) == 1 && !owner.HasSkill(SKILL_MINING),
        "The native quest did not give the Pick to an eligible non-miner");

    {
        auto const& placement = positions.front();
        map.LoadGrid(placement.position.GetPositionX(), placement.position.GetPositionY());
        Supply supply(map, placement);
        SupplyMove(other, supply.GetPosition());
        other.SendLoot(supply.GetGUID(), LOOT_SKINNING);
#ifdef INCENDICITE_ORE_BASELINE
        Require(other.GetLootGUID() == supply.GetGUID(), "Original direct loot window for an inactive player was not reproduced");
        ReleaseSupply(other, supply);
#else
        Require(!other.GetLootGUID() && supply.getLootState() == GO_READY && !supply.ActivateToQuest(&other),
            "An inactive client could open a quest mining source");
        GiveSupply(other, Pick, 1);
        Require(other.CastSpell(&supply, Mine, false, other.GetItemByEntry(Pick)) != SPELL_CAST_OK && other.GetItemCount(Pick) == 1,
            "A stale Pick activated an unaccepted quest source");
#endif
    }

#ifndef INCENDICITE_ORE_BASELINE
    {
        Placement legacy = positions.front();
        legacy.entry = 1610;
        legacy.guid = 835735;
        Supply lookalike(map, legacy);
        SupplyMove(owner, lookalike.GetPosition());
        Require(owner.CastSpell(&lookalike, Mine, false, owner.GetItemByEntry(Pick)) != SPELL_CAST_OK && !owner.GetItemCount(Ore),
            "The provided Pick activated the old same-name mineral template");
    }
#endif

    auto gather = [&](uint32 count)
    {
        for (auto const& placement : positions)
        {
            map.LoadGrid(placement.position.GetPositionX(), placement.position.GetPositionY());
            Supply supply(map, placement);
            SupplyMove(owner, supply.GetPosition());
            if (owner.GetItemCount(Ore) == count)
            {
                if (count == 8)
                    Require(!supply.ActivateToQuest(&owner), "A vein stayed selectable after collecting all eight ores");
                continue;
            }
            Require(supply.ActivateToQuest(&owner) && !supply.ActivateToQuest(&other), "A real vein did not announce its quest guard");
#ifdef INCENDICITE_ORE_BASELINE
            Require(owner.CastSpell(&supply, Mine, false, owner.GetItemByEntry(Pick)) == SPELL_CAST_OK, "Original native mining item could not start");
            Tick(owner, 6500);
#else
            ClickVein(owner, supply);
#endif
            TakeSupply(owner, supply, Ore);
            ReleaseSupply(owner, supply);
            Require(owner.GetItemCount(Pick) == 1, "An eligible mining cast consumed the reusable Pick");
        }
        Require(owner.GetItemCount(Ore) == count, "Actual vein loot failed to provide the required ore count");
    };

    gather(3);
    AbandonSupply(owner, OreQuest);
#ifdef INCENDICITE_ORE_BASELINE
    Require(owner.GetItemCount(Ore) == 3 && !owner.GetItemCount(Pick), "Original abandonment no longer reproduces the leftover quest ore");
    std::cout << "EXPECTED_BROKEN original raw opening admitted an outsider and abandonment left three quest ores\n";
#else
    Require(!owner.GetItemCount(Ore) && !owner.GetItemCount(Pick), "Abandonment retained ore or Pick");
    AcceptOre(owner, dunlor);
    Require(owner.GetItemCount(Pick) == 1, "A real retry failed to provide the Pick");
    gather(8);
    Require(owner.GetQuestStatus(OreQuest) == QUEST_STATUS_COMPLETE, "Eight actual ores did not complete the objective");
    auto choose = sDB2Manager.GetQuestPackageItems(quest->PackageID);
    uint32 choice = 0;
    if (choose)
        for (auto item : *choose)
            if (owner.CanSelectQuestPackageItem(item))
            {
                choice = item->ItemID;
                break;
            }
    if (!choice)
        if (auto fallback = sDB2Manager.GetQuestPackageItemsFallback(quest->PackageID); fallback && !fallback->empty())
            choice = fallback->front()->ItemID;
    SupplyMove(owner, origin);
    Require(choice && owner.CanRewardQuest(quest, choice, false), "The original class reward choice was unavailable");
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestID = OreQuest;
    reward.QuestGiverGUID = dunlor.GetGUID();
    reward.ItemChoiceID = choice;
    owner.GetSession()->HandleQuestGiverChooseReward(reward);
    Require(owner.GetQuestRewardStatus(OreQuest) && !owner.GetItemCount(Ore) && !owner.GetItemCount(Pick), "Dunlor did not remove eight ores and the loaned Pick");
    std::cout << "PASS Thelgen quest eligibility, 12 placed veins, eight genuine native mining loots, partial abandonment, retry and Dunlor reward\n";
#endif
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{0, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{0, {}}});
        LoadGameObjectModelList(argv[2]);
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "Incendicite probes require isolated characters");
        auto quest = LoadIncendiciteInputs();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckIncendicite(quest);
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
