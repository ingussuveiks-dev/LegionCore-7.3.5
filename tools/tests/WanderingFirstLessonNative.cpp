#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "GossipDef.h"
#include "ItemPackets.h"
#include "SpellMgr.h"

namespace
{
constexpr uint32 Area = 5834;
constexpr uint32 GiverSpawn = 243564;
constexpr uint32 RackSpawn = 102286;
constexpr uint32 RackEntry = 210005;
constexpr uint32 SandyFist = 29406;
constexpr uint32 AcademyQuest = 29409;
constexpr uint32 AcademyAura = 56267;
struct Lesson
{
    uint8 playerClass;
    uint32 quest;
    uint32 mainHand;
    uint32 offHand;
};
constexpr Lesson Lessons[] = {
    {CLASS_MONK, 30027, 77279, 77278}, {CLASS_MAGE, 30033, 76390, 76392},
    {CLASS_HUNTER, 30034, 73211, 0}, {CLASS_PRIEST, 30035, 73207, 76393},
    {CLASS_ROGUE, 30036, 73208, 73212}, {CLASS_SHAMAN, 30037, 76391, 73213},
    {CLASS_WARRIOR, 30038, 73210, 0}
};

void LoadLessonInputs()
{
    auto templates = WorldDatabase.Query(
#include "NativeWanderingLesson.inc"
    );
    Require(templates && templates->GetRowCount() == 16, "Missing a lesson, old introduction or academy quest");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new Quest(templates->Fetch());
        quests[quest->Id] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->Id, quest);
    } while (templates->NextRow());
    auto addons = WorldDatabase.Query(
        "SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,RewardMailTemplateID,RewardMailDelay,"
        "RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMinRepValue,RequiredMaxRepFaction,RequiredMaxRepValue,"
        "ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID BETWEEN 30033 AND 30045 OR ID IN (30027,29406,29409)");
    Require(addons && addons->GetRowCount() == 16, "Missing the actual lesson prerequisites and class masks");
    do
    {
        auto row = addons->Fetch();
        auto quest = quests[row[0].GetUInt32()];
        quest->LoadQuestTemplateAddon(row);
        if (quest->PrevQuestID)
            quest->prevQuests.push_back(quest->PrevQuestID);
        auto const next = uint32(std::abs(quest->NextQuestID));
        if (next && next < quests.size() && quests[next])
            quests[next]->prevQuests.push_back(quest->NextQuestID < 0 ? -int32(quest->Id) : int32(quest->Id));
        if (quest->NextQuestIdChain && quest->NextQuestIdChain < quests.size() && quests[quest->NextQuestIdChain])
            quests[quest->NextQuestIdChain]->prevChainQuests.push_back(quest->Id);
    } while (addons->NextRow());
    auto objectives = WorldDatabase.Query(
        "SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged FROM quest_objectives "
        "WHERE QuestID IN (29406,29409,30027) OR QuestID BETWEEN 30033 AND 30038");
    Require(bool(objectives), "Missing the original equipment or training objectives");
    do
    {
        auto row = objectives->Fetch();
        auto quest = quests[row[1].GetUInt32()];
        quest->LoadQuestObjective(row);
        quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
    } while (objectives->NextRow());

    auto oldAreaStarts = WorldDatabase.Query("SELECT COUNT(*) FROM area_queststart WHERE id=5834");
    Require(oldAreaStarts && !oldAreaStarts->Fetch()[0].GetUInt32(), "Expected the migrated area to have no automatic introduction");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature WHERE guid=243564");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest IN(29406,30027,30033,30034,30035,30036,30037,30038)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest IN(29406,30027,30033,30034,30035,30036,30037,30038,30039,30040,30041,30042,30043,30044,30045)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_area AS SELECT * FROM legion_world.spell_area WHERE spell=56267 AND area=5834");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sObjectMgr->LoadCreatures();
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    auto& lootTable = LootTemplates_Gameobject.*Access(LootTableNameTag{});
    auto name = lootTable;
    lootTable += " WHERE Entry=40856";
    auto loaded = (LootTemplates_Gameobject.*Access(LoadLootTableTag{}))();
    lootTable = name;
    Require(loaded && LootTemplates_Gameobject.GetLootFor(40856), "The original weapon rack loot is unavailable");
    sQuestDataStore->LoadGameObjectForQuests();
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sSpellMgr->LoadSpellAreas();
}

struct LessonGiver : Creature
{
    explicit LessonGiver(Map& map) : Creature(false)
    {
        auto data = sObjectMgr->GetCreatureData(GiverSpawn);
        Require(data && data->id == 53566, "Missing Shang Xi's original training-grounds spawn");
        sObjectMgr->RemoveCreatureFromGrid(GiverSpawn, data);
        map.LoadGrid(data->posX, data->posY);
        Require(LoadCreatureFromDB(GiverSpawn, &map, true) && IsInWorld(), "Could not publish Shang Xi");
    }

    ~LessonGiver()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<Creature*>(this), false);
    }
};

void SetStudent(LootPlayer& player, uint8 race, uint8 playerClass)
{
    player.SetByteValue(UNIT_FIELD_BYTES_0, 0, race);
    player.SetByteValue(UNIT_FIELD_BYTES_0, 1, playerClass);
    player.setFactionForRace(race);
    player.InitDisplayIds();
    player.GetReputationMgr().LoadFromDB({});
    player.SetLevel(1);
    player.SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 400);
}

void Accept(LootPlayer& player, Creature& giver, uint32 quest)
{
    WorldPackets::Quest::QuestGiverAcceptQuest request{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    request.QuestGiverGUID = giver.GetGUID();
    request.QuestID = quest;
    player.GetSession()->HandleQuestGiverAcceptQuest(request);
}

void Reward(LootPlayer& player, Creature& giver, uint32 quest)
{
    WorldPackets::Quest::QuestGiverChooseReward request{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    request.QuestGiverGUID = giver.GetGUID();
    request.QuestID = quest;
    player.GetSession()->HandleQuestGiverChooseReward(request);
}

void Equip(LootPlayer& player, uint32 itemId)
{
    Item* item = player.GetItemByEntry(itemId);
    Require(item && !item->IsEquipped(), "Expected a newly looted weapon in the backpack");
    uint16 destination = 0;
    InventoryResult result = player.CanEquipItem(NULL_SLOT, destination, item, !item->IsBag());
    if (result != EQUIP_ERR_OK)
        std::cout << "CASE weapon equip item=" << itemId << " result=" << uint32(result)
                  << " dual-wield=" << player.CanDualWield() << " skill=" << player.GetSkillValue(item->GetSkill()) << '\n';
    WorldPackets::Item::AutoEquipItem request{WorldPacket(CMSG_AUTO_EQUIP_ITEM)};
    request.PackSlot = item->GetBagSlot();
    request.Slot = item->GetSlot();
    request.Inv.Items.push_back({request.PackSlot, request.Slot});
    player.GetSession()->HandleAutoEquipItem(request);
    Require(item->IsEquipped(), "Native equipment packet refused the training weapon");
    Tick(player, 100);
}

void CheckAcademyAura(LootPlayer& player, uint32 lesson)
{
    auto const rules = sSpellMgr->GetSpellAreaMapBounds(AcademyAura);
    auto const rule = std::find_if(rules.first, rules.second, [lesson](auto const& entry) {
        return entry.second.areaId == Area && entry.second.questStart == lesson &&
            entry.second.questEnd == AcademyQuest && entry.second.autocast;
    });
    Require(rule != rules.second && !rule->second.IsFitToRequirements(&player, 5736, Area),
        "Academy aura was missing or eligible before the later quest");

    // Start at the later area's checkpoint to exercise the real aura predicate.
    player.AddQuestAndCheckCompletion(sQuestDataStore->GetQuestTemplate(AcademyQuest), nullptr);
    Require(player.GetQuestStatus(AcademyQuest) == QUEST_STATUS_INCOMPLETE, "The academy checkpoint is not active");
    Require(rule->second.IsFitToRequirements(&player, 5736, Area) &&
        !rule->second.IsFitToRequirements(&player, 5736, Area + 1), "The lesson reward did not satisfy the academy aura in its area");
    AbandonSupply(player, AcademyQuest);
    Require(!rule->second.IsFitToRequirements(&player, 5736, Area), "The academy aura still matched after abandoning the later quest");
}

void CheckEquipment(LootPlayer& player, Creature& giver, Lesson const& lesson)
{
    auto row = WorldDatabase.PQuery(
        "SELECT spawntimesecs,position_x,position_y,position_z,orientation,rotation0,rotation1,rotation2,rotation3 "
        "FROM gameobject WHERE guid=%u AND id=%u AND map=860 AND areaId=5834", RackSpawn, RackEntry);
    Require(bool(row), "Missing the original weapon rack placement");
    auto fields = row->Fetch();
    Placement placement{RackEntry, RackSpawn, fields[0].GetUInt32(),
        {fields[1].GetFloat(), fields[2].GetFloat(), fields[3].GetFloat(), fields[4].GetFloat()},
        {fields[5].GetFloat(), fields[6].GetFloat(), fields[7].GetFloat(), fields[8].GetFloat()}};
    player.GetMap()->LoadGrid(placement.position.GetPositionX(), placement.position.GetPositionY());
    Supply rack(*player.GetMap(), placement);
    SupplyMove(player, placement.position);
    Require(rack.ActivateToQuest(&player), "An eligible student cannot identify the original rack");
    OpenSupply(player, rack);
    Require(player.GetLootGUID() == rack.GetGUID(), "The original rack did not open native loot");
    WorldPackets::Loot::LootResponse view;
    rack.loot.BuildLootResponse(view, &player, ALL_PERMISSION);
    Require(view.Items.size() == (lesson.offHand ? 2u : 1u), "The rack offered the wrong number of training items");
    WorldPackets::Loot::AutoStoreLootItem request{WorldPacket(CMSG_LOOT_ITEM)};
    for (auto const& item : view.Items)
    {
        Require(item.Loot.ItemID == lesson.mainHand || item.Loot.ItemID == lesson.offHand, "The rack offered another lesson's equipment");
        request.Loot.push_back({rack.loot.GetGUID(), item.LootListID});
    }
    player.GetSession()->HandleAutostoreLootItemOpcode(request);
    ReleaseSupply(player, rack);
    Require(player.GetQuestStatus(lesson.quest) == QUEST_STATUS_INCOMPLETE, "Looting alone completed the equipment lesson");
    Equip(player, lesson.mainHand);
    Require(player.GetQuestObjectiveData(lesson.quest, 54139) == 1, "Main-hand equip did not credit its objective");
    if (lesson.offHand)
    {
        Require(player.GetQuestStatus(lesson.quest) == QUEST_STATUS_INCOMPLETE &&
            player.GetQuestObjectiveData(lesson.quest, 57849) == 0, "Main-hand equip also credited the offhand objective");
        Equip(player, lesson.offHand);
        Require(player.GetQuestObjectiveData(lesson.quest, 57849) == 1, "Offhand equip did not credit its objective");
    }
    Require(player.GetQuestStatus(lesson.quest) == QUEST_STATUS_COMPLETE, "Equipping the full set did not complete the lesson");
    SupplyMove(player, giver.GetPosition());
    Reward(player, giver, lesson.quest);
    Require(player.IsQuestRewarded(lesson.quest), "Shang Xi did not record the native lesson reward");
    Require(player.GetQuestStatus(lesson.quest) == QUEST_STATUS_REWARDED, "The lesson reward is not visible to spell_area");
    player.PrepareQuestMenu(giver.GetGUID());
    Require(!player.PlayerTalkClass->GetQuestMenu().HasItem(lesson.quest), "The one-time lesson was offered again");
    Require(player.GetItemByEntry(lesson.mainHand) && (!lesson.offHand || player.GetItemByEntry(lesson.offHand)),
        "Rewarding the lesson removed training equipment");
    if (player.GetQuestStatus(SandyFist) == QUEST_STATUS_NONE)
        Accept(player, giver, SandyFist);
    Require(player.GetQuestStatus(SandyFist) == QUEST_STATUS_INCOMPLETE, "The completed lesson did not admit Sandy Fist");
    CheckAcademyAura(player, lesson.quest);
    std::cout << "PASS native rack, equipment, turn-in, Sandy Fist and academy aura for quest " << lesson.quest << std::endl;
}

void CheckFirstLesson()
{
    Map map(860, 300, 0, DIFFICULTY_NONE);
    LessonGiver giver(map);
    uint64 guid = 30000;
    for (Lesson const& route : Lessons)
    {
        uint32 lesson = route.quest;
        LootPlayer student(SupplySession(), map, ++guid, giver.GetPosition());
        SetStudent(student, RACE_PANDAREN_NEUTRAL, route.playerClass);
        student.PrepareAreaQuest(Area);
        for (uint32 introduction = 30039; introduction <= 30045; ++introduction)
            Require(student.GetQuestStatus(introduction) == QUEST_STATUS_NONE, "An obsolete introduction auto-started");
        student.PrepareQuestMenu(giver.GetGUID());
        for (Lesson const& other : Lessons)
            Require(student.PlayerTalkClass->GetQuestMenu().HasItem(other.quest) == (other.playerClass == route.playerClass),
                "Shang Xi's quest menu did not offer only the correct class lesson");
        Accept(student, giver, lesson);
        Require(student.GetQuestStatus(lesson) == QUEST_STATUS_INCOMPLETE &&
            !student.CanTakeQuest(sQuestDataStore->GetQuestTemplate(SandyFist), false),
            "The real acceptance handler failed or skipped the equipment objective");
        AbandonSupply(student, lesson);
        Accept(student, giver, lesson);
        Require(student.GetQuestStatus(lesson) == QUEST_STATUS_INCOMPLETE, "The first lesson could not be accepted again");
        student.InitTalentForLevel();
        student.LearnDefaultSkills();
        student.LearnCustomSpells();
        if (route.playerClass == CLASS_MONK)
            student.AddAura(103985, &student);
        Tick(student, 100);
        if (route.playerClass == CLASS_MONK)
            Require(student.GetPrimarySpecialization() == 269 && student.HasSpell(124146) && student.CanDualWield(),
                "The new monk's native Windwalker specialization did not teach Dual Wield");
        CheckEquipment(student, giver, route);
    }
    LootPlayer visitor(SupplySession(), map, ++guid, giver.GetPosition());
    SetStudent(visitor, RACE_DRAENEI, CLASS_SHAMAN);
    visitor.PrepareQuestMenu(giver.GetGUID());
    for (auto const& lesson : Lessons)
        Require(!visitor.PlayerTalkClass->GetQuestMenu().HasItem(lesson.quest), "Non-Pandaren could see a starter lesson");

    LootPlayer legacy(SupplySession(), map, ++guid, giver.GetPosition());
    SetStudent(legacy, RACE_PANDAREN_NEUTRAL, CLASS_MONK);
    legacy.AddQuestAndCheckCompletion(sQuestDataStore->GetQuestTemplate(30039), nullptr);
    Require(legacy.GetQuestStatus(30039) == QUEST_STATUS_COMPLETE, "The saved introduction cannot be completed");
    Reward(legacy, giver, 30039);
    Require(legacy.GetQuestRewardStatus(30039) && legacy.GetQuestStatus(30027) == QUEST_STATUS_INCOMPLETE,
        "The legacy turn-in did not hand off to the correct weapon lesson");
    std::cout << "PASS seven native class offers, acceptance, abandonment and retry, race restriction, and legacy turn-in\n";
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{860, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{860, {}}});
        LoadGameObjectModelList(argv[2]);
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
        LoginDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        std::exception_ptr failure;
        try
        {
            auto schema = CharacterDatabase.Query("SELECT DATABASE()");
            Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "The lesson check requires isolated characters");
            LoadLessonInputs();
            sObjectMgr->LoadPlayerInfo();
            HashMapHolder<Player>::SetSize(32);
            HashMapHolder<GameObject>::SetSize(32);
            GameTime::UpdateGameTimers();
            CheckFirstLesson();
        }
        catch (...)
        {
            failure = std::current_exception();
        }
        CharacterDatabase.Close();
        LoginDatabase.Close();
        HotfixDatabase.Close();
        WorldDatabase.Close();
        if (failure)
            std::rethrow_exception(failure);
        return 0;
    }
    catch (std::exception const& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
