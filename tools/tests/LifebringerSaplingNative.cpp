#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "SmartAI.h"
#include "SmartScriptMgr.h"

void AddSC_SmartScripts();

namespace
{
constexpr uint32 SaplingQuest = 13925;
constexpr uint32 Sapling = 46363;
constexpr uint32 Protection = 65188;
constexpr uint32 DevouringPresence = 34328;
constexpr uint32 TrialCredit = 34373;
constexpr uint32 Kathrena = 34301;
constexpr uint32 Animals[] = {2165, 2237, 34318, 2071};

void ShadowRows(char const* table, char const* restriction)
{
    WorldDatabase.DirectExecute(Trinity::StringFormat("CREATE TEMPORARY TABLE sapling_%s AS SELECT * FROM %s WHERE %s",
        table, table, restriction).c_str());
    WorldDatabase.DirectExecute(Trinity::StringFormat("CREATE TEMPORARY TABLE %s AS SELECT * FROM sapling_%s", table, table).c_str());
}

Quest const* LoadSapling()
{
    auto row = WorldDatabase.Query(
#include "NativeLifebringerSapling.inc"
    );
    Require(bool(row), "Missing An Ounce of Prevention quest");
    auto quest = new Quest(row->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    quests[SaplingQuest] = quest;
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(SaplingQuest, quest);
    auto objectives = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=13925 ORDER BY Type");
    Require(objectives && objectives->GetRowCount() == 2, "Expected the trial and delivery objectives");
    Require(objectives->Fetch()[2].GetUInt8() == QUEST_OBJECTIVE_MONSTER && objectives->Fetch()[4].GetUInt32() == TrialCredit &&
        objectives->Fetch()[5].GetUInt32() == 1, "The wildlife trial must require one real credit");
    quest->LoadQuestObjective(objectives->Fetch());
    objectives->NextRow();
    Require(objectives->Fetch()[2].GetUInt8() == QUEST_OBJECTIVE_ITEM && objectives->Fetch()[4].GetUInt32() == Sapling &&
        objectives->Fetch()[5].GetUInt32() == 1, "The wildlife trial must require return of the sapling");
    quest->LoadQuestObjective(objectives->Fetch());
    auto addon = WorldDatabase.Query(
        "SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,RewardMailTemplateID,RewardMailDelay,"
        "RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMinRepValue,RequiredMaxRepFaction,RequiredMaxRepValue,"
        "ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=13925");
    Require(addon && addon->Fetch()[4].GetInt32() == 13882 && addon->Fetch()[5].GetUInt32() == 13885,
        "Original Darkshore prerequisites changed");
    quest->LoadQuestTemplateAddon(addon->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST | QUEST_SPECIAL_FLAGS_DELIVER);
    Require(quest->SourceItemId == Sapling && quest->SourceItemIdCount == 1, "Acceptance must provide one trial sapling");
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    AddSC_SmartScripts();
    sScriptMgr->SwapScriptContext(true);
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    ShadowRows("conditions", "SourceTypeOrReferenceId=17 AND SourceEntry=65188");
    ShadowRows("smart_scripts", "source_type=0 AND entryorguid IN(2165,2237,34318,2071)");
    ShadowRows("creature_questender", "quest=13925");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sSmartScriptMgr->LoadSmartAIFromDB();
    sConditionMgr->LoadConditions();
    Require(sConditionMgr->GetConditionsForNotGroupedEntry(CONDITION_SOURCE_TYPE_SPELL, Protection).size() == 16,
        "The four living-animal gates did not load");
    return quest;
}

struct TrialAnimal : Actor
{
    using Actor::Actor;
    void AddToWorld() override
    {
        sObjectAccessor->AddObject(static_cast<Creature*>(this));
        Unit::AddToWorld();
        Require(AIM_Initialize(new SmartAI(this)), "Real SmartAI did not initialize for the trial animal");
    }
};

void CheckSapling(Quest const* quest)
{
    auto location = [](uint32 entry)
    {
        auto result = WorldDatabase.PQuery("SELECT guid,position_x,position_y,position_z,orientation FROM creature "
            "WHERE id=%u AND map=1 AND zoneId=148 ORDER BY guid LIMIT 1", entry);
        Require(bool(result), "A named Darkshore actor had no live placement");
        auto f = result->Fetch();
        return std::make_pair(f[0].GetUInt32(), Position{f[1].GetFloat(),f[2].GetFloat(),f[3].GetFloat(),f[4].GetFloat()});
    };
    auto const giver = location(Kathrena);
    Require(bool(WorldDatabase.Query("SELECT 1 FROM creature_queststarter WHERE quest=13925 AND id=34301")),
        "Kathrena is missing the original offer");
    Map map(1, 300, 0, DIFFICULTY_NONE);
    Actor receiver(&map, Kathrena, giver.first, giver.second);
    Require(map.AddToMap(static_cast<Creature*>(&receiver)), "Could not publish the original quest giver");
    LootPlayer owner(SupplySession(), map, 13925, giver.second), other(SupplySession(), map, 13926, giver.second);
    for (auto player : {&owner, &other})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_NIGHTELF);
        player->setFactionForRace(RACE_NIGHTELF);
        player->InitDisplayIds();
        player->SetLevel(18);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
    }
    auto cast = [](LootPlayer& player, Unit& animal)
    {
        auto item = player.GetItemByEntry(Sapling);
        Require(item, "Expected the real carried sapling");
        return player.CastSpell(&animal, Protection, false, item);
    };
    GiveSupply(other, Sapling, 1);
    for (uint32 entry : Animals)
    {
        auto placed = location(entry);
        SupplyMove(owner, placed.second);
        SupplyMove(other, placed.second);
        TrialAnimal animal(&map, entry, placed.first, placed.second);
        Require(map.AddToMap(static_cast<Creature*>(&animal)), "Could not publish the real quest animal");
        Require(cast(other, animal) != SPELL_CAST_OK, "A retained sapling worked without the quest");
        owner.AddQuestAndCheckCompletion(quest, &receiver);
        Require(owner.GetItemCount(Sapling) == 1 && !owner.CanRewardQuest(quest, false), "One supplied sapling must leave the trial pending");
        Actor unrelated(&map, Kathrena, 60001, placed.second);
        Require(map.AddToMap(static_cast<Creature*>(&unrelated)), "Could not publish an unrelated target");
        Require(cast(owner, unrelated) != SPELL_CAST_OK, "The sapling accepted an unrelated nearby creature");
        if (entry == 2071)
        {
            other.AddQuestAndCheckCompletion(quest, &receiver);
            animal.SetLootRecipient(&other);
        }
        Require(cast(owner, animal) == SPELL_CAST_OK, "A named living animal rejected the actual sapling");
        Tick(owner, 4000);
        auto presence = animal.FindNearestCreature(DevouringPresence, 20, true);
        Require(presence && presence->ToTempSummon() && presence->ToTempSummon()->GetSummonerGUID() == owner.GetGUID(),
            "The trial did not summon the authored Devouring Presence from the sapling user");
        std::cout << "CASE animal=" << entry << " credit=" << owner.GetQuestObjectiveData(SaplingQuest, TrialCredit)
            << " bystander-credit=" << other.GetQuestObjectiveData(SaplingQuest, TrialCredit)
            << " faction=" << animal.getFaction() << " combat=" << animal.isInCombat() << std::endl;
        Require(owner.GetQuestObjectiveData(SaplingQuest, TrialCredit) == 1 && owner.CanRewardQuest(quest, false) &&
            animal.getFaction() == 35 && animal.getVictim() == presence && !other.GetQuestObjectiveData(SaplingQuest, TrialCredit),
            "Trial credit or scripted reaction was incorrect");
        Require(cast(owner, animal) != SPELL_CAST_OK, "The already complete trial allowed another cast");
        animal.CombatStop(true);
        presence->DespawnOrUnsummon();
        map.RemoveAllObjectsInRemoveList();
        if (entry == 2071)
        {
            AbandonSupply(other, SaplingQuest);
            SupplyMove(owner, giver.second);
            WorldPackets::Quest::QuestGiverChooseReward packet{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
            packet.QuestGiverGUID = receiver.GetGUID();
            packet.QuestID = SaplingQuest;
            packet.ItemChoiceID = 0;
            owner.GetSession()->HandleQuestGiverChooseReward(packet);
            Require(owner.GetQuestRewardStatus(SaplingQuest) && !owner.GetItemCount(Sapling), "Actual turn-in did not remove the sapling");
        }
        else
        {
            AbandonSupply(owner, SaplingQuest);
            Require(!owner.GetItemCount(Sapling), "Abandon left the sapling in the bag");
        }
    }
    std::cout << "PASS actual sapling, four world animals, exact target permission, summon, kill credit and abandon cleanup\n";
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
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "The sapling test requires the isolated character schema");
        auto quest = LoadSapling();
        sObjectMgr->LoadPlayerInfo();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckSapling(quest);
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
