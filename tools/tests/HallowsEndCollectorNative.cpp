#define MASTER_LOOT_NATIVE_SUPPORT_ONLY
#include "MasterLootNative.cpp"
#include "GameEventMgr.h"
#include "CreatureTextMgr.h"
#include "GameObject.h"
#include "HallowsEndCrate.h"

void AddSC_npcs_special();

struct CollectorAddonTag
{
    using Type = CreatureAddonContainer ObjectMgr::*;
    friend Type Access(CollectorAddonTag);
};
template struct FixtureAccess<CollectorAddonTag, &ObjectMgr::_creatureAddonStore>;

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
        if (IsInWorld())
        {
            CombatStop(true);
            GetMap()->RemoveFromMap(static_cast<Creature*>(this), false);
        }
        if (FindMap())
            ResetMap();
    }

    void Spawn(Map& map, uint64 guid, uint32 entry, Position const& pos)
    {
        auto runtimeGuid = sObjectMgr->GetGenerator<HighGuid::Creature>()->Generate();
        Require(Create(runtimeGuid, &map, 1, entry, 0, ALLIANCE, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation()),
            "Native Collector scene actor could not be created");
        m_DBTableGuid = guid;
        SetHomePosition(pos);
        Require(map.AddToMap(static_cast<Creature*>(this)) && map.GetCreature(GetGUID()) == this, "Scene actor could not enter the world grid");
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

    void Spawn(Map& map, uint32 entry, Position const& pos)
    {
        auto guid = sObjectMgr->GetGenerator<HighGuid::GameObject>()->Generate();
        Require(Create(guid, entry, &map, 1, pos, G3D::Quat(0, 0, 0, 1), 255, GO_STATE_READY) &&
            map.AddToMap(static_cast<GameObject*>(this)) && map.GetGameObject(GetGUID()) == this,
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

Position ReadPlacement(uint64 guid, uint32 entry)
{
    auto rows = WorldDatabase.PQuery("SELECT position_x,position_y,position_z,orientation FROM creature WHERE guid=" UI64FMTD " AND id=%u", guid, entry);
    Require(bool(rows), "No authored Collector scene placement for the faction");
    auto position = rows->Fetch();
    return {position[0].GetFloat(), position[1].GetFloat(), position[2].GetFloat(), position[3].GetFloat()};
}

void CheckRoute(WorldSession* first, WorldSession* second, Route const& route, uint8 race, bool usePet)
{
    Position spot = ReadPlacement(route.SanathSpawn, 54072);
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
    sanath.Spawn(map, route.SanathSpawn, 54072, spot);
    SceneCrate stolen;
    SceneCrate reward;
    stolen.Spawn(map, route.StolenCrate, spot);
    reward.Spawn(map, route.RewardCrate, spot);
    Require(stolen.IsNeverVisible(&outsider) && reward.IsNeverVisible(&outsider), "Uninvolved character could see the crates");
    Require(!outsider.GetGameObjectIfCanInteractWith(stolen.GetGUID(), GAMEOBJECT_TYPE_QUESTGIVER), "Hidden crate accepted a direct interaction");
    Accept(owner, route.Encounter);
    Quest const* quest = sQuestDataStore->GetQuestTemplate(route.Encounter);
    Require(!stolen.IsNeverVisible(&owner) && !owner.CanRewardQuest(quest, false), "Approach awarded the objective before the kill");
    auto* intros = owner.GetSummonList(54072);
    auto* roster = owner.GetSummonList(54114);
    sanath.AI()->MoveInLineOfSight_Safe(&owner);
    Require(intros->empty() && roster->empty(), "Missing Ren started an encounter");
    SceneActor ren;
    auto addonRows = WorldDatabase.PQuery("SELECT bytes1 FROM creature_addon WHERE guid=" UI64FMTD, route.RenSpawn);
    Require(bool(addonRows), "Ren is missing the corpse addon");
    auto& renAddon = (sObjectMgr->*Access(CollectorAddonTag{}))[route.RenSpawn];
    renAddon.bytes1 = (*addonRows)[0].GetUInt32();
    ren.Spawn(map, route.RenSpawn, 54205, ReadPlacement(route.RenSpawn, 54205));
    Require(ren.LoadCreaturesAddon() && ren.getStandState() == UNIT_STAND_STATE_DEAD && ren.HasAura(100922), "Native Ren corpse addon did not load");
    Require(owner.canSeeOrDetect(&ren) && !outsider.canSeeOrDetect(&ren) && !owner.canSeeOrDetect(&sanath), "Corpse or controller visibility is wrong");
    sanath.AI()->MoveInLineOfSight_Safe(&outsider);
    Require(outsider.GetSummonList(54072)->empty() && outsider.GetSummonList(54114)->empty(), "Bystander created a private encounter");
    sanath.AI()->MoveInLineOfSight_Safe(&owner);
    Require(intros->size() == 1 && roster->empty(), "Approach did not begin with a private Sanath");
    auto* intro = map.GetCreature(intros->front());
    Require(intro && owner.canSeeOrDetect(intro) && !outsider.canSeeOrDetect(intro), "Sanath scene crossed personal visibility");
    Require(owner.CastSpell(intro, 100668, true) != SPELL_CAST_OK && roster->empty(), "Direct summon bypassed the dialogue and explosion");
    intro->AI()->UpdateAI(8999);
    Require(roster->empty(), "Unleashed Void appeared early");
    owner.RemoveActiveQuest(route.Encounter);
    owner.SetQuestSlot(0, 0);
    Accept(owner, route.Encounter);
    intro->AI()->UpdateAI(10000);
    map.RemoveAllObjectsInRemoveList();
    Require(intros->empty() && roster->empty(), "Abandoned scene reached the new attempt");

    sanath.AI()->MoveInLineOfSight_Safe(&owner);
    Require(intros->size() == 1, "Approach did not restart Sanath after abandonment");
    active.erase(12);
    map.GetCreature(intros->front())->AI()->UpdateAI(9000);
    map.RemoveAllObjectsInRemoveList();
    Require(intros->empty() && roster->empty() && !owner.canSeeOrDetect(&ren), "Deactivating the holiday left an accessible scene");
    active.insert(12);

    std::unique_ptr<ProbeParty> party;
    if (!usePet)
        party = std::make_unique<ProbeParty>(owner, outsider);
    Accept(outsider, route.Encounter);
    sanath.AI()->MoveInLineOfSight_Safe(&outsider);
    auto* otherIntros = outsider.GetSummonList(54072);
    Require(otherIntros->size() == 1, "A second eligible player could not start their own presentation");
    sanath.AI()->MoveInLineOfSight_Safe(&owner);
    Require(intros->size() == 1, "Approach did not restart Sanath when the holiday returned");
    intro = map.GetCreature(intros->front());
    intro->AI()->UpdateAI(9000);
    intro->AI()->UpdateAI(5500);
    intro->AI()->UpdateAI(4499);
    Require(roster->empty() && intro->getStandState() == UNIT_STAND_STATE_KNEEL, "Sanath skipped kneeling by the crate before summoning the Void");
    intro->AI()->UpdateAI(1);
    Require(roster->size() == 1 && outsider.GetSummonList(54114)->empty(), "Native 100669 to 100668 did not summon exclusively for the player");
    auto* boss = map.GetCreature(roster->front());
    Require(boss && boss->HasQuestObjectScope() && boss->IsHostileTo(&owner) && boss->GetHealth() > 0 &&
        boss->HasScalableLevels() && boss->GetLevelForTarget(&owner) == 110 && outsider.canSeeOrDetect(boss) == bool(party),
        "Encounter is missing scaling, hostility or selective visibility");
    owner.SetLevel(10);
    Require(boss->GetLevelForTarget(&owner) == 10 && boss->GetHealthMultiplierForTarget(&owner) > 0, "Low-level combat scaling failed");
    owner.SetLevel(110);
    owner.CastSpell(&owner, 100668, true);
    Require(roster->size() == 1, "Repeated native summon duplicated the encounter");
    auto* otherIntro = map.GetCreature(otherIntros->front());
    otherIntro->AI()->UpdateAI(9000);
    map.RemoveAllObjectsInRemoveList();
    if (party)
        Require(otherIntros->empty() && outsider.GetSummonList(54114)->empty(), "The party received a second independent encounter");
    else
        Require(otherIntros->size() == 1 && outsider.GetSummonList(54114)->empty(), "The explosion consumed a second solo player's separate scene");

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
    Require(outsider.GetQuestStatus(route.Encounter) == (party ? QUEST_STATUS_COMPLETE : QUEST_STATUS_INCOMPLETE),
        "Death credit eligibility differed for group member or bystander");
    intro->AI()->UpdateAI(3500);
    Require(intros->size() == 1, "One-hit victory removed Sanath before the final cue");
    intro->AI()->UpdateAI(3500);
    map.RemoveAllObjectsInRemoveList();
    Require(intros->empty() && owner.canSeeOrDetect(&ren), "The finished introduction did not remove just Sanath");
    owner.RemoveActiveQuest(route.Encounter);
    owner.SetQuestSlot(0, 0);
    auto& rewarded = const_cast<RewardedQuestSet&>(owner.getRewardedQuests());
    rewarded.insert(route.Encounter);
    Require(!stolen.IsNeverVisible(&owner) && reward.IsNeverVisible(&owner), "Stolen crate lost the next offer or unlocked the reward early");

    Quest const* delivery = sQuestDataStore->GetQuestTemplate(route.Return);
    Accept(owner, route.Return);
    Require(owner.GiveQuestSourceItem(delivery) && owner.GetItemCount(71057) == 1 && stolen.IsNeverVisible(&owner), "Native pickup did not create one recoverable crate or hide the source");
    Require(!owner.canSeeOrDetect(&ren), "Ren stayed visible after claiming the crate");
    Require(owner.TakeQuestSourceItem(route.Return, false) && owner.GetItemCount(71057) == 0, "Abandon did not clean up the quest crate");
    owner.RemoveActiveQuest(route.Return);
    owner.SetQuestSlot(0, 0);
    Require(!stolen.IsNeverVisible(&owner), "Abandoned return could not recover from the stolen crate again");
    Require(owner.canSeeOrDetect(&ren), "Ren did not return alongside the recovered crate offer");
    rewarded.insert(route.Return);
    Require(!reward.IsNeverVisible(&owner) && stolen.IsNeverVisible(&owner), "Turn-in did not unlock only the final crate");
    rewarded.insert(route.Reward);
    Require(reward.IsNeverVisible(&owner), "Already claimed reward crate was still available");
    active.erase(12);
    if (!otherIntros->empty())
        map.GetCreature(otherIntros->front())->AI()->UpdateAI(1);
    map.RemoveAllObjectsInRemoveList();
    std::cout << "PASS native Collector quest " << route.Encounter << " private intro, 100669/100668, " << (usePet ? "pet" : "player")
        << " kill, scaling, scoped combat and crate state transitions\n";
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
