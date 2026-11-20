#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>

using uint64 = uint64_t;
constexpr int UNIT_STATE_CONTROLLED = 1;
constexpr float INVALID_HEIGHT = -100000.0f;
constexpr int SPELL_CAST_OK = 0;

namespace Feltotem
{
constexpr int SummonBileSpitter = 243167;
}

struct Position
{
    float m_positionZ = 626;
    float GetPositionX() const { return 5211; }
    float GetPositionY() const { return 5069; }
    float GetPositionZ() const { return m_positionZ; }
};

std::array<Position, 2> const EggSites{};
constexpr int EggMissile = 242384;

struct Creature
{
    bool controlled = false;
    bool failedFloor = false;
    bool* publication = nullptr;
    bool canPublish = true;
    unsigned eggs = 0;
    unsigned hatchCasts = 0;
    unsigned despawns = 0;
    bool HasUnitState(int) const { return controlled; }
    float GetHeight(float, float, float, bool, float) const { return failedFloor ? INVALID_HEIGHT : 626; }
    unsigned GetInstanceId() const { return 1; }
    Position GetPosition() const { return {}; }
    void DespawnOrUnsummon(int) { ++despawns; }

    int CastSpell(Position, int spell, bool triggered)
    {
        assert(triggered);
        if (spell == EggMissile)
            ++eggs;
        else
        {
            assert(spell == Feltotem::SummonBileSpitter);
            ++hatchCasts;
            if (canPublish)
                *publication = true;
        }
        return SPELL_CAST_OK;
    }
};

unsigned warnings = 0;
#define TC_LOG_WARN(...) ++warnings

struct Tugar
{
    Creature* me;
    uint64 _eggWave = 3, currentAttempt = 3;
    bool _eggInterrupted = false;
    uint64 EggAttempt() const { return currentAttempt; }
#include "FeltotemEggWave.inc"
};

struct Egg
{
    Creature* me;
    bool _published = false, _hatching = false, active = true;
    unsigned _failedHatches = 0;
    bool IsActive() const { return active; }
#include "FeltotemEggHatch.inc"
};

int main()
{
    Creature boss;
    Tugar tugar{ &boss };
    tugar.FinishEggWave(2, true);
    assert(!boss.eggs && tugar._eggWave == 3);
    tugar.FinishEggWave(3, false);
    tugar.FinishEggWave(3, true);
    assert(!boss.eggs && !tugar._eggWave);

    tugar._eggWave = 3;
    tugar._eggInterrupted = true;
    tugar.FinishEggWave(3, true);
    assert(!boss.eggs && !tugar._eggWave);
    tugar._eggInterrupted = false;
    tugar._eggWave = 3;
    boss.controlled = true;
    tugar.FinishEggWave(3, true);
    assert(!boss.eggs && !tugar._eggWave);
    boss.controlled = false;
    tugar._eggWave = 3;
    tugar.FinishEggWave(3, true);
    tugar.FinishEggWave(3, true);
    assert(boss.eggs == 2 && !tugar._eggWave);

    Creature creature;
    Egg egg{ &creature };
    creature.publication = &egg._published;
    egg.active = false;
    egg.Hatch();
    assert(!creature.hatchCasts);
    egg.active = true;
    egg.Hatch();
    egg.Hatch();
    assert(creature.hatchCasts == 1 && egg._published && creature.despawns == 1 && !warnings);

    Creature failedCreature;
    Egg failed{ &failedCreature };
    failedCreature.canPublish = false;
    for (unsigned attempt = 0; attempt < 5; ++attempt)
        failed.Hatch();
    assert(failedCreature.hatchCasts == 3 && !failed._published && failedCreature.despawns == 1 && warnings == 1);
    std::cout << "PASS eggs require uninterrupted expiry, terminate on stale attempts, and hatch once only after real publication.\n";
}
