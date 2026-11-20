#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>

using uint32 = std::uint32_t;
constexpr unsigned CURRENT_GENERIC_SPELL = 1;
constexpr unsigned SPELL_STATE_FINISHED = 3;
constexpr unsigned SPELL_CAST_OK = 0;
constexpr unsigned UNIT_STATE_CASTING = 1;

struct SpellTargetPosition
{
    uint32 target_mapId;
    float target_X, target_Y, target_Z;
};

struct SpellMgr
{
    SpellTargetPosition const* GetSpellTargetPosition(uint32 spell) const
    {
        if (spell == 242987) return &upper;
        if (spell == 242989) return &lower;
        return nullptr;
    }
    SpellTargetPosition upper{1616, -331, 7688, 63.55f}, lower{1616, -367, 7589, 53.247f};
};
SpellMgr manager;
auto* sSpellMgr = &manager;

struct SpellInfo { unsigned Id = 0; };
struct Spell
{
    SpellInfo const* GetSpellInfo() const { return &info; }
    unsigned getState() const { return state; }
    SpellInfo info;
    unsigned state = SPELL_STATE_FINISHED;
};

struct Creature
{
    Spell const* GetCurrentSpell(unsigned slot) const
    {
        assert(slot == CURRENT_GENERIC_SPELL);
        return &cast;
    }
    bool HasUnitState(unsigned state) const
    {
        assert(state == UNIT_STATE_CASTING);
        return cast.state != SPELL_STATE_FINISHED;
    }
    unsigned CastSpell(Creature* target, uint32 id, bool triggered)
    {
        assert(target == this && !triggered);
        ++attempts;
        if (!accept) return 1;
        cast.info.Id = id;
        cast.state = 1;
        return SPELL_CAST_OK;
    }
    uint32 GetMapId() const { return 1616; }
    float GetPositionZ() const { return z; }
    float GetHealthPct() const { return health; }
    float GetExactDist2d(float targetX, float targetY) const { return std::hypot(x - targetX, y - targetY); }
    void EndCast(bool arrives)
    {
        if (arrives)
        {
            auto const* dest = sSpellMgr->GetSpellTargetPosition(cast.info.Id);
            x = dest->target_X;
            y = dest->target_Y;
            z = dest->target_Z;
        }
        cast.state = SPELL_STATE_FINISHED;
    }
    float x = -353.774f, y = 7620.3f, z = 53.2769f, health = 100.0f;
    unsigned attempts = 0;
    bool accept = true;
    Spell cast;
};

#include "AgathaTranslocateChoice.inc"

struct AgathaAI
{
    void TranslocateFailed(uint32 spell);
    void UpdateTranslocate(uint32 diff);
    Creature* me = nullptr;
    AgathaPosition _position = AgathaPosition::Center;
    uint32 _pendingTranslocate = 0, _nextTranslocate = 0, _retry = 0, _failures = 0;
    bool _reachedLower = false;
};

unsigned errors = 0;
#define TC_LOG_ERROR(category, message, spell) do { assert((spell) == 242987 || (spell) == 242989); ++errors; } while (false)
#include "AgathaTranslocateUpdate.inc"

int main()
{
    Creature agatha;
    AgathaAI ai{&agatha};
    agatha.health = 50.1f;
    ai.UpdateTranslocate(90000);
    assert(agatha.attempts == 0);
    agatha.health = 50.0f;
    agatha.cast.state = 1;
    ai.UpdateTranslocate(1);
    assert(agatha.attempts == 0);
    agatha.cast.state = SPELL_STATE_FINISHED;
    ai.UpdateTranslocate(1);
    assert(ai._pendingTranslocate == 242987 && ai._position == AgathaPosition::Center && agatha.attempts == 1);
    ai.UpdateTranslocate(2200);
    assert(ai._pendingTranslocate == 242987 && agatha.attempts == 1);
    agatha.EndCast(false);
    ai.UpdateTranslocate(1);
    assert(ai._pendingTranslocate == 0 && ai._position == AgathaPosition::Center && ai._failures == 1);
    ai.UpdateTranslocate(1499);
    assert(agatha.attempts == 1);
    ai.UpdateTranslocate(1);
    agatha.EndCast(true);
    ai.UpdateTranslocate(1);
    assert(ai._position == AgathaPosition::Upper && ai._nextTranslocate == 68000 && !ai._reachedLower && ai._failures == 0);

    agatha.health = 36;
    ai.UpdateTranslocate(10000);
    assert(agatha.attempts == 2);
    agatha.health = 35;
    ai.UpdateTranslocate(1);
    assert(ai._pendingTranslocate == 242989 && agatha.attempts == 3);
    agatha.EndCast(true);
    ai.UpdateTranslocate(1);
    assert(ai._position == AgathaPosition::Lower && ai._reachedLower);
    ai.UpdateTranslocate(67999);
    assert(agatha.attempts == 3);
    ai.UpdateTranslocate(1);
    assert(ai._pendingTranslocate == 242987 && agatha.attempts == 4);
    agatha.EndCast(true);
    ai.UpdateTranslocate(1);
    ai.UpdateTranslocate(50000);
    assert(ai._position == AgathaPosition::Upper && ai._reachedLower && agatha.attempts == 4);

    agatha.accept = false;
    ai.UpdateTranslocate(20000);
    ai.UpdateTranslocate(1500);
    ai.UpdateTranslocate(1500);
    assert(errors == 1 && ai._retry == 6000 && ai._position == AgathaPosition::Upper);
    ai.UpdateTranslocate(5999);
    assert(agatha.attempts == 7);
    agatha.accept = true;
    ai.UpdateTranslocate(1);
    assert(ai._pendingTranslocate == 242989 && agatha.attempts == 8);
    std::cout << "PASS retained combat casts, native Translocate arrival, 50/35 thresholds, timed alternation and bounded retry cadence.\n";
}
