#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "GameObjectPackets.h"
#include "GilneasQuestNative.h"

namespace
{
constexpr uint32 QuestId = 25937;
constexpr uint32 Drugan = 50631;
constexpr uint32 DruganSpawn = 82854;
constexpr uint32 ArtifactChest = 203385;
constexpr uint32 CopperVein = 1731;
constexpr uint32 Pick = 56226;
constexpr uint32 Artifact = 56225;
constexpr uint32 Dig = 77821;

Quest const* LoadTreasureInputs()
{
    auto row = WorldDatabase.Query(
#include "NativePricelessTreasures.inc"
    );
    Require(row && row->GetRowCount() == 1, "The actual quarry quest is missing");
    auto quest = new Quest(row->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    quests[QuestId] = quest;
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(QuestId, quest);
    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=25937");
    Require(objective && objective->GetRowCount() == 1 && objective->Fetch()[2].GetUInt8() == QUEST_OBJECTIVE_ITEM &&
        objective->Fetch()[4].GetUInt32() == Artifact && objective->Fetch()[5].GetUInt32() == 8, "Wrong native Frozen Artifact objective");
    quest->LoadQuestObjective(objective->Fetch());
    auto addon = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,"
        "RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,"
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=25937");
    Require(addon && !addon->Fetch()[9].GetUInt32() && quest->SourceItemId == Pick && quest->MinLevel == 7,
        "The native quest's level, profession requirements or provided pick changed");
    quest->LoadQuestTemplateAddon(addon->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER);

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    auto item = sObjectMgr->GetItemTemplate(Pick);
    auto lock = sLockStore.LookupEntry(1912);
    auto copperLock = sLockStore.LookupEntry(38);
    Require(item && std::any_of(item->Effects.begin(), item->Effects.end(), [](ItemEffectEntry const* effect)
        { return effect && effect->SpellID == Dig && !effect->Charges; }) && lock && lock->Type[0] == LOCK_KEY_ITEM && lock->Index[0] == Pick &&
        copperLock && copperLock->Type[0] == LOCK_KEY_SKILL && copperLock->Index[0] == LOCKTYPE_MINING,
        "The original Excavator's Pick or native lock metadata changed");

    auto& table = LootTemplates_Gameobject.*Access(LootTableNameTag{});
    auto original = table;
    table += " WHERE Entry=203385";
    auto rowsLoaded = (LootTemplates_Gameobject.*Access(LoadLootTableTag{}))();
    table = original;
    Require(rowsLoaded == 1, "Missing the real artifact loot table");
    sQuestDataStore->LoadGameObjectForQuests();

    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest=25937");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest=25937");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    return quest;
}

std::vector<Placement> ReadMinePlacements(uint32 entry, char const* filter)
{
    auto rows = WorldDatabase.PQuery("SELECT guid,spawntimesecs,position_x,position_y,position_z,orientation,rotation0,rotation1,rotation2,rotation3 "
        "FROM gameobject WHERE id=%u AND map=0 AND zoneId=1 AND areaId=134 %s ORDER BY guid", entry, filter);
    Require(bool(rows), "An actual quarry placement is missing");
    std::vector<Placement> placements;
    do
    {
        auto row = rows->Fetch();
        placements.push_back({entry, row[0].GetUInt32(), row[1].GetUInt32(), ReadGilneasPoint(row + 2),
            {row[6].GetFloat(), row[7].GetFloat(), row[8].GetFloat(), row[9].GetFloat()}});
    } while (rows->NextRow());
    return placements;
}

void AcceptTreasures(LootPlayer& player, Creature& drugan)
{
    MoveGilneasPlayer(player, drugan.GetPosition());
    WorldPackets::Quest::QuestGiverAcceptQuest accept{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    accept.QuestID = QuestId;
    accept.QuestGiverGUID = drugan.GetGUID();
    player.GetSession()->HandleQuestGiverAcceptQuest(accept);
}

void ClickArtifact(LootPlayer& player, GameObject const& chest)
{
    WorldPacket wire(CMSG_GAME_OBJ_USE);
    wire << chest.GetGUID();
    WorldPackets::GameObject::GameObjectUse packet(std::move(wire));
    packet.Read();
    player.GetSession()->HandleGameObjectUse(packet);
}

void CheckTreasures(Quest const* quest)
{
    auto npc = WorldDatabase.PQuery("SELECT position_x,position_y,position_z,orientation FROM creature WHERE id=%u AND guid=%u AND map=0 AND areaId=134",
        Drugan, DruganSpawn);
    auto artifacts = ReadMinePlacements(ArtifactChest, "");
    auto copper = ReadMinePlacements(CopperVein, "AND guid=32098");
    Require(npc && artifacts.size() == 34 && copper.size() == 1, "Expected the real Prospector, 34 artifact placements and the nearby copper vein");
    Map map(0, 614, 0, DIFFICULTY_NONE);
    auto origin = ReadGilneasPoint(npc->Fetch());
    map.LoadGrid(origin.GetPositionX(), origin.GetPositionY());
    Actor drugan(&map, Drugan, DruganSpawn, origin);
    Require(map.AddToMap(static_cast<Creature*>(&drugan)), "Could not publish actual Prospector Drugan");
    LootPlayer owner(SupplySession(), map, 2593701, origin), other(SupplySession(), map, 2593702, origin);
    for (auto player : {&owner, &other})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_DWARF);
        player->SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_HUNTER);
        player->setFactionForRace(RACE_DWARF);
        player->InitDisplayIds();
        player->SetLevel(10);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
    }
    AcceptTreasures(owner, drugan);
    Require(owner.GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE && owner.GetItemCount(Pick) == 1 && !owner.HasSkill(SKILL_MINING),
        "The real Prospector did not lend the pick to an eligible non-miner");

    {
        auto const& placement = artifacts.front();
        map.LoadGrid(placement.position.GetPositionX(), placement.position.GetPositionY());
        Supply chest(map, placement);
        MoveGilneasPlayer(other, chest.GetPosition());
        other.SendLoot(chest.GetGUID(), LOOT_SKINNING);
#ifdef PRICELESS_TREASURES_BASELINE
        Require(other.GetLootGUID() == chest.GetGUID() && !other.GetItemCount(Artifact), "Original empty loot window for an ineligible player was not reproduced");
        ReleaseSupply(other, chest);
#else
        Require(!other.GetLootGUID() && chest.getLootState() == GO_READY && !chest.ActivateToQuest(&other),
            "An inactive user could still open the protected artifact object");
        GiveSupply(other, Pick, 1);
        Require(other.CastSpell(&chest, Dig, false, other.GetItemByEntry(Pick)) != SPELL_CAST_OK,
            "A stale pick opened the artifact without an active quest");
#endif
    }

    {
        auto const& placement = copper.front();
        map.LoadGrid(placement.position.GetPositionX(), placement.position.GetPositionY());
        Supply vein(map, placement);
        MoveGilneasPlayer(owner, vein.GetPosition());
        Require(owner.CastSpell(&vein, Dig, false, owner.GetItemByEntry(Pick)) != SPELL_CAST_OK,
            "The authored pick can dig up the unrelated copper vein");
    }

    auto gather = [&](uint32 quantity)
    {
        for (auto const& placement : artifacts)
        {
            map.LoadGrid(placement.position.GetPositionX(), placement.position.GetPositionY());
            Supply chest(map, placement);
            MoveGilneasPlayer(owner, chest.GetPosition());
            if (owner.GetItemCount(Artifact) == quantity)
                break;
            Require(chest.ActivateToQuest(&owner), "An authored artifact was not accessible to its eligible player");
#ifndef PRICELESS_TREASURES_BASELINE
            if (owner.GetItemCount(Artifact) % 2 == 0)
                ClickArtifact(owner, chest);
            else
#endif
                Require(owner.CastSpell(&chest, Dig, false, owner.GetItemByEntry(Pick)) == SPELL_CAST_OK, "The authored pick did not open its actual lock");
            Tick(owner, 6500);
            TakeSupply(owner, chest, Artifact);
            ReleaseSupply(owner, chest);
            Require(owner.GetItemCount(Pick) == 1, "Gathering consumed the reusable Excavator's Pick");
        }
        Require(owner.GetItemCount(Artifact) == quantity, "Actual artifact sources did not supply the expected number of items");
    };

    gather(3);
    AbandonSupply(owner, QuestId);
#ifdef PRICELESS_TREASURES_BASELINE
    Require(owner.GetItemCount(Artifact) == 3 && !owner.GetItemCount(Pick), "Original abandonment did not retain the three quest artifacts");
    std::cout << "EXPECTED_BROKEN original dormant loot window and three retained artifacts on abandonment\n";
#else
    Require(!owner.GetItemCount(Artifact) && !owner.GetItemCount(Pick), "Abandonment retained the artifacts or the Excavator's Pick");
    AcceptTreasures(owner, drugan);
    gather(8);
    Require(owner.GetQuestStatus(QuestId) == QUEST_STATUS_COMPLETE, "Eight actual artifacts did not complete the original objective");
    {
        auto const& placement = artifacts[8];
        map.LoadGrid(placement.position.GetPositionX(), placement.position.GetPositionY());
        Supply another(map, placement);
        MoveGilneasPlayer(owner, another.GetPosition());
        Require(!another.ActivateToQuest(&owner) && owner.CastSpell(&another, Dig, false, owner.GetItemByEntry(Pick)) != SPELL_CAST_OK,
            "An already completed quest left an extra artifact available");
    }
    uint32 choice = 0;
    if (auto rewards = sDB2Manager.GetQuestPackageItems(quest->PackageID))
        for (auto item : *rewards)
            if (owner.CanSelectQuestPackageItem(item))
            {
                choice = item->ItemID;
                break;
            }
    if (!choice)
        if (auto fallback = sDB2Manager.GetQuestPackageItemsFallback(quest->PackageID); fallback && !fallback->empty())
            choice = fallback->front()->ItemID;
    MoveGilneasPlayer(owner, drugan.GetNearPosition(2.0f, 0.0f));
    Require(choice && owner.CanRewardQuest(quest, choice, false), "The original choice at Prospector Drugan is unavailable");
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestID = QuestId;
    reward.QuestGiverGUID = drugan.GetGUID();
    reward.ItemChoiceID = choice;
    owner.GetSession()->HandleQuestGiverChooseReward(reward);
    Require(owner.GetQuestRewardStatus(QuestId) && !owner.GetItemCount(Artifact) && !owner.GetItemCount(Pick),
        "The actual reward did not remove the loot and reusable pick");
    std::cout << "PASS real Excavator's Pick, the frozen artifact lock, three abandoned and eight rewarded artifacts, native cleanup\n";
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
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "Priceless Treasures requires isolated characters");
        auto quest = LoadTreasureInputs();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckTreasures(quest);
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
