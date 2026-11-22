#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "GameObjectPackets.h"
#include "GilneasQuestNative.h"
#include "CreatureTextMgr.h"
#include "SmartScriptMgr.h"
#include <thread>

namespace
{
constexpr uint32 PrisonersQuest = 26646;
constexpr uint32 PreviousQuest = 26640;
constexpr uint32 Key = 59261;
constexpr uint32 Kit = 60384;
constexpr uint32 EquipKit = 82580;
constexpr uint32 OpenPen = 81515;
constexpr uint32 Pen = 204441;
constexpr uint32 PenSpawn = 1379;
constexpr uint32 Credit = 43574;
constexpr uint32 Keeshan = 43458;
constexpr uint32 KeeshanSpawn = 343676;
constexpr uint32 Captive = 43572;
constexpr std::array<uint32, 7> CaptiveSpawns{39156, 39157, 39158, 39159, 39160, 39161, 39162};

Quest const* LoadPrisoners()
{
    auto rows = WorldDatabase.Query(
#include "NativeBlackrockPrisoners.inc"
    );
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    Require(rows && rows->GetRowCount() == 2, "The original Prisoners of War prerequisite is missing");
    do
    {
        auto quest = new Quest(rows->Fetch());
        quests[quest->GetQuestId()] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->GetQuestId(), quest);
    } while (rows->NextRow());
    auto quest = quests[PrisonersQuest];
    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged"
        " FROM quest_objectives WHERE QuestID=26646");
    auto addon = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,"
        "RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,"
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=26646");
    Require(objective && objective->GetRowCount() == 1 && objective->Fetch()[4].GetUInt32() == Credit &&
        objective->Fetch()[5].GetUInt32() == 1 && addon && addon->Fetch()[4].GetUInt32() == PreviousQuest,
        "The authored rescue objective or story prerequisite changed");
    quest->LoadQuestObjective(objective->Fetch());
    quest->LoadQuestTemplateAddon(addon->Fetch());
    quest->prevQuests.push_back(PreviousQuest);
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL);
    Require(quest->SourceItemId == Kit && quest->ItemDrop[3] == Key, "Wrong native kit or key cleanup data");

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    sObjectMgr->LoadEquipmentTemplates();
    sSpellMgr->LoadSpellBonusess();
    sSpellMgr->LoadSpellThreats();
    auto key = sObjectMgr->GetItemTemplate(Key);
    Require(key && std::any_of(key->Effects.begin(), key->Effects.end(), [](ItemEffectEntry const* effect)
        { return effect && effect->SpellID == OpenPen; }), "The carried key does not have its native open-lock effect");
    auto pouch = WorldDatabase.Query("SELECT COUNT(*) FROM gameobject g JOIN gameobject_template t ON t.entry=g.id "
        "JOIN gameobject_loot_template l ON l.Entry=t.Data1 WHERE g.guid=105634 AND g.id=204389 AND g.map=0 "
        "AND g.areaId=997 AND l.Item=59261 AND l.QuestRequired=1");
    Require(pouch && pouch->Fetch()[0].GetUInt32() == 1, "The original key pouch loot source is missing");

    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature "
        "WHERE guid IN(343676,39156,39157,39158,39159,39160,39161,39162)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE smart_scripts AS SELECT * FROM legion_world.smart_scripts "
        "WHERE (source_type=0 AND entryorguid IN(43572,43458)) OR (source_type=1 AND entryorguid=204441)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_text AS SELECT * FROM legion_world.creature_text WHERE CreatureID IN(43458,43572)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE conditions AS SELECT * FROM legion_world.conditions WHERE SourceTypeOrReferenceId=17 AND SourceEntry=82580");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_area AS SELECT * FROM legion_world.spell_area WHERE spell=82580");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest=26646");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest=26646");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sSmartScriptMgr->LoadSmartAIFromDB();
    sCreatureTextMgr->LoadCreatureTexts();
    sConditionMgr->LoadConditions();
    sSpellMgr->LoadSpellAreas();
    sObjectMgr->LoadCreatures();
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    return quest;
}

struct PlacedActor : Creature
{
    PlacedActor(Map& map, uint32 spawn) : Creature(false)
    {
        auto data = sObjectMgr->GetCreatureData(spawn);
        Require(data, "Missing native captive or questgiver placement");
        map.LoadGrid(data->posX, data->posY);
        Require(LoadCreatureFromDB(spawn, &map, true), "Could not load the actual Redridge actor");
    }

    ~PlacedActor()
    {
        CombatStop(true);
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<Creature*>(this), false);
    }
};

struct PlacedPen : GameObject
{
    explicit PlacedPen(Map& map, uint32 spawn = PenSpawn)
    {
        auto result = WorldDatabase.Query("SELECT position_x,position_y,position_z,orientation,rotation0,rotation1,rotation2,rotation3 "
            "FROM gameobject WHERE guid=1379 AND id=204441 AND map=0 AND areaId=997");
        Require(bool(result), "Missing the native Blackrock Holding Pen");
        auto row = result->Fetch();
        Position position(row[0].GetFloat(), row[1].GetFloat(), row[2].GetFloat(), row[3].GetFloat());
        G3D::Quat rotation(row[4].GetFloat(), row[5].GetFloat(), row[6].GetFloat(), row[7].GetFloat());
        m_DBTableGuid = spawn;
        map.LoadGrid(position.GetPositionX(), position.GetPositionY());
        Require(Create(spawn, Pen, &map, 1, position, rotation, 255, GO_STATE_READY) &&
            map.AddToMap(static_cast<GameObject*>(this)), "Could not publish the actual Blackrock pen");
        SetLootState(GO_READY);
        auto lock = sLockStore.LookupEntry(GetGOInfo()->GetLockId());
        Require(lock && lock->ID == 1930 && lock->Type[0] == LOCK_KEY_ITEM && lock->Index[0] == Key,
            "The real Blackrock pen does not require its matching key");
    }

    ~PlacedPen()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<GameObject*>(this), false);
        ResetMap();
    }
};

void ClickPen(LootPlayer& player, PlacedPen& pen)
{
    WorldPacket wire(CMSG_GAME_OBJ_USE);
    wire << pen.GetGUID();
    WorldPackets::GameObject::GameObjectUse packet(std::move(wire));
    packet.Read();
    player.GetSession()->HandleGameObjectUse(packet);
}

void CheckPrisoners(Quest const* quest, bool baseline)
{
    sObjectMgr->RemoveCreatureFromGrid(KeeshanSpawn, sObjectMgr->GetCreatureData(KeeshanSpawn));
    for (uint32 spawn : CaptiveSpawns)
        sObjectMgr->RemoveCreatureFromGrid(spawn, sObjectMgr->GetCreatureData(spawn));
    Map map(0, 628, 0, DIFFICULTY_NONE);
    PlacedActor giver(map, KeeshanSpawn);
    PlacedPen pen(map), decoy(map, 3999999);
    LootPlayer owner(SupplySession(), map, 2664601, giver.GetPosition()), outsider(SupplySession(), map, 2664602, pen.GetPosition());
    for (auto player : {&owner, &outsider})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_HUMAN);
        player->SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_WARRIOR);
        player->setFactionForRace(RACE_HUMAN);
        player->InitDisplayIds();
        player->GetReputationMgr().LoadFromDB({});
        player->SetLevel(20);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
        for (uint16 field : {UNIT_FIELD_MOD_TIME_RATE, UNIT_FIELD_MOD_CASTING_SPEED, UNIT_FIELD_MOD_SPELL_HASTE, UNIT_FIELD_MOD_HASTE})
            player->SetFloatValue(field, 1.0f);
    }
    owner.SetRewardedQuest(PreviousQuest);
    WorldPackets::Quest::QuestGiverAcceptQuest accept{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    accept.QuestGiverGUID = giver.GetGUID();
    accept.QuestID = PrisonersQuest;
    owner.GetSession()->HandleQuestGiverAcceptQuest(accept);
    Require(owner.GetQuestStatus(PrisonersQuest) == QUEST_STATUS_INCOMPLETE && owner.GetItemCount(Kit) == 1,
        "Keeshan did not supply the actual field kit");
    MoveGilneasPlayer(owner, pen.GetPosition());
    MoveGilneasPlayer(outsider, pen.GetPosition());
    ClickPen(owner, pen);
    if (baseline)
    {
        Require(pen.getLootState() == GO_ACTIVATED && owner.GetQuestObjectiveData(PrisonersQuest, Credit) == 1,
            "The original no-key button did not reproduce the rescue credit");
        std::cout << "EXPECTED_BROKEN ordinary click opened the pen and credited the rescue without the key\n";
        return;
    }
    Require(pen.getLootState() == GO_READY && !owner.GetQuestObjectiveData(PrisonersQuest, Credit),
        "An ordinary click opened the pen without the key");
    GiveSupply(outsider, Kit, 1);
    Require(outsider.CastSpell(&outsider, EquipKit, false, outsider.GetItemByEntry(Kit)) != SPELL_CAST_OK && !outsider.HasAura(EquipKit),
        "A player outside the rescue could activate a stale kit");
    GiveSupply(outsider, Key, 1);
    Require(outsider.CastSpell(&pen, OpenPen, false, outsider.GetItemByEntry(Key)) != SPELL_CAST_OK,
        "A player outside the mission could open the pen");
    GiveSupply(owner, Key, 1);
    Require(owner.CastSpell(&decoy, OpenPen, false, owner.GetItemByEntry(Key)) != SPELL_CAST_OK,
        "A same-template but unrelated object accepted the key");
    Require(owner.CastSpell(&pen, OpenPen, false, owner.GetItemByEntry(Key)) != SPELL_CAST_OK,
        "A completely empty pen accepted the key");
    std::vector<std::unique_ptr<PlacedActor>> captives;
    for (uint32 spawn : CaptiveSpawns)
        captives.push_back(std::make_unique<PlacedActor>(map, spawn));
    auto equip = owner.CastSpell(&owner, EquipKit, false, owner.GetItemByEntry(Kit));
    auto equipInfo = sSpellMgr->GetSpellInfo(EquipKit);
    Tick(owner, equipInfo->CalcCastTime() + 1000);
    Require(equip == SPELL_CAST_OK && owner.HasAura(EquipKit) && owner.HasSpell(82577) && !owner.HasSpell(82585),
        "The genuine rescue kit did not expose its original abilities");
    if (equipInfo->Cooldowns.StartRecoveryTime)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(equipInfo->Cooldowns.StartRecoveryTime + 60));
        GameTime::UpdateGameTimers();
    }
    ClickPen(owner, pen);
    Tick(owner, sSpellMgr->GetSpellInfo(OpenPen)->CalcCastTime() + 1000);
    Require(pen.getLootState() == GO_ACTIVATED && owner.GetQuestObjectiveData(PrisonersQuest, Credit) == 1 &&
        owner.GetQuestStatus(PrisonersQuest) == QUEST_STATUS_COMPLETE && !owner.HasAura(EquipKit) &&
        !outsider.GetQuestObjectiveData(PrisonersQuest, Credit),
        "The original lock and SmartAI did not open and credit the real rescue");
    auto refill = owner.GetItemCount(Key);
    ClickPen(owner, pen);
    Require(owner.GetItemCount(Key) == refill && !pen.ActivateToQuest(&owner), "A completed rescue could reopen the pen");
    MoveGilneasPlayer(owner, giver.GetPosition());
    Require(owner.CanRewardQuest(quest, 0, false), "Keeshan did not accept the completed original objective");
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestID = PrisonersQuest;
    reward.QuestGiverGUID = giver.GetGUID();
    owner.GetSession()->HandleQuestGiverChooseReward(reward);
    Require(owner.GetQuestRewardStatus(PrisonersQuest) && !owner.GetItemCount(Key) && !owner.GetItemCount(Kit),
        "The genuine reward did not remove the quest key and kit");
    std::cout << "PASS real button, carried key, placed captives, source-scoped credit and Keeshan turn-in\n";
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
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "The prisoners check requires isolated characters");
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
#ifdef BLACKROCK_PRISONERS_BASELINE
        CheckPrisoners(LoadPrisoners(), true);
#else
        CheckPrisoners(LoadPrisoners(), false);
#endif
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
