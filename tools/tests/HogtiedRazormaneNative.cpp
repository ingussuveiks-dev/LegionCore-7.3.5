#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "SmartAI.h"
#include "SmartScriptMgr.h"
#include "TemporarySummon.h"

void AddSC_SmartScripts();

namespace
{
constexpr uint32 CaptureQuest = 13961;
constexpr uint32 NetItem = 46722;
constexpr uint32 NetSpell = 65580;
constexpr uint32 Pillager = 34503;
constexpr uint32 Captive = 34514;
constexpr uint32 Togrik = 34513;

Quest const* LoadCaptureQuest()
{
    auto row = WorldDatabase.Query(
#include "NativeHogtiedRazormane.inc"
    );
    Require(bool(row), "Missing Drag it Out of Them");
    auto quest = new Quest(row->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    quests[CaptureQuest] = quest;
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->Id, quest);
    auto objectives = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=13961 ORDER BY StorageIndex");
    Require(objectives && objectives->GetRowCount() == 2 && objectives->Fetch()[2].GetUInt8() == QUEST_OBJECTIVE_ITEM &&
        objectives->Fetch()[4].GetUInt32() == NetItem && objectives->Fetch()[5].GetUInt32() == 1,
        "Expected return of one net");
    quest->LoadQuestObjective(objectives->Fetch());
    objectives->NextRow();
    Require(objectives->Fetch()[2].GetUInt8() == QUEST_OBJECTIVE_AREATRIGGER && objectives->Fetch()[4].GetInt32() == -1,
        "Expected the authored non-geometric arrival objective");
    quest->LoadQuestObjective(objectives->Fetch());
    auto addon = WorldDatabase.Query(
        "SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,RewardMailTemplateID,RewardMailDelay,"
        "RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMinRepValue,RequiredMaxRepFaction,RequiredMaxRepValue,"
        "ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=13961");
    Require(bool(addon), "Missing Drag it Out of Them addon");
    quest->LoadQuestTemplateAddon(addon->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER | QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT);
    Require(quest->SourceItemId == NetItem && quest->SourceItemIdCount == 1, "Expected one supplied net");
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sSpellMgr->LoadSpellBonusess();
    sSpellMgr->LoadSpellThreats();
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    AddSC_SmartScripts();
    sScriptMgr->SwapScriptContext(true);

    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE hogtie_scripts AS SELECT * FROM smart_scripts WHERE source_type=0 AND entryorguid IN(34503,34513,34514)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE smart_scripts AS SELECT * FROM hogtie_scripts");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE hogtie_receiver AS SELECT * FROM creature_questender WHERE quest=13961");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM hogtie_receiver");
#ifndef HOGTIED_RAZORMANE_BASELINE
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE hogtie_conditions AS SELECT * FROM conditions WHERE SourceTypeOrReferenceId=17 AND SourceEntry=65580");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE conditions AS SELECT * FROM hogtie_conditions");
#endif
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sSmartScriptMgr->LoadSmartAIFromDB();
#ifndef HOGTIED_RAZORMANE_BASELINE
    sConditionMgr->LoadConditions();
    Require(sConditionMgr->GetConditionsForNotGroupedEntry(CONDITION_SOURCE_TYPE_SPELL, NetSpell).size() == 4,
        "The four net cast gates did not load");
#endif
    return quest;
}

struct CaptureActor : Actor
{
    using Actor::Actor;
    void AddToWorld() override
    {
        sObjectAccessor->AddObject(static_cast<Creature*>(this));
        Unit::AddToWorld();
        Require(AIM_Initialize(new SmartAI(this)), "The actor's original SmartAI did not initialize");
    }
};

struct WorldPlacement
{
    uint32 Guid;
    Position Point;
};

std::vector<WorldPlacement> CaptureLocations(uint32 entry)
{
    auto rows = WorldDatabase.PQuery("SELECT guid,position_x,position_y,position_z,orientation FROM creature "
        "WHERE id=%u AND map=1 AND zoneId=17 AND areaId=1704 ORDER BY guid", entry);
    Require(bool(rows), "Missing original Grol'dom Farm placement");
    std::vector<WorldPlacement> result;
    do
    {
        auto row = rows->Fetch();
        result.push_back({row[0].GetUInt32(), {row[1].GetFloat(), row[2].GetFloat(), row[3].GetFloat(), row[4].GetFloat()}});
    } while (rows->NextRow());
    return result;
}

void FacePillager(LootPlayer& player, Creature const& pillager)
{
    Position point = pillager.GetPosition();
    point.m_positionX -= 8.0f;
    point.SetOrientation(0.0f);
    SupplyMove(player, point);
}

SpellCastResult CastNet(LootPlayer& player, Creature& pillager)
{
    Item* net = player.GetItemByEntry(NetItem);
    Require(net, "The test must use a real carried net");
    return player.CastSpell(&pillager, NetSpell, false, net);
}

TempSummon* FindCaptive(LootPlayer& player)
{
    for (ObjectGuid const& guid : *player.GetSummonList(Captive))
        if (Creature* capture = ObjectAccessor::GetCreature(player, guid))
            if (!capture->IsDespawn())
                return capture->ToTempSummon();
    return nullptr;
}

void CheckCapture(Quest const* quest)
{
    auto receivers = CaptureLocations(Togrik);
    auto targets = CaptureLocations(Pillager);
    Require(receivers.size() == 1 && targets.size() == 9 &&
        WorldDatabase.Query("SELECT 1 FROM creature_queststarter WHERE quest=13961 AND id=34513"),
        "Missing Togrik's actual offer or known world targets");
    auto const& at = receivers.front();
    Map map(1, 400, 0, DIFFICULTY_NONE);
    CaptureActor receiver(&map, Togrik, at.Guid, at.Point);
    Require(map.AddToMap(static_cast<Creature*>(&receiver)), "Could not publish Togrik");
    LootPlayer owner(SupplySession(), map, 13961, at.Point), other(SupplySession(), map, 13962, at.Point);
    for (auto player : {&owner, &other})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_ORC);
        player->setFactionForRace(RACE_ORC);
        player->InitDisplayIds();
        player->SetLevel(16);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
    }

    CaptureActor first(&map, Pillager, targets[0].Guid, targets[0].Point);
    CaptureActor second(&map, Pillager, targets[1].Guid, targets[1].Point);
    Require(map.AddToMap(static_cast<Creature*>(&first)) && map.AddToMap(static_cast<Creature*>(&second)),
        "Could not publish the existing pillagers");
    FacePillager(other, first);
    GiveSupply(other, NetItem, 1);
#ifdef HOGTIED_RAZORMANE_BASELINE
    Require(CastNet(other, first) == SPELL_CAST_OK, "Expected the original ungated net cast");
    Tick(other, 2500);
    first.AI()->UpdateAI(10);
    Require(!first.IsVisible() && other.GetQuestStatus(CaptureQuest) == QUEST_STATUS_NONE,
        "Expected the original graph to hide the shared pillager without an active quest");
    std::cout << "EXPECTED_BROKEN a net without an active quest removed a shared Razormane Pillager\n";
    return;
#endif
    Require(CastNet(other, first) != SPELL_CAST_OK && !first.IsDespawn(), "A leftover net was usable without the quest");
    other.DestroyItemCount(NetItem, 1, true);
    owner.AddQuestAndCheckCompletion(quest, &receiver);
    Require(owner.GetItemCount(NetItem) == 1 && !owner.CanRewardQuest(quest, false), "Acceptance must grant only the net");
    Require(CastNet(owner, receiver) != SPELL_CAST_OK, "The net accepted the wrong creature");
    FacePillager(owner, first);
    Require(CastNet(owner, first) == SPELL_CAST_OK, "The net could not launch on its native target");
    Tick(owner, 260);
    AbandonSupply(owner, CaptureQuest);
    Require(!owner.GetItemCount(NetItem), "Abandon did not remove the supplied net");
    owner.AddQuestAndCheckCompletion(quest, &receiver);
    Tick(owner, 1700);
    Require(!first.IsDespawn() && !FindCaptive(owner), "A projectile from the abandoned attempt removed a new attempt's pillager");

    auto capture = [&](LootPlayer& player, CaptureActor& pillager)
    {
        FacePillager(player, pillager);
        Require(CastNet(player, pillager) == SPELL_CAST_OK, "The accepted net cast failed");
        Tick(player, 2000);
        TempSummon* captive = FindCaptive(player);
        Require(captive && captive->GetSummoner() == &player && captive->HasQuestObjectScope() &&
            captive->GetUInt32Value(UNIT_FIELD_CREATED_BY_SPELL) == 65595 && pillager.IsDespawn(),
            "The accepted net must replace the hit pillager with a real owner-bound captive");
        Require(captive->HasAura(58951) && player.GetItemCount(NetItem) == 1 && !player.CanRewardQuest(quest, false),
            "Capture must feign death, retain the net and await physical delivery");
        Require(captive->GetMotionMaster()->GetCurrentMovementGeneratorType() == FOLLOW_MOTION_TYPE,
            "The original SmartAI summon must actually enter native follow movement");
        return captive;
    };
    TempSummon* captive = capture(owner, first);
    FacePillager(owner, second);
    Require(CastNet(owner, second) == SPELL_CAST_OK, "Could not exercise a subsequent net");
    Tick(owner, 2000);
    Require(!second.IsDespawn() && FindCaptive(owner) == captive, "A second net consumed another target without a new capture");
    other.AddQuestAndCheckCompletion(quest, &receiver);
    TempSummon* otherCaptive = capture(other, second);
    Require(otherCaptive != captive && !captive->CanTargetQuestObject(&other) &&
        !otherCaptive->CanTargetQuestObject(&owner), "The two players must have independent private captives");

    captive->NearTeleportTo(at.Point.GetPositionX(), at.Point.GetPositionY(), at.Point.GetPositionZ(), at.Point.GetOrientation());
    map.MoveAllCreaturesInMoveList();
    Require(captive->FindNearestCreature(Togrik, 7.0f, true) == &receiver, "The captive never entered the actual handoff range");
    receiver.AI()->UpdateAI(4000);
    Require(!owner.CanRewardQuest(quest, false), "Togrik must not accept a delivery with the captor absent");
    SupplyMove(owner, at.Point);
    receiver.AI()->UpdateAI(4000);
    std::cout << "CASE delivery status=" << owner.GetQuestStatus(CaptureQuest)
        << " arrival=" << owner.GetQuestObjectiveData(CaptureQuest, uint32(-1))
        << " net=" << owner.GetQuestObjectiveData(CaptureQuest, NetItem)
        << " nearbyOwner=" << captive->IsWithinDistInMap(&owner, 12.0f)
        << " nearbyOther=" << captive->IsWithinDistInMap(&other, 12.0f) << '\n';
    Require(owner.GetQuestStatus(CaptureQuest) == QUEST_STATUS_COMPLETE && owner.GetQuestObjectiveData(CaptureQuest, uint32(-1)) == 1 &&
        owner.CanRewardQuest(quest, false) && other.GetQuestStatus(CaptureQuest) == QUEST_STATUS_INCOMPLETE,
        "The real Togrik arrival did not advance only the responsible player");
    captive->AI()->UpdateAI(10);
    captive->AI()->UpdateAI(5010);
    map.RemoveAllObjectsInRemoveList();
    Require(!FindCaptive(owner) && !owner.HasAura(65596) && FindCaptive(other) == otherCaptive, "Delivery cleaned the wrong captive or rope");

    WorldPackets::Quest::QuestGiverChooseReward packet{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    packet.QuestGiverGUID = receiver.GetGUID();
    packet.QuestID = CaptureQuest;
    packet.ItemChoiceID = 0;
    owner.GetSession()->HandleQuestGiverChooseReward(packet);
    Require(owner.GetQuestRewardStatus(CaptureQuest) && !owner.GetItemCount(NetItem), "Togrik's native reward must remove the net");
    AbandonSupply(other, CaptureQuest);
    map.RemoveAllObjectsInRemoveList();
    Require(!other.GetItemCount(NetItem) && !FindCaptive(other) && !other.HasAura(65596), "Abandon must remove the net, captive and rope");
    std::cout << "PASS real net hit, private native captives, concurrent owners, native Togrik arrival, reward and abandon\n";
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
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "The capture test requires the isolated character schema");
        auto quest = LoadCaptureQuest();
        sObjectMgr->LoadPlayerInfo();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckCapture(quest);
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
