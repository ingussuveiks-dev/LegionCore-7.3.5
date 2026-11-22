#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "GameObjectPackets.h"
#include "QuestItemObjectUse.h"
#include "ScriptSystem.h"

void AddSC_azuremyst_isle();

struct CaptiveSpawnTag { using Type = uint64 Creature::*; friend Type Access(CaptiveSpawnTag); };
template struct FixtureAccess<CaptiveSpawnTag, &Creature::m_DBTableGuid>;

namespace
{
using namespace ProphecyOfAkida;

Position ReadPosition(Field* row)
{
    return {row[0].GetFloat(), row[1].GetFloat(), row[2].GetFloat(), row[3].GetFloat()};
}

struct StillpineCaptive : Actor
{
    StillpineCaptive(Map& map, Field* row) : Actor(&map, Captive, row[0].GetUInt32(), ReadPosition(row + 1))
    {
        this->*Access(CaptiveSpawnTag{}) = row[0].GetUInt64();
        Require(map.AddToMap(static_cast<Creature*>(this)), "Could not publish the original captive");
    }

    void AddToWorld() override
    {
        sObjectAccessor->AddObject(static_cast<Creature*>(this));
        Unit::AddToWorld();
        Require(AIM_Initialize(), "Could not load the registered Stillpine captive AI");
    }
};

struct BristlelimbCage : GameObject
{
    BristlelimbCage(Map& map, Field* row, uint32 decoy = 0)
    {
        m_DBTableGuid = decoy ? decoy : row[0].GetUInt32();
        Position position = ReadPosition(row + 1);
        G3D::Quat rotation(row[5].GetFloat(), row[6].GetFloat(), row[7].GetFloat(), row[8].GetFloat());
        map.LoadGrid(position.GetPositionX(), position.GetPositionY());
        Require(Create(m_DBTableGuid, Cage, &map, 1, position, rotation, 255, GO_STATE_READY) &&
            map.AddToMap(static_cast<GameObject*>(this)), "Could not publish the original Bristlelimb cage");
        if (!GetGOInfo()->GetDespawnPossibility() && !GetGOInfo()->IsDespawnAtAction())
            SetRespawnDelayTime(0);
        Update(5000);
        auto lock = sLockStore.LookupEntry(GetGOInfo()->GetLockId());
        Require(GetAreaId() == 3571 && getLootState() == GO_READY && lock && lock->ID == 1664 &&
            lock->Type[0] == LOCK_KEY_ITEM && lock->Index[0] == Key, "Original native cage, area or key lock changed");
    }

    ~BristlelimbCage()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<GameObject*>(this), false);
        ResetMap();
    }
};

::Quest const* LoadAkidaQuest()
{
    auto row = WorldDatabase.Query(
#include "NativeAkidaCages.inc"
    );
    Require(bool(row), "The actual Prophecy of Akida quest is missing");
    auto quest = new ::Quest(row->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    quests[ProphecyOfAkida::Quest] = quest;
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(ProphecyOfAkida::Quest, quest);
    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=9544");
    Require(objective && objective->GetRowCount() == 1 && objective->Fetch()[2].GetUInt8() == QUEST_OBJECTIVE_MONSTER &&
        objective->Fetch()[4].GetUInt32() == Captive && objective->Fetch()[5].GetUInt32() == 8, "Expected eight actual captive releases");
    quest->LoadQuestObjective(objective->Fetch());
    auto addon = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,"
        "RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,"
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=9544");
    Require(addon && addon->Fetch()[4].GetUInt32() == 9542 && !quest->SourceItemId && quest->ItemDrop[1] == Key &&
        quest->ItemDropQuantity[1] == 8, "Expected Water Totem prerequisite and up to eight looted keys");
    quest->LoadQuestTemplateAddon(addon->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    sSpellMgr->LoadSpellBonusess();
    sSpellMgr->LoadSpellThreats();
    sScriptSystemMgr->LoadScriptTexts();
    auto& lootTable = LootTemplates_Creature.*Access(LootTableNameTag{});
    auto original = lootTable;
    lootTable += " WHERE Entry IN (17183,17184,17185) AND Item=23801 AND QuestRequired=1";
    auto loaded = (LootTemplates_Creature.*Access(LoadLootTableTag{}))();
    lootTable = original;
    Require(loaded == 3, "The authentic Bristlelimb key loot rows did not load");

    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    AddSC_azuremyst_isle();
    sScriptMgr->SwapScriptContext(true);
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE akida_questender AS SELECT * FROM creature_questender WHERE quest=9544");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM akida_questender");
#ifndef AKIDA_CAGES_BASELINE
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE akida_conditions AS SELECT * FROM conditions WHERE SourceTypeOrReferenceId=17 AND SourceEntry=30406");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE conditions AS SELECT * FROM akida_conditions");
#endif
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sQuestDataStore->LoadCreatureInvolvedRelations();
#ifndef AKIDA_CAGES_BASELINE
    sConditionMgr->LoadConditions();
#endif
    return quest;
}

void ClickCage(Player& player, GameObject const& cage)
{
    WorldPacket wire(CMSG_GAME_OBJ_USE);
    wire << cage.GetGUID();
    WorldPackets::GameObject::GameObjectUse packet(std::move(wire));
    packet.Read();
    player.GetSession()->HandleGameObjectUse(packet);
}

void FollowCaptive(StillpineCaptive& captive, uint32 duration)
{
    for (uint32 elapsed = 0; elapsed < duration; elapsed += 25)
    {
        captive.Unit::Update(25);
        captive.GetMap()->MoveAllCreaturesInMoveList();
    }
}

bool LootKey(LootPlayer& player, uint32 source, bool take)
{
    Loot loot;
    Require(loot.FillLoot(source, LootTemplates_Creature, &player, true), "A Bristlelimb loot source failed to roll");
    WorldPackets::Loot::LootResponse response;
    loot.BuildLootResponse(response, &player, ALL_PERMISSION);
    if (response.Items.empty())
        return false;
    Require(response.Items.size() == 1 && response.Items.front().Loot.ItemID == Key && response.Items.front().Quantity == 1,
        "The native loot response did not contain the original single key");
    if (take)
    {
        auto before = player.GetItemCount(Key);
        player.StoreLootItem(response.Items.front().LootListID - 1, &loot);
        Require(player.GetItemCount(Key) == before + 1, "The genuine loot slot did not store the Bristlelimb key");
    }
    return true;
}

void CheckAkida(::Quest const* quest)
{
    auto cages = WorldDatabase.Query("SELECT guid,position_x,position_y,position_z,orientation,rotation0,rotation1,rotation2,rotation3 "
        "FROM gameobject WHERE id=181714 AND map=530 AND zoneId=3524 AND areaId=3571 ORDER BY guid");
    auto giver = WorldDatabase.Query("SELECT c.guid,c.position_x,c.position_y,c.position_z,c.orientation FROM creature c "
        "JOIN creature_queststarter s ON s.id=c.id AND s.quest=9544 WHERE c.id=17364 AND c.guid=16561 AND c.map=530");
    auto receiver = WorldDatabase.Query("SELECT c.guid,c.position_x,c.position_y,c.position_z,c.orientation FROM creature c "
        "JOIN creature_questender e ON e.id=c.id AND e.quest=9544 WHERE c.id=17114 AND c.guid=13382 AND c.map=530");
    Require(cages && cages->GetRowCount() == 15 && giver && receiver, "The original cage placements or totem and Arugoo are missing");
    Map map(530, 501, 0, DIFFICULTY_NONE);
    Position receiverPos = ReadPosition(receiver->Fetch() + 1);
    map.LoadGrid(receiverPos.GetPositionX(), receiverPos.GetPositionY());
    Actor arugoo(&map, 17114, receiver->Fetch()[0].GetUInt32(), receiverPos);
    Require(map.AddToMap(static_cast<Creature*>(&arugoo)), "Could not publish the actual Arugoo");

    Position firstCage = ReadPosition(cages->Fetch() + 1);
    LootPlayer owner(SupplySession(), map, 954401, firstCage), other(SupplySession(), map, 954402, firstCage);
    for (LootPlayer* player : {&owner, &other})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_DRAENEI);
        player->setFactionForRace(RACE_DRAENEI);
        player->InitDisplayIds();
        player->SetLevel(15);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
    }
    owner.AddQuestAndCheckCompletion(quest, nullptr);
    Require(!owner.GetItemCount(Key) && owner.HasQuestForItem(Key) && !other.HasQuestForItem(Key),
        "The native quest should admit looting a key only after acceptance");
    uint32 rescues = 0;
    do
    {
        auto cageRow = cages->Fetch();
        BristlelimbCage cage(map, cageRow);
        auto row = WorldDatabase.PQuery("SELECT guid,position_x,position_y,position_z,orientation FROM creature "
            "WHERE id=17375 AND map=530 AND areaId=3571 AND (POW(position_x-%f,2)+POW(position_y-%f,2)+POW(position_z-%f,2))<1",
            cage.GetPositionX(), cage.GetPositionY(), cage.GetPositionZ());
        Require(row && row->GetRowCount() == 1, "Expected exactly one original captive next to the cage");
        StillpineCaptive captive(map, row->Fetch());
        SupplyMove(owner, cage.GetPosition());
        SupplyMove(other, cage.GetPosition());
#ifdef AKIDA_CAGES_BASELINE
        ClickCage(owner, cage);
        Require(captive.GetMotionMaster()->GetCurrentMovementGeneratorType() == POINT_MOTION_TYPE && !owner.GetItemCount(Key),
            "The old direct click did not reproduce a keyless captive release");
        FollowCaptive(captive, 2500);
        Require(owner.GetQuestObjectiveData(ProphecyOfAkida::Quest, Captive) == 1, "The old keyless release did not award its quest count");
        std::cout << "EXPECTED_BROKEN the old keyless click freed the actual captive and advanced the quest\n";
        return;
#else
        Require(IsPlacedPair(cage, captive) && FindCaptive(cage) == &captive, "The actual cage and captive pairing is incorrect");
        if (!rescues)
        {
            ClickCage(owner, cage);
            Require(!cage.ActivateToQuest(&owner) && cage.getLootState() == GO_READY, "The keyless click changed the cage");
            for (uint32 source : {17183u, 17184u, 17185u})
            {
                bool yielded = false;
                for (uint32 roll = 0; roll < 24; ++roll)
                {
                    Require(!LootKey(other, source, false), "An inactive player saw a quest key in Bristlelimb loot");
                    if (!yielded)
                        yielded = LootKey(owner, source, true);
                }
                Require(yielded, "A native Bristlelimb key source failed all rolls");
            }
            GiveSupply(other, Key, 1);
            ClickCage(other, cage);
            Require(!cage.ActivateToQuest(&other) && !other.GetCurrentSpell(CURRENT_GENERIC_SPELL), "An inactive quest opened the cage with a retained key");
            other.DestroyItemCount(Key, 1, true);
            {
                BristlelimbCage decoy(map, cageRow, 450001);
                Require(!decoy.ActivateToQuest(&owner) && owner.CastSpell(&decoy, OpenCage, false, owner.GetItemByEntry(Key)) != SPELL_CAST_OK,
                    "The key affected a wrong placement of the same template");
            }
            ClickCage(owner, cage);
            Tick(owner, 1000);
            Require(owner.GetCurrentSpell(CURRENT_GENERIC_SPELL) && cage.GetGoState() == GO_STATE_READY && owner.GetItemCount(Key) == 3,
                "The original five-second key cast opened the cage or used a key early");
            AbandonSupply(owner, ProphecyOfAkida::Quest);
            Require(!owner.GetItemCount(Key), "Abandonment retained the quest drop keys");
            owner.AddQuestAndCheckCompletion(quest, nullptr);
            Tick(owner, 6000);
            Require(cage.GetGoState() == GO_STATE_READY && !owner.GetQuestObjectiveData(ProphecyOfAkida::Quest, Captive),
                "A cancelled attempt freed the captive after reaccepting the quest");
            Sleep(1800);
            GameTime::UpdateGameTimers();
            other.AddQuestAndCheckCompletion(quest, nullptr);
            GiveSupply(other, Key, 1);
        }
        if (rescues < 8)
        {
            GiveSupply(owner, Key, 2);
            ClickCage(owner, cage);
            Tick(owner, 5500);
            Require(cage.GetGoState() == GO_STATE_ACTIVE && !cage.ActivateToQuest(&other) && owner.GetItemCount(Key) == rescues + 1 &&
                owner.GetQuestObjectiveData(ProphecyOfAkida::Quest, Captive) == rescues, "An authorized key did not open one cage and consume one key before movement credit");
            ClickCage(other, cage);
            Tick(other, 5500);
            Require(other.GetItemCount(Key) == 1, "An already open cage consumed a second player's key");
            FollowCaptive(captive, 2600);
            ++rescues;
            Require(owner.GetQuestObjectiveData(ProphecyOfAkida::Quest, Captive) == rescues && !other.GetQuestObjectiveData(ProphecyOfAkida::Quest, Captive),
                "The actual exit movement did not credit exactly its owner once");
            Require(!cage.ActivateToQuest(&owner), "The freed cage is still usable");
            Sleep(1600);
            GameTime::UpdateGameTimers();
        }
        else
            Require(!cage.ActivateToQuest(&owner), "A completed quest can still use its cage and keys");
#endif
    } while (cages->NextRow());

    Require(rescues == 8 && owner.GetQuestStatus(ProphecyOfAkida::Quest) == QUEST_STATUS_COMPLETE && owner.GetItemCount(Key) == 8 &&
        !owner.HasQuestForItem(Key), "Completion did not match eight rescues or failed to block additional key loot");
    SupplyMove(owner, receiverPos);
    Require(owner.CanInteractWithQuestGiver(&arugoo), "Arugoo is not an actual reachable quest recipient");
    uint32 choice = 0;
    if (auto items = sDB2Manager.GetQuestPackageItems(quest->PackageID))
        for (QuestPackageItemEntry const* item : *items)
            if (owner.CanSelectQuestPackageItem(item))
            {
                choice = item->ItemID;
                break;
            }
    if (!choice)
        if (auto fallback = sDB2Manager.GetQuestPackageItemsFallback(quest->PackageID); fallback && !fallback->empty())
            choice = fallback->front()->ItemID;
    Require(quest->PackageID == 1360 && choice && owner.CanRewardQuest(quest, choice, false), "Expected Arugoo's actual class-appropriate choice reward");
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestGiverGUID = arugoo.GetGUID();
    reward.QuestID = ProphecyOfAkida::Quest;
    reward.ItemChoiceID = choice;
    owner.GetSession()->HandleQuestGiverChooseReward(reward);
    Require(owner.GetQuestRewardStatus(ProphecyOfAkida::Quest) && !owner.GetItemCount(Key), "Arugoo did not take the spare keys at actual reward");
    AbandonSupply(other, ProphecyOfAkida::Quest);
    Require(!other.GetItemCount(Key), "The second player retained their spare key after abandoning");
    std::cout << "PASS native Bristlelimb loot, 15 actual cage and captive bindings, eight real releases, exact key cleanup and Arugoo reward\n";
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{530, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{530, {}}});
        LoadGameObjectModelList(argv[2]);
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "The reward requires an isolated character schema");
        auto quest = LoadAkidaQuest();
        sObjectMgr->LoadPlayerInfo();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckAkida(quest);
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
