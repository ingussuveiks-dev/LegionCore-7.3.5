#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "GameObjectPackets.h"
#include "GilneasQuestNative.h"

namespace
{
constexpr uint32 DoaneQuest = 26694;
constexpr uint32 Previous = 26693;
constexpr uint32 Key = 59522;
constexpr uint32 OpenWard = 81776;
constexpr uint32 Ward = 204582;
constexpr uint32 WardSpawn = 472;
constexpr uint32 Doane = 397;
constexpr uint32 DoaneSpawn = 39198;
constexpr uint32 KeeshanSpawn = 260620;

Quest const* LoadTower()
{
    auto rows = WorldDatabase.Query(
#include "NativeIlgalarWard.inc"
    );
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    Require(rows && rows->GetRowCount() == 2, "The original Ilgalar prerequisite is missing");
    do
    {
        auto quest = new Quest(rows->Fetch());
        quests[quest->GetQuestId()] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->GetQuestId(), quest);
    } while (rows->NextRow());
    auto quest = quests[DoaneQuest];
    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=26694");
    auto addon = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,"
        "RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,"
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=26694");
    Require(objective && objective->GetRowCount() == 1 && objective->Fetch()[2].GetUInt8() == QUEST_OBJECTIVE_MONSTER &&
        objective->Fetch()[4].GetUInt32() == Doane && objective->Fetch()[5].GetUInt32() == 1 && addon &&
        addon->Fetch()[4].GetUInt32() == Previous, "The actual Grand Magus route changed");
    quest->LoadQuestObjective(objective->Fetch());
    quest->LoadQuestTemplateAddon(addon->Fetch());
    quest->prevQuests.push_back(Previous);
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL);
    Require(quest->SourceItemId == Key && quest->PackageID == 2955, "Wrong native Ilgalar key or reward package");

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    sObjectMgr->LoadEquipmentTemplates();
    sSpellMgr->LoadSpellBonusess();
    sSpellMgr->LoadSpellThreats();
    auto& lootTable = LootTemplates_Creature.*Access(LootTableNameTag{});
    auto originalLootTable = lootTable;
    lootTable += " WHERE Entry=397";
    auto lootRows = (LootTemplates_Creature.*Access(LoadLootTableTag{}))();
    lootTable = originalLootTable;
    Require(lootRows == 2, "The actual Grand Magus loot rows are missing");
    auto item = sObjectMgr->GetItemTemplate(Key);
    Require(item && std::any_of(item->Effects.begin(), item->Effects.end(), [](ItemEffectEntry const* effect)
        { return effect && effect->SpellID == OpenWard && effect->Charges == 0; }), "The actual Ilgalar key stopped being reusable");

    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature WHERE guid IN(39198,260620)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest=26694");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest=26694");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sObjectMgr->LoadCreatures();
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    return quest;
}

struct TowerActor : Creature
{
    TowerActor(Map& map, uint32 spawn) : Creature(false)
    {
        auto data = sObjectMgr->GetCreatureData(spawn);
        Require(data, "The Grand Magus or Keeshan placement is missing");
        map.LoadGrid(data->posX, data->posY);
        Require(LoadCreatureFromDB(spawn, &map, true), "Could not load the actual tower actor");
    }

    ~TowerActor()
    {
        CombatStop(true);
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<Creature*>(this), false);
    }
};

struct TowerPet : LootPet
{
    TowerPet(LootPlayer& owner, Map& map) : LootPet(owner, map)
    {
        m_ControlledByPlayer = true;
        SetLevel(owner.getLevel());
        setFaction(owner.getFaction());
    }
};

struct TowerWard : GameObject
{
    explicit TowerWard(Map& map, uint32 spawn = WardSpawn)
    {
        auto result = WorldDatabase.Query("SELECT position_x,position_y,position_z,orientation,rotation0,rotation1,rotation2,rotation3 "
            "FROM gameobject WHERE guid=472 AND id=204582 AND map=0 AND areaId=96");
        Require(bool(result), "The real Ward of Ilgalar is missing");
        auto row = result->Fetch();
        Position point(row[0].GetFloat(), row[1].GetFloat(), row[2].GetFloat(), row[3].GetFloat());
        G3D::Quat rotation(row[4].GetFloat(), row[5].GetFloat(), row[6].GetFloat(), row[7].GetFloat());
        m_DBTableGuid = spawn;
        map.LoadGrid(point.GetPositionX(), point.GetPositionY());
        Require(Create(spawn, Ward, &map, 1, point, rotation, 255, GO_STATE_READY) && map.AddToMap(static_cast<GameObject*>(this)),
            "The actual Ward of Ilgalar could not enter the map");
        SetLootState(GO_READY);
        auto lock = sLockStore.LookupEntry(GetGOInfo()->GetLockId());
        Require(GetGoType() == GAMEOBJECT_TYPE_DOOR && lock && lock->ID == 1931 && lock->Index[0] == Key &&
            lock->Type[0] == LOCK_KEY_ITEM, "The actual Ward does not have the Ilgalar key lock");
    }

    ~TowerWard()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<GameObject*>(this), false);
        ResetMap();
    }
};

void ClickWard(LootPlayer& player, TowerWard& ward)
{
    WorldPacket wire(CMSG_GAME_OBJ_USE);
    wire << ward.GetGUID();
    WorldPackets::GameObject::GameObjectUse packet(std::move(wire));
    packet.Read();
    player.GetSession()->HandleGameObjectUse(packet);
    Tick(player, sSpellMgr->GetSpellInfo(OpenWard)->CalcCastTime() + 1000);
}

void CheckTower(Quest const* quest, bool baseline)
{
    for (uint32 spawn : {KeeshanSpawn, DoaneSpawn})
        sObjectMgr->RemoveCreatureFromGrid(spawn, sObjectMgr->GetCreatureData(spawn));
    Map map(0, 629, 0, DIFFICULTY_NONE);
    TowerWard ward(map), decoy(map, 3999998);
    TowerActor keeshan(map, KeeshanSpawn), doane(map, DoaneSpawn);
    LootPlayer owner(SupplySession(), map, 2669401, keeshan.GetPosition()), outsider(SupplySession(), map, 2669402, ward.GetPosition());
    for (auto player : {&owner, &outsider})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_HUMAN);
        player->SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_HUNTER);
        player->setFactionForRace(RACE_HUMAN);
        player->InitDisplayIds();
        player->GetReputationMgr().LoadFromDB({});
        player->SetLevel(20);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
        for (uint16 field : {UNIT_FIELD_MOD_TIME_RATE, UNIT_FIELD_MOD_CASTING_SPEED, UNIT_FIELD_MOD_SPELL_HASTE, UNIT_FIELD_MOD_HASTE})
            player->SetFloatValue(field, 1.0f);
    }
    auto accept = [&]
    {
        MoveGilneasPlayer(owner, keeshan.GetPosition());
        WorldPackets::Quest::QuestGiverAcceptQuest packet{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
        packet.QuestGiverGUID = keeshan.GetGUID();
        packet.QuestID = DoaneQuest;
        owner.GetSession()->HandleQuestGiverAcceptQuest(packet);
    };
    accept();
    Require(owner.GetQuestStatus(DoaneQuest) == QUEST_STATUS_NONE, "Keeshan still requires the previous Redridge mission");
    owner.SetRewardedQuest(Previous);
    accept();
    Require(owner.GetQuestStatus(DoaneQuest) == QUEST_STATUS_INCOMPLETE && owner.GetItemCount(Key) == 1,
        "Keeshan did not supply the actual Ilgalar key");
    owner.DestroyItemCount(Key, 1, true);
    MoveGilneasPlayer(owner, ward.GetPosition());
    ClickWard(owner, ward);
    if (baseline)
    {
        Require(ward.getLootState() == GO_ACTIVATED, "The original keyless click did not open the Ward");
        std::cout << "EXPECTED_BROKEN ordinary click opened the original Ward of Ilgalar without its key\n";
        return;
    }
    Require(ward.getLootState() == GO_READY, "The unkeyed generic click opened the ward");
    GiveSupply(outsider, Key, 1);
    Require(outsider.CastSpell(&ward, OpenWard, false, outsider.GetItemByEntry(Key)) != SPELL_CAST_OK, "A stale key opened without the quest");
    AbandonSupply(owner, DoaneQuest);
    accept();
    MoveGilneasPlayer(owner, ward.GetPosition());
    Require(owner.CastSpell(&decoy, OpenWard, false, owner.GetItemByEntry(Key)) != SPELL_CAST_OK,
        "A different spawn of the same template accepted the real key");
    ClickWard(owner, ward);
    Require(ward.getLootState() == GO_ACTIVATED && owner.GetItemCount(Key) == 1 &&
        !owner.GetQuestObjectiveData(DoaneQuest, Doane), "The genuine lock failed or advanced the death objective");
    MoveGilneasPlayer(owner, doane.GetPosition());
    Require(owner.IsValidAttackTarget(&doane), "The actual Grand Magus is not hostile");
    TowerPet pet(owner, map);
    doane.SetHealth(1);
    Require(pet.DealDamage(&doane, 1, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false) && !doane.IsAlive(),
        "A real pet hit did not kill the Grand Magus");
    Tick(owner, 100);
    Require(owner.GetQuestStatus(DoaneQuest) == QUEST_STATUS_COMPLETE && owner.GetQuestObjectiveData(DoaneQuest, Doane) == 1,
        "The delayed native pet-owner credit did not complete the objective exactly once");

    ward.ResetDoorOrButton();
    ward.SetLootState(GO_READY);
    MoveGilneasPlayer(owner, ward.GetPosition());
    GameTime::UpdateGameTimers();
    ClickWard(owner, ward);
    Require(ward.getLootState() == GO_ACTIVATED, "A completed unrewarded player could not leave through the real Ward");
    MoveGilneasPlayer(owner, keeshan.GetPosition());
    uint32 choice = 0;
    if (auto items = sDB2Manager.GetQuestPackageItems(quest->PackageID))
        for (auto item : *items)
            if (owner.CanSelectQuestPackageItem(item))
            {
                choice = item->ItemID;
                break;
            }
    if (!choice)
        if (auto items = sDB2Manager.GetQuestPackageItemsFallback(quest->PackageID); items && !items->empty())
            choice = items->front()->ItemID;
    Require(choice && owner.CanRewardQuest(quest, choice, false), "The genuine Keeshan reward did not become available");
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestID = DoaneQuest;
    reward.QuestGiverGUID = keeshan.GetGUID();
    reward.ItemChoiceID = choice;
    owner.GetSession()->HandleQuestGiverChooseReward(reward);
    Require(owner.GetQuestRewardStatus(DoaneQuest) && !owner.GetItemCount(Key), "The reward did not clean up the Ward key");
    std::cout << "PASS exact lock, actual hostile Doane, pet-owner credit, unrewarded exit and Keeshan package\n";
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
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "The Ilgalar check requires isolated characters");
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
#ifdef ILGALAR_WARD_BASELINE
        CheckTower(LoadTower(), true);
#else
        CheckTower(LoadTower(), false);
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
