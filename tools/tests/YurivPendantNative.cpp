#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "GossipDef.h"

namespace
{
constexpr uint32 Delivery = 264;
constexpr uint32 Pendant = 6145;
constexpr uint32 PlacePendant = 8001;
constexpr uint32 Tomb = 24776;
constexpr uint32 Plaque = 24777;

Quest const* LoadPendant()
{
    auto row = WorldDatabase.Query(
#include "NativeYurivPendant.inc"
    );
    Require(bool(row), "Missing Until Death Do Us Part template");
    auto quest = new Quest(row->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    quests[Delivery] = quest;
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(Delivery, quest);
    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=264");
    Require(objective && objective->GetRowCount() == 1 && objective->Fetch()[2].GetUInt8() == QUEST_OBJECTIVE_ITEM &&
        objective->Fetch()[4].GetUInt32() == Pendant && objective->Fetch()[5].GetUInt32() == 1, "Missing original pendant delivery");
    quest->LoadQuestObjective(objective->Fetch());
    auto addon = WorldDatabase.Query(
        "SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,RewardMailTemplateID,RewardMailDelay,"
        "RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMinRepValue,RequiredMaxRepFaction,RequiredMaxRepValue,"
        "ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=264");
    Require(bool(addon), "Missing original pendant provision");
    quest->LoadQuestTemplateAddon(addon->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER);
    Require(quest->SourceItemId == Pendant && quest->SourceItemIdCount == 1, "Quest must supply one pendant");
    Require(bool(WorldDatabase.Query("SELECT 1 FROM creature c JOIN creature_queststarter q ON q.id=c.id "
        "WHERE q.quest=264 AND c.id=5543 AND c.map=1 AND c.zoneId=1638")), "Clarice must be placed in Thunder Bluff");
    auto spell = sSpellMgr->GetSpellInfo(PlacePendant);
    Require(spell && spell->GetEffect(EFFECT_0)->Effect == SPELL_EFFECT_OPEN_LOCK &&
        spell->GetEffect(EFFECT_0)->MiscValue == LOCKTYPE_OPEN_KNEELING, "The linked spell does not open the actual kneeling lock");
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE pendant_questender AS SELECT * FROM gameobject_questender WHERE quest=264");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE gameobject_questender AS SELECT * FROM pendant_questender");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE pendant_conditions AS SELECT * FROM conditions WHERE SourceTypeOrReferenceId=17 AND SourceEntry=8001");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE conditions AS SELECT * FROM pendant_conditions");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sQuestDataStore->LoadGameobjectInvolvedRelations();
    sConditionMgr->LoadConditions();
    Require(sConditionMgr->GetConditionsForNotGroupedEntry(CONDITION_SOURCE_TYPE_SPELL, PlacePendant).size() == 6,
        "Native conditional admission did not load both accepted quest states");
    return quest;
}

struct PendantObject : GameObject
{
    ~PendantObject()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<GameObject*>(this), false);
        ResetMap();
    }
};

void CreatePendantObject(PendantObject& object, Map& map, uint32 entry)
{
    auto rows = WorldDatabase.PQuery("SELECT guid,position_x,position_y,position_z,orientation,rotation0,rotation1,rotation2,rotation3 "
        "FROM gameobject WHERE id=%u AND map=0", entry);
    Require(rows && rows->GetRowCount() == 1, "Missing Silverpine tomb object placement");
    auto row = rows->Fetch();
    Position position{row[1].GetFloat(),row[2].GetFloat(),row[3].GetFloat(),row[4].GetFloat()};
    map.LoadGrid(position.GetPositionX(), position.GetPositionY());
    Require(object.Create(row[0].GetUInt32(), entry, &map, 1, position,
        G3D::Quat(row[5].GetFloat(),row[6].GetFloat(),row[7].GetFloat(),row[8].GetFloat()), 255, GO_STATE_READY) &&
        map.AddToMap(static_cast<GameObject*>(&object)), "Could not publish the original tomb object");
}

void CheckPendant(Quest const* quest)
{
    Map map(0, 300, 0, DIFFICULTY_NONE);
    PendantObject tomb, plaque;
    CreatePendantObject(tomb, map, Tomb);
    CreatePendantObject(plaque, map, Plaque);
    auto lock = sLockStore.LookupEntry(tomb.GetGOInfo()->GetLockId());
    bool kneelingLock = false;
    if (lock)
        for (std::size_t index = 0; index < std::size(lock->Type); ++index)
            kneelingLock |= lock->Type[index] == LOCK_KEY_SKILL && lock->Index[index] == LOCKTYPE_OPEN_KNEELING;
    std::cout << "CASE tomb type=" << unsigned(tomb.GetGoType()) << " lock=" << tomb.GetGOInfo()->GetLockId()
        << " kneeling=" << kneelingLock << std::endl;
    Require(tomb.GetGoType() == GAMEOBJECT_TYPE_QUESTGIVER && lock && lock->ID == 179 && kneelingLock,
        "Actual tomb lock metadata changed");
    LootPlayer owner(SupplySession(), map, 264, tomb.GetPosition()), other(SupplySession(), map, 265, tomb.GetPosition());
    for (auto player : {&owner, &other})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_UNDEAD_PLAYER);
        player->setFactionForRace(RACE_UNDEAD_PLAYER);
        player->InitDisplayIds();
        player->SetLevel(15);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 20000);
    }
    auto use = [](LootPlayer& player, GameObject& object)
    {
        auto pendant = player.GetItemByEntry(Pendant);
        Require(pendant, "Expected one real carried pendant");
        return player.CastSpell(&object, PlacePendant, false, pendant);
    };
    GiveSupply(other, Pendant, 1);
    Require(use(other, tomb) != SPELL_CAST_OK, "A retained pendant was usable without the quest");
    owner.AddQuestAndCheckCompletion(quest, nullptr);
    Require(owner.GetItemCount(Pendant) == 1 && owner.CanRewardQuest(quest, false), "Native acceptance did not provision the original delivery");
    Require(use(owner, plaque) != SPELL_CAST_OK, "The pendant accepted an unrelated plaque");
    Require(use(owner, tomb) == SPELL_CAST_OK, "The real pendant refused its actual kneeling lock");
    Tick(owner, 8000);
    Require(owner.PlayerTalkClass->GetQuestMenu().HasItem(Delivery) && owner.GetItemCount(Pendant) == 1,
        "Tombstone use did not open the real turn-in while preserving the pendant");
    owner.RewardQuest(quest, 0, &tomb, false);
    Require(owner.GetQuestRewardStatus(Delivery) && !owner.GetItemCount(Pendant) && tomb.isSpawned(),
        "Turn-in failed to consume the pendant or wrongly removed the shared tomb");
    other.AddQuestAndCheckCompletion(quest, nullptr);
    AbandonSupply(other, Delivery);
    Require(!other.GetItemCount(Pendant), "Abandon left the pendant in the bag");
    std::cout << "PASS native quest supply, lock and target condition, real tomb quest menu, reward and abandon cleanup\n";
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
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "The pendant test requires the isolated character schema");
        auto quest = LoadPendant();
        sObjectMgr->LoadPlayerInfo();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckPendant(quest);
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
