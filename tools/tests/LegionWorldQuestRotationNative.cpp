#define AREA_DIALOGUE_SUPPORT_ONLY
#include "LegionAreaDialogueNative.cpp"
#include "GameObjectPackets.h"
#include "ItemEnchantmentMgr.h"
#include "LootMgr.h"
#include "WorldStateMgr.h"
#include <chrono>
#include <cmath>
#include <thread>

struct RotationUpdateTag
{
    using Type = WorldQuestUpdate const* (QuestDataStoreMgr::*)(uint32, uint32, uint32);
    friend Type Access(RotationUpdateTag);
};
template struct FixtureAccess<RotationUpdateTag, &QuestDataStoreMgr::GetWorldQuestUpdate>;

struct RotationGenerateTag
{
    using Type = WorldQuest const* (QuestDataStoreMgr::*)(WorldQuestUpdate const*, WorldQuestTemplate const*);
    friend Type Access(RotationGenerateTag);
};
template struct FixtureAccess<RotationGenerateTag, &QuestDataStoreMgr::GenerateWorldQuest>;

struct RotationAdmissionTag
{
    using Type = bool (QuestDataStoreMgr::*)(WorldQuestTemplate const*, WorldQuestUpdate const*);
    friend Type Access(RotationAdmissionTag);
};
template struct FixtureAccess<RotationAdmissionTag, &QuestDataStoreMgr::CanBeActivate>;

struct RotationRemoveTaskTag
{
    using Type = void (QuestDataStoreMgr::*)(Quest const*);
    friend Type Access(RotationRemoveTaskTag);
};
template struct FixtureAccess<RotationRemoveTaskTag, &QuestDataStoreMgr::RemoveWorldQuestTask>;

struct RotationCallbacksTag
{
    using Type = FunctionProcessor Unit::*;
    friend Type Access(RotationCallbacksTag);
};
template struct FixtureAccess<RotationCallbacksTag, &Unit::m_Functions>;

struct RotationAchievementsTag
{
    using Type = std::vector<CompletedAchievementData*> AchievementMgr<Player>::*;
    friend Type Access(RotationAchievementsTag);
};
template struct FixtureAccess<RotationAchievementsTag, &AchievementMgr<Player>::_completedAchievementsArr>;

class RotationPlayer : public DungeonPlayer
{
public:
    RotationPlayer(Map& map, uint32 low, Position const& location) : DungeonPlayer(map, RACE_HUMAN, low)
    {
        PlaceInMap(location);
        Require(Sessions().emplace(low, WorldSessionPtr(GetSession(), [](WorldSession*) {})).second, "Test session key collision");
    }

    void PlaceInMap(Position const& location)
    {
        RemoveFromGrid();
        Relocate(location);
        GetZoneAndAreaId(m_zoneId, m_areaId);
        Map& map = *GetMap();
        map.LoadGrid(GetPositionX(), GetPositionY());
        map.AddToGrid(static_cast<Player*>(this), Cell(Trinity::ComputeCellCoord(GetPositionX(), GetPositionY())));
    }

    ~RotationPlayer() { Sessions().erase(GetGUID().GetCounter()); }

    void UnlockBrokenShore()
    {
        for (uint32 id : {41694u, 46734u})
        {
            uint32 bit = sDB2Manager.GetQuestUniqueBitFlag(id);
            Require(bit, "Native Broken Shore prerequisite bit is missing");
            SetRewardedQuest(id);
            SetQuestCompletedBit(bit, true);
        }
    }

    void UnlockAssaults()
    {
        static CompletedAchievementData unlocked;
        auto manager = GetAchievementMgr().operator->();
        (manager.operator->()->*Access(RotationAchievementsTag{})).at(11473) = &unlocked;
    }

    void PumpCallbacks(uint32 elapsed = 1) { (this->*Access(RotationCallbacksTag{})).Update(elapsed); }

private:
    static SessionMap& Sessions() { return const_cast<SessionMap&>(sWorld->GetAllSessions()); }
};

struct RotationCase
{
    uint32 QuestId;
    uint32 EventId;
    uint32 Conversation;
    uint32 Area;
    uint32 Timer;
    uint32 Variable;
    uint32 Value;
    uint32 SecondaryVariable;
};

#ifdef LEGION_ROTATION_PROGRESSION
struct RotationControlledTag
{
    using Type = bool Unit::*;
    friend Type Access(RotationControlledTag);
};
template struct FixtureAccess<RotationControlledTag, &Unit::m_ControlledByPlayer>;

static QueryResult QueryRewardFixture(char const* sql)
{
    QueryResult result;
    auto query = CharacterDatabase.AsyncQuery(sql).WithCallback([&](QueryResult rows) { result = std::move(rows); });
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!query.InvokeIfReady())
    {
        Require(std::chrono::steady_clock::now() < deadline, "Timed out waiting for the isolated character worker");
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return result;
}

static void InitializeRewardFixture()
{
    auto expectedSchema = std::getenv("LEGION_REWARD_SCHEMA");
    auto schema = QueryRewardFixture("SELECT DATABASE()");
    Require(expectedSchema && std::string_view(expectedSchema).rfind("codex_wq_reward_", 0) == 0 && schema &&
        schema->Fetch()[0].GetString() == expectedSchema, "Native reward test requires its disposable character schema");

    // The single asynchronous worker executes these statements in order.
    // Check persistence in its connection-local tables inside the isolated schema.
    CharacterDatabase.Execute("SET SESSION TRANSACTION READ WRITE");
    CharacterDatabase.Execute("CREATE TEMPORARY TABLE characters (guid BIGINT UNSIGNED PRIMARY KEY, money BIGINT UNSIGNED NOT NULL, native_fixture TINYINT NOT NULL)");
    CharacterDatabase.Execute("INSERT INTO characters (guid,money,native_fixture) VALUES (101,0,1)");
    for (char const* table : {"character_queststatus", "character_queststatus_objectives", "character_queststatus_rewarded",
         "character_queststatus_world", "character_currency", "creature_respawn"})
    {
        std::string name = table;
        CharacterDatabase.Execute(("CREATE TEMPORARY TABLE reward_schema LIKE " + name).c_str());
        CharacterDatabase.Execute(("CREATE TEMPORARY TABLE " + name + " LIKE reward_schema").c_str());
        CharacterDatabase.Execute("DROP TEMPORARY TABLE reward_schema");
    }
    CharacterDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto ready = QueryRewardFixture("SELECT @@tx_read_only,native_fixture FROM characters WHERE guid=101");
    Require(ready && ready->Fetch()[0].GetUInt32() == 1 && ready->Fetch()[1].GetUInt32() == 1,
        "Character worker did not establish its read-only reward shadows");
}

class RotationVictim : public Creature
{
public:
    RotationVictim(RotationPlayer& player, uint32 entry, uint32 guid) : Creature(false)
    {
        Map* map = player.GetMap();
        Require(Create(guid, map, 1, entry, 0, ALLIANCE, player.GetPositionX() + 2, player.GetPositionY(), player.GetPositionZ(), 0),
            "Could not create the real objective creature entry");
        SetPhaseId(player.GetPhases(), false);
        Require(map->AddToMap(static_cast<Creature*>(this)), "Could not publish the objective creature");
        Require(GetHealth() && GetHealth() < std::numeric_limits<uint32>::max(), "Objective creature needs native one-hit damage range");
    }

    ~RotationVictim()
    {
        CombatStop(true);
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<Creature*>(this), false);
        ResetMap();
    }
};

static void CheckPartyKill(Map& map, Position const& origin, uint32 area)
{
    constexpr uint32 questId = 46073;
    constexpr uint32 victimEntry = 117559;
    RotationPlayer owner(map, 106, origin);
    RotationPlayer nearby(map, 107, origin);
    RotationPlayer distant(map, 108, origin);
    RotationPlayer unqualified(map, 109, origin);
    for (RotationPlayer* player : {&owner, &nearby, &distant})
    {
        player->UnlockBrokenShore();
        player->UpdateAreaQuestTasks(area, 0);
        Require(player->FindQuestSlot(questId) < MAX_QUEST_LOG_SIZE, "Party fixture did not accept the real world task");
    }

    distant.Relocate(origin.GetPositionX() + sWorld->getFloatConfig(CONFIG_GROUP_XP_DISTANCE) + 30,
        origin.GetPositionY(), origin.GetPositionZ(), 0);
    Group party;
    struct PartyMembers
    {
        std::array<RotationPlayer*, 4> Players;
        ~PartyMembers() { for (RotationPlayer* player : Players) player->SetGroup(nullptr); }
    } members{{&owner, &nearby, &distant, &unqualified}};
    for (RotationPlayer* player : members.Players)
        player->SetGroup(&party, 0);

    DungeonPet pet(owner, map);
    pet.*Access(RotationControlledTag{}) = true;
    RotationVictim victim(owner, victimEntry, 190090);
    Require(nearby.IsAtGroupRewardDistance(&victim) && !distant.IsAtGroupRewardDistance(&victim),
        "Party distance counterfactual is invalid");
    uint64 health = victim.GetHealth(&pet);
    float multiplier = victim.GetHealthMultiplierForTarget(&pet);
    double lethalDamage = std::ceil(double(health) * multiplier * 1.001);
    Require(lethalDamage > 0 && lethalDamage <= std::numeric_limits<uint32>::max(), "Invalid scaled pet damage");
    pet.DealDamage(&victim, uint32(lethalDamage), nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false);
    Require(victim.isDead(), "Party pet did not kill the native quest target");
    pet.Kill(&victim);
    for (RotationPlayer* player : members.Players)
        player->PumpCallbacks(50);

    Require(owner.GetQuestObjectiveData(questId, victimEntry) == 1 && nearby.GetQuestObjectiveData(questId, victimEntry) == 1 &&
        !distant.GetQuestObjectiveData(questId, victimEntry) && !unqualified.GetQuestObjectiveData(questId, victimEntry),
        "Pet kill failed exactly-once owner/nearby eligible party credit or leaked credit");
    std::cout << "PROGRESSION 46073 pet party credit owner=1 nearby=1 distant=0 without_task=0\n";
}

#ifdef LEGION_ROTATION_GARRISON_PLACEMENTS
namespace GameTime
{
    extern time_t GameTime;
}

class RotationClock
{
public:
    ~RotationClock() { GameTime::GameTime = start; }
    void At(time_t deadline) const { GameTime::GameTime = deadline; }
    time_t Start() const { return start; }

private:
    time_t const start = GameTime::GetGameTime();
};

static void CheckCreatureRespawn(Map& map, uint64 guid, uint32 entry, uint32 delay)
{
    auto row = WorldDatabase.PQuery("SELECT position_x,position_y,position_z,orientation,PhaseId,spawntimesecs "
        "FROM creature WHERE guid=%llu AND id=%u AND map=1220 AND zoneId=7543 AND areaId=8551 "
        "AND spawnMask=1 AND phaseMask=1", static_cast<unsigned long long>(guid), entry);
    Require(row && row->Fetch()[4].GetString() == "7147" && row->Fetch()[5].GetUInt32() == delay,
        "Placed creature respawn data differs");
    auto fields = row->Fetch();
    // The test's accessor is intentionally bounded. Keep the DB row's spawn data and use a test alias.
    auto const runtimeGuid = 180200 + (guid - 146980460);
    CreatureData& data = sObjectMgr->NewOrExistCreatureData(runtimeGuid);
    data.guid = runtimeGuid;
    data.id = entry;
    data.mapid = 1220;
    data.zoneId = 7543;
    data.areaId = 8551;
    data.posX = fields[0].GetFloat();
    data.posY = fields[1].GetFloat();
    data.posZ = fields[2].GetFloat();
    data.orientation = fields[3].GetFloat();
    data.PhaseID = {7147};
    data.spawntimesecs = fields[5].GetUInt32();
    map.LoadGrid(data.posX, data.posY);
    Creature creature(false);
    Require(creature.LoadCreatureFromDB(runtimeGuid, &map) && creature.HasPhaseId(7147) && creature.IsAlive()
        && creature.GetRespawnDelay() == delay, "Native DB spawn did not load its phase and respawn settings");
    struct UnpublishCreature
    {
        Creature& Spawn;
        ~UnpublishCreature()
        {
            Spawn.CombatStop(true);
            if (Spawn.IsInWorld())
                Spawn.GetMap()->RemoveFromMap(&Spawn, false);
            Spawn.ResetMap();
        }
    } cleanup{creature};

    RotationClock clock;
    creature.setDeathState(JUST_DIED);
    auto const corpseEnd = clock.Start() + creature.GetCorpseDelay();
    auto const deadline = creature.GetRespawnTime();
    Require(creature.getDeathState() == CORPSE && deadline == corpseEnd + delay, "Death used the wrong native respawn deadline");
    std::string const timerRow = " FROM creature_respawn WHERE guid=" + std::to_string(runtimeGuid) + " AND mapId=1220 AND instanceId=0";
    CharacterDatabase.Execute(("INSERT INTO creature_respawn (guid,respawnTime,mapId,instanceId) VALUES (" +
        std::to_string(runtimeGuid) + "," + std::to_string(deadline) + ",1220,0)").c_str());
    clock.At(corpseEnd);
    creature.Update(5000);
    Require(creature.getDeathState() == DEAD && creature.GetRespawnTime() == deadline,
        "Corpse removal did not preserve the scheduled respawn");
    clock.At(deadline - 1);
    creature.Update(5000);
    auto saved = QueryRewardFixture(("SELECT respawnTime" + timerRow).c_str());
    Require(creature.getDeathState() == DEAD && saved && saved->Fetch()[0].GetUInt32() == deadline,
        "Creature appeared or cleared its saved respawn before its deadline");
    clock.At(deadline);
    creature.Update(5000);
    Require(creature.IsAlive() && creature.GetHealth() == creature.GetMaxHealth() && creature.HasPhaseId(7147)
        && creature.GetRespawnTime() == 0, "Placed creature did not respawn in its native phase");
    Require(!QueryRewardFixture(("SELECT guid" + timerRow).c_str()), "Creature retained a stale saved respawn after returning");
    std::cout << "GARRISON creature " << entry << " returned after corpse removal and " << delay << " seconds\n";
}

class RotationLibram : public GameObject
{
public:
    RotationLibram(Map& map, uint64 guid)
    {
        auto row = WorldDatabase.PQuery("SELECT position_x,position_y,position_z,orientation,rotation2,rotation3,PhaseId,spawntimesecs "
            "FROM gameobject WHERE guid=%llu AND id=269154 AND map=1220 AND zoneId=7543 AND areaId=8551 "
            "AND spawnMask=1 AND phaseMask=1 AND state=1", static_cast<unsigned long long>(guid));
        Require(row && row->Fetch()[6].GetString() == "7147" && row->Fetch()[7].GetUInt32() == 120,
            "A real Libram placement is missing its expected phase or lifecycle");
        auto fields = row->Fetch();
        Position pose{fields[0].GetFloat(), fields[1].GetFloat(), fields[2].GetFloat(), fields[3].GetFloat()};
        G3D::Quat rotation(0, 0, fields[4].GetFloat(), fields[5].GetFloat());
        // The native test reserves a small accessor vector. Use an in-range runtime GUID for the loaded pose.
        auto runtimeGuid = 180100 + (guid - 25760460);
        Require(Create(runtimeGuid, 269154, &map, 1, pose, rotation, 255, GO_STATE_READY), "Native Libram create failed");
        SetRespawnDelayTime(fields[7].GetUInt32());
        SetPhaseId({7147}, false);
        SetLootState(GO_READY);
        map.LoadGrid(GetPositionX(), GetPositionY());
        Require(map.AddToMap(static_cast<GameObject*>(this)) && isSpawned(), "Native Libram publication failed");
    }

    ~RotationLibram()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<GameObject*>(this), false);
        ResetMap();
    }

    void UseThroughSession(Player& player)
    {
        Require(player.CanContact() && player.GetUnitBeingMoved() == &player, "Libram packet is not testing a live mover");
        WorldPackets::GameObject::GameObjectUse packet{WorldPacket(CMSG_GAME_OBJ_USE, 0)};
        packet.Guid = GetGUID();
        player.GetSession()->HandleGameObjectUse(packet);
    }
};

static void CheckLibramRespawn(RotationLibram& book, RotationPlayer& eligible)
{
    RotationClock clock;
    clock.At(clock.Start() + std::max(60u, book.GetGOInfo()->GetAutoCloseTime()) + 1);
    book.Update(5000);
    Require(book.getLootState() == GO_JUST_DEACTIVATED, "The used Libram did not leave its active state");
    book.Update(5000);
    auto const deadline = book.GetRespawnTime();
    Require(deadline == GameTime::GetGameTime() + 120 && !book.isSpawned(), "Native Libram did not schedule its hidden interval");
    book.UseThroughSession(eligible);
    Require(eligible.GetQuestObjectiveData(46073, 269154) == 1 && !book.HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE),
        "Using the hidden Libram changed progress or consumed it again");
    clock.At(deadline - 1);
    book.Update(5000);
    Require(!book.isSpawned(), "Libram appeared before its deadline");
    clock.At(deadline);
    book.Update(5000);
    Require(book.isSpawned() && book.getLootState() == GO_READY && book.GetGoState() == GO_STATE_READY && book.HasPhaseId(7147)
        && !book.HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE), "Libram did not respawn ready for interaction");
    std::cout << "GARRISON Libram cooldown rejected interaction while hidden and restored the ready phase after 120 seconds\n";
}

static void CheckFelskornVisibility(Map& map, uint32 otherQuest)
{
    RotationLibram book(map, 25760463);
    RotationPlayer visitor(map, 104, {book.GetPositionX() + 1, book.GetPositionY(), book.GetPositionZ(), 0});
    // The native task separates Warriors (45743) from the other classes (45744).
    visitor.SetByteValue(UNIT_FIELD_BYTES_0, 1, otherQuest == 45743 ? CLASS_WARRIOR : CLASS_HUNTER);
    visitor.UnlockBrokenShore();
    visitor.GetAchievementMgr()->m_canUpdateAchiev = 1; // Online characters finish achievement loading before area entry.
    visitor.SetMovedUnit(&visitor);
    Require(visitor.GetCurrentAreaID() == 8551, "Competing phase fixture is outside the shared objective area");
    visitor.RefreshWorldQuestAreaTasks();
    visitor.GetPhaseMgr().NotifyStoresReloaded();
    Require(visitor.GetQuestStatus(otherQuest) == QUEST_STATUS_INCOMPLETE && visitor.HasPhaseId(7148)
        && !visitor.HasPhaseId(7147) && !book.InSamePhase(&visitor),
        "The other live rotation did not remove Garrison visibility through native phase conditions");
    book.UseThroughSession(visitor);
    Require(!book.HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE), "Competing rotation consumed a Garrison Libram");
    std::cout << "GARRISON competitor " << otherQuest << " switched the objective area from phase 7147 to 7148\n";
}

static void CheckLibramAdmission(RotationPlayer& eligible, RotationPlayer& locked, WorldQuest& active)
{
    auto cohort = WorldDatabase.Query("SELECT SUM(id=117555 AND spawntimesecs=45), SUM(id=118720 AND spawntimesecs=240) "
        "FROM creature WHERE guid BETWEEN 146980460 AND 146980472 AND map=1220 AND zoneId=7543 AND areaId=8551 "
        "AND spawnMask=1 AND phaseMask=1 AND PhaseId='7147'");
    Require(cohort && cohort->Fetch()[0].GetUInt32() == 12 && cohort->Fetch()[1].GetUInt32() == 1, "Garrison creature placement cohort differs");
    auto info = sObjectMgr->GetGameObjectTemplate(269154);
    Require(info && info->type == GAMEOBJECT_TYPE_GOOBER && info->displayId == 26998 && info->goober.questID == 46073
        && info->goober.spell == 241545 && info->goober.conditionID1 == 48995, "Libram lost its native data or task guard");

    RotationLibram book(*eligible.GetMap(), 25760460);
    auto noUse = [&] {
        Require(eligible.GetQuestObjectiveData(46073, 269154) == 0 && !book.HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE),
            "Rejected Libram use changed objective or consumed the book");
    };
    eligible.SetMovedUnit(&eligible);
    locked.SetMovedUnit(&locked);
    Require(!book.IsWithinDistInMap(&eligible, book.GetInteractionDistance()), "Far test must start out of range");
    book.UseThroughSession(eligible);
    noUse();

    Position close{book.GetPositionX() + 1, book.GetPositionY(), book.GetPositionZ(), 0};
    eligible.PlaceInMap(close);
    locked.PlaceInMap(close);
    auto const eligiblePhases = eligible.GetPhases();
    locked.SetPhaseId(eligiblePhases, false);
    Require(book.IsWithinDistInMap(&eligible, book.GetInteractionDistance()) && book.IsWithinLOSInMap(&eligible),
        "Selected Libram cannot be used from its real approach");

    book.UseThroughSession(locked);
    noUse();
    eligible.SetPhaseId({8662}, false);
    Require(!book.InSamePhase(&eligible), "Wrong phase fixture would see the Libram");
    book.UseThroughSession(eligible);
    noUse();
    eligible.SetPhaseId(eligiblePhases, false);

    auto resetTime = active.ResetTime;
    active.ResetTime = GameTime::GetGameTime();
    book.UseThroughSession(eligible);
    noUse();
    active.ResetTime = resetTime;

    Require(eligible.CanRewardWorldQuest(sQuestDataStore->GetQuestTemplate(46073))
        && eligible.GetQuestStatus(46073) == QUEST_STATUS_INCOMPLETE
        && book.IsWithinDistInMap(&eligible, book.GetInteractionDistance()) && book.IsWithinLOSInMap(&eligible),
        "Eligible Libram admission prerequisites changed after the negative checks");
    Require(eligible.GetMap()->GetGameObject(book.GetGUID()) == &book && !eligible.IsSpectator(),
        "The session cannot resolve the native Libram in the player's map");
    book.UseThroughSession(eligible);
    if (eligible.GetQuestObjectiveData(46073, 269154) != 1 || !book.HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE))
        std::cout << "GARRISON eligible credit=" << eligible.GetQuestObjectiveData(46073, 269154)
                  << " flags=" << book.GetUInt32Value(GAMEOBJECT_FIELD_FLAGS) << " loot=" << unsigned(book.getLootState())
                  << " ready=" << book.isSpawned() << " alive=" << eligible.IsAlive() << std::endl;
    Require(eligible.GetQuestObjectiveData(46073, 269154) == 1 && book.HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE),
        "An eligible use did not consume and credit the book through native goober dispatch");
    book.UseThroughSession(eligible);
    Require(eligible.GetQuestObjectiveData(46073, 269154) == 1, "Repeated Libram use gave another credit before reset");
    for (uint32 tick = 0; tick < 12; ++tick)
        eligible.m_Events.Update(500);
    Aura const* libramAura = eligible.GetAura(241545);
    Require(libramAura && libramAura->GetMaxDuration() == 120000, "Native Libram use did not deliver its two-minute player aura");
    std::cout << "GARRISON Libram native packet rejected far, wrong phase, inactive player, expired rotation and immediate replay; eligible credit=1 and aura active\n";
    CheckLibramRespawn(book, eligible);
}
#endif

static void CheckProgressAndReward(RotationPlayer& player, RotationPlayer& locked, Quest const* quest, WorldQuest& active)
{
    Require(!quest->RewardMailTemplateId, "Review new reward mail before exercising shadow persistence");
    uint32 const questId = quest->GetQuestId();
    uint32 const fixtureGold = 1234;
    uint64 expectedMoney = 0;
    active.Gold = fixtureGold;
    active.CurrencyID = 0;
    active.Recipe = nullptr;
    for (WorldQuestItem& item : active.ItemList)
        item = {};

    DungeonPet pet(player, *player.GetMap());
    DungeonGuardian guardian(player, *player.GetMap());
    pet.*Access(RotationControlledTag{}) = true;
    guardian.*Access(RotationControlledTag{}) = true;
    Unit* killers[] = {&player, &pet, &guardian};
    uint32 guid = 190000;
    auto kill = [&](uint32 target, uint32 killer, bool finalKill = false)
    {
        RotationVictim victim(player, target, guid++);
        uint32 before = player.GetQuestObjectiveData(questId, target);
        Unit* attacker = killers[killer];
        uint64 health = victim.GetHealth(attacker);
        float multiplier = victim.GetHealthMultiplierForTarget(attacker);
        double lethalDamage = std::ceil(double(health) * multiplier * 1.001);
        Require(lethalDamage > 0 && lethalDamage <= std::numeric_limits<uint32>::max(), "Invalid scaled one-hit damage");
        uint32 dealt = attacker->DealDamage(&victim, uint32(lethalDamage), nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false);
        if (!victim.isDead())
            std::cout << "PROGRESSION " << questId << " target=" << target << " killer=" << killer << " health=" << health
                      << " multiplier=" << multiplier << " damage=" << dealt << " remaining=" << victim.GetHealth() << '\n';
        Require(victim.isDead(), "Native one-hit damage did not kill the quest target");
        attacker->Kill(&victim); // Duplicate death dispatch must award nothing.
        uint64 const moneyBeforeCredit = player.GetMoney();
        player.PumpCallbacks(50);
        Require(locked.GetQuestStatus(questId) == QUEST_STATUS_NONE, "Unaffiliated player gained quest credit");
        if (finalKill)
        {
            expectedMoney = moneyBeforeCredit + fixtureGold;
            std::cout << "PROGRESSION " << questId << " final target=" << target << " kills=" << player.GetKillCreature(target)
                      << " objective=" << player.GetQuestObjectiveData(questId, target) << " status=" << unsigned(player.GetQuestStatus(questId))
                      << " money=" << player.GetMoney() << " moneyBeforeCredit=" << moneyBeforeCredit
                      << " slot=" << player.FindQuestSlot(questId)
                      << " reward=" << player.WorldQuestCompleted(questId) << '\n';
        }
        if (!finalKill)
            Require(player.GetQuestObjectiveData(questId, target) == before + 1 &&
                !player.WorldQuestCompleted(questId) && player.GetMoney() == moneyBeforeCredit,
                "Native kill credit duplicated, failed, or rewarded the task early");
    };

    if (questId == 46021)
        kill(118002, 1, true);
    else if (questId == 46063)
        for (uint32 index = 0; index < 15; ++index)
            kill(118657, index % 3, index == 14);
#ifdef LEGION_ROTATION_GARRISON_PLACEMENTS
    else
    {
        CheckLibramAdmission(player, locked, active);
        CheckCreatureRespawn(*player.GetMap(), 146980460, 117555, 45);
        CheckCreatureRespawn(*player.GetMap(), 146980472, 118720, 240);
        kill(117555, 2);
        kill(118720, 1);
        for (uint32 index = 0; index < 26; ++index)
            kill(117559, index % 3);
        Require(player.GetQuestObjectiveData(46073, 269154) == 1 && player.GetQuestObjectiveData(46073, 117555) == 1
            && player.GetQuestObjectiveData(46073, 118720) == 1, "Mixed Garrison objective credits were lost");

        RotationLibram finish(*player.GetMap(), 25760461);
        player.PlaceInMap({finish.GetPositionX() + 1, finish.GetPositionY(), finish.GetPositionZ(), 0});
        expectedMoney = player.GetMoney() + fixtureGold;
        finish.UseThroughSession(player);
        finish.UseThroughSession(player);
        for (uint32 tick = 0; tick < 12; ++tick)
            player.m_Events.Update(500);
        auto outro = LiveConversations();
        Require(quest->RewardSpell == 235644 && outro.size() == 1 && outro[0]->GetEntry() == 4440 &&
            outro[0]->GetCasterGUID() == player.GetGUID(), "The second Libram did not send one personal Illidan outro");
        ClearConversations(*player.GetMap());
        RotationLibram rewarded(*player.GetMap(), 25760462);
        player.PlaceInMap({rewarded.GetPositionX() + 1, rewarded.GetPositionY(), rewarded.GetPositionZ(), 0});
        rewarded.UseThroughSession(player);
        Require(!rewarded.HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE), "A finished quest consumed another Libram");
        std::cout << "GARRISON guardian imp, pet Flaz, Dreadblades and two real Librams finish through native progression\n";
    }
#else
    else
    {
        for (uint32 index = 0; index < 17; ++index)
            kill(117559, index % 3);
        for (uint32 index = 0; index < 3; ++index)
            kill(117546, index);
        Require(player.GetQuestObjectiveData(questId, 117559) == 17 && player.GetQuestObjectiveData(questId, 117546) == 3,
            "The bounded first batch must remain at 57.5 percent");
        for (uint32 index = 0; index < 17; ++index)
            kill(117559, (index + 1) % 3, index == 16);
    }
#endif

    Require(player.WorldQuestCompleted(questId) && player.FindQuestSlot(questId) == MAX_QUEST_LOG_SIZE &&
        player.GetMoney() == expectedMoney, "Final kill did not award the native world quest exactly once");
    player.RewardQuest(quest, 0, &player, true);
    Require(player.GetMoney() == expectedMoney && !player.CanRewardWorldQuest(quest), "Repeated reward call bypassed the native receipt");
    auto row = QueryRewardFixture(("SELECT c.money,c.native_fixture,s.resetTime FROM characters c JOIN character_queststatus_world s "
        "ON c.guid=s.guid WHERE c.guid=101 AND s.quest=" + std::to_string(questId)).c_str());
    Require(row && row->GetRowCount() == 1 && row->Fetch()[0].GetUInt64() == expectedMoney &&
        row->Fetch()[1].GetUInt32() == 1 && row->Fetch()[2].GetUInt32() == active.StartTime + active.Timer,
        "Native gold and receipt did not commit together to the shadow transaction");
    std::cout << "PROGRESSION " << questId << " kills=" << guid - 190000 << " completion and one persisted reward\n";
}
#endif

int main(int argc, char** argv)
{
    try
    {
        InitializeDungeon(argc, argv, true);
#ifdef LEGION_ROTATION_PROGRESSION
        InitializeRewardFixture();
        LoadRandomEnchantmentsTable();
#endif
        DisableMgr::LoadDisables();
        sObjectMgr->LoadItemTemplates();
        sObjectMgr->LoadItemTemplateAddon();
        sQuestDataStore->LoadQuests();
#ifdef LEGION_ROTATION_PROGRESSION
        LoadLootTables();
#endif
#ifdef LEGION_ROTATION_GARRISON_PLACEMENTS
        sAchievementMgr->LoadCriteriaList();
#endif

        std::unordered_map<uint32, std::vector<uint32>> outdoor{{1220, {}}};
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe(outdoor);
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe(outdoor);

        WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
        ShadowDialogue("world_quest_update", "QuestID IN (45743, 45744, 46011, 46021, 46046, 46063, 46073, 46821)");
        ShadowDialogue("smart_scripts", "source_type=13 AND entryorguid IN (219, 230, 232)");
        ShadowDialogue("conditions", "(SourceTypeOrReferenceId=22 AND SourceId=13 AND SourceEntry IN (219, 230, 232)) OR "
            "(SourceTypeOrReferenceId=23 AND ((SourceGroup=7541 AND SourceEntry=141) OR (SourceGroup=7543 AND SourceEntry IN (126, 142))))");
        auto migration = std::getenv("LEGION_ROTATION_MIGRATION");
        Require(migration, "Rotation SQL path is missing");
        std::ifstream input(migration);
        Require(bool(input), "Rotation SQL is unavailable");
        std::string line, sql;
        while (std::getline(input, line))
            if (line.rfind("--", 0) != 0)
                sql += line + '\n';

        WorldDatabase.DirectExecute("DELETE FROM world_quest_update WHERE QuestID IN (46021, 46063, 46073)");
        WorldDatabase.DirectExecute("INSERT INTO world_quest_update (QuestID, Timer, VariableID, Value, AreaID) VALUES (46063, 1234, 777, 9, '123')");
        WorldDatabase.DirectExecute(sql.c_str());
        auto custom = WorldDatabase.Query("SELECT Timer, VariableID, Value, AreaID FROM world_quest_update WHERE QuestID=46063");
        Require(custom && custom->GetRowCount() == 1 && custom->Fetch()[0].GetUInt32() == 1234 &&
            custom->Fetch()[1].GetUInt32() == 777 && custom->Fetch()[2].GetUInt32() == 9 &&
            custom->Fetch()[3].GetString() == "123", "Migration overwrote local rotation policy");
        WorldDatabase.DirectExecute("DELETE FROM world_quest_update WHERE QuestID=46063");
        for (unsigned replay = 0; replay < 2; ++replay)
            WorldDatabase.DirectExecute(sql.c_str());
        auto count = WorldDatabase.Query("SELECT COUNT(*) FROM world_quest_update");
        Require(count && count->Fetch()[0].GetUInt32() == 8, "Repeated migration disturbed the local rotation cohort");

        CharacterDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
        CharacterDatabase.DirectExecute("CREATE TEMPORARY TABLE native_rotation_seed LIKE world_quest");
        CharacterDatabase.DirectExecute("CREATE TEMPORARY TABLE world_quest LIKE native_rotation_seed");
        CharacterDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
        sQuestDataStore->LoadWorldQuestTemplates();
        auto rotations = const_cast<WorldQuestMap*>(sQuestDataStore->GetWorldQuestMap());
        Require(rotations && rotations->empty(), "Fixture loaded persisted rotation data");

        sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
        AddSC_SmartScripts();
        sScriptMgr->SwapScriptContext(true);
        sEventObjectDataStore->LoadEventObjectTemplates();
        sSmartScriptMgr->LoadSmartAIFromDB();
        sObjectMgr->LoadPhaseDefinitions();
        sConditionMgr->LoadConditions();
        sConversationDataStore->LoadConversationData();
        HashMapHolder<EventObject>::SetSize(1000);
        HashMapHolder<Conversation>::SetSize(20000);

        RotationCase const cases[] = {
            {46021, 219, 4603, 7651, 21600, 13005, 1, 13084},
            {46063, 230, 4429, 8584, 86400, 13114, 2, 0},
            {46073, 232, 4439, 8551, 86400, 12978, 3, 0}
        };
        for (RotationCase const& test : cases)
        {
            Quest const* quest = sQuestDataStore->GetQuestTemplate(test.QuestId);
            auto task = sQuestV2CliTaskStore.LookupEntry(test.QuestId);
            Require(quest && quest->IsWorld() && task, "Not a native world task");
            auto update = (sQuestDataStore->*Access(RotationUpdateTag{}))(test.QuestId, quest->QuestInfoID, 0);
            Require(update && update->worldQuest && update->Timer == test.Timer && update->VariableID == test.Variable &&
                update->VariableID1 == test.SecondaryVariable && update->Value == test.Value && !update->EventID &&
                update->Value1 == (test.SecondaryVariable ? 1u : 0u), "Loader changed the scoped rotation policy");
            auto areas = sDB2Manager.GetAreasForGroup(quest->AreaGroupID);
            Require(std::find(areas.begin(), areas.end(), test.Area) != areas.end(), "Event is outside its native area group");
            std::set<WorldQuestState> states{{test.Variable, test.Value}};
            if (test.SecondaryVariable)
                states.emplace(test.SecondaryVariable, 1);
            std::set<WorldQuestState> nativeStates;
            auto expression = sWorldStateExpressionStore.LookupEntry(task->WorldStateExpressionID);
            Require(expression, "Native world state expression missing");
            expression->Eval(nullptr, &nativeStates);
            Require(states == nativeStates, "Rotation data differ from the client's published states");

            auto spawn = WorldDatabase.PQuery("SELECT position_x,position_y,position_z,orientation FROM eventobject WHERE id=%u AND map=1220", test.EventId);
            Require(spawn && spawn->GetRowCount() == 1, "Unexpected outdoor trigger spawn count");
            auto fields = spawn->Fetch();
            Position origin{fields[0].GetFloat(), fields[1].GetFloat(), fields[2].GetFloat(), fields[3].GetFloat()};
            Map map(1220, 300, 0, DIFFICULTY_NONE);
            RotationPlayer eligible(map, 101, origin);
            RotationPlayer locked(map, 102, origin);
            RotationPlayer late(map, 103, origin);
            Require(eligible.GetCurrentAreaID() == test.Area, "Actual client map resolves outside the authored task area");
            if (test.QuestId == 46021)
            {
                Require(update->worldQuest->PrimaryID == QUEST_INFO_LEGION_INVASION_WORLD_QUEST_WRAPPER, "Assault task lost its native wrapper gate");
                eligible.UnlockAssaults();
                late.UnlockAssaults();
                sQuestDataStore->WorldLegionInvasionZoneID = quest->QuestSortID;
            }
            else
            {
                eligible.UnlockBrokenShore();
                late.UnlockBrokenShore();
            }
            Require(!sConditionMgr->IsPlayerMeetingCondition(&locked, task->ConditionID), "Native unlock condition unexpectedly offered the task");
            eligible.GetPhaseMgr().NotifyStoresReloaded();
            if (test.QuestId == 46021)
                Require(!eligible.HasPhaseId(6788), "Inactive Thel'draz phase was preseeded");
            AreaDialogueEvent event(map, eligible, test.EventId);
            event.Update(1);
            Require(!event.ActivatedForPlayer(&eligible) && LiveConversations().empty(), "Inactive rotation consumed entry or played a conversation");
            Require((sQuestDataStore->*Access(RotationAdmissionTag{}))(update->worldQuest, update), "Scheduler rejected a compatible candidate");

            auto published = (sQuestDataStore->*Access(RotationGenerateTag{}))(update, update->worldQuest);
            Require(published && published->State == states, "Native scheduler did not publish the task states");
            for (auto const& state : states)
                Require(sWorldStateMgr.GetWorldStateValue(state.first) == uint32(state.second), "World state manager lost the published variant");
            for (uint32 area : areas)
                Require(sQuestDataStore->GetWorldQuestTask(area) && sQuestDataStore->GetWorldQuestTask(area)->count(quest), "Publication omitted a native area group member");

            eligible.PumpCallbacks();
            Require(eligible.FindQuestSlot(test.QuestId) == MAX_QUEST_LOG_SIZE, "Fixture moved or admitted the player before publication was queued");
            sQuestDataStore->QueueWorldQuestTaskRefreshes();
            Require(eligible.FindQuestSlot(test.QuestId) == MAX_QUEST_LOG_SIZE, "World thread admitted the task directly");
            eligible.PumpCallbacks();
            locked.PumpCallbacks();
            bool added = eligible.FindQuestSlot(test.QuestId) < MAX_QUEST_LOG_SIZE;
            std::cout << "ROTATION " << test.QuestId << " area=" << test.Area << " added=" << added
                      << " condition=" << sConditionMgr->IsPlayerMeetingCondition(&eligible, task->ConditionID) << std::endl;
            Require(added && locked.FindQuestSlot(test.QuestId) == MAX_QUEST_LOG_SIZE, "Stationary world task eligibility failed");
            eligible.PumpCallbacks(100);
            if (test.QuestId == 46021)
                Require(eligible.NeedPhaseRecalculate, "Native quest acceptance did not request a phase update");
            eligible.GetPhaseMgr().NotifyStoresReloaded();
            uint32 questPhase = test.QuestId == 46021 ? 6788 : test.QuestId == 46063 ? 7061 : 7147;
            Require(eligible.HasPhaseId(questPhase), "Accepted world task cannot see its native objective phase");
            event.Update(1);
            auto conversations = LiveConversations();
            Require(conversations.size() == 1 && conversations[0]->GetEntry() == test.Conversation &&
                conversations[0]->GetCasterGUID() == eligible.GetGUID(), "Stationary accepted task failed its personal introduction");
            ClearConversations(map);
            sQuestDataStore->QueueWorldQuestTaskRefreshes();
            eligible.PumpCallbacks();
            event.Update(1);
            Require(LiveConversations().empty() && eligible.GetQuestStatus(test.QuestId) == QUEST_STATUS_INCOMPLETE,
                "Repeated polling replayed the introduction or changed objective progress");

#ifdef LEGION_ROTATION_PROGRESSION
            CheckProgressAndReward(eligible, locked, quest, (*rotations)[quest->QuestSortID][test.QuestId]);
            if (test.QuestId == 46073)
                CheckPartyKill(map, origin, test.Area);
#endif
            (*rotations)[quest->QuestSortID][test.QuestId].ResetTime = GameTime::GetGameTime();
            late.PumpCallbacks();
            Require(late.FindQuestSlot(test.QuestId) == MAX_QUEST_LOG_SIZE, "An expired task was offered from a queued refresh");

            std::vector<uint32> alternatives;
            if (test.QuestId == 46063)
                alternatives = {46821, 46046};
            if (test.QuestId == 46073)
                alternatives = {45743, 45744};
            for (uint32 otherId : alternatives)
            {
                auto otherQuest = sQuestDataStore->GetQuestTemplate(otherId);
                Require(otherQuest, "Native phase competitor quest missing");
                auto alternative = (sQuestDataStore->*Access(RotationUpdateTag{}))(otherId, otherQuest->QuestInfoID, 0);
                Require(alternative, "Native phase competitor rotation missing");
                Require(!(sQuestDataStore->*Access(RotationAdmissionTag{}))(alternative->worldQuest, alternative),
                    "An alternative state was reused before the expired task was cleaned up");
                (*rotations)[quest->QuestSortID][test.QuestId].ResetTime += 600;
                Require(!(sQuestDataStore->*Access(RotationAdmissionTag{}))(alternative->worldQuest, alternative),
                    "An alternative could overwrite the active task's state or hide its objective phase");
                rotations->clear();
                auto other = (sQuestDataStore->*Access(RotationGenerateTag{}))(alternative, alternative->worldQuest);
                Require(other && !(sQuestDataStore->*Access(RotationAdmissionTag{}))(update->worldQuest, update),
                    "The restored task could overwrite an active alternative state or phase");
#ifdef LEGION_ROTATION_GARRISON_PLACEMENTS
                if (test.QuestId == 46073)
                    CheckFelskornVisibility(map, otherId);
#endif
                (sQuestDataStore->*Access(RotationRemoveTaskTag{}))(alternative->quest);
                rotations->clear();
                auto original = (sQuestDataStore->*Access(RotationGenerateTag{}))(update, update->worldQuest);
                Require(original, "Native state could not resume after the competing rotation ended");
                (*rotations)[quest->QuestSortID][test.QuestId].ResetTime = GameTime::GetGameTime();
            }
            (sQuestDataStore->*Access(RotationRemoveTaskTag{}))(quest);
            rotations->clear();
            sQuestDataStore->WorldLegionInvasionZoneID = 0;
            sQuestDataStore->QueueWorldQuestTaskRefreshes();
        }

        std::cout << "PASS three native rotation loads, SQL replay/custom rows, client expressions and all area-group members, queued stationary offers and personal introductions, repeated poll, and shared-state exclusion\n";
        CharacterDatabase.Close();
        LoginDatabase.Close();
        WorldDatabase.Close();
        HotfixDatabase.Close();
        MySQL::Library_End();
        return 0;
    }
    catch (std::exception const& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
