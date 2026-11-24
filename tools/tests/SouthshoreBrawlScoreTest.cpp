#include <algorithm>
#include <cstdint>
#include <iostream>
#include <unordered_map>

using uint8 = std::uint8_t;
using uint32 = std::uint32_t;
using uint64 = std::uint64_t;
enum TeamId { TEAM_ALLIANCE, TEAM_HORDE, MAX_TEAMS };
namespace MS::Battlegrounds { TeamId GetTeamByTeamId(TeamId team) { return team; } }
enum { STATUS_IN_PROGRESS, STATUS_WAIT_LEAVE, SCORE_HONORABLE_KILLS };

struct Player
{
    uint32 guid;
    TeamId team;
    uint32 aura = 0;
    uint32 GetGUID() const { return guid; }
    TeamId GetBGTeam() const { return team; }
    TeamId GetBGTeamId() const { return team; }
    TeamId GetTeamId() const { return team; }
    void CastSpell(Player*, uint32 spell, bool) { aura = spell; }
    void RemoveAura(uint32 spell) { if (aura == spell) aura = 0; }
};

struct Battleground
{
    uint32 m_TeamScores[MAX_TEAMS] = {};
    int status = STATUS_IN_PROGRESS;
    TeamId victor = MAX_TEAMS;
    int sent = 0;
    bool IsPlayerInBattleground(uint32 guid) const { return guid >= 1 && guid <= 3; }
    int GetStatus() const { return status; }
    void HandleKillPlayer(Player*, Player* killer) { if (killer) UpdatePlayerScore(killer, SCORE_HONORABLE_KILLS, 1, true); }
    virtual bool UpdatePlayerScore(Player*, uint32, uint64, bool) { return true; }
    void SendBattleGroundPoints(bool, uint32) { ++sent; }
    void EndBattleground(TeamId team) { victor = team; status = STATUS_WAIT_LEAVE; }
};

struct BrawlBattlegroundSouthshoreVsTarrenMill : Battleground
{
    std::unordered_map<uint32, uint8> _honorableKills;
    void ResetRank(Player* player);
    void HandleKillPlayer(Player* player, Player* killer);
    bool UpdatePlayerScore(Player* player, uint32 type, uint64 value, bool doAddHonor) override;
};

#include "southshore-score.inc"

int main()
{
    BrawlBattlegroundSouthshoreVsTarrenMill match;
    Player alliance{1, TEAM_ALLIANCE}, horde{2, TEAM_HORDE}, friendly{3, TEAM_ALLIANCE}, outsider{4, TEAM_HORDE};
    match.HandleKillPlayer(&alliance, nullptr);
    match.HandleKillPlayer(&alliance, &alliance);
    match.HandleKillPlayer(&alliance, &friendly);
    match.HandleKillPlayer(&alliance, &outsider);
    if (match.sent || alliance.aura || horde.aura)
        return 1;

    for (unsigned i = 0; i < 9; ++i)
        match.HandleKillPlayer(&horde, &alliance);
    if (match.m_TeamScores[TEAM_ALLIANCE] != 9 || alliance.aura)
        return 2;
    match.HandleKillPlayer(&horde, &alliance);
    if (match.m_TeamScores[TEAM_ALLIANCE] != 10 || alliance.aura != 168413)
        return 3;

    match.UpdatePlayerScore(&alliance, SCORE_HONORABLE_KILLS, UINT64_MAX, true);
    if (alliance.aura != 168428)
        return 4;
    match.HandleKillPlayer(&alliance, &horde);
    if (alliance.aura || match.m_TeamScores[TEAM_HORDE] != 1 || match._honorableKills.count(alliance.guid))
        return 5;

    for (unsigned i = 0; i < 141; ++i)
        match.HandleKillPlayer(&horde, &alliance);
    if (match.victor != TEAM_ALLIANCE || match.sent != 151 || match.m_TeamScores[TEAM_ALLIANCE] != 150)
        return 6;
    std::cout << "Southshore score, rank bounds, death cleanup and 150-point finish passed.\n";
}
