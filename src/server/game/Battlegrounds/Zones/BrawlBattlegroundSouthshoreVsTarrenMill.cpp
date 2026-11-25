
#include "BrawlBattlegroundSouthshoreVsTarrenMill.h"
#include "BattlegroundPackets.h"
#include "Creature.h"
#include "WaypointManager.h"

static constexpr uint8 RankCount = 16;
static constexpr uint8 MaxScores = 150;
static constexpr uint8 KillsPerRank = 10;
static constexpr uint32 Graveyards[MAX_TEAMS] = {4850, 4851};

namespace
{
    constexpr uint32 GuardRespawnSeconds = 30;
    constexpr uint32 GuardCorpseSeconds = 5;
    constexpr uint32 OrchardPatrolPath = 845930;

    struct TownGuard
    {
        uint32 Entry;
        TeamId Team;
        Position Spawn;
        uint32 Path;
    };

    // Reconstructed on the Legion map from the well, town fork and orchard in retail footage.
    TownGuard const TownGuards[] =
    {
        {84557, TEAM_ALLIANCE, {1815.50f, 1087.00f, 12.2660f, 5.9f}, 0}, // Near the well and stairs
        {84557, TEAM_ALLIANCE, {1831.00f, 1063.00f, 13.4681f, 2.2f}, 0}, // Across the road
        {84593, TEAM_HORDE, {2585.00f, 712.00f, 55.1379f, 0.0f}, 0}, // Inside the orchard fence
        {84593, TEAM_HORDE, {2605.25f, 699.00f, 55.4913f, 2.7f}, OrchardPatrolPath}, // Farm-side path
    };
}

static constexpr uint32 RankSpells[RankCount][MAX_TEAMS] =
{
    { 168412, 168429 }, // 0%
    { 168413, 168430 }, // 1%
    { 168414, 168431 }, // 2%
    { 168415, 168432 }, // 3%
    { 168416, 168433 }, // 4%
    { 168417, 168434 }, // 5%
    { 168418, 168435 }, // 6%
    { 168419, 168436 }, // 7%
    { 168420, 168437 }, // 8%
    { 168421, 168438 }, // 9%
    { 168422, 168439 }, // 10%
    { 168423, 168440 }, // 11%
    { 168424, 168441 }, // 12%
    { 168425, 168442 }, // 13%
    { 168426, 168443 }, // 14%
    { 168428, 168444 }, // 15%
};

struct BrawlBattlegroundSouthshoreVsTarrenMillScore final : BattlegroundScore
{
    friend class BrawlBattlegroundSouthshoreVsTarrenMill;

protected:
    BrawlBattlegroundSouthshoreVsTarrenMillScore(ObjectGuid playerGuid, TeamId team) : BattlegroundScore(playerGuid, team) { }

    void UpdateScore(uint32 type, uint64 value) override
    {
        BattlegroundScore::UpdateScore(type, value);
    }

    void BuildObjectivesBlock(std::vector<int32>& /*stats*/) override
    {
    }
};

BrawlBattlegroundSouthshoreVsTarrenMill::BrawlBattlegroundSouthshoreVsTarrenMill()
{
    BgCreatures.resize(MAX_TEAMS + std::size(TownGuards));
}

void BrawlBattlegroundSouthshoreVsTarrenMill::Reset()
{
    Battleground::Reset();
    _honorableKills.clear();
    _battleElapsed = Milliseconds::zero();
}

bool BrawlBattlegroundSouthshoreVsTarrenMill::SetupBattleground()
{
    auto const* orchardPath = sWaypointMgr->GetPath(OrchardPatrolPath);
    if (!orchardPath || orchardPath->empty())
    {
        TC_LOG_ERROR("bg.battleground", "Southshore brawl: missing orchard patrol path %u", OrchardPatrolPath);
        return false;
    }

    for (TeamId team : {TEAM_ALLIANCE, TEAM_HORDE})
    {
        auto const* graveyard = sWorldSafeLocsStore.LookupEntry(Graveyards[team]);
        if (!graveyard || graveyard->MapID != GetMapId() || !AddSpiritGuide(team, graveyard->Loc, team))
            return false;
    }

    for (uint32 index = 0; index < std::size(TownGuards); ++index)
    {
        TownGuard const& spawn = TownGuards[index];
        Creature* guard = AddCreature(spawn.Entry, MAX_TEAMS + index, MS::Battlegrounds::GetTeamByTeamId(spawn.Team), spawn.Spawn, GuardRespawnSeconds);
        if (!guard)
            return false;

        guard->SetCorpseDelay(GuardCorpseSeconds);
        if (spawn.Path)
        {
            guard->LoadPath(spawn.Path);
            guard->SetDefaultMovementType(WAYPOINT_MOTION_TYPE);
            guard->GetMotionMaster()->InitDefault();
        }
    }
    return true;
}

void BrawlBattlegroundSouthshoreVsTarrenMill::PostUpdateImpl(uint32 diff)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    _battleElapsed += Milliseconds(diff);
    if (_battleElapsed >= Minutes(30))
        BattlegroundTimedWin();
}

void BrawlBattlegroundSouthshoreVsTarrenMill::AddPlayer(Player* player)
{
    Battleground::AddPlayer(player);
    auto playerGuid = player->GetGUID();
    PlayerScores[playerGuid] = new BrawlBattlegroundSouthshoreVsTarrenMillScore(playerGuid, player->GetBGTeamId());
    player->SendDirectMessage(WorldPackets::Battleground::Init(MaxScores).Write());
    SendBattleGroundPoints(false, m_TeamScores[TEAM_ALLIANCE], false, player);
    SendBattleGroundPoints(true, m_TeamScores[TEAM_HORDE], false, player);
}

void BrawlBattlegroundSouthshoreVsTarrenMill::RemovePlayer(Player* player, ObjectGuid guid, uint32 /*team*/)
{
    if (player)
        ResetRank(player);
    else
        _honorableKills.erase(guid);
}

WorldSafeLocsEntry const* BrawlBattlegroundSouthshoreVsTarrenMill::GetClosestGraveYard(Player* player)
{
    TeamId team = player->GetBGTeamId();
    return team < MAX_TEAMS ? sWorldSafeLocsStore.LookupEntry(Graveyards[team]) : nullptr;
}

void BrawlBattlegroundSouthshoreVsTarrenMill::ResetRank(Player* player)
{
    auto kills = _honorableKills.find(player->GetGUID());
    if (kills == _honorableKills.end())
        return;

    player->RemoveAura(RankSpells[kills->second / KillsPerRank][player->GetTeamId()]);
    _honorableKills.erase(kills);
}

void BrawlBattlegroundSouthshoreVsTarrenMill::HandleKillPlayer(Player* player, Player* killer)
{
    if (GetStatus() != STATUS_IN_PROGRESS || !player)
        return;

    bool enemyKill = killer && IsPlayerInBattleground(killer->GetGUID()) && player->GetBGTeam() != killer->GetBGTeam();
    Battleground::HandleKillPlayer(player, enemyKill ? killer : nullptr);
    ResetRank(player);

    if (!enemyKill)
        return;

    AwardTeamPoint(killer->GetBGTeamId());
}

void BrawlBattlegroundSouthshoreVsTarrenMill::HandleKillUnit(Creature* unit, Player* killer)
{
    if (GetStatus() != STATUS_IN_PROGRESS || !unit || !killer || !IsPlayerInBattleground(killer->GetGUID()))
        return;

    TeamId team = killer->GetBGTeamId();
    for (uint32 index = 0; index < std::size(TownGuards); ++index)
    {
        if (BgCreatures[MAX_TEAMS + index] == unit->GetGUID() && TownGuards[index].Team != team)
        {
            AwardTeamPoint(team);
            return;
        }
    }
}

void BrawlBattlegroundSouthshoreVsTarrenMill::AwardTeamPoint(TeamId team)
{
    if (team >= MAX_TEAMS)
        return;

    ++m_TeamScores[team];
    SendBattleGroundPoints(team == TEAM_HORDE, m_TeamScores[team]);
    if (m_TeamScores[team] >= MaxScores)
        EndBattleground(MS::Battlegrounds::GetTeamByTeamId(team));
}

bool BrawlBattlegroundSouthshoreVsTarrenMill::UpdatePlayerScore(Player* player, uint32 type, uint64 value, bool doAddHonor)
{
    if (!Battleground::UpdatePlayerScore(player, type, value, doAddHonor))
        return false;

    if (type == SCORE_HONORABLE_KILLS && GetStatus() == STATUS_IN_PROGRESS && value)
    {
        auto& kills = _honorableKills[player->GetGUID()];
        uint8 oldRank = kills / KillsPerRank;
        kills = static_cast<uint8>(kills + std::min<uint64>(value, (RankCount - 1) * KillsPerRank - kills));
        uint8 newRank = kills / KillsPerRank;
        if (newRank != oldRank)
        {
            TeamId faction = player->GetTeamId();
            player->RemoveAura(RankSpells[oldRank][faction]);
            player->CastSpell(player, RankSpells[newRank][faction], true);
        }
    }
    return true;
}
