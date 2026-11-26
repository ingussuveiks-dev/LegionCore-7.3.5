#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "ConditionMgr.h"
#include "CreatureTextMgr.h"
#include "GilneasQuestNative.h"
#include "MapInstanced.h"
#include "SmartScriptMgr.h"

namespace
{
constexpr uint32 Vision = 26320;
constexpr uint32 Secrets = 26319;
constexpr uint32 Incense = 58147;
constexpr uint32 IncenseSpell = 79586;
constexpr uint32 VisionActor = 42693;
constexpr uint32 Focus = 301550;

void LoadVision()
{
    auto rows = WorldDatabase.Query(
#include "NativeWestfallVision.inc"
    );
    Require(rows && rows->GetRowCount() == 2, "The two original Westfall quest templates are required");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new Quest(rows->Fetch());
        quests[quest->Id] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->Id, quest);
    } while (rows->NextRow());
    auto addon = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,"
        "RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,"
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=26320");
    Require(bool(addon), "The original Vision quest requirements are missing");
    quests[Vision]->LoadQuestTemplateAddon(addon->Fetch());
    quests[Vision]->prevQuests.push_back(Secrets);
    Require(quests[Vision]->SourceItemId == Incense && quests[Vision]->HasSpecialFlag(QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT),
        "The original quest tool and event requirement are required");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature WHERE guid=107503 AND id=42651");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE gameobject AS SELECT * FROM legion_world.gameobject WHERE guid=106562 AND id=301550");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest=26320");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_text AS SELECT * FROM legion_world.creature_text WHERE CreatureID IN (42651,42693)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE smart_scripts AS SELECT * FROM legion_world.smart_scripts WHERE source_type=0 AND entryorguid IN (42651,42693)");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    sObjectMgr->LoadGameobjects();
    sObjectMgr->LoadCreatures();
    sQuestDataStore->LoadCreatureQuestRelations();
    sCreatureTextMgr->LoadCreatureTexts();
    sSmartScriptMgr->LoadSmartAIFromDB();
}

void Prepare(LootPlayer& player)
{
    player.SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_HUNTER);
    player.setFactionForRace(player.getRace());
    player.InitDisplayIds();
    player.SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);
    player.SetLevel(20);
    player.SetRewardedQuest(Secrets);
    MoveGilneasPlayer(player, player.GetPosition());
}

void CheckVision()
{
    Map westfall(0, 300, 0, DIFFICULTY_NONE);
    MapInstanced parent(36, 300);
    InstanceMap deadmines(36, 301, 26320, DIFFICULTY_NORMAL, &parent);
    auto giverData = sObjectMgr->GetCreatureData(107503);
    auto focusData = sObjectMgr->GetGOData(106562);
    Require(giverData && focusData, "The original offer and Deadmines focus must be loaded");
    westfall.LoadGrid(giverData->posX, giverData->posY);
    deadmines.LoadGrid(focusData->posX, focusData->posY);
    auto giver = westfall.GetCreature(ObjectGuid::Create<HighGuid::Creature>(0, 42651, 107503));
    LootPlayer inside(SupplySession(), deadmines, 2632002, Position{focusData->posX, focusData->posY, focusData->posZ, focusData->orientation});
    Prepare(inside);
    auto focus = inside.FindNearestGameObject(Focus, 5.0f);
    Require(giver && focus, "The original offer and focus must be present on their normal maps");
    LootPlayer outside(SupplySession(), westfall, 2632001, giver->GetPosition());
    Prepare(outside);
    WorldPackets::Quest::QuestGiverAcceptQuest accept{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    accept.QuestGiverGUID = giver->GetGUID();
    accept.QuestID = Vision;
    outside.GetSession()->HandleQuestGiverAcceptQuest(accept);
    Require(outside.GetQuestStatus(Vision) == QUEST_STATUS_INCOMPLETE && outside.GetItemCount(Incense) == 1,
        "Thoralius did not offer the tool while retaining the active event requirement");
    Require(outside.CastSpell(&outside, IncenseSpell, false, outside.GetItemByEntry(Incense)) != SPELL_CAST_OK,
        "The genuine item could be used outside the dungeon focus");
#ifndef WESTFALL_VISION_OBSERVE
    Require(!WorldDatabase.Query("SELECT 1 FROM smart_scripts WHERE entryorguid=42651 AND event_type=19 AND action_type=15"),
        "The retired acceptance credit is still attached to Thoralius");
    std::cout << "PASS actual Thoralius acceptance provides the item, stays incomplete and refuses an exterior cast\n";
#else
    // The current vision observation is intentionally separate from the clean admission regression.
    inside.AddQuestAndCheckCompletion(sQuestDataStore->GetQuestTemplate(Vision), nullptr);
    Require(inside.GetQuestStatus(Vision) == QUEST_STATUS_INCOMPLETE && inside.GetItemCount(Incense) == 1,
        "The genuine accepted quest did not provide the item");
    auto const result = inside.CastSpell(&inside, IncenseSpell, false, inside.GetItemByEntry(Incense));
    Tick(inside, 6000);
    auto const summons = inside.GetSummonList(VisionActor);
    std::list<Creature*> actors;
    inside.GetCreatureListWithEntryInGrid(actors, VisionActor, 55.0f);
    std::cout << "CASE Vision normal focus cast=" << uint32(result) << " quest=" << inside.GetQuestStatus(Vision)
        << " owned summons=" << summons->size() << " nearby actors=" << actors.size() << std::endl;
    Require(result == SPELL_CAST_OK, "The original item did not admit a cast at the normal dungeon focus");
    Require(inside.GetQuestStatus(Vision) == QUEST_STATUS_COMPLETE, "The original native item and summoned actor did not finish their recorded event");
    std::cout << "OBSERVED the native Deadmines summon completed the quest during the initial item cast\n";
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{0, {}}, {36, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{0, {}}, {36, {}}});
        LoadGameObjectModelList(argv[2]);
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "The isolated login pool is required");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "The isolated character pool is required");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "The test must use isolated fixture characters");
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        LoadVision();
        CheckVision();
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
