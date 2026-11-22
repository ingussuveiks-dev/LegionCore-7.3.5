#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "CreatureTextMgr.h"
#include "GameObjectPackets.h"
#include "QuestItemObjectUse.h"
#include "SmartScriptMgr.h"

void AddSC_bloodmyst_isle();

struct BloodmystSpawnTag { using Type = uint64 Creature::*; friend Type Access(BloodmystSpawnTag); };
template struct FixtureAccess<BloodmystSpawnTag, &Creature::m_DBTableGuid>;

namespace
{
using namespace SavingPrincessStillpine;
constexpr uint32 Chief = 17702;
constexpr uint32 Frasaboo = 18803;
constexpr uint32 StillpineHold = 9538;
constexpr uint32 Immunities = UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC;

Position ReadPosition(Field* row)
{
    return {row[0].GetFloat(), row[1].GetFloat(), row[2].GetFloat(), row[3].GetFloat()};
}

struct BloodmystActor : Actor
{
    BloodmystActor(Map& map, Field* row) : Actor(&map, row[1].GetUInt32(), row[0].GetUInt32(), ReadPosition(row + 2))
    {
        this->*Access(BloodmystSpawnTag{}) = row[0].GetUInt64();
        auto& data = sObjectMgr->NewOrExistCreatureData(GetDBTableGUIDLow());
        data.id = GetEntry();
        data.posX = GetPositionX();
        data.posY = GetPositionY();
        data.posZ = GetPositionZ();
        data.orientation = GetOrientation();
        map.LoadGrid(GetPositionX(), GetPositionY());
        Require(map.AddToMap(static_cast<Creature*>(this)), "Could not publish a genuine Bristlelimb actor");
    }

    void AddToWorld() override
    {
        sObjectAccessor->AddObject(static_cast<Creature*>(this));
        Unit::AddToWorld();
        Require(AIM_Initialize(), "Could not load the registered Bloodmyst actor AI");
    }
};

struct PrincessButton : GameObject
{
    PrincessButton(Map& map, Field* row, uint32 spawn = CageSpawn)
    {
        m_DBTableGuid = spawn;
        Position position = ReadPosition(row);
        G3D::Quat rotation(row[4].GetFloat(), row[5].GetFloat(), row[6].GetFloat(), row[7].GetFloat());
        map.LoadGrid(position.GetPositionX(), position.GetPositionY());
        Require(Create(spawn, Cage, &map, 1, position, rotation, 255, GO_STATE_READY) &&
            map.AddToMap(static_cast<GameObject*>(this)), "Could not publish the original Stillpine button");
        if (!GetGOInfo()->GetDespawnPossibility() && !GetGOInfo()->IsDespawnAtAction())
            SetRespawnDelayTime(0);
        Update(5000);
        auto lock = sLockStore.LookupEntry(GetGOInfo()->GetLockId());
        Require(GetAreaId() == 3600 && GetGoType() == GAMEOBJECT_TYPE_BUTTON && getLootState() == GO_READY &&
            lock && lock->ID == 1670 && lock->Type[0] == LOCK_KEY_ITEM && lock->Index[0] == Key, "Original button or key lock changed");
    }

    ~PrincessButton()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<GameObject*>(this), false);
        ResetMap();
    }
};

::Quest const* LoadPrincessQuest()
{
    auto row = WorldDatabase.Query(
#include "NativePrincessStillpine.inc"
    );
    Require(row && row->GetRowCount() == 2, "Expected native prerequisite and princess quests");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new ::Quest(row->Fetch());
        quests[quest->GetQuestId()] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->GetQuestId(), quest);
    } while (row->NextRow());
    auto quest = quests[SavingPrincessStillpine::Quest];
    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=9667");
    Require(objective && objective->GetRowCount() == 1 && objective->Fetch()[2].GetUInt8() == QUEST_OBJECTIVE_MONSTER &&
        objective->Fetch()[4].GetUInt32() == Princess && objective->Fetch()[5].GetUInt32() == 1, "Expected one genuine princess rescue");
    quest->LoadQuestObjective(objective->Fetch());
    auto addon = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,"
        "RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,"
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=9667");
    Require(addon && addon->Fetch()[4].GetUInt32() == StillpineHold && quest->ItemDrop[1] == Key && quest->ItemDropQuantity[1] == 1,
        "Original Stillpine Hold prerequisite or chief key drop changed");
    quest->LoadQuestTemplateAddon(addon->Fetch());
    quest->prevQuests.push_back(quest->PrevQuestID); // The full quest loader links this separately from the addon parser.
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    sSpellMgr->LoadSpellBonusess();
    sSpellMgr->LoadSpellThreats();
    auto& lootTable = LootTemplates_Creature.*Access(LootTableNameTag{});
    auto original = lootTable;
    lootTable += " WHERE Entry IN (17320,17321,17702)";
    auto loaded = (LootTemplates_Creature.*Access(LoadLootTableTag{}))();
    lootTable = original;
    auto keyRow = WorldDatabase.Query("SELECT Entry,QuestRequired FROM creature_loot_template WHERE Item=24099");
    Require(loaded && LootTemplates_Creature.HaveLootFor(Chief) && keyRow && keyRow->GetRowCount() == 1 &&
        keyRow->Fetch()[0].GetUInt32() == Chief && keyRow->Fetch()[1].GetUInt8(), "The actual chief must be the only key source");

    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    AddSC_bloodmyst_isle();
    sScriptMgr->SwapScriptContext(true);
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE stillpine_starter AS SELECT * FROM creature_queststarter WHERE quest=9667");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM stillpine_starter");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE stillpine_receiver AS SELECT * FROM creature_questender WHERE quest=9667");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM stillpine_receiver");
#ifndef PRINCESS_STILLPINE_BASELINE
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE stillpine_conditions AS SELECT * FROM conditions WHERE SourceTypeOrReferenceId=17 AND SourceEntry=31003");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE conditions AS SELECT * FROM stillpine_conditions");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE stillpine_combat AS SELECT * FROM smart_scripts WHERE source_type=0 AND entryorguid IN (17320,17321,17702)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE smart_scripts AS SELECT * FROM stillpine_combat");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE stillpine_text AS SELECT * FROM creature_text WHERE CreatureID IN (17682,17702)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_text AS SELECT * FROM stillpine_text");
#endif
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
#ifndef PRINCESS_STILLPINE_BASELINE
    sConditionMgr->LoadConditions();
    sSmartScriptMgr->LoadSmartAIFromDB();
    sCreatureTextMgr->LoadCreatureTexts();
#endif
    return quest;
}

void AcceptPrincessQuest(Player& player, Creature const& princess)
{
    WorldPackets::Quest::QuestGiverAcceptQuest packet{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    packet.QuestGiverGUID = princess.GetGUID();
    packet.QuestID = SavingPrincessStillpine::Quest;
    player.GetSession()->HandleQuestGiverAcceptQuest(packet);
}

void ClickPrincessCage(Player& player, GameObject const& cage)
{
    WorldPacket wire(CMSG_GAME_OBJ_USE);
    wire << cage.GetGUID();
    WorldPackets::GameObject::GameObjectUse packet(std::move(wire));
    packet.Read();
    player.GetSession()->HandleGameObjectUse(packet);
}

void LootChiefKey(LootPlayer& owner, LootPlayer& stranger, Map& map)
{
    auto chiefRow = WorldDatabase.Query("SELECT guid,id,position_x,position_y,position_z,orientation FROM creature "
        "WHERE guid=35410 AND id=17702 AND map=530 AND areaId=3600");
    auto kills = WorldDatabase.Query("SELECT guid,id,position_x,position_y,position_z,orientation FROM creature "
        "WHERE map=530 AND areaId=3600 AND guid IN (15928,15947) ORDER BY id");
    Require(chiefRow && kills && kills->GetRowCount() == 2, "Missing the actual chief and camp spawns");
    for (uint32 cycle = 0; cycle < 4; ++cycle)
    {
        BloodmystActor chief(map, chiefRow->Fetch());
        Require(chief.HasFlag(UNIT_FIELD_FLAGS, Immunities) && chief.GetReactState() == REACT_PASSIVE,
            "The original chief should be hidden before eligible Bristlelimb deaths");
        SupplyMove(stranger, chief.GetPosition());
        Require(!stranger.canSeeOrDetect(&chief), "An inactive observer could see the sleeping chief");
        LootPet pet(stranger, map);
        pet.SetLevel(stranger.getLevel());
        pet.SetEffectiveLevel(stranger.GetEffectiveLevel());
        pet.SetOwnerGUID(ObjectGuid::Empty);
        stranger.SetMinion(&pet, true);
        Require(pet.IsControlledByPlayer(), "Expected a genuine controlled pet for the public camp event");
        for (uint32 attempt = 0; attempt < 64 && chief.HasFlag(UNIT_FIELD_FLAGS, Immunities); ++attempt)
        {
            if (!kills->NextRow())
                kills = WorldDatabase.Query("SELECT guid,id,position_x,position_y,position_z,orientation FROM creature "
                    "WHERE map=530 AND areaId=3600 AND guid IN (15928,15947) ORDER BY id");
            BloodmystActor camp(map, kills->Fetch());
            Require(pet.DealDamage(&camp, 1000000, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false) && camp.isDead(false),
                "A real pet final blow did not kill the original camp occupant");
        }
        Require(!chief.HasFlag(UNIT_FIELD_FLAGS, Immunities) && stranger.canSeeOrDetect(&chief) && chief.GetReactState() == REACT_AGGRESSIVE,
            "The real shared chief did not reveal after camp deaths by a non-quest owner's pet");
        stranger.SetMinion(&pet, false);
        SupplyMove(owner, chief.GetPosition());
        Require(owner.DealDamage(&chief, 1000000, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false) && chief.isDead(false),
            "The revealed chief could not be killed by the accepted player");
        auto loot = &chief.loot; // The native shared loot view carries the quest-only items alongside personal drops.
        WorldPackets::Loot::LootResponse response;
        loot->BuildLootResponse(response, &owner, ALL_PERMISSION);
        for (auto const& item : response.Items)
        {
            if (item.Loot.ItemID != Key)
                continue;
            WorldPackets::Loot::LootResponse foreignResponse;
            loot->BuildLootResponse(foreignResponse, &stranger, ALL_PERMISSION);
            Require(item.Quantity == 1 && !stranger.GetItemCount(Key) && std::none_of(foreignResponse.Items.begin(), foreignResponse.Items.end(),
                [](auto const& row) { return row.Loot.ItemID == Key; }), "Expected one quest-only key from the authentic corpse");
            owner.StoreLootItem(item.LootListID - 1, loot);
            Require(owner.GetItemCount(Key) == 1, "The real slain chief's loot slot did not store its key");
            chief.Respawn(false, 0); // Remove the corpse and start the same native respawn path as the elapsed timer.
            chief.Respawn(false, 0);
            chief.Update(100);
            Require(chief.IsAlive() && chief.HasFlag(UNIT_FIELD_FLAGS, Immunities) && !stranger.canSeeOrDetect(&chief),
                "The respawned chief became available without another camp encounter");
            return;
        }
    }
    Require(false, "No key appeared in four successful native chief corpse rolls");
}

void CheckPrincess(::Quest const* quest)
{
    auto cageRow = WorldDatabase.Query("SELECT position_x,position_y,position_z,orientation,rotation0,rotation1,rotation2,rotation3 "
        "FROM gameobject WHERE guid=19182 AND id=181928 AND map=530 AND areaId=3600");
    auto princessRow = WorldDatabase.Query("SELECT guid,id,position_x,position_y,position_z,orientation FROM creature "
        "WHERE guid=17225 AND id=17682 AND map=530 AND areaId=3600");
    auto receiverRow = WorldDatabase.Query("SELECT guid,id,position_x,position_y,position_z,orientation FROM creature "
        "WHERE guid=20902 AND id=18803 AND map=530 AND areaId=3584");
    Require(cageRow && princessRow && receiverRow, "Original princess, cage or Frasaboo placement missing");
    Map map(530, 502, 0, DIFFICULTY_NONE);
    PrincessButton cage(map, cageRow->Fetch());
    BloodmystActor princess(map, princessRow->Fetch()), frasaboo(map, receiverRow->Fetch());
    LootPlayer owner(SupplySession(), map, 966701, cage.GetPosition()), other(SupplySession(), map, 966702, cage.GetPosition());
    for (LootPlayer* player : {&owner, &other})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_DRAENEI);
        player->setFactionForRace(RACE_DRAENEI);
        player->InitDisplayIds();
        player->SetLevel(15);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
    }
    AcceptPrincessQuest(owner, princess);
    Require(owner.GetQuestStatus(SavingPrincessStillpine::Quest) == QUEST_STATUS_NONE, "Princess bypassed Stillpine Hold prerequisite");
    owner.SetRewardedQuest(StillpineHold);
    AcceptPrincessQuest(owner, princess);
    Require(owner.GetQuestStatus(SavingPrincessStillpine::Quest) == QUEST_STATUS_INCOMPLETE && !owner.GetItemCount(Key),
        "The actual princess did not offer her quest without giving a key");
    ClickPrincessCage(owner, cage);
#ifdef PRINCESS_STILLPINE_BASELINE
    Require(cage.GetGoState() == GO_STATE_ACTIVE && princess.GetMotionMaster()->GetCurrentMovementGeneratorType() == POINT_MOTION_TYPE &&
        !owner.GetItemCount(Key), "The original click did not reproduce a keyless opening and departure");
    std::cout << "EXPECTED_BROKEN a keyless direct click opened the real button and released the princess\n";
#else
    Require(cage.GetGoState() == GO_STATE_READY && !cage.ActivateToQuest(&owner) && !other.HasQuestForItem(Key),
        "A keyless click or inactive loot admission changed the real cage");
    LootChiefKey(owner, other, map);
    SupplyMove(owner, cage.GetPosition());
    SupplyMove(other, cage.GetPosition());
    {
        PrincessButton decoy(map, cageRow->Fetch(), 450002);
        Require(!decoy.ActivateToQuest(&owner) && owner.CastSpell(&decoy, OpenCage, false, owner.GetItemByEntry(Key)) != SPELL_CAST_OK,
            "A copy of the binding used the real chief's key");
    }
    other.SetRewardedQuest(StillpineHold);
    AcceptPrincessQuest(other, princess);
    GiveSupply(other, Key, 1);
    Require(cage.ActivateToQuest(&owner) && cage.ActivateToQuest(&other), "The two exact players could not start the timed lock");
    ClickPrincessCage(owner, cage);
    ClickPrincessCage(other, cage);
    Tick(owner, 1000);
    Require(cage.GetGoState() == GO_STATE_READY && owner.GetItemCount(Key) == 1 && princess.HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER),
        "The native five-second cast consumed the key or released the actor early");
    Tick(owner, 4500);
    Tick(other, 5500);
    Require(cage.GetGoState() == GO_STATE_ACTIVE && !owner.GetItemCount(Key) && other.GetItemCount(Key) == 1 &&
        owner.GetQuestStatus(SavingPrincessStillpine::Quest) == QUEST_STATUS_COMPLETE && other.GetQuestStatus(SavingPrincessStillpine::Quest) == QUEST_STATUS_INCOMPLETE &&
        !princess.HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER) && !cage.ActivateToQuest(&other),
        "The real lock did not consume one key, release its princess or grant one personal credit");
    Position before = princess.GetPosition();
    for (uint32 elapsed = 0; elapsed < 2200; elapsed += 25)
    {
        princess.Unit::Update(25);
        map.MoveAllCreaturesInMoveList();
    }
    Require(princess.GetExactDist(before) > 3.0f, "The native release did not move the princess away from the actual cage");
    princess.AI()->UpdateAI(10000);
    Require(!princess.IsAlive() && !cage.ActivateToQuest(&other), "The escaped princess stayed available before respawn");
    Sleep(6200);
    GameTime::UpdateGameTimers();
    cage.Update(5000);
    cage.Update(5000);
    princess.Respawn(false, 0);
    princess.Update(100);
    map.MoveAllCreaturesInMoveList();
    std::cout << "Princess respawn alive=" << princess.IsAlive() << " giver=" << princess.HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER)
        << " cageState=" << unsigned(cage.GetGoState()) << " lootState=" << unsigned(cage.getLootState())
        << " distance=" << princess.GetExactDist(cage) << " secondAvailable=" << cage.ActivateToQuest(&other) << '\n';
    Require(princess.IsAlive() && princess.HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER) && cage.GetGoState() == GO_STATE_READY &&
        cage.ActivateToQuest(&other), "The native respawn did not restore the quest giver and cage for the next eligible player");
    GiveSupply(owner, Key, 1);
    Require(!cage.ActivateToQuest(&owner) && !owner.HasQuestForItem(Key), "The completed attempt can use another chief key");
    SupplyMove(owner, frasaboo.GetPosition());
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
    Require(quest->PackageID == 1419 && choice && owner.CanInteractWithQuestGiver(&frasaboo) && owner.CanRewardQuest(quest, choice, false),
        "Expected Frasaboo's genuine class reward");
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestGiverGUID = frasaboo.GetGUID();
    reward.QuestID = SavingPrincessStillpine::Quest;
    reward.ItemChoiceID = choice;
    owner.GetSession()->HandleQuestGiverChooseReward(reward);
    Require(owner.GetQuestRewardStatus(SavingPrincessStillpine::Quest) && !owner.GetItemCount(Key), "Frasaboo did not remove the spare key on real turn-in");
    AbandonSupply(other, SavingPrincessStillpine::Quest);
    Require(!other.GetItemCount(Key) && !cage.ActivateToQuest(&other), "Abandonment retained the unspent key or cage availability");
    std::cout << "PASS genuine prerequisite, pet-triggered public chief, real corpse key, timed button, princess departure, personal credit and Frasaboo reward\n";
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{530, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{530, {}}});
        LoadGameObjectModelList(argv[2]);
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "The reward requires an isolated character schema");
        auto quest = LoadPrincessQuest();
        sObjectMgr->LoadPlayerInfo();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckPrincess(quest);
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
