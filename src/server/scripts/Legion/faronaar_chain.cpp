#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "TemporarySummon.h"
#include "GameObject.h"
#include "DatabaseEnv.h"
#include <map>
#include <mutex>

namespace Faronaar
{
constexpr uint32 Rescue = 37656, Saving = 37450, Revelations = 37449;
Position const BoundDragon = {-736.529f, 7336.21f, 26.2122f, 5.56755f};
Position const Traitor = {-521.031f, 7494.2f, 75.0567f, 0.645079f};
struct Chain { uint64 spawn; Position beam; };
Chain const Chains[] = {
    {109169, {-725.292f, 7310.92f, 20.2072f, 2.00997f}},
    {109175, {-723.945f, 7348.98f, 21.1371f, 3.92747f}},
    {109181, {-758.127f, 7326.33f, 20.1855f, 0.4367f}}
};

bool Active(Player* player, uint32 quest)
{
    return player && player->IsAlive() && player->GetMapId() == 1220 && player->GetZoneId() == 7334 &&
        player->GetQuestStatus(quest) == QUEST_STATUS_INCOMPLETE;
}
bool Done(Player* player, uint32 quest, uint32 object)
{
    return player->GetQuestObjectiveData(quest, object) != 0;
}
uint32 Count(Player* player, uint32 quest)
{
    return player->GetQuestObjectiveData(quest, quest == Saving ? 239455 : 90487);
}

struct State
{
    uint32 timer = 0;
    bool loaded = false, released = false;
    std::map<uint32, std::map<uint64, uint32>> uses;
    ObjectGuid dragon, beams[3];
};
std::map<ObjectGuid, State> states;
std::mutex mutex;

void Remove(Player* player, ObjectGuid& guid)
{
    if (Creature* creature = ObjectAccessor::GetCreature(*player, guid))
        creature->DespawnOrUnsummon();
    guid.Clear();
}
void ClearDragon(Player* player, State& state)
{
    Remove(player, state.dragon);
    for (ObjectGuid& guid : state.beams)
        Remove(player, guid);
    state.released = false;
}
Creature* Owned(Player* player, uint32 entry)
{
    for (ObjectGuid const& guid : *player->GetSummonList(entry))
        if (Creature* creature = ObjectAccessor::GetCreature(*player, guid))
            if (creature->IsAlive() && creature->ToTempSummon() &&
                creature->ToTempSummon()->GetSummonerGUID() == player->GetGUID())
                return creature;
    return nullptr;
}
void ClearOwned(Player* player, uint32 entry)
{
    GuidList copies = *player->GetSummonList(entry);
    for (ObjectGuid const& guid : copies)
        if (Creature* creature = ObjectAccessor::GetCreature(*player, guid))
            creature->DespawnOrUnsummon();
}

void Load(Player* player, State& state)
{
    if (state.loaded)
        return;
    state.loaded = true;
    // A map-change callback cannot resolve old-map actors. Remove surviving
    // personal copies when that player returns before their finite expiry.
    ClearOwned(player, 90546);
    ClearOwned(player, 90578);
    if (QueryResult result = CharacterDatabase.PQuery(
        "SELECT quest,spawn,ordinal FROM character_faronaar_interactions WHERE guid=%u", player->GetGUIDLow()))
        do
        {
            Field* fields = result->Fetch();
            state.uses[fields[0].GetUInt32()][fields[1].GetUInt64()] = fields[2].GetUInt32();
        } while (result->NextRow());
}

// An ordinal beyond saved progress belongs to a rolled-back or abandoned run.
// Keep earlier uses on relog; remove all uses when the quest is removed/rewarded.
void Reconcile(Player* player, State& state, uint32 quest)
{
    auto status = player->GetQuestStatus(quest);
    uint32 progress = status == QUEST_STATUS_INCOMPLETE || status == QUEST_STATUS_COMPLETE ? Count(player, quest) : 0;
    auto& uses = state.uses[quest];
    for (auto it = uses.begin(); it != uses.end();)
        if (it->second > progress)
        {
            CharacterDatabase.DirectPExecute("DELETE FROM character_faronaar_interactions WHERE guid=%u AND quest=%u AND spawn=%llu",
                player->GetGUIDLow(), quest, static_cast<unsigned long long>(it->first));
            it = uses.erase(it);
        }
        else
            ++it;
}

bool RecordUse(Player* player, State& state, uint32 quest, uint64 spawn)
{
    Load(player, state);
    Reconcile(player, state, quest);
    if (!spawn || state.uses[quest].count(spawn) || Count(player, quest) >= (quest == Saving ? 3u : 6u))
        return false;
    uint32 ordinal = Count(player, quest) + 1;
    CharacterDatabase.DirectPExecute(
        "INSERT INTO character_faronaar_interactions (guid,quest,spawn,ordinal) VALUES (%u,%u,%llu,%u)",
        player->GetGUIDLow(), quest, static_cast<unsigned long long>(spawn), ordinal);
    state.uses[quest][spawn] = ordinal;
    return true;
}

void UpdateDragon(Player* player, State& state)
{
    auto status = player->GetQuestStatus(Saving);
    if (!player->IsAlive() || player->GetMapId() != 1220 ||
        (status != QUEST_STATUS_INCOMPLETE && status != QUEST_STATUS_COMPLETE) || player->GetDistance(BoundDragon) > 100.0f)
    {
        ClearDragon(player, state);
        return;
    }
    Creature* dragon = ObjectAccessor::GetCreature(*player, state.dragon);
    if (Count(player, Saving) >= 3)
    {
        for (ObjectGuid& guid : state.beams)
            Remove(player, guid);
        if (dragon && !state.released)
        {
            state.released = true;
            dragon->RemoveAurasDueToSpell(65612);
            dragon->SetDisableGravity(true);
            dragon->GetMotionMaster()->MovePoint(1, BoundDragon.GetPositionX(), BoundDragon.GetPositionY(), BoundDragon.GetPositionZ() + 35.0f);
            dragon->DespawnOrUnsummon(15000);
        }
        return;
    }
    if (!dragon)
    {
        dragon = player->SummonCreature(90546, BoundDragon, TEMPSUMMON_TIMED_DESPAWN, 300000, 0, player->GetGUID());
        if (!dragon)
            return;
        state.dragon = dragon->GetGUID();
        state.released = false;
        dragon->SetDisableGravity(true);
    }
    if (!Done(player, Saving, 90546) && player->IsWithinDistInMap(dragon, 25.0f) && player->IsWithinLOSInMap(dragon))
        player->KilledMonsterCredit(90546);
    for (uint32 i = 0; i < 3; ++i)
    {
        if (state.uses[Saving].count(Chains[i].spawn))
        {
            Remove(player, state.beams[i]);
            continue;
        }
        if (!ObjectAccessor::GetCreature(*player, state.beams[i]))
            if (Creature* beam = player->SummonCreature(90578, Chains[i].beam, TEMPSUMMON_TIMED_DESPAWN, 300000, 0, player->GetGUID()))
            {
                state.beams[i] = beam->GetGUID();
                beam->CastSpell(dragon, 65612, true);
            }
    }
}

bool NeedsCompanion(Player* player)
{
    return player->IsAlive() && player->GetMapId() == 1220 && player->GetZoneId() == 7334 &&
        (player->GetQuestRewardStatus(36920) || player->GetQuestRewardStatus(40815) || player->GetQuestRewardStatus(44140)) &&
        player->GetQuestStatus(Revelations) != QUEST_STATUS_COMPLETE && !player->GetQuestRewardStatus(Revelations);
}
} // namespace Faronaar

struct npc_faronaar_personal_actor : public ScriptedAI
{
    npc_faronaar_personal_actor(Creature* creature) : ScriptedAI(creature) { me->SetReactState(REACT_PASSIVE); }
};

class go_faronaar_objective : public GameObjectScript
{
public:
    go_faronaar_objective() : GameObjectScript("go_faronaar_objective") {}
    bool OnGossipHello(Player* player, GameObject* go) override
    {
        using namespace Faronaar;
        uint32 quest = go->GetEntry() == 239455 ? Saving : Rescue;
        if (!Active(player, quest) || !player->IsWithinDistInMap(go, 6.0f) || !player->IsWithinLOSInMap(go))
            return true;
        std::lock_guard<std::mutex> lock(mutex);
        State& state = states[player->GetGUID()];
        if (quest == Saving)
        {
            bool nativeLock = false;
            for (Chain const& chain : Chains)
                nativeLock |= chain.spawn == go->GetDBTableGUIDLow();
            if (!nativeLock || !Done(player, Saving, 90546) || !player->HasItemCount(120359, 1) ||
                !RecordUse(player, state, quest, go->GetDBTableGUIDLow()))
                return true;
            player->QuestObjectiveSatisfy(239455, 1, QUEST_OBJECTIVE_GAMEOBJECT, go->GetGUID());
            if (!Done(player, Saving, 105635))
                player->KilledMonsterCredit(105635);
            UpdateDragon(player, state);
        }
        else
        {
            if (go->GetEntry() != 240075 && go->GetEntry() != 240121 && go->GetEntry() != 240122 && go->GetEntry() != 240123)
                return true;
            Creature* captive = go->FindNearestCreature(90487, 10.0f, true);
            if (!captive || !RecordUse(player, state, quest, go->GetDBTableGUIDLow()))
                return true;
            player->KilledMonsterCredit(90487, captive->GetGUID());
            captive->AI()->SetData(0, 1); // Original release dialogue and departure.
        }
        return true; // Suppress shared SmartGO/generic credit paths.
    }
};

class player_faronaar_chain : public PlayerScript
{
public:
    player_faronaar_chain() : PlayerScript("player_faronaar_chain") {}
    void OnLogout(Player* player) override
    {
        using namespace Faronaar;
        std::lock_guard<std::mutex> lock(mutex);
        auto it = states.find(player->GetGUID());
        if (it != states.end())
        {
            Reconcile(player, it->second, Saving);
            Reconcile(player, it->second, Rescue);
            ClearDragon(player, it->second);
            states.erase(it);
        }
        ClearOwned(player, 90474);
        ClearOwned(player, 90982);
    }
    void OnMapChanged(Player* player) override
    {
        if (player->GetMapId() != 1220)
            OnLogout(player); // Old-map summons also have finite lifetimes.
    }
    void OnUpdate(Player* player, uint32 diff) override
    {
        using namespace Faronaar;
        bool companion = NeedsCompanion(player);
        bool relevant = companion || Active(player, Saving) || Active(player, Rescue) || Active(player, Revelations);
        std::lock_guard<std::mutex> lock(mutex);
        auto it = states.find(player->GetGUID());
        if (!relevant && it == states.end())
            return;
        State& state = states[player->GetGUID()];
        state.timer += diff;
        if (state.timer < 1000)
            return;
        state.timer = 0;
        Load(player, state);
        Reconcile(player, state, Saving);
        Reconcile(player, state, Rescue);
        UpdateDragon(player, state);
        if (companion)
        {
            if (!Owned(player, 90474))
            {
                // Native effect 0 expects an explicit destination (target 142).
                player->CastSpell(player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), 178860, true);
                if (Creature* follower = Owned(player, 90474))
                {
                    follower->AddPlayerInPersonnalVisibilityList(player->GetGUID());
                    follower->DespawnOrUnsummon(300000);
                }
            }
            if (Creature* follower = Owned(player, 90474))
                if (!follower->isInCombat() && follower->GetMotionMaster()->GetCurrentMovementGeneratorType() != FOLLOW_MOTION_TYPE)
                    follower->GetMotionMaster()->MoveFollow(player, 3.0f, 3.14f);
        }
        else
            ClearOwned(player, 90474);

        if (Active(player, Revelations) && Done(player, Revelations, 90621))
        {
            if (!Done(player, Revelations, 112175) && !player->GetVehicle() && !player->isInFlight() && !player->IsFlying())
            {
                Creature* leader = player->FindNearestCreature(89362, 15.0f);
                if (!leader)
                    leader = player->FindNearestCreature(101927, 15.0f);
                if (leader && player->IsWithinLOSInMap(leader))
                    player->KilledMonsterCredit(112175);
            }
            if (!Done(player, Revelations, 112175) && player->GetDistance(Traitor) < 25.0f &&
                !player->GetVehicle() && !Owned(player, 90982))
                // 178923's summon category 4 forces boarding. The native NPC's
                // spellclick/vehicle instead lets the player choose this ride.
                if (Creature* ride = player->SummonCreature(90982, player->GetPosition(),
                    TEMPSUMMON_TIMED_DESPAWN, 300000, 0, player->GetGUID()))
                {
                    ride->SetCanFly(true);
                    ride->SetDisableGravity(true);
                }
        }
        else
            ClearOwned(player, 90982);
        if (!relevant)
            states.erase(player->GetGUID());
    }
};

void AddSC_faronaar_chain()
{
    RegisterCreatureAI(npc_faronaar_personal_actor);
    new go_faronaar_objective();
    new player_faronaar_chain();
}
