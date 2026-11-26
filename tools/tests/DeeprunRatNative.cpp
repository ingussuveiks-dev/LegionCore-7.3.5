#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "ConditionMgr.h"
#include "GilneasQuestNative.h"
#include "SmartScriptMgr.h"

namespace
{
constexpr uint32 Tram = 369;
constexpr uint32 Roundup = 6661;
constexpr uint32 Monty = 12997;
constexpr uint32 Rat = 13016;
constexpr uint32 CapturedRat = 13017;
constexpr uint32 Flute = 17117;
constexpr uint32 Entice = 21050;
constexpr uint32 AffectedSpawn = 26;

Quest const* LoadRoundup()
{
    auto rows = WorldDatabase.Query(
#include "NativeDeeprunRat.inc"
    );
    Require(rows && rows->GetRowCount() == 2, "The original Roundup and historical override quest are required");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(26323);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto row = new Quest(rows->Fetch());
        quests[row->Id] = row;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(row->Id, row);
    } while (rows->NextRow());
    auto quest = quests[Roundup];
    auto oldFlags = WorldDatabase.Query("SELECT SpecialFlags FROM quest_template_addon WHERE ID=26322");
    Require(oldFlags && (oldFlags->Fetch()[0].GetUInt8() & QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT), "The baseline story event requires its original flag");
    quests[26322]->SetSpecialFlag(QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT);
    auto objectives = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=6661 ORDER BY StorageIndex");
    Require(objectives && objectives->GetRowCount() == 2 && objectives->Fetch()[4].GetUInt32() == CapturedRat &&
        objectives->Fetch()[5].GetUInt32() == 5, "Roundup requires five captured rats and the original flute");
    quest->LoadQuestObjective(objectives->Fetch());
    objectives->NextRow();
    Require(objectives->Fetch()[4].GetUInt32() == Flute && objectives->Fetch()[5].GetUInt32() == 1, "The original flute must be returned");
    quest->LoadQuestObjective(objectives->Fetch());
    auto addon = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,"
        "RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,"
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=6661");
    Require(bool(addon), "The original Roundup requirements are missing");
    quest->LoadQuestTemplateAddon(addon->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER | QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);

    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature WHERE map=369 AND id IN (12997,13016)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest=6661");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest=6661");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE smart_scripts AS SELECT * FROM legion_world.smart_scripts WHERE source_type=0 AND entryorguid IN (-26,13016)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE conditions AS SELECT * FROM legion_world.conditions "
        "WHERE (SourceTypeOrReferenceId=17 AND SourceEntry=21050) OR (SourceTypeOrReferenceId=22 AND SourceEntry=13016)");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    sObjectMgr->LoadCreatures();
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sConditionMgr->LoadConditions();
    sSmartScriptMgr->LoadSmartAIFromDB();
    return quest;
}

Creature* FindRatActor(Map& map, uint32 spawn, uint32 entry)
{
    auto data = sObjectMgr->GetCreatureData(spawn);
    Require(data && data->id == entry && data->mapid == Tram, "The original Deeprun spawn is missing");
    map.LoadGrid(data->posX, data->posY);
    auto actor = map.GetCreature(ObjectGuid::Create<HighGuid::Creature>(Tram, entry, spawn));
    Require(actor && actor->IsAlive(), "The original Deeprun actor did not load");
    return actor;
}

SpellCastResult PlayFlute(LootPlayer& player, Creature& rat)
{
    Position approach = rat.GetPosition();
    approach.m_positionX += 0.5f;
    approach.SetOrientation(approach.GetAngle(&rat));
    MoveGilneasPlayer(player, approach);
    Item* flute = player.GetItemByEntry(Flute);
    Require(flute && std::any_of(flute->GetTemplate()->Effects.begin(), flute->GetTemplate()->Effects.end(),
        [](auto effect) { return effect->SpellID == Entice; }), "The matching client flute must provide Entice Deeprun Rat");
    GameTime::UpdateGameTimers();
    SpellCastResult result = player.CastSpell(&rat, Entice, false, flute);
    std::cout << "CASE flute rat=" << rat.GetDBTableGUIDLow() << " spell=" << uint32(result)
        << " zone=" << player.GetCurrentZoneID() << " area=" << player.GetCurrentAreaID()
        << " LOS=" << player.IsWithinLOSInMap(&rat) << " distance=" << player.GetDistance(&rat) << '\n';
    Tick(player, 6000);
    return result;
}

void CheckRoundup(Quest const* quest)
{
    Map map(Tram, 300, 0, DIFFICULTY_NONE);
    auto monty = FindRatActor(map, 34, Monty);
    Require(monty->hasQuest(Roundup) && monty->hasInvolvedQuest(Roundup), "Monty must offer and receive the original quest");
    LootPlayer owner(SupplySession(), map, 666101, monty->GetPosition());
    LootPlayer bystander(SupplySession(), map, 666102, monty->GetPosition());
    for (LootPlayer* player : {&owner, &bystander})
    {
        player->setFactionForRace(player->getRace());
        player->InitDisplayIds();
        player->SetLevel(15);
        WorldPackets::Quest::QuestGiverAcceptQuest accept{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
        accept.QuestGiverGUID = monty->GetGUID();
        accept.QuestID = Roundup;
        player->GetSession()->HandleQuestGiverAcceptQuest(accept);
        Require(player->GetQuestStatus(Roundup) == QUEST_STATUS_INCOMPLETE && player->GetItemCount(Flute) == 1,
            "Monty must give one original flute and start an incomplete quest");
    }

    auto affected = FindRatActor(map, AffectedSpawn, Rat);
    SupplyMove(bystander, affected->GetPosition());
    Require(PlayFlute(owner, *affected) == SPELL_CAST_OK, "The real flute did not start its native spell on the affected rat");
#ifdef DEEPRUN_RAT_OBSERVE
    Require(!sSmartScriptMgr->GetScript(-26, SMART_SCRIPT_TYPE_CREATURE).empty() && affected->GetEntry() == Rat &&
        owner.GetQuestObjectiveData(Roundup, CapturedRat) == 0 && bystander.GetQuestObjectiveData(Roundup, CapturedRat) == 0,
        "The baseline override did not reproduce the missing catch");
    std::cout << "EXPECTED_BROKEN: wrong spawn override accepted Entice with no capture or credit\n";
#else
    Require(sSmartScriptMgr->GetScript(-26, SMART_SCRIPT_TYPE_CREATURE).empty() && affected->GetEntry() == CapturedRat &&
        owner.GetQuestObjectiveData(Roundup, CapturedRat) == 1 && bystander.GetQuestObjectiveData(Roundup, CapturedRat) == 0,
        "The affected rat did not convert and give only its flute user credit");
    Require(PlayFlute(owner, *affected) == SPELL_FAILED_BAD_TARGETS && PlayFlute(bystander, *affected) == SPELL_FAILED_BAD_TARGETS &&
        owner.GetQuestObjectiveData(Roundup, CapturedRat) == 1 && bystander.GetQuestObjectiveData(Roundup, CapturedRat) == 0,
        "A previously caught rat supplied another quest credit");
    affected->AI()->SpellHit(&bystander, sSpellMgr->GetSpellInfo(Entice));
    Require(bystander.GetQuestObjectiveData(Roundup, CapturedRat) == 0, "The spell hit must also reject a target converted before it landed");
    Require(PlayFlute(owner, *monty) == SPELL_FAILED_BAD_TARGETS, "Other creatures must reject the rat flute");
    for (uint32 spawn : {24u, 25u, 27u, 28u})
    {
        auto rat = FindRatActor(map, spawn, Rat);
        Require(PlayFlute(owner, *rat) == SPELL_CAST_OK && rat->GetEntry() == CapturedRat,
            "The shared template must still capture an ordinary rat");
    }
    Require(owner.GetQuestObjectiveData(Roundup, CapturedRat) == 5 && owner.CanRewardQuest(quest, false),
        "Five actual captures and the surviving flute must enable the reward");
    SupplyMove(owner, monty->GetPosition());
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestGiverGUID = monty->GetGUID();
    reward.QuestID = Roundup;
    reward.ItemChoiceID = 0;
    owner.GetSession()->HandleQuestGiverChooseReward(reward);
    Require(owner.GetQuestRewardStatus(Roundup) && !owner.GetItemCount(Flute), "Monty must issue the reward and retrieve the original flute");
    AbandonSupply(bystander, Roundup);
    auto fresh = FindRatActor(map, 20, Rat);
    MoveGilneasPlayer(bystander, fresh->GetPosition());
    Require(bystander.GetQuestStatus(Roundup) == QUEST_STATUS_NONE && !bystander.GetItemCount(Flute) &&
        bystander.CastSpell(fresh, Entice, false) != SPELL_CAST_OK && fresh->GetEntry() == Rat,
        "Abandoning must remove the flute and leave fresh rats available");
    MoveGilneasPlayer(bystander, monty->GetPosition());
    WorldPackets::Quest::QuestGiverAcceptQuest accept{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    accept.QuestGiverGUID = monty->GetGUID();
    accept.QuestID = Roundup;
    bystander.GetSession()->HandleQuestGiverAcceptQuest(accept);
    Require(PlayFlute(bystander, *fresh) == SPELL_CAST_OK && bystander.GetQuestObjectiveData(Roundup, CapturedRat) == 1,
        "A new acceptance must restore the original flute and capture path");
    std::cout << "CASE five distinct rats, rejected duplicate and wrong targets, Monty reward, abandon and reacceptance\n";
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{Tram, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{Tram, {}}});
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "The isolated login pool is required");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "The isolated character pool is required");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "The test must use isolated fixture characters");
        HashMapHolder<Player>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckRoundup(LoadRoundup());
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
