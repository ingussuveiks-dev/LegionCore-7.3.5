#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "GilneasQuestNative.h"

namespace
{
constexpr uint32 EtchedNote = 3092;
constexpr uint32 Prisoners = 24852;
constexpr uint32 NoteItem = 9565;

Quest const* LoadNote()
{
    auto rows = WorldDatabase.Query(
#include "NativeEtchedNote.inc"
    );
    Require(rows && rows->GetRowCount() == 3, "The original Camp Narache quest sequence is required");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(27022);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new Quest(rows->Fetch());
        quests[quest->Id] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->Id, quest);
    } while (rows->NextRow());

    Quest* note = quests[EtchedNote];
    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=3092");
    auto addon = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,"
        "RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,"
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=3092");
    Require(objective && objective->GetRowCount() == 1 && objective->Fetch()[4].GetUInt32() == NoteItem && addon,
        "The original note item and class quest prerequisites are required");
    note->LoadQuestObjective(objective->Fetch());
    note->LoadQuestTemplateAddon(addon->Fetch());
    note->SetSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER);
    note->prevQuests.push_back(Prisoners);

    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature WHERE guid IN (169364,169370)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest IN (3092,27021)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest=3092");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE quest_poi AS SELECT * FROM legion_world.quest_poi WHERE QuestID=3092");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE quest_poi_points AS SELECT * FROM legion_world.quest_poi_points WHERE QuestID=3092");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    sObjectMgr->LoadCreatures();
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    if (WorldDatabase.Query("SELECT 1 FROM quest_poi_points LIMIT 1"))
        sQuestDataStore->LoadQuestPOI();
    return note;
}

Creature* LoadTauren(Map& map, uint32 entry, uint32 spawn)
{
    auto data = sObjectMgr->GetCreatureData(spawn);
    Require(data && data->id == entry && data->mapid == 1, "The verified Camp Narache placement is required");
    map.LoadGrid(data->posX, data->posY);
    auto actor = map.GetCreature(ObjectGuid::Create<HighGuid::Creature>(1, entry, spawn));
    Require(actor && actor->IsAlive(), "The required Camp Narache NPC must load");
    return actor;
}

void CheckNote(Quest const* note)
{
    Map map(1, 300, 0, DIFFICULTY_NONE);
    Creature* rohaku = LoadTauren(map, 44927, 169370);
    Creature* lanka = LoadTauren(map, 3061, 169364);
    Require(rohaku->hasQuest(EtchedNote) && lanka->hasInvolvedQuest(EtchedNote), "The actual giver and receiver must retain the original quest links");
    LootPlayer player(SupplySession(), map, 309201, rohaku->GetPosition());
    player.SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_TAUREN);
    player.SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_HUNTER);
    player.setFactionForRace(player.getRace());
    player.InitDisplayIds();
    player.SetLevel(3);
    MoveGilneasPlayer(player, rohaku->GetPosition());
    Require(!player.CanTakeQuest(note, false), "The prisoner rescue is still required");
    player.SetRewardedQuest(Prisoners);

    WorldPackets::Quest::QuestGiverAcceptQuest accept{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    accept.QuestGiverGUID = rohaku->GetGUID();
    accept.QuestID = EtchedNote;
    player.GetSession()->HandleQuestGiverAcceptQuest(accept);
    Require(player.GetItemCount(NoteItem) == 1 && player.FindQuestSlot(EtchedNote) < MAX_QUEST_LOG_SIZE,
        "Rohaku must provide the actual note to the eligible hunter");
    AbandonSupply(player, EtchedNote);
    Require(!player.GetItemCount(NoteItem), "Abandoning Etched Note must remove its source item");
    player.GetSession()->HandleQuestGiverAcceptQuest(accept);
    Require(player.GetItemCount(NoteItem) == 1, "Reacceptance must give a fresh note");

    MoveGilneasPlayer(player, lanka->GetPosition());
    Require(player.CanInteractWithQuestGiver(lanka) && player.CanRewardQuest(note, false), "Lanka must receive the real note");
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestGiverGUID = lanka->GetGUID();
    reward.QuestID = EtchedNote;
    reward.ItemChoiceID = 0;
    player.GetSession()->HandleQuestGiverChooseReward(reward);
    Require(player.GetQuestRewardStatus(EtchedNote) && !player.GetItemCount(NoteItem), "Lanka must reward the quest and remove only its provided note");
    std::cout << "CASE Rohaku admission, note lifecycle and Lanka reward at " << lanka->GetZoneId() << '/' << lanka->GetAreaId() << '\n';

    for (WorldMapAreaEntry const* area : sWorldMapAreaStore)
        if (area->ID == 9 || area->ID == 890 || area->AreaID == lanka->GetZoneId() || area->AreaID == lanka->GetAreaId())
            std::cout << "CASE world map area=" << area->ID << " map=" << area->MapID << " zone=" << area->AreaID << '\n';
    auto markers = sQuestDataStore->GetQuestPOIVector(EtchedNote);
    Require(markers && markers->size() == 1, "The quest needs the original single receiver marker");
    auto const& marker = markers->front();
    auto area = sWorldMapAreaStore.LookupEntry(marker.WorldMapAreaID);
    Require(marker.MapID == 1 && marker.ObjectiveIndex == -1 && area && area->MapID == 1 && area->AreaID == lanka->GetZoneId() &&
        marker.points.size() == 1 && marker.points.front().X == int32(std::lround(lanka->GetPositionX())) &&
        marker.points.front().Y == int32(std::lround(lanka->GetPositionY())), "The receiver marker must match the current Camp Narache placement and map page");
    std::cout << "CASE Etched Note map loads its real receiver coordinate\n";
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
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "The isolated login pool is required");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "The isolated character pool is required");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "The test must use isolated fixture characters");
        HashMapHolder<Player>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckNote(LoadNote());
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
