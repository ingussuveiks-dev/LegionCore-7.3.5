#define MASTER_LOOT_NATIVE_SUPPORT_ONLY
#include "MasterLootNative.cpp"
#include "GameEventMgr.h"
#include "CreatureTextMgr.h"
#include "GameObject.h"
#include "HallowsEndCrate.h"

void AddSC_npcs_special();

namespace
{
using HallowsEndCrate::Route;

void LoadCollectorQuests()
{
    auto rows = WorldDatabase.Query(
#include "NativeCollectorQuest.inc"
    );
    Require(bool(rows) && rows->GetRowCount() == 6, "Missing Collector quest rows");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new Quest(rows->Fetch());
        quests[quest->Id] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates())[quest->Id] = quest;
    } while (rows->NextRow());

    auto objectives = WorldDatabase.Query("SELECT ID, QuestID, Type, StorageIndex, ObjectID, Amount, Flags, Flags2, TaskStep, Description, Bugged "
        "FROM quest_objectives WHERE QuestID IN (29403,29411,29413,29427,29428,29429) ORDER BY QuestID,StorageIndex");
    Require(bool(objectives) && objectives->GetRowCount() == 4, "Missing real kill and delivery objectives");
    do
    {
        auto row = objectives->Fetch();
        Quest* quest = quests[row[1].GetUInt32()];
        quest->LoadQuestObjective(row);
        quest->SetSpecialFlag(row[2].GetUInt8() == QUEST_OBJECTIVE_MONSTER ? QUEST_SPECIAL_FLAGS_KILL : QUEST_SPECIAL_FLAGS_DELIVER);
        if (row[2].GetUInt8() == QUEST_OBJECTIVE_ITEM)
            quest->SourceItemIdCount = 1;
    } while (objectives->NextRow());
}

struct SceneActor : Creature
{
    ~SceneActor()
    {
        CombatStop(true);
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<Creature*>(this), false);
        ResetMap();
    }

    void Spawn(Map& map, uint64 guid, Position const& pos)
    {
        Require(Create(guid, &map, 1, 54072, 0, ALLIANCE, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation()),
            "Native Sanath template could not be created");
        m_DBTableGuid = guid;
        SetHomePosition(pos);
        Require(map.AddToMap(static_cast<Creature*>(this)), "Sanath could not enter the world grid");
    }
};

struct SceneCrate : GameObject
{
    ~SceneCrate()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<GameObject*>(this), false);
        ResetMap();
    }

    void Spawn(Map& map, uint64 guid, uint32 entry, Position const& pos)
    {
        Require(Create(guid, entry, &map, 1, pos, G3D::Quat(0, 0, 0, 1), 255, GO_STATE_READY) && map.AddToMap(static_cast<GameObject*>(this)),
            "Native crate could not enter the world grid");
    }
};

struct ScenePet : LootPet
{
    ScenePet(LootPlayer& owner, Map& map) : LootPet(owner, map)
    {
        // The socketless pet did not enter the normal owner-control path.
        m_ControlledByPlayer = true;
    }
};

void Accept(LootPlayer& player, uint32 quest)
{
    player.SetQuestStatus(quest, QUEST_STATUS_INCOMPLETE);
    player.SetQuestSlot(0, quest);
}

void CheckRoute(WorldSession* first, WorldSession* second, Route const& route, uint8 race, bool usePet)
{
    auto rows = WorldDatabase.PQuery("SELECT position_x,position_y,position_z,orientation FROM creature WHERE guid=" UI64FMTD " AND id=54072", route.SanathSpawn);
    Require(bool(rows), "No authored Sanath placement for the faction");
    auto position = rows->Fetch();
    Position spot(position[0].GetFloat(), position[1].GetFloat(), position[2].GetFloat(), position[3].GetFloat());
    Map map(0, 300, 9999995 + uint32(usePet), DIFFICULTY_NONE);
    LootPlayer owner(first, map, 100, spot);
    LootPlayer outsider(second, map, 101, spot);
    for (LootPlayer* player : {&owner, &outsider})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, race);
        player->setFactionForRace(race);
    }
    auto& active = const_cast<GameEventMgr::ActiveEvents&>(sGameEventMgr->GetActiveEventList());
    active.insert(12);
    SceneActor sanath;
    sanath.Spawn(map, route.SanathSpawn, spot);
    SceneCrate stolen;
    SceneCrate reward;
    stolen.Spawn(map, route.SanathSpawn, route.StolenCrate, spot);
    reward.Spawn(map, route.SanathSpawn + 5, route.RewardCrate, spot);
    Require(stolen.IsNeverVisible(&outsider) && reward.IsNeverVisible(&outsider), "Uninvolved character could see the crates");
    Require(!outsider.GetGameObjectIfCanInteractWith(stolen.GetGUID(), GAMEOBJECT_TYPE_QUESTGIVER), "Hidden crate accepted a direct interaction");
    Accept(owner, route.Encounter);
    Quest const* quest = sQuestDataStore->GetQuestTemplate(route.Encounter);
    Require(!stolen.IsNeverVisible(&owner) && !owner.CanRewardQuest(quest, false), "Approach awarded the objective before the kill");
    sanath.AI()->MoveInLineOfSight_Safe(&outsider);
    Require(outsider.GetSummonList(54114)->empty(), "Bystander created a private encounter");
    std::unique_ptr<ProbeParty> party;
    if (!usePet)
    {
        party = std::make_unique<ProbeParty>(owner, outsider);
        Accept(outsider, route.Encounter);
    }
    sanath.AI()->MoveInLineOfSight_Safe(&owner);
    auto roster = owner.GetSummonList(54114);
    Require(roster->size() == 1, "Native 100668 did not create an Unleashed Void");
    auto* boss = map.GetCreature(roster->front());
    Require(boss && boss->HasQuestObjectScope() && boss->IsHostileTo(&owner) && boss->GetHealth() > 0 &&
        boss->HasScalableLevels() && boss->GetLevelForTarget(&owner) == 110 && outsider.canSeeOrDetect(boss) == bool(party),
        "Encounter is missing scaling, hostility or selective visibility");
    owner.SetLevel(10);
    Require(boss->GetLevelForTarget(&owner) == 10 && boss->GetHealthMultiplierForTarget(&owner) > 0, "Low-level combat scaling failed");
    owner.SetLevel(110);
    owner.CastSpell(&owner, 100668, true);
    Require(roster->size() == 1, "Repeated native summon duplicated the encounter");

    if (usePet)
    {
        ScenePet pet(owner, map);
        pet.DealDamage(boss, std::numeric_limits<uint32>::max());
    }
    else
        owner.DealDamage(boss, std::numeric_limits<uint32>::max());
    Tick(owner, 150);
    Tick(outsider, 150);
    std::cout << "ENCOUNTER " << route.Encounter << " alive=" << boss->IsAlive() << " damageTagged=" << boss->IsDamageEnoughForLootingAndReward()
        << " credit=" << owner.GetQuestObjectiveData(route.Encounter, 54114) << " status=" << owner.GetQuestStatus(route.Encounter) << '\n';
    Require(owner.GetQuestObjectiveData(route.Encounter, 54114) == 1 && owner.CanRewardQuest(quest, false), "Native death did not grant one completion");
    Require(outsider.GetQuestStatus(route.Encounter) == (party ? QUEST_STATUS_COMPLETE : QUEST_STATUS_NONE),
        "Death credit eligibility differed for group member or bystander");
    owner.RemoveActiveQuest(route.Encounter);
    owner.SetQuestSlot(0, 0);
    auto& rewarded = const_cast<RewardedQuestSet&>(owner.getRewardedQuests());
    rewarded.insert(route.Encounter);
    Require(!stolen.IsNeverVisible(&owner) && reward.IsNeverVisible(&owner), "Stolen crate lost the next offer or unlocked the reward early");

    Quest const* delivery = sQuestDataStore->GetQuestTemplate(route.Return);
    Accept(owner, route.Return);
    Require(owner.GiveQuestSourceItem(delivery) && owner.GetItemCount(71057) == 1 && stolen.IsNeverVisible(&owner), "Native pickup did not create one recoverable crate or hide the source");
    Require(owner.TakeQuestSourceItem(route.Return, false) && owner.GetItemCount(71057) == 0, "Abandon did not clean up the quest crate");
    owner.RemoveActiveQuest(route.Return);
    owner.SetQuestSlot(0, 0);
    Require(!stolen.IsNeverVisible(&owner), "Abandoned return could not recover from the stolen crate again");
    rewarded.insert(route.Return);
    Require(!reward.IsNeverVisible(&owner) && stolen.IsNeverVisible(&owner), "Turn-in did not unlock only the final crate");
    rewarded.insert(route.Reward);
    Require(reward.IsNeverVisible(&owner), "Already claimed reward crate was still available");
    active.erase(12);
    map.RemoveAllObjectsInRemoveList();
    std::cout << "PASS native Collector quest " << route.Encounter << " " << (usePet ? "pet" : "player") << " kill, scaling, scoped combat and crate state transitions\n";
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
        LoadCollectorQuests();
        sObjectMgr->LoadItemTemplates();
        sObjectMgr->LoadItemTemplateAddon();
        sObjectMgr->LoadPageTexts();
        sObjectMgr->LoadGameObjectTemplate();
        sCreatureTextMgr->LoadCreatureTexts();
        sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
        AddSC_npcs_special();
        sScriptMgr->SwapScriptContext(true);
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
        LoginDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        // Account-zero socketless sessions are never persisted or sent to the login server.
        auto first = new WorldSession(0, std::string("Native Collector owner"), {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {});
        auto second = new WorldSession(0, std::string("Native Collector outsider"), {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {});
        CheckRoute(first, second, HallowsEndCrate::Alliance, RACE_HUMAN, false);
        CheckRoute(first, second, HallowsEndCrate::Horde, RACE_ORC, true);
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
