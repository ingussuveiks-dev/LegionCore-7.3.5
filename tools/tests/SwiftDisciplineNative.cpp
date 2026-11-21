#define LOST_ARMY_NATIVE_SUPPORT_ONLY
#include "LostArmyLootNative.cpp"
#include "GameTime.h"
#include "SmartAI.h"

struct SwiftConditionStoreTag
{
    using Type = ConditionContainer ConditionMgr::*;
    friend Type Access(SwiftConditionStoreTag);
};
template struct FixtureAccess<SwiftConditionStoreTag, &ConditionMgr::ConditionStore>;

struct SwiftSmartStoreTag
{
    using Type = SmartAIEventMap (SmartAIMgr::*)[SMART_SCRIPT_TYPE_MAX];
    friend Type Access(SwiftSmartStoreTag);
};
template struct FixtureAccess<SwiftSmartStoreTag, &SmartAIMgr::mEventMap>;

namespace
{
constexpr uint32 QuestId = 9066;
constexpr uint32 Wand = 22473;
constexpr uint32 Discipline = 27907;
constexpr uint32 Ralen = 15941;
constexpr uint32 Meledor = 15945;

Quest const* LoadQuest()
{
    auto rows = WorldDatabase.Query(
#include "NativeSwiftDisciplineQuest.inc"
    );
    Require(rows && rows->GetRowCount() == 1, "Missing Swift Discipline quest");
    auto quest = new Quest(rows->Fetch());
    auto& vector = sQuestDataStore->*Access(QuestVectorTag{});
    vector.resize(45089);
    vector[QuestId] = quest;
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(vector.size());
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates())[QuestId] = quest;
    auto objectives = WorldDatabase.Query(
        "SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=9066 ORDER BY StorageIndex");
    Require(objectives && objectives->GetRowCount() == 3 && quest->SourceItemId == Wand, "Unexpected Swift Discipline objectives");
    do
        quest->LoadQuestObjective(objectives->Fetch());
    while (objectives->NextRow());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST | QUEST_SPECIAL_FLAGS_DELIVER);
    auto addon = WorldDatabase.Query("SELECT ProvidedItemCount FROM quest_template_addon WHERE ID=9066");
    Require(addon && addon->Fetch()[0].GetUInt8() == 1, "Expected one provided wand");
    quest->SourceItemIdCount = addon->Fetch()[0].GetUInt8();
    return quest;
}

void LoadSourceConditions()
{
    auto rows = WorldDatabase.Query(
        "SELECT ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition "
        "FROM conditions WHERE SourceTypeOrReferenceId=17 AND SourceGroup=0 AND SourceEntry=27907 AND SourceId=0 ORDER BY ElseGroup,ConditionTypeOrReference");
#ifdef SWIFT_DISCIPLINE_BASELINE
    Require(rows && rows->GetRowCount() == 2, "Expected the two original spell conditions");
#else
    Require(rows && rows->GetRowCount() == 6, "Expected three conditions per apprentice");
#endif
    auto& store = sConditionMgr->*Access(SwiftConditionStoreTag{});
    do
    {
        auto row = rows->Fetch();
        auto condition = new Condition;
        condition->SourceType = CONDITION_SOURCE_TYPE_SPELL;
        condition->SourceEntry = Discipline;
        condition->ElseGroup = row[0].GetUInt32();
        condition->ConditionType = ConditionTypes(row[1].GetUInt32());
        condition->ConditionTarget = row[2].GetUInt8();
        condition->ConditionValue1 = row[3].GetUInt32();
        condition->ConditionValue2 = row[4].GetUInt32();
        condition->ConditionValue3 = row[5].GetUInt32();
        condition->NegativeCondition = row[6].GetBool();
        Require(sConditionMgr->isConditionTypeValid(condition) && (sConditionMgr->*Access(ValidateSourceTag{}))(condition), "Native spell condition rejected");
        store[CONDITION_SOURCE_TYPE_SPELL][Discipline].push_back(condition);
    } while (rows->NextRow());
}

void LoadApprenticeEvents()
{
    auto rows = WorldDatabase.Query(
        "SELECT entryorguid,id,link,event_type,event_phase_mask,event_chance,event_flags,event_param1,event_param2,event_param3,event_param4,"
        "action_type,action_param1,target_type FROM smart_scripts WHERE source_type=0 AND entryorguid IN(15941,15945)");
    Require(rows && rows->GetRowCount() == 2, "Unexpected apprentice SmartAI rows");
    auto& store = sSmartScriptMgr->*Access(SwiftSmartStoreTag{});
    do
    {
        auto row = rows->Fetch();
        SmartScriptHolder holder;
        holder.entryOrGuid = row[0].GetInt64();
        holder.event_id = row[1].GetUInt32();
        holder.link = row[2].GetUInt32();
        holder.event.type = SMART_EVENT(row[3].GetUInt32());
        holder.event.event_phase_mask = row[4].GetUInt32();
        holder.event.event_chance = row[5].GetUInt32();
        holder.event.event_flags = row[6].GetUInt32();
        holder.event.spellHit = {row[7].GetUInt32(), row[8].GetUInt32(), row[9].GetUInt32(), row[10].GetUInt32()};
        holder.action.type = SMART_ACTION(row[11].GetUInt32());
        holder.action.killedMonster.creature = row[12].GetUInt32();
        holder.target = {};
        holder.target.type = SMARTAI_TARGETS(row[13].GetUInt32());
        Require(holder.event.type == SMART_EVENT_SPELLHIT && holder.event.spellHit.spell == Discipline &&
            holder.action.type == SMART_ACTION_CALL_KILLEDMONSTER && holder.target.type == SMART_TARGET_ACTION_INVOKER,
            "The actual spell-hit credit graph changed");
        store[SMART_SCRIPT_TYPE_CREATURE][holder.entryOrGuid].push_back(holder);
    } while (rows->NextRow());
}

struct Apprentice : Actor
{
    using Actor::Actor;
    void AddToWorld() override
    {
        sObjectAccessor->AddObject(static_cast<Creature*>(this));
        Unit::AddToWorld();
        Require(AIM_Initialize(new SmartAI(this)), "Native apprentice SmartAI initialization failed");
    }
};

WorldSession* Session()
{
    return new WorldSession(0, "Swift Discipline native probe", {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {});
}

SpellCastResult CastWand(LootPlayer& player, Creature& target)
{
    auto const spell = sSpellMgr->GetSpellInfo(Discipline);
    player.GetGlobalCooldownMgr().CancelGlobalCooldown(spell);
    player.RemoveSpellCooldown(Discipline, false);
    auto result = player.CastSpell(&target, Discipline, false, player.GetItemByEntry(Wand));
    Tick(player, 5000);
    return result;
}

void CheckApprentices(Quest const* quest)
{
    auto rows = WorldDatabase.Query("SELECT position_x,position_y,position_z,orientation FROM creature WHERE guid=10359 AND id=15941 AND map=530");
    Require(bool(rows), "Missing actual Ralen spawn");
    auto row = rows->Fetch();
    Position pos{row[0].GetFloat(), row[1].GetFloat(), row[2].GetFloat(), row[3].GetFloat()};
    Map map(530, 300, 9999921, DIFFICULTY_NONE);
    LootPlayer owner(Session(), map, 337, pos), other(Session(), map, 338, pos);
    for (auto player : {&owner, &other})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_BLOODELF);
        player->setFactionForRace(RACE_BLOODELF);
    }
    Apprentice ralen(&map, Ralen, 192, pos), meledor(&map, Meledor, 193, pos);
    Actor instructor(&map, 15970, 194, pos);
    Require(map.AddToMap(static_cast<Creature*>(&ralen)) && map.AddToMap(static_cast<Creature*>(&meledor)) &&
        map.AddToMap(static_cast<Creature*>(&instructor)), "Missing actual apprentice templates");
    Require(other.AddItem(Wand, 1), "Unable to place a retained wand in the counterfactual bag");
#ifdef SWIFT_DISCIPLINE_BASELINE
    Require(CastWand(other, ralen) == SPELL_CAST_OK, "Original spell should accept a retained wand before the quest");
    owner.AddQuestAndCheckCompletion(quest, nullptr);
    Require(CastWand(owner, ralen) == SPELL_CAST_OK && owner.GetQuestObjectiveData(QuestId, Ralen) == 1, "Original SmartAI did not credit Ralen");
    Require(CastWand(owner, ralen) == SPELL_CAST_OK, "Original spell already denied repeated discipline");
    std::cout << "EXPECTED_BROKEN a retained wand cast without the quest and on an already credited apprentice\n";
#else
    Require(CastWand(other, ralen) != SPELL_CAST_OK && CastWand(other, meledor) != SPELL_CAST_OK, "A retained wand works without the quest");
    owner.AddQuestAndCheckCompletion(quest, nullptr);
    Require(owner.GetItemCount(Wand) == 1 && CastWand(owner, instructor) != SPELL_CAST_OK, "Wrong NPC admitted or acceptance failed");
    Require(CastWand(owner, ralen) == SPELL_CAST_OK && owner.GetQuestObjectiveData(QuestId, Ralen) == 1 &&
        !owner.GetQuestObjectiveData(QuestId, Meledor), "Ralen did not advance exactly his own objective");
    Require(CastWand(owner, ralen) != SPELL_CAST_OK && owner.GetQuestObjectiveData(QuestId, Ralen) == 1, "Repeat use was not rejected");
    Require(CastWand(owner, meledor) == SPELL_CAST_OK && owner.GetQuestObjectiveData(QuestId, Meledor) == 1 &&
        !other.GetQuestObjectiveData(QuestId, Meledor) && owner.CanRewardQuest(quest, false), "Meledor did not advance the correct owner");
    Require(CastWand(owner, meledor) != SPELL_CAST_OK && owner.GetItemCount(Wand) == 1, "Completed quest can still discipline");
    owner.RewardQuest(quest, 0, &instructor, false);
    Require(owner.GetQuestRewardStatus(QuestId) && !owner.GetItemCount(Wand), "Actual reward left the wand");
    other.AddQuestAndCheckCompletion(quest, nullptr);
    Require(other.TakeQuestSourceItem(QuestId, false) && !other.GetItemCount(Wand), "Abandon left the wand");
    other.RemoveActiveQuest(QuestId);
    other.SetQuestSlot(0, 0);
    other.AddQuestAndCheckCompletion(quest, nullptr);
    Require(other.GetItemCount(Wand) == 1 && CastWand(other, meledor) == SPELL_CAST_OK, "Reaccept did not reopen the apprentice objective");
    std::cout << "PASS native apprentice target groups, unfinished objectives, two separate credits, reward and retry\n";
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
        Quest const* quest = LoadQuest();
        LoadSourceConditions();
        LoadApprenticeEvents();
        sObjectMgr->LoadItemTemplates();
        sObjectMgr->LoadItemTemplateAddon();
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
        LoginDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
#ifndef SWIFT_DISCIPLINE_BASELINE
        auto characters = sConfigMgr->GetStringDefault("CharacterDatabaseInfo", "");
        Require(characters.substr(characters.find_last_of(';') + 1) == "fixture_char", "Reward requires the disposable character schema");
        CharacterDatabase.SetConnectionInfo(characters, 1, 1);
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto fixture = CharacterDatabase.Query("SELECT DATABASE(), (SELECT COUNT(*) FROM characters)");
        Require(fixture && fixture->Fetch()[0].GetString() == "fixture_char" && !fixture->Fetch()[1].GetUInt32(), "Reward requires an empty fixture");
#endif
        HashMapHolder<Player>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckApprentices(quest);
#ifndef SWIFT_DISCIPLINE_BASELINE
        CharacterDatabase.Close();
#endif
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
