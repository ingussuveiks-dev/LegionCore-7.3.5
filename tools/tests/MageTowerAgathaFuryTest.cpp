#include <cassert>
#include <cstdint>
#include <iostream>

struct Position { float x, y, z; };
struct Player
{
    Position position { 1, 2, 3 };
    bool alive = true, present = true, teleport = false;
    bool IsAlive() const { return alive; }
    bool IsInWorld() const { return present; }
    bool HasPendingTeleport() const { return teleport; }
    Position GetPosition() const { return position; }
};

struct Unit
{
    Player* controller = nullptr;
    Player* GetCharmerOrOwnerPlayerOrPlayerItself() const { return controller; }
};

struct TempSummon
{
    uint32_t summoner = 1;
    uint32_t GetSummonerGUID() const { return summoner; }
};

struct Creature : Unit
{
    bool alive = true, combat = true, sameMap = true, samePhase = true;
    uint32_t map = 1616, entry = 115638;
    Unit* victim = nullptr;
    TempSummon* summon = nullptr;
    unsigned casts = 0;
    bool IsAlive() const { return alive; }
    bool isInCombat() const { return combat; }
    uint32_t GetMapId() const { return map; }
    uint32_t GetInstanceId() const { return 12; }
    uint32_t GetGUID() const { return 1; }
    uint32_t GetEntry() const { return entry; }
    Unit* getVictim() const { return victim; }
    TempSummon* ToTempSummon() const { return summon; }
    bool IsInMap(TempSummon const*) const { return sameMap; }
    bool IsInMap(Player const*) const { return sameMap; }
    bool InSamePhase(Player const*) const { return samePhase; }
    void CastSpell(Position at, uint32_t spell, bool triggered)
    {
        assert(at.x == 1 && at.y == 2 && at.z == 3);
        assert(spell == 242879 && triggered);
        ++casts;
    }
};

struct Scenario
{
    bool complete = false;
    uint32_t id = 1221;
    uint32_t GetScenarioId() const { return id; }
    bool IsCompleted(bool bonus) const { assert(!bonus); return complete; }
};

struct Manager
{
    Scenario* active;
    Scenario* GetScenario(uint32_t instance) const { assert(instance == 12); return active; }
};

Manager* sScenarioMgr;

struct SmartAI
{
    Creature* me;
    virtual void SummonedCreatureDies(Creature*, Unit*) {}
    virtual ~SmartAI() = default;
};

struct AgathaAI : SmartAI
{
#include "AgathaFury.inc"
};

int main()
{
    Player player;
    Unit playerVictim { &player }, petVictim { &player }, guardianVictim { &player }, unowned;
    TempSummon summon;
    Creature agatha, imp;
    imp.summon = &summon;
    imp.alive = false;
    Scenario scenario;
    Manager manager { &scenario };
    sScenarioMgr = &manager;
    AgathaAI ai;
    ai.me = &agatha;

    for (Unit* victim : { &playerVictim, &petVictim, &guardianVictim })
    {
        agatha.victim = victim;
        for (uint32_t entry : { 115640, 115641, 115642, 115643, 115719 })
        {
            imp.entry = entry;
            unsigned before = agatha.casts;
            ai.SummonedCreatureDies(&imp, victim);
            assert(agatha.casts == before + 1);
        }
    }

    auto rejects = [&](auto& field, auto invalid)
    {
        auto previous = field;
        field = invalid;
        ai.SummonedCreatureDies(&imp, &petVictim);
        assert(agatha.casts == 15);
        field = previous;
    };
    rejects(imp.alive, true);
    rejects(imp.entry, 107739u);
    rejects(imp.summon, static_cast<TempSummon*>(nullptr));
    rejects(summon.summoner, 2u);
    rejects(agatha.alive, false);
    rejects(agatha.combat, false);
    rejects(agatha.map, 1u);
    rejects(agatha.sameMap, false);
    rejects(agatha.samePhase, false);
    rejects(agatha.victim, &unowned);
    rejects(manager.active, static_cast<Scenario*>(nullptr));
    rejects(scenario.id, 1122u);
    rejects(scenario.complete, true);
    rejects(player.alive, false);
    rejects(player.present, false);
    rejects(player.teleport, true);
    std::cout << "PASS: actual Agatha callback chooses the player position on each owned imp death; rejects 16 out-of-scope cases.\n";
}
