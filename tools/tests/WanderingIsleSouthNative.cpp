#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "GilneasQuestNative.h"

void AddSC_go_scripts();

namespace
{
constexpr uint32 Bidden = 29792;
constexpr uint32 Stocking = 29795;
constexpr uint32 Wreck = 30589;
constexpr uint32 Handle = 30590;
constexpr uint32 Preying = 30591;
constexpr uint32 EvilFromSeas = 29793;
constexpr uint32 UrgentNews = 29796;
constexpr uint32 MedicalSupplies = 29797;
constexpr uint32 Tiger = 55946;
constexpr uint32 DarkenedHorror = 56007;
constexpr uint32 Bamboo = 80806;
constexpr uint32 Explosives = 74955;
constexpr uint32 MedicalCrateItem = 74958;

struct SouthPet : LootPet
{
    SouthPet(LootPlayer& owner, Map& map) : LootPet(owner, map)
    {
        m_ControlledByPlayer = true;
        setFaction(owner.getFaction());
        SetPhaseMask(owner.GetPhaseMask(), false);
        SetPhaseId(owner.GetPhases(), false);
        SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);
    }
};

void LoadSouth()
{
    auto templates = WorldDatabase.Query(
#include "NativeWanderingIsleSouth.inc"
    );
    Require(templates && templates->GetRowCount() == 8, "The southern Wandering Isle quest templates are missing");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new Quest(templates->Fetch());
        quests[quest->Id] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->Id, quest);
    } while (templates->NextRow());
    auto addons = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,"
        "RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,"
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID IN (29792,29793,29795,29796,29797,30589,30590,30591)");
    Require(addons && addons->GetRowCount() == 8, "The original southern prerequisites are missing");
    do
    {
        auto fields = addons->Fetch();
        auto quest = quests[fields[0].GetUInt32()];
        quest->LoadQuestTemplateAddon(fields);
        if (quest->PrevQuestID && quests[std::abs(quest->PrevQuestID)])
            quest->prevQuests.push_back(quest->PrevQuestID);
    } while (addons->NextRow());
    auto objectives = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID IN (29793,29795,29797,30590,30591)");
    Require(objectives && objectives->GetRowCount() == 5, "The original forest, bamboo or wreck objectives are missing");
    do
    {
        auto row = objectives->Fetch();
        auto quest = quests[row[1].GetUInt32()];
        quest->LoadQuestObjective(row);
        quest->SetSpecialFlag(row[2].GetUInt8() == QUEST_OBJECTIVE_ITEM ? QUEST_SPECIAL_FLAGS_DELIVER : QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
    } while (objectives->NextRow());

    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature WHERE map=860 AND (guid IN (245135,245264,245266,245384,245389) OR id IN (55946,56007))");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest IN (29793,29795,29796,29797,30589,30590,30591)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest IN (29793,29795,29796,29797,30589,30590,30591)");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    sObjectMgr->LoadCreatures();
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();

    auto& gameobjectLoot = LootTemplates_Gameobject.*Access(LootTableNameTag{});
    auto const originalGameobjectLoot = gameobjectLoot;
    gameobjectLoot += " WHERE Entry IN (40474,209792,211394,211397,211398,211399,211400,211401)";
    auto const count = (LootTemplates_Gameobject.*Access(LoadLootTableTag{}))();
    gameobjectLoot = originalGameobjectLoot;
    Require(count == 26, "The eight objective rows and original bonus loot did not load together");
    auto& tigerLoot = LootTemplates_Creature.*Access(LootTableNameTag{});
    auto const originalTigerLoot = tigerLoot;
    tigerLoot += " WHERE Entry IN (55946,56007)";
    (LootTemplates_Creature.*Access(LoadLootTableTag{}))();
    tigerLoot = originalTigerLoot;
    Require(LootTemplates_Creature.GetLootFor(Tiger) && LootTemplates_Creature.GetLootFor(DarkenedHorror), "Missing the original tiger or horror loot");
}

Creature& FindSouthActor(Map& map, uint32 spawn)
{
    auto data = sObjectMgr->GetCreatureData(spawn);
    Require(data, "An original forest actor has no world placement");
    map.LoadGrid(data->posX, data->posY);
    auto actor = map.GetCreature(ObjectGuid::Create<HighGuid::Creature>(860, data->id, spawn));
    Require(actor && actor->GetDBTableGUIDLow() == spawn && actor->IsAlive(), "Could not load the original forest actor");
    return *actor;
}

void AcceptSouth(LootPlayer& player, Creature& actor, uint32 quest)
{
    MoveGilneasPlayer(player, actor.GetPosition());
    WorldPackets::Quest::QuestGiverAcceptQuest request{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    request.QuestGiverGUID = actor.GetGUID();
    request.QuestID = quest;
    player.GetSession()->HandleQuestGiverAcceptQuest(request);
    Require(player.GetQuestStatus(quest) != QUEST_STATUS_NONE, "The actual actor did not offer its forest quest");
}

void RewardSouth(LootPlayer& player, Creature& actor, uint32 quest)
{
    MoveGilneasPlayer(player, actor.GetPosition());
    WorldPackets::Quest::QuestGiverChooseReward request{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    request.QuestGiverGUID = actor.GetGUID();
    request.QuestID = quest;
    if (auto package = sDB2Manager.GetQuestPackageItems(sQuestDataStore->GetQuestTemplate(quest)->PackageID))
        for (auto item : *package)
            if (player.CanSelectQuestPackageItem(item))
            {
                request.ItemChoiceID = item->ItemID;
                break;
            }
    player.GetSession()->HandleQuestGiverChooseReward(request);
    Require(player.IsQuestRewarded(quest), "The actual actor did not reward the forest quest");
}

std::vector<Placement> SouthSupplies(uint32 item, uint32 count)
{
    auto rows = WorldDatabase.PQuery("SELECT g.id,g.guid,g.spawntimesecs,g.position_x,g.position_y,g.position_z,g.orientation,"
        "g.rotation0,g.rotation1,g.rotation2,g.rotation3 FROM gameobject g JOIN gameobject_template t ON t.entry=g.id JOIN gameobject_loot_template l ON l.Entry=t.Data1 "
        "WHERE g.map=860 AND l.Item=%u AND l.QuestRequired=1 ORDER BY g.id,g.guid", item);
    Require(rows && rows->GetRowCount() == count, "The original bamboo or bundle placements changed");
    std::vector<Placement> placements;
    do
    {
        auto row = rows->Fetch();
        placements.push_back({row[0].GetUInt32(), row[1].GetUInt32(), row[2].GetUInt32(),
            {row[3].GetFloat(), row[4].GetFloat(), row[5].GetFloat(), row[6].GetFloat()},
            G3D::Quat(row[7].GetFloat(), row[8].GetFloat(), row[9].GetFloat(), row[10].GetFloat())});
    } while (rows->NextRow());
    return placements;
}

void CheckSouthSupplies(LootPlayer& player, LootPlayer& visitor, Creature& giver, uint32 quest, uint32 item, uint32 needed, uint32 count, bool exclusiveFirst,
    bool expectAbandonRetention = false)
{
    auto locations = SouthSupplies(item, count);
    Map& map = *player.GetMap();
    AcceptSouth(player, giver, quest);
    if (exclusiveFirst)
    {
        Supply first(map, locations[0]);
        MoveGilneasPlayer(visitor, locations[0].position);
        OpenSupply(visitor, first);
#ifdef WANDERING_ISLE_SOUTH_BASELINE
        Require(visitor.GetLootGUID() == first.GetGUID() && first.getLootState() == GO_ACTIVATED && first.loot.isLooted(),
            "The previous exclusive source did not reproduce an empty foreign loot activation");
        ReleaseSupply(visitor, first);
#else
        Require(!visitor.GetLootGUID() && first.getLootState() == GO_READY, "The wrong player consumed exclusive forest supplies");
#endif
    }
    auto collect = [&](Placement const& location)
    {
        Supply source(map, location);
        MoveGilneasPlayer(player, location.position);
        OpenSupply(player, source);
        Require(TakeSupply(player, source, item), "The genuine forest supply did not yield its required item");
        ReleaseSupply(player, source);
    };
    collect(locations[1]);
    AbandonSupply(player, quest);
#ifdef WANDERING_ISLE_SOUTH_BASELINE
    Require(player.GetItemCount(item) == 1, "The previous quest did not reproduce abandoned item retention");
    if (exclusiveFirst)
        std::cout << "EXPECTED_BROKEN the exclusively quest-only bamboo source opened without its quest\n";
    std::cout << "EXPECTED_BROKEN abandoning the original quest retained its required item " << item << '\n';
#else
    if (expectAbandonRetention)
    {
        Require(player.GetItemCount(item) == 1, "The previous medical quest did not reproduce item retention");
        std::cout << "EXPECTED_BROKEN abandoning Medical Supplies kept its required bundle\n";
        return;
    }
    Require(!player.GetItemCount(item), "Abandoning a supply quest retained its exclusive item");
    AcceptSouth(player, giver, quest);
    for (uint32 index = 0; index < needed; ++index)
        collect(locations[index + 2]);
    Require(player.GetItemCount(item) == needed && player.GetQuestStatus(quest) == QUEST_STATUS_COMPLETE, "The real forest supplies did not reach their quota");
    RewardSouth(player, giver, quest);
    Require(!player.GetItemCount(item), "The native reward did not consume its supplies");
    std::cout << "PASS " << needed << " real supplies from " << count << " placements, abandon, retry and reward for " << quest << '\n';
#endif
}

void CheckSouthCombat(LootPlayer& pupil, Creature& giver, uint32 quest, uint32 entry, uint32 needed, uint32 placements)
{
    Map& map = *pupil.GetMap();
    auto spawns = WorldDatabase.PQuery("SELECT guid FROM creature WHERE map=860 AND id=%u ORDER BY ((phaseMask & 1)=0),guid", entry);
    Require(spawns && spawns->GetRowCount() == placements, "The original combat placements changed");
    AcceptSouth(pupil, giver, quest);
    SouthPet pet(pupil, map);
    for (uint32 index = 0; index < needed; ++index)
    {
        auto& opponent = FindSouthActor(map, spawns->Fetch()[0].GetUInt32());
        MoveGilneasPlayer(pupil, opponent.GetPosition());
        map.CreatureRelocation(&pet, opponent.GetPositionX(), opponent.GetPositionY(), opponent.GetPositionZ(), opponent.GetOrientation());
        map.MoveAllCreaturesInMoveList();
        Unit& attacker = index % 2 == 0 ? static_cast<Unit&>(pupil) : static_cast<Unit&>(pet);
        Require(attacker.IsValidAttackTarget(&opponent), "A placed opponent could not be attacked");
        attacker.DealDamage(&opponent, uint32(opponent.GetMaxHealth()) * 2);
        Tick(pupil, 150);
        Require(!opponent.IsAlive() && pupil.GetQuestObjectiveData(quest, entry) == int32(index + 1), "A real player or pet defeat did not progress the quest");
        spawns->NextRow();
    }
    RewardSouth(pupil, giver, quest);
    std::cout << "PASS " << needed << " actual player or pet defeats and original reward for " << quest << '\n';
}

void CheckSouth()
{
    Map map(860, 305, 0, DIFFICULTY_NONE);
    auto& korga = FindSouthActor(map, 245264);
    auto& wei = FindSouthActor(map, 245266);
    auto& makael = FindSouthActor(map, 245384);
    LootPlayer pupil(SupplySession(), map, 2979501, korga.GetPosition());
    LootPlayer visitor(SupplySession(), map, 2979502, korga.GetPosition());
    for (auto player : {&pupil, &visitor})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_PANDAREN_NEUTRAL);
        player->SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_HUNTER);
        player->setFactionForRace(RACE_PANDAREN_NEUTRAL);
        player->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);
        player->SetLevel(11);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 20000);
    }
    pupil.SetRewardedQuest(Bidden);
    CheckSouthSupplies(pupil, visitor, korga, Stocking, Bamboo, 10, 84, true);
#ifdef WANDERING_ISLE_SOUTH_BASELINE
    pupil.SetRewardedQuest(Stocking);
#endif
    AcceptSouth(pupil, korga, Wreck);
    RewardSouth(pupil, makael, Wreck);
    CheckSouthSupplies(pupil, visitor, makael, Handle, Explosives, 6, 35, false);
#ifndef WANDERING_ISLE_SOUTH_BASELINE
    CheckSouthCombat(pupil, wei, Preying, Tiger, 9, 98);
    std::cout << "PASS Makael handoff and nine distinct player or pet tiger defeats with Wei's reward\n";
    auto& aysa = FindSouthActor(map, 245389);
    auto& ji = FindSouthActor(map, 245135);
    CheckSouthCombat(pupil, aysa, EvilFromSeas, DarkenedHorror, 8, 40);
    AcceptSouth(pupil, aysa, UrgentNews);
    RewardSouth(pupil, ji, UrgentNews);
    std::cout << "PASS actual Urgent News handoff from Aysa to Ji at the crash site\n";
#ifdef WANDERING_ISLE_MEDICAL_BASELINE
    CheckSouthSupplies(pupil, visitor, ji, MedicalSupplies, MedicalCrateItem, 8, 31, false, true);
#else
    CheckSouthSupplies(pupil, visitor, ji, MedicalSupplies, MedicalCrateItem, 8, 31, false);
#endif
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{860, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{860, {}}});
        LoadGameObjectModelList(argv[2]);
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "The native southern quest test requires isolated characters");
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        LoadSouth();
        sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
        AddSC_go_scripts();
        sScriptMgr->SwapScriptContext(true);
        CheckSouth();
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
