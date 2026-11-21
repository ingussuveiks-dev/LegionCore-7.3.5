#define MASTER_LOOT_NATIVE_SUPPORT_ONLY
#include "MasterLootNative.cpp"
#include "GameObject.h"
#include "GameObjectAI.h"
#include "GameTime.h"
#include "ScriptsData.h"
#include <thread>

void AddSC_ghostlands();

struct AquantionScriptTickTag { using Type = void (Map::*)(); friend Type Access(AquantionScriptTickTag); };
template struct FixtureAccess<AquantionScriptTickTag, &Map::ScriptsProcess>;

namespace
{
constexpr uint32 QuestId = 9174;
constexpr uint32 Medallions = 22675;
constexpr uint32 Opening = 28226;
constexpr uint32 Boss = 16292;

Quest const* LoadQuest()
{
    auto rows = WorldDatabase.Query(
#include "NativeAquantionQuest.inc"
    );
    Require(bool(rows) && rows->GetRowCount() == 1, "Missing Aquantion quest");
    auto quest = new Quest(rows->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    quests[QuestId] = quest;
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size() - 1);
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates())[quest->Id] = quest;
    auto objectives = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=9174 ORDER BY StorageIndex");
    Require(bool(objectives) && objectives->GetRowCount() == 2, "Missing native Aquantion objectives");
    do
        quest->LoadQuestObjective(objectives->Fetch());
    while (objectives->NextRow());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_DELIVER);
    quest->SourceItemIdCount = 1;
    return quest;
}

void LoadLegacyEvent()
{
    auto rows = WorldDatabase.Query("SELECT delay,command,datalong,datalong2,dataint,x,y,z,o FROM event_scripts WHERE id=9980");
#ifdef AQUANTION_UNGUARDED_BASELINE
    Require(bool(rows) && rows->GetRowCount() == 1, "Counterfactual requires the original event and data");
    auto row = rows->Fetch();
    ScriptInfo info{};
    info.type = SCRIPTS_EVENT;
    info.id = 9980;
    info.delay = row[0].GetUInt32();
    info.command = ScriptCommands(row[1].GetUInt32());
    info.TempSummonCreature = {row[2].GetUInt32(), row[3].GetUInt32(), row[4].GetInt32(),
        row[5].GetFloat(), row[6].GetFloat(), row[7].GetFloat(), row[8].GetFloat()};
    Require(info.command == SCRIPT_COMMAND_TEMP_SUMMON_CREATURE && info.TempSummonCreature.CreatureEntry == Boss,
        "Counterfactual loaded a different event");
    sEventScripts[info.id].emplace(info.delay, info);
#else
    Require(!rows, "The migrated altar still has a second legacy summon");
#endif
}

struct TestAltar : GameObject
{
    ~TestAltar()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<GameObject*>(this), false);
        if (FindMap())
            ResetMap();
    }

    void Spawn(Map& map, uint32 entry, Position const& pos)
    {
        auto guid = sObjectMgr->GetGenerator<HighGuid::GameObject>()->Generate();
        Require(Create(guid, entry, &map, 1, pos, G3D::Quat(0, 0, 0, 1), 255, GO_STATE_READY) &&
            map.AddToMap(static_cast<GameObject*>(this)) && map.GetGameObject(GetGUID()) == this, "Altar did not enter the grid");
        SetRespawnDelayTime(0);
        Update(10000);
    }
};

struct QuestPet : LootPet
{
    QuestPet(LootPlayer& owner, Map& map) : LootPet(owner, map) { m_ControlledByPlayer = true; }
};

Item* Accept(LootPlayer& player, Quest const* quest)
{
    player.SetQuestStatus(QuestId, QUEST_STATUS_INCOMPLETE);
    player.SetQuestSlot(0, QuestId);
    Require(player.GiveQuestSourceItem(quest) && player.GetItemCount(Medallions) == 1, "Native medallion handout failed");
    Item* item = player.GetItemByEntry(Medallions);
    Require(item, "Missing medallions after accepting");
    return item;
}

void Cast(LootPlayer& player, GameObject& altar, Item* item)
{
    player.CastSpell(&altar, Opening, false, item);
    Tick(player, 5000);
}

void ResetPresentation(TestAltar& altar)
{
    std::this_thread::sleep_for(std::chrono::seconds(4));
    GameTime::UpdateGameTimers();
    altar.Update(10000);
    altar.Update(10000);
    altar.Update(10000);
    Require(altar.isSpawned() && !altar.HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE), "Native shared altar did not reset");
}

std::list<Creature*> Aquantions(TestAltar& altar)
{
    std::list<Creature*> actors;
    altar.GetAliveCreatureListWithEntryInGrid(actors, Boss, 30.0f);
    return actors;
}

void CheckAltar(Quest const* quest, WorldSession* first, WorldSession* other)
{
    GameTime::UpdateGameTimers();
    auto rows = WorldDatabase.Query("SELECT position_x,position_y,position_z,orientation FROM gameobject WHERE id=181157 AND map=530");
    Require(bool(rows) && rows->GetRowCount() == 1, "Missing original Altar of Tidal Mastery");
    auto row = rows->Fetch();
    Position pos(row[0].GetFloat(), row[1].GetFloat(), row[2].GetFloat(), row[3].GetFloat());
    Map map(530, 300, 9999968, DIFFICULTY_NONE);
    LootPlayer owner(first, map, 100, pos), second(other, map, 101, pos);
    for (LootPlayer* player : {&owner, &second})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_BLOODELF);
        player->setFactionForRace(RACE_BLOODELF);
    }
    TestAltar altar;
    altar.Spawn(map, 181157, pos);
    altar.Use(&owner);

#ifdef AQUANTION_UNGUARDED_BASELINE
    Require(altar.GetGOInfo()->goober.eventID == 9980 && !owner.GetItemCount(Medallions), "Counterfactual has different altar admission");
    std::this_thread::sleep_for(std::chrono::seconds(3));
    GameTime::UpdateGameTimers();
    (map.*Access(AquantionScriptTickTag{}))();
    auto actors = Aquantions(altar);
    Require(actors.size() == 1 && owner.GetQuestStatus(QuestId) == QUEST_STATUS_NONE, "Counterfactual did not spawn without quest or item");
    actors.front()->DespawnOrUnsummon();
    map.RemoveAllObjectsInRemoveList();
    std::cout << "EXPECTED_BROKEN direct altar use scheduled public Aquantion before the quest and item checks\n";
#else
    Require(!altar.GetGOInfo()->goober.eventID && altar.GetGOInfo()->goober.questID == QuestId && altar.GetGOInfo()->GetLockId() == 1637,
        "Altered fixture did not preserve the native lock and quest");
    altar.AI()->GossipHello(&owner, false);
    altar.AI()->UpdateAI(5000);
    Require(Aquantions(altar).empty() && !altar.HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE), "Uninvolved player armed the altar");
    Item* medallions = Accept(owner, quest);
    Item* secondMedallions = Accept(second, quest);
    TestAltar unrelated;
    unrelated.Spawn(map, 181359, pos);
    altar.Use(&owner);
    Cast(owner, altar, nullptr);
    Cast(owner, unrelated, medallions);
    altar.AI()->UpdateAI(5000);
    Require(Aquantions(altar).empty() && !altar.HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE) && owner.GetItemCount(Medallions) == 1,
        "Direct use or invalid opening armed an encounter or removed the medallions");

    Cast(owner, altar, medallions);
    Require(altar.HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE), "Accepted native cast did not use the altar");
    altar.AI()->UpdateAI(1999);
    Require(Aquantions(altar).empty(), "Aquantion appeared before its original delay");
    Require(owner.TakeQuestSourceItem(QuestId, false) && !owner.GetItemCount(Medallions), "Native abandon did not remove the medallions");
    owner.RemoveActiveQuest(QuestId);
    owner.SetQuestSlot(0, 0);
    medallions = Accept(owner, quest);
    altar.AI()->UpdateAI(1);
    Require(Aquantions(altar).empty(), "An earlier abandoned attempt survived reacceptance");
    ResetPresentation(altar);

    Cast(owner, altar, medallions);
    map.PlayerRelocation(&owner, pos.GetPositionX() + 75, pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation());
    altar.AI()->UpdateAI(2000);
    Require(Aquantions(altar).empty(), "The encounter appeared after the opener left the altar");
    map.PlayerRelocation(&owner, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation());
    ResetPresentation(altar);

    ProbeParty party(owner, second);
    Cast(owner, altar, medallions);
    altar.AI()->UpdateAI(2000);
    auto actors = Aquantions(altar);
    Require(actors.size() == 1, "Native shared Aquantion was not published");
    Creature* boss = actors.front();
    Require(std::abs(boss->GetPositionX() - 7941.15f) < 0.01f && std::abs(boss->GetPositionY() + 7636.45f) < 0.01f &&
        std::abs(boss->GetPositionZ() - 116.19f) < 0.01f && second.canSeeOrDetect(boss) && boss->IsHostileTo(&owner),
        "Aquantion was relocated, privatized or made friendly");
    Require(owner.TakeQuestSourceItem(QuestId, false), "Could not abandon a published encounter");
    owner.RemoveActiveQuest(QuestId);
    owner.SetQuestSlot(0, 0);
    altar.AI()->UpdateAI(2000);
    Require(Aquantions(altar).size() == 1 && Aquantions(altar).front() == boss && second.canSeeOrDetect(boss),
        "Abandoning the quest broke the other player's published encounter");
    medallions = Accept(owner, quest);
    ResetPresentation(altar);
    Cast(second, altar, secondMedallions);
    altar.AI()->UpdateAI(2000);
    Require(Aquantions(altar).size() == 1 && !altar.HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE), "Another eligible player duplicated the live encounter");
    Require(!owner.GetQuestObjectiveData(QuestId, Boss) && !owner.CanRewardQuest(quest, false), "Summoning granted kill credit");
    QuestPet pet(owner, map);
    pet.DealDamage(boss, std::numeric_limits<uint32>::max());
    Tick(owner, 150);
    Tick(second, 150);
    Require(owner.GetQuestObjectiveData(QuestId, Boss) == 1 && second.GetQuestObjectiveData(QuestId, Boss) == 1 &&
        owner.CanRewardQuest(quest, false) && second.CanRewardQuest(quest, false) && owner.GetItemCount(Medallions) == 1,
        "Native pet kill did not give the owner and eligible group member one rewardable objective");
    Cast(owner, altar, medallions);
    altar.AI()->UpdateAI(2000);
    Require(Aquantions(altar).empty(), "A completed quest could resummon Aquantion");
    if (boss->IsInWorld())
        boss->DespawnOrUnsummon();
    map.RemoveAllObjectsInRemoveList();

    for (LootPlayer* player : {&owner, &second})
    {
        Require(player->TakeQuestSourceItem(QuestId, false), "Could not reset the completed fixture quest");
        player->RemoveActiveQuest(QuestId);
        player->SetQuestSlot(0, 0);
    }
    Cast(owner, altar, Accept(owner, quest));
    altar.AI()->UpdateAI(2000);
    actors = Aquantions(altar);
    Require(actors.size() == 1, "A fresh quest could not summon after the previous defeat");
    boss = actors.front();
    owner.DealDamage(boss, std::numeric_limits<uint32>::max());
    Tick(owner, 150);
    Tick(second, 150);
    Require(owner.GetQuestObjectiveData(QuestId, Boss) == 1 && owner.CanRewardQuest(quest, false) && !second.GetQuestObjectiveData(QuestId, Boss),
        "Native player kill did not preserve ordinary quest admission for group credit");
    if (boss->IsInWorld())
        boss->DespawnOrUnsummon();
    map.RemoveAllObjectsInRemoveList();
    std::cout << "PASS native Aquantion item and altar use, pending and published abandon, one shared spawn, player/pet/group credit, original delivery item\n";
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
        LoadLegacyEvent();
        sObjectMgr->LoadItemTemplates();
        sObjectMgr->LoadItemTemplateAddon();
        sObjectMgr->LoadPageTexts();
        sObjectMgr->LoadGameObjectTemplate();
        sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
#ifndef AQUANTION_UNGUARDED_BASELINE
        AddSC_ghostlands();
#endif
        sScriptMgr->SwapScriptContext(true);
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
        LoginDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        auto first = new WorldSession(0, std::string("Altar owner"), {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {});
        auto other = new WorldSession(0, std::string("Altar group member"), {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {});
        CheckAltar(quest, first, other);
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
