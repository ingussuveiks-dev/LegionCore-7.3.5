#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "ScriptSystem.h"

void AddSC_lost_isle();

struct CluckConditionTag
{
    using Type = ConditionContainer ConditionMgr::*;
    friend Type Access(CluckConditionTag);
};
template struct FixtureAccess<CluckConditionTag, &ConditionMgr::ConditionStore>;

namespace
{
constexpr uint32 Cluster = 24671;
constexpr uint32 Fireworks = 52712;
constexpr uint32 AttachFireworks = 71170;
constexpr uint32 Clucker = 38111;
constexpr uint32 Capture = 38117;

Quest const* LoadCluck()
{
    auto row = WorldDatabase.Query(
#include "NativeClusterCluck.inc"
    );
    Require(bool(row), "Missing Cluster Cluck template");
    auto quest = new Quest(row->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    quests[Cluster] = quest;
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(Cluster, quest);
    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=24671");
    Require(objective && objective->GetRowCount() == 1 && objective->Fetch()[4].GetUInt32() == Capture &&
        objective->Fetch()[5].GetUInt32() == 10, "Missing real clucker objective");
    quest->LoadQuestObjective(objective->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
    auto addon = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,RewardMailTemplateID,RewardMailDelay,"
        "RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMinRepValue,RequiredMaxRepFaction,RequiredMaxRepValue,"
        "ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=24671");
    Require(bool(addon), "Missing native provided item count");
    quest->LoadQuestTemplateAddon(addon->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
    Require(quest->SourceItemId == Fireworks && quest->SourceItemIdCount == 1, "Original provided item changed");

    sObjectMgr->LoadItemTemplates();
    auto rows = WorldDatabase.Query("SELECT ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2 "
        "FROM conditions WHERE SourceTypeOrReferenceId=17 AND SourceGroup=0 AND SourceEntry=71170 AND ElseGroup=0");
    Require(rows && rows->GetRowCount() == 4, "The actual fireworks spell conditions are missing");
    do
    {
        auto fields = rows->Fetch();
        auto condition = new Condition;
        condition->SourceType = CONDITION_SOURCE_TYPE_SPELL;
        condition->SourceEntry = AttachFireworks;
        condition->ConditionType = ConditionTypes(fields[0].GetUInt32());
        condition->ConditionTarget = fields[1].GetUInt8();
        condition->ConditionValue1 = fields[2].GetUInt32();
        condition->ConditionValue2 = fields[3].GetUInt32();
        Require((sConditionMgr->*Access(ValidateSourceTag{}))(condition) && sConditionMgr->isConditionTypeValid(condition),
            "Invalid fireworks condition");
        (sConditionMgr->*Access(CluckConditionTag{}))[condition->SourceType][AttachFireworks].push_back(condition);
    } while (rows->NextRow());

    sScriptSystemMgr->LoadScriptWaypoints();
    auto const& path = sScriptSystemMgr->GetPointMoveList(Clucker);
    Require(path.size() == 12 && path.back().uiPointId == 12, "Native clucker escort path is missing");
    auto spell = sSpellMgr->GetSpellInfo(AttachFireworks);
    auto item = sObjectMgr->GetItemTemplate(Fireworks);
    Require(spell && item && item->Effects.size() == 1 && item->Effects.front()->SpellID == AttachFireworks,
        "Matching native item spell is absent");
    return quest;
}

struct CluckActor : Actor
{
    using Actor::Actor;
    void AddToWorld() override
    {
        sObjectAccessor->AddObject(static_cast<Creature*>(this));
        Unit::AddToWorld();
        Require(AIM_Initialize(sScriptMgr->GetCreatureAI(this)), "Linked clucker AI could not initialize");
    }
};

SpellCastResult UseFireworks(LootPlayer& player, Unit& target)
{
    auto result = player.CastSpell(&target, AttachFireworks, false, player.GetItemByEntry(Fireworks));
    Tick(player, 100);
    return result;
}

void CheckCluck(Quest const* quest)
{
    auto row = WorldDatabase.Query("SELECT position_x,position_y,position_z,orientation FROM creature "
        "WHERE id=38111 AND map=648 ORDER BY guid LIMIT 1");
    Require(bool(row), "Missing a real Wild Clucker spawn");
    auto fields = row->Fetch();
    Position point{fields[0].GetFloat(),fields[1].GetFloat(),fields[2].GetFloat(),fields[3].GetFloat()};
    Map map(648, 300, 0, DIFFICULTY_NONE);
    LootPlayer owner(SupplySession(), map, 24671, point);
    LootPlayer other(SupplySession(), map, 24672, point);
    CluckActor clucker(&map, Clucker, 43620, point);
    Require(map.AddToMap(static_cast<Creature*>(&clucker)), "Clucker could not publish");
    GiveSupply(owner, Fireworks, 1);
    Require(UseFireworks(owner, clucker) != SPELL_CAST_OK && !owner.GetQuestObjectiveData(Cluster, Capture),
        "The item could be used without the quest");
    owner.DestroyItemCount(Fireworks, 1, true);
    owner.AddQuestAndCheckCompletion(quest, nullptr);
    other.AddQuestAndCheckCompletion(quest, nullptr);
    Require(owner.GetItemCount(Fireworks) == 1 && other.GetItemCount(Fireworks) == 1, "Quest acceptance does not provide one native item");
    Actor decoy(&map, 299, 43621, point);
    Require(map.AddToMap(static_cast<Creature*>(&decoy)), "Decoy could not publish");
    Require(UseFireworks(owner, decoy) != SPELL_CAST_OK && owner.GetItemCount(Fireworks) == 1,
        "The item could be used on an unrelated creature");
    Require(UseFireworks(owner, clucker) == SPELL_CAST_OK && owner.GetQuestObjectiveData(Cluster, Capture) == 1,
        "A valid native item cast did not capture the original creature");
    UseFireworks(other, clucker);
    Require(!other.GetQuestObjectiveData(Cluster, Capture) && owner.GetQuestObjectiveData(Cluster, Capture) == 1,
        "A clucker already being captured was counted a second time");
    AbandonSupply(owner, Cluster);
    Require(!owner.GetItemCount(Fireworks) && UseFireworks(owner, clucker) != SPELL_CAST_OK,
        "Abandon left an active fireworks item");
    owner.AddQuestAndCheckCompletion(quest, nullptr);
    Require(owner.GetItemCount(Fireworks) == 1 && !owner.GetQuestObjectiveData(Cluster, Capture),
        "Retry did not start with the correct item and a clean counter");
    LootPlayer finishing(SupplySession(), map, 24673, point);
    finishing.AddQuestAndCheckCompletion(quest, nullptr);
    finishing.SetQuestObjectiveData(quest, &quest->GetObjectives()[0], 9);
    CluckActor finalClucker(&map, Clucker, 43622, point);
    Require(map.AddToMap(static_cast<Creature*>(&finalClucker)), "Last clucker could not publish");
    Require(UseFireworks(finishing, finalClucker) == SPELL_CAST_OK && finishing.CanRewardQuest(quest, false),
        "The tenth capture did not open reward admission");
    finishing.RewardQuest(quest, 0, nullptr, false);
    Require(finishing.IsQuestRewarded(Cluster) && !finishing.GetItemCount(Fireworks), "Turn-in left the supplied item");
    std::cout << "PASS native item admission, wrong target, single clucker credit, abandon and reaccept\n";
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{648, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{648, {}}});
        LoadGameObjectModelList(argv[2]);
        sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
        AddSC_lost_isle();
        sScriptMgr->SwapScriptContext(true);
        auto quest = LoadCluck();
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
        LoginDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
        auto characters = sConfigMgr->GetStringDefault("CharacterDatabaseInfo", "");
        Require(characters.substr(characters.find_last_of(';') + 1) == "fixture_char", "Reward writes require a disposable character schema");
        CharacterDatabase.SetConnectionInfo(characters, 1, 1);
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto fixture = CharacterDatabase.Query("SELECT DATABASE(), (SELECT COUNT(*) FROM characters)");
        Require(fixture && fixture->Fetch()[0].GetString() == "fixture_char" && !fixture->Fetch()[1].GetUInt32(), "Reward requires an empty fixture");
        sObjectMgr->LoadPlayerInfo();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckCluck(quest);
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
