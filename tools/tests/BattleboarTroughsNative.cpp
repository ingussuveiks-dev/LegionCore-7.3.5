#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "SmartAI.h"
#include "SmartScriptMgr.h"
#include <thread>

void AddSC_SmartScripts();

namespace
{
constexpr uint32 FeedQuest = 14461;
constexpr uint32 Torch = 49539;
constexpr uint32 ThrowTorch = 69228;
constexpr uint32 Adana = 36694;
constexpr std::array<uint32, 3> Troughs = {36727, 37155, 37156};

Quest const* LoadTroughQuest()
{
    auto row = WorldDatabase.Query(
#include "NativeBattleboarTroughs.inc"
    );
    Require(bool(row), "Missing Feed of Evil");
    auto quest = new Quest(row->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    quests[FeedQuest] = quest;
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->Id, quest);
    auto objectives = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=14461 ORDER BY StorageIndex");
    Require(objectives && objectives->GetRowCount() == Troughs.size(), "Expected the three native trough objectives");
    for (uint32 entry : Troughs)
    {
        auto fields = objectives->Fetch();
        Require(fields[2].GetUInt8() == QUEST_OBJECTIVE_MONSTER && fields[4].GetUInt32() == entry && fields[5].GetUInt32() == 1,
            "The quest does not require each exact trough independently");
        quest->LoadQuestObjective(fields);
        objectives->NextRow();
    }
    auto addon = WorldDatabase.Query(
        "SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,RewardMailTemplateID,RewardMailDelay,"
        "RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMinRepValue,RequiredMaxRepFaction,RequiredMaxRepValue,"
        "ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=14461");
    Require(bool(addon), "Missing Feed of Evil quest addon");
    quest->LoadQuestTemplateAddon(addon->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
    Require(quest->SourceItemId == Torch && quest->SourceItemIdCount == 1 && !quest->PackageID, "Expected one reusable torch");
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    AddSC_SmartScripts();
    sScriptMgr->SwapScriptContext(true);
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE trough_scripts AS SELECT * FROM smart_scripts WHERE source_type=0 AND entryorguid IN(36727,37155,37156)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE smart_scripts AS SELECT * FROM trough_scripts");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE trough_ender AS SELECT * FROM creature_questender WHERE quest=14461");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM trough_ender");
#ifndef BATTLEBOAR_TROUGHS_BASELINE
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE trough_conditions AS SELECT * FROM conditions WHERE SourceTypeOrReferenceId IN(13,17) AND SourceEntry=69228");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE conditions AS SELECT * FROM trough_conditions");
#endif
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sSmartScriptMgr->LoadSmartAIFromDB();
#ifndef BATTLEBOAR_TROUGHS_BASELINE
    sConditionMgr->LoadConditions();
    auto spell = sSpellMgr->GetSpellInfo(ThrowTorch);
    for (SpellEffIndex effect : {EFFECT_0, EFFECT_1, EFFECT_2})
        Require(spell->GetEffect(effect)->ImplicitTargetConditions && spell->GetEffect(effect)->ImplicitTargetConditions->size() == 6,
            "The three native target selectors did not load the living trough list");
    Require(sConditionMgr->GetConditionsForNotGroupedEntry(CONDITION_SOURCE_TYPE_SPELL, ThrowTorch).size() == 2,
        "The active quest and torch check did not load");
#endif
    return quest;
}

struct TroughActor : Actor
{
    using Actor::Actor;
    void AddToWorld() override
    {
        sObjectAccessor->AddObject(static_cast<Creature*>(this));
        Unit::AddToWorld();
        Require(AIM_Initialize(new SmartAI(this)), "The original trough SmartAI did not load");
    }
};

struct TorchWitness : NullCreatureAI
{
    using NullCreatureAI::NullCreatureAI;
    void SpellHit(Unit*, SpellInfo const* spell) override { if (spell && spell->Id == ThrowTorch) Hit = true; }
    bool Hit = false;
};

SpellCastResult UseTorch(LootPlayer& player, Unit* selected = nullptr)
{
    auto item = player.GetItemByEntry(Torch);
    Require(item, "The test needs the real carried torch");
    return player.CastSpell(selected, ThrowTorch, false, item);
}

void WaitForTorch()
{
    auto spell = sSpellMgr->GetSpellInfo(ThrowTorch);
    uint32 recovery = std::max(spell->GetRecoveryTime(), spell->Cooldowns.StartRecoveryTime);
    if (recovery)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(recovery + 100));
        GameTime::UpdateGameTimers();
    }
}

void CheckTroughs(Quest const* quest)
{
    auto adana = WorldDatabase.Query("SELECT guid,position_x,position_y,position_z,orientation FROM creature "
        "WHERE id=36694 AND map=1 AND zoneId=6452 AND guid=169368");
    auto markers = WorldDatabase.Query("SELECT guid,position_x,position_y,position_z,orientation,id FROM creature "
        "WHERE id IN(36727,37155,37156) AND map=1 AND zoneId=6452 AND areaId=4879 ORDER BY id");
    Require(adana && markers && markers->GetRowCount() == Troughs.size() &&
        WorldDatabase.Query("SELECT 1 FROM creature_queststarter WHERE quest=14461 AND id=36694"),
        "Missing the original Adana offer or trough world placements");
    auto point = [](Field* row) { return Position{row[1].GetFloat(), row[2].GetFloat(), row[3].GetFloat(), row[4].GetFloat()}; };
    Map map(1, 530, 0, DIFFICULTY_NONE);
    Position receiverPoint = point(adana->Fetch());
    Actor receiver(&map, Adana, adana->Fetch()[0].GetUInt32(), receiverPoint);
    Require(map.AddToMap(static_cast<Creature*>(&receiver)), "Could not publish Adana");
    LootPlayer owner(SupplySession(), map, 14461, receiverPoint), other(SupplySession(), map, 14462, receiverPoint);
    for (auto player : {&owner, &other})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_TAUREN);
        player->setFactionForRace(RACE_TAUREN);
        player->InitDisplayIds();
        player->SetLevel(7);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
    }
    GiveSupply(other, Torch, 1);
    owner.AddQuestAndCheckCompletion(quest, &receiver);
    Require(owner.GetItemCount(Torch) == 1 && !owner.CanRewardQuest(quest, false), "Acceptance should provide only gear");
    for (uint32 entry : Troughs)
    {
        auto row = markers->Fetch();
        Require(row[5].GetUInt32() == entry, "Unexpected ordering of the original troughs");
        Position location = point(row);
        Position start{location.GetPositionX() - 7.0f, location.GetPositionY(), location.GetPositionZ(), 0.0f};
        SupplyMove(owner, start);
        SupplyMove(other, start);
        TroughActor trough(&map, entry, row[0].GetUInt32(), location);
        Require(map.AddToMap(static_cast<Creature*>(&trough)), "Could not publish an actual trough");
        if (entry == Troughs.front())
        {
            Position bystander{start.GetPositionX() + 2.0f,start.GetPositionY(),start.GetPositionZ(),0.0f};
            Actor stranger(&map, Adana, 65500, bystander);
            Require(map.AddToMap(static_cast<Creature*>(&stranger)), "Could not publish the nearby unrelated actor");
            auto witness = new TorchWitness(&stranger);
            Require(stranger.AIM_Initialize(witness), "Could not observe the unrelated target");
#ifdef BATTLEBOAR_TROUGHS_BASELINE
            Require(UseTorch(other, &stranger) == SPELL_CAST_OK, "Expected the old torch cast without the active quest");
            Tick(other, 2500);
            Require(witness->Hit && !other.GetQuestObjectiveData(FeedQuest, entry),
                "Expected the unfiltered native throw to reach the unrelated actor without a quest");
            std::cout << "EXPECTED_BROKEN the ungated native throw hit an unrelated creature without a quest\n";
            return;
#endif
            Require(UseTorch(other, &trough) != SPELL_CAST_OK, "A leftover torch was usable without the quest");
            Require(UseTorch(owner, &stranger) == SPELL_CAST_OK, "Could not exercise a throw with a closer unrelated actor");
            Tick(owner, 2500);
            Require(!witness->Hit && owner.GetQuestObjectiveData(FeedQuest, entry) == 1,
                "The native selector chose the unrelated actor instead of the actual trough");
            AbandonSupply(owner, FeedQuest);
            owner.AddQuestAndCheckCompletion(quest, &receiver);
            WaitForTorch();
            SpellCastResult retry = UseTorch(owner, &trough);
            if (retry != SPELL_CAST_OK)
                std::cout << "CASE repeat torch result=" << retry << " status=" << owner.GetQuestStatus(FeedQuest)
                    << " item=" << owner.GetItemCount(Torch) << '\n';
            Require(retry == SPELL_CAST_OK, "Could not launch the original throwable spell");
            Tick(owner, 50);
            AbandonSupply(owner, FeedQuest);
            Require(!owner.GetItemCount(Torch), "Abandon did not remove the torch");
            owner.AddQuestAndCheckCompletion(quest, &receiver);
            Tick(owner, 2500);
            Require(!owner.GetQuestObjectiveData(FeedQuest, entry) && !witness->Hit, "A stale projectile advanced the new quest attempt");
            WaitForTorch();
        }
        Require(UseTorch(owner, &trough) == SPELL_CAST_OK, "A legal trough did not accept the native throw");
        Tick(owner, 2500);
        Require(owner.GetQuestObjectiveData(FeedQuest, entry) == 1 && !other.GetQuestObjectiveData(FeedQuest, entry) &&
            owner.GetItemCount(Torch) == 1, "The actual trough did not grant its own credit and retain the torch");
        WaitForTorch();
        if (entry == Troughs[1])
        {
            Require(UseTorch(owner, &trough) == SPELL_CAST_OK, "Could not exercise an already credited trough");
            Tick(owner, 2500);
            Require(owner.GetQuestObjectiveData(FeedQuest, entry) == 1 && !owner.CanRewardQuest(quest, false),
                "A repeat hit replaced another required trough");
            WaitForTorch();
        }
        markers->NextRow();
    }
    SupplyMove(owner, receiverPoint);
    Require(owner.CanRewardQuest(quest, false) && UseTorch(owner) != SPELL_CAST_OK, "The completed quest still accepted a throw");
    WorldPackets::Quest::QuestGiverChooseReward packet{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    packet.QuestGiverGUID = receiver.GetGUID();
    packet.QuestID = FeedQuest;
    packet.ItemChoiceID = 0;
    owner.GetSession()->HandleQuestGiverChooseReward(packet);
    Require(owner.GetQuestRewardStatus(FeedQuest) && !owner.GetItemCount(Torch), "Adana's native handoff did not remove the torch");
    other.DestroyItemCount(Torch, 1, true);
    other.AddQuestAndCheckCompletion(quest, &receiver);
    AbandonSupply(other, FeedQuest);
    Require(!other.GetItemCount(Torch), "Abandon left the supplied torch");
    std::cout << "PASS three real troughs, native throw target filtering, stale missile, turn-in and abandon cleanup\n";
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
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "The trough test requires the isolated character schema");
        auto quest = LoadTroughQuest();
        sObjectMgr->LoadPlayerInfo();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckTroughs(quest);
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
