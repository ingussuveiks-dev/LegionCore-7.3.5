#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
using uint8 = uint8_t;
using uint32 = uint32_t;
using ObjectGuid = uint64_t;
constexpr uint8 TEAM_ALLIANCE = 0, TEAM_HORDE = 1, TEAM_NEUTRAL = 2;
constexpr uint32 EventKorlokTheOgreKing = 0, EventStadiumRacing = 1, MaxEvents = 2, MaxStadiumRacingLaps = 3;
constexpr uint32 MaxRacingFlags = 12, MaxRacingCreatures = 4, AllianceRacingFlagSpawn1 = 100,
                 SpeedyHordeRacerSpawn = 200;
constexpr uint32 NeutralKorlokTheOgreKing = 300, OgreAllianceChampion = 301, OgreHordeChapion = 302;
constexpr uint32 WorldStateLapsAlliance = 1, WorldStateLapsHorde = 2, WorldStateEnableLapsEvent = 3,
                 WorldStateDisabled = 0;
constexpr uint32 AshranEventTimer = 30, MINUTE = 60, IN_MILLISECONDS = 1000, AshranMapID = 1191;
constexpr int AllianceVictorious = 1, HordeVictorious = 2;
struct Player
{
    uint32 map = 1191, area = 7279, team = 0, credits = 0;
    uint32 GetMapId()
    {
        return map;
    }
    uint32 GetAreaId()
    {
        return area;
    }
    uint32 GetTeamId()
    {
        return team;
    }
    void KilledMonsterCredit(uint32 id)
    {
        assert(id == 95099);
        ++credits;
    }
};
std::map<ObjectGuid, Player*> players;
struct ObjectAccessor
{
    static Player* GetObjectInMap(ObjectGuid id, void*, Player*)
    {
        return players.count(id) ? players[id] : nullptr;
    }
};
struct CreatureAI
{
    void Talk(int, ObjectGuid)
    {
    }
};
struct Creature
{
    bool IsAIEnabled = false;
    CreatureAI* AI()
    {
        return nullptr;
    }
};
// Production uses ObjectGuid::Empty only for the unrelated herald chat.
struct OutdoorPvPAshran
{
    void* m_map = nullptr;
    std::array<bool, 2> m_AshranEventsLaunched{}, m_AshranEventsWarned{};
    std::array<uint32, 2> m_AshranEvents{}, m_StadiumRacingLaps{};
    std::array<std::set<ObjectGuid>, 2> m_PlayersInWar;
    void EndEvent(uint8, bool = true);
    void SetEventData(uint8, uint8, uint32);
    void DelCreature(uint32)
    {
    }
    void DelObject(uint32)
    {
    }
    void SendUpdateWorldState(uint32, uint32)
    {
    }
    Creature* GetHerald()
    {
        return nullptr;
    }
};
#include "ashran-handlers.inc"
struct Dungeon
{
    uint32 id, map;
};
void ConfigureTrial(Dungeon* dungeon, uint8& minGroupSize, uint8& maxGroupSize, bool& forceMinPlayers,
                    bool& noNeedWaightConfirm)
{
#include "trial-queue.inc"
}
int main()
{
    Player winner, loser, away, wrongMap, foreign;
    loser.team = 1;
    away.area = 7099;
    wrongMap.map = 1116;
    foreign.team = 1;
    players = {{1, &winner}, {2, &loser}, {3, &away}, {4, &wrongMap}, {5, &foreign}};
    OutdoorPvPAshran battle;
    battle.m_PlayersInWar[0] = {1, 3, 4, 5, 99};
    battle.m_PlayersInWar[1] = {2};
    battle.SetEventData(EventStadiumRacing, 0, 3);
    assert(!winner.credits);
    battle.m_AshranEventsLaunched[EventStadiumRacing] = true;
    battle.SetEventData(MaxEvents, 0, 3);
    battle.SetEventData(EventStadiumRacing, TEAM_NEUTRAL, 3);
    assert(!winner.credits);
    battle.SetEventData(EventStadiumRacing, 0, 1);
    battle.SetEventData(EventStadiumRacing, 0, 1);
    assert(battle.m_StadiumRacingLaps[0] == 2 && !winner.credits);
    battle.SetEventData(EventStadiumRacing, 0, 1);
    assert(winner.credits == 1 && !loser.credits && !away.credits && !wrongMap.credits && !foreign.credits);
    assert(!battle.m_AshranEventsLaunched[EventStadiumRacing] && !battle.m_StadiumRacingLaps[0] &&
           !battle.m_StadiumRacingLaps[1]);
    battle.SetEventData(EventStadiumRacing, 0, 1);
    assert(winner.credits == 1);
    battle.m_AshranEventsLaunched[EventStadiumRacing] = true;
    battle.SetEventData(EventStadiumRacing, 0, 1);
    assert(winner.credits == 1);
    battle.EndEvent(EventStadiumRacing, false);
    assert(winner.credits == 1);
    battle.m_AshranEventsLaunched[EventStadiumRacing] = true;
    battle.SetEventData(EventStadiumRacing, 1, 3);
    assert(loser.credits == 1 && winner.credits == 1);
    for (Dungeon d : {Dungeon{870, 1374}, Dungeon{820, 1182}, Dungeon{908, 1460}, Dungeon{870, 1182}})
    {
        uint8 min = 3, max = 5;
        bool force = false, confirm = false;
        ConfigureTrial(&d, min, max, force, confirm);
        if (d.id == 870 && d.map == 1374)
            assert(min == 1 && max == 1 && force && confirm);
        else
            assert(min == 3 && max == 5 && !force && !confirm);
    }
    std::cout << "PASS: actual racing victory/reset handlers, winner/area/map/duplicate guards and isolated solo-trial "
                 "queue rule\n";
}
