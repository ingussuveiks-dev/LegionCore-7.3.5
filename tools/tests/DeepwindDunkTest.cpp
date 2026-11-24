#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>

using uint32 = std::uint32_t;
using Milliseconds = std::chrono::milliseconds;
using Seconds = std::chrono::seconds;
enum TeamId { TEAM_ALLIANCE, TEAM_HORDE, MAX_TEAMS };
enum { STATUS_IN_PROGRESS, STATUS_WAIT_LEAVE, SPELL_CAST_OK, SPELL_FAILED, RESPAWN_ONE_DAY };
namespace MS::Battlegrounds
{
    TeamId GetOtherTeamID(TeamId team) { return team == TEAM_ALLIANCE ? TEAM_HORDE : TEAM_ALLIANCE; }
    TeamId GetTeamByTeamId(TeamId team) { return team; }
}

struct ObjectGuid
{
    uint32 Id = 0;
    void Clear() { Id = 0; }
    bool IsEmpty() const { return !Id; }
    bool operator==(ObjectGuid other) const { return Id == other.Id; }
    bool operator!=(ObjectGuid other) const { return !(*this == other); }
};

struct GameObject
{
    ObjectGuid Guid;
    float X;
    bool Spawned = true;
    bool isSpawned() const { return Spawned; }
    ObjectGuid GetGUID() const { return Guid; }
};

struct Player
{
    ObjectGuid Guid;
    TeamId Team;
    float X;
    bool Alive = true;
    bool LineOfSight = true;
    bool AcceptCast = true;
    std::set<uint32> Auras;
    ObjectGuid GetGUID() const { return Guid; }
    TeamId GetBGTeamId() const { return Team; }
    bool IsAlive() const { return Alive; }
    bool IsWithinDistInMap(GameObject* object, float range) const { return std::abs(X - object->X) <= range; }
    bool IsWithinLOSInMap(GameObject*) const { return LineOfSight; }
    bool HasAura(uint32 spell) const { return Auras.count(spell) != 0; }
    void RemoveAurasDueToSpell(uint32 spell) { Auras.erase(spell); }
    int CastSpell(Player*, uint32 spell, bool triggered)
    {
        if (!AcceptCast)
            return SPELL_FAILED;
        if (triggered)
            Auras.insert(spell);
        return SPELL_CAST_OK;
    }
};

constexpr uint32 BallHome = 0;
constexpr uint32 BallDrop = 3;
constexpr uint32 Basket = 6;
constexpr uint32 DunkScoreLimit = 1500;
constexpr uint32 LongRangeAction = 231658;
constexpr uint32 DunkRangeAction = 231660;
constexpr float DunkRange = 5.0f;
constexpr float ShotRange = 60.0f;
constexpr float INTERACTION_DISTANCE = 5.0f;
struct Placement { uint32 Aura; };
constexpr Placement Dunkballs[] = {{231529}, {231814}, {231813}};

struct BattlegroundDeepwindDunk
{
    static constexpr uint32 CaptureSpell = 97388;
    static constexpr uint32 DunkSpell = 231651;
    static constexpr uint32 ShotSpell = 231789;
    struct Ball { ObjectGuid Carrier, CapturingPlayer, CaptureTarget; Milliseconds Returning{0}; };
    std::array<Ball, 3> _balls{};
    std::array<ObjectGuid, 8> BgObjects{};
    std::map<uint32, GameObject> Objects;
    uint32 m_TeamScores[MAX_TEAMS] = {};
    int Status = STATUS_IN_PROGRESS;
    TeamId Winner = MAX_TEAMS;
    int Updates = 0;

    int GetStatus() const { return Status; }
    BattlegroundDeepwindDunk* GetBgMap() { return this; }
    GameObject* GetGameObject(ObjectGuid guid)
    {
        auto found = Objects.find(guid.Id);
        return found == Objects.end() ? nullptr : &found->second;
    }
    GameObject* GetBGObject(uint32 slot) { return GetGameObject(BgObjects[slot]); }
    void SpawnBGObject(uint32 slot, uint32 wait) { GetBGObject(slot)->Spawned = wait == 0; }
    void DelObject(uint32 slot) { Objects.erase(BgObjects[slot].Id); BgObjects[slot].Clear(); }
    void SendBattleGroundPoints(bool, uint32) { ++Updates; }
    void EndBattleground(TeamId team) { Winner = team; Status = STATUS_WAIT_LEAVE; }
    void EventPlayerClickedOnFlag(Player* player, GameObject* object, bool& canRemove);
    void OnObjectiveCastFinished(Player* player, uint32 spellId, bool succeeded);
    GameObject* GetScoringBasket(Player* player, uint32 spellId);
    void UpdateShotAction(Player* carrier);
    void ReturnBall(uint32 index, Player* carrier);
    void AddPoints(TeamId team, uint32 points);
};

#include "deepwind-objectives.inc"

int main()
{
    BattlegroundDeepwindDunk match;
    match.BgObjects[BallHome] = {1};
    match.BgObjects[Basket + TEAM_ALLIANCE] = {2};
    match.BgObjects[Basket + TEAM_HORDE] = {3};
    match.Objects = {{1, {{1}, 0}}, {2, {{2}, -100}}, {3, {{3}, 100}}};
    Player alliance{{10}, TEAM_ALLIANCE, 0}, horde{{20}, TEAM_HORDE, 0};
    auto* home = match.GetBGObject(BallHome);
    bool canRemove = true;
    match.EventPlayerClickedOnFlag(&alliance, home, canRemove);
    match.EventPlayerClickedOnFlag(&horde, home, canRemove);
    match.OnObjectiveCastFinished(&horde, match.CaptureSpell, true);
    if (canRemove || horde.HasAura(231529) || !home->Spawned || match._balls[0].CapturingPlayer != alliance.Guid)
        return 1;

    match.OnObjectiveCastFinished(&alliance, match.CaptureSpell, false);
    alliance.AcceptCast = false;
    match.EventPlayerClickedOnFlag(&alliance, home, canRemove);
    if (!match._balls[0].CapturingPlayer.IsEmpty() || !home->Spawned)
        return 2;
    alliance.AcceptCast = true;
    match.EventPlayerClickedOnFlag(&alliance, home, canRemove);
    match.OnObjectiveCastFinished(&alliance, match.CaptureSpell, true);
    if (!alliance.HasAura(231529) || home->Spawned)
        return 3;

    alliance.X = -100;
    if (match.GetScoringBasket(&alliance, match.ShotSpell))
        return 4;
    alliance.X = 45;
    match.UpdateShotAction(&alliance);
    if (!alliance.HasAura(LongRangeAction) || match.GetScoringBasket(&alliance, match.DunkSpell))
        return 5;
    alliance.LineOfSight = false;
    match.OnObjectiveCastFinished(&alliance, match.ShotSpell, true);
    if (match.Updates || !alliance.HasAura(231529))
        return 6;

    alliance.LineOfSight = true;
    alliance.X = 98;
    match.UpdateShotAction(&alliance);
    if (!alliance.HasAura(DunkRangeAction) || alliance.HasAura(LongRangeAction))
        return 7;
    match.OnObjectiveCastFinished(&alliance, match.DunkSpell, false);
    match.m_TeamScores[TEAM_ALLIANCE] = 1450;
    match.OnObjectiveCastFinished(&alliance, match.DunkSpell, true);
    match.OnObjectiveCastFinished(&alliance, match.DunkSpell, true);
    if (match.Winner != TEAM_ALLIANCE || match.Updates != 1 || match.m_TeamScores[TEAM_ALLIANCE] != 1500 ||
        alliance.HasAura(231529) || alliance.HasAura(DunkRangeAction) || match._balls[0].Returning != Seconds(20))
        return 8;
    std::cout << "Deepwind capture, interruption, competing player, enemy basket, both range actions and exactly-once scoring passed.\n";
}
