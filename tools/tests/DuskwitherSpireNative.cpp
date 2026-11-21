#define LOST_ARMY_NATIVE_SUPPORT_ONLY
#include "LostArmyLootNative.cpp"
#include "GameObject.h"
#include "GameObjectAI.h"
#include "GameTime.h"

void AddSC_eversong_woods();
void AddSC_ghostlands();

namespace
{
constexpr uint32 QuestId = 8889;
constexpr uint32 Jewel = 24337;
constexpr uint32 Deactivate = 31614;

Quest const* LoadQuest()
{
    auto rows = WorldDatabase.Query(
#include "NativeDuskwitherQuest.inc"
    );
    Require(rows && rows->GetRowCount() == 1, "Native Spire quest is unavailable");
    auto quest = new Quest(rows->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    quests[quest->Id] = quest;
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates())[quest->Id] = quest;
    auto objectives = WorldDatabase.Query(
        "SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=8889 ORDER BY StorageIndex");
    Require(objectives && objectives->GetRowCount() == 4, "Expected three sources and one jewel objective");
    do
    {
        auto fields = objectives->Fetch();
        quest->LoadQuestObjective(fields);
        quest->SetSpecialFlag(fields[2].GetUInt8() == QUEST_OBJECTIVE_ITEM ? QUEST_SPECIAL_FLAGS_DELIVER : QUEST_SPECIAL_FLAGS_CAST);
    } while (objectives->NextRow());
    const auto addon = WorldDatabase.Query("SELECT ProvidedItemCount FROM quest_template_addon WHERE ID=8889");
    Require(addon && addon->Fetch()[0].GetUInt8() == 1 && quest->SourceItemId == Jewel &&
        quest->Objectives.back().ObjectID == Jewel && quest->Objectives.back().Amount == 1, "Jewel provision or delivery changed");
    quest->SourceItemIdCount = addon->Fetch()[0].GetUInt8();
    return quest;
}

struct PowerSource : GameObject
{
    ~PowerSource()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<GameObject*>(this), false);
        if (FindMap())
            ResetMap();
    }

    void Spawn(Map& map, uint32 spawn, uint32 entry, Position const& pos)
    {
        m_DBTableGuid = spawn;
        auto guid = sObjectMgr->GetGenerator<HighGuid::GameObject>()->Generate();
        Require(Create(guid, entry, &map, 1, pos, G3D::Quat(0, 0, 0, 1), 100, GO_STATE_READY) &&
            map.AddToMap(static_cast<GameObject*>(this)), "Power source failed to enter the native map");
        Update(100);
    }
};

Position Location(uint32 spawn, uint32 entry)
{
    auto rows = WorldDatabase.PQuery("SELECT position_x,position_y,position_z,orientation FROM gameobject WHERE id=%u AND guid=%u AND map=530 AND phaseMask=1", entry, spawn);
    Require(rows && rows->GetRowCount() == 1, "Actual Eversong source spawn missing");
    auto row = rows->Fetch();
    return {row[0].GetFloat(), row[1].GetFloat(), row[2].GetFloat(), row[3].GetFloat()};
}

void Move(LootPlayer& player, Position const& pos)
{
    player.GetMap()->PlayerRelocation(&player, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation());
}

void Activate(LootPlayer& player, GameObject& source, Item* key)
{
    player.CastSpell(&source, Deactivate, false, key);
    Tick(player, 5000);
}

WorldSession* Session()
{
    return new WorldSession(0, std::string("Duskwither Spire native probe"), {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {});
}

void CheckSpire(Quest const* quest)
{
    uint32 const entries[]{180916, 180919, 180920};
    uint32 const spawns[]{6944, 17583, 6953};
    Position const first = Location(spawns[0], entries[0]);
    Map map(530, 300, 9999797, DIFFICULTY_NONE);
    LootPlayer owner(Session(), map, 120, first), other(Session(), map, 121, first);
    for (auto player : {&owner, &other})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_BLOODELF);
        player->setFactionForRace(RACE_BLOODELF);
    }
    PowerSource sources[3];
    for (unsigned i = 0; i < 3; ++i)
    {
        Require(quest->Objectives[i].Type == QUEST_OBJECTIVE_GAMEOBJECT && uint32(quest->Objectives[i].ObjectID) == entries[i] &&
            quest->Objectives[i].Amount == 1, "Spire source objectives changed");
        sources[i].Spawn(map, spawns[i], entries[i], Location(spawns[i], entries[i]));
        Require(sources[i].GetGOInfo()->GetLockId() == 1673 && sources[i].GetGOInfo()->GetAutoCloseTime() == 1, "Spire lock or animation changed");
        Require(!sources[i].ActivateToQuest(&owner), "A source appears as a quest target before acceptance");
    }
    owner.AddQuestAndCheckCompletion(quest, nullptr);
    Item* jewel = owner.GetItemByEntry(Jewel);
    Require(jewel && owner.GetItemCount(Jewel) == 1 && owner.GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE,
        "Normal acceptance did not give one jewel for the pending quest");
#ifdef DUSKWITHER_BASELINE
    for (auto& source : sources)
    {
        Move(owner, source.GetPosition());
        source.Use(&owner);
        Require(owner.GetQuestObjectiveData(QuestId, source.GetEntry()) == 1, "Baseline missed a bare click giving credit");
    }
    Require(owner.CanRewardQuest(quest, false) && owner.GetItemCount(Jewel) == 1, "Baseline did not bypass all jewel casts");
    std::cout << "EXPECTED_BROKEN three bare clicks completed the Spire with an unused jewel\n";
#else
    PowerSource wrong;
    wrong.Spawn(map, 12008, 181359, first);
    Activate(owner, wrong, jewel);
    Require(!wrong.HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE), "Worked on the wrong native spell lock");
    for (auto& source : sources)
    {
        Move(owner, source.GetPosition());
        Move(other, source.GetPosition());
        Require(source.ActivateToQuest(&owner) && !source.ActivateToQuest(&other), "Quest visibility on a power source is incorrect");
        source.Use(&other);
        source.Use(&owner);
        source.AI()->GossipHello(&owner, false);
        Activate(owner, source, nullptr);
        Require(!owner.GetQuestObjectiveData(QuestId, source.GetEntry()) && !source.HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE),
            "A direct click, report-use or a cast without the jewel deactivated a source");
        Activate(owner, source, jewel);
        Require(owner.GetQuestObjectiveData(QuestId, source.GetEntry()) == 1 && !source.ActivateToQuest(&owner) &&
            !other.GetQuestObjectiveData(QuestId, source.GetEntry()) && owner.GetItemCount(Jewel) == 1,
            "Native activation did not award exactly one source and keep the jewel");
        Activate(owner, source, jewel);
        Require(owner.GetQuestObjectiveData(QuestId, source.GetEntry()) == 1, "A repeated power source awarded duplicate credit");
    }
    Require(owner.GetQuestStatus(QuestId) == QUEST_STATUS_COMPLETE && owner.CanRewardQuest(quest, false), "All three sources and jewel did not permit reward");
    owner.RewardQuest(quest, 0, nullptr, false);
    Require(owner.GetQuestRewardStatus(QuestId) && !owner.GetItemCount(Jewel), "The native reward did not consume the jewel");
    other.AddQuestAndCheckCompletion(quest, nullptr);
    Require(other.GetItemCount(Jewel) == 1 && other.TakeQuestSourceItem(QuestId, false) && !other.GetItemCount(Jewel),
        "Abandon did not remove the provided jewel");
    other.RemoveActiveQuest(QuestId);
    other.SetQuestSlot(0, 0);
    other.AddQuestAndCheckCompletion(quest, nullptr);
    Require(other.GetItemCount(Jewel) == 1 && other.GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE,
        "Retry did not provide a new jewel");
    std::cout << "PASS three native source casts, bare/foreign/repeat rejection, bag cleanup on reward and abandon\n";
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
        sObjectMgr->LoadItemTemplates();
        sObjectMgr->LoadItemTemplateAddon();
        sObjectMgr->LoadPageTexts();
        sObjectMgr->LoadGameObjectTemplate();
        sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
        AddSC_ghostlands();
#ifndef DUSKWITHER_BASELINE
        AddSC_eversong_woods();
#endif
        sScriptMgr->SwapScriptContext(true);
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
        LoginDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
#ifndef DUSKWITHER_BASELINE
        auto characters = sConfigMgr->GetStringDefault("CharacterDatabaseInfo", "");
        Require(characters.substr(characters.find_last_of(';') + 1) == "fixture_char", "Reward requires the disposable character schema");
        CharacterDatabase.SetConnectionInfo(characters, 1, 1);
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto fixture = CharacterDatabase.Query("SELECT DATABASE(), (SELECT COUNT(*) FROM characters)");
        Require(fixture && fixture->Fetch()[0].GetString() == "fixture_char" && !fixture->Fetch()[1].GetUInt32(),
            "Quest reward must use an empty fixture");
#endif
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckSpire(quest);
#ifndef DUSKWITHER_BASELINE
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
