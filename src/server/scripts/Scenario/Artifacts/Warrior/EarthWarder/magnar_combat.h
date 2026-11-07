#ifndef LEGION_MAGNAR_COMBAT_H
#define LEGION_MAGNAR_COMBAT_H

#include "ObjectGuid.h"
#include <array>
#include <cstdint>
class Creature;
class CreatureAI;
class InstanceMap;
class SummonList;

namespace MagnarCombat
{
    // Instance lifetime, actual original boss full GUID and guard epoch only.
    // Track actual owned reserves for removal; only ten real owned deaths
    // prepare the earned receipt before native46427 can call scripts;
    // restoration additionally requires the observed native combat step4.
    struct State
    {
        ObjectGuid actor;
        unsigned epoch = 0;
        bool earned = false;
        std::array<ObjectGuid, 12> roster{};
        std::array<bool, 12> deaths{};
    };
    // Only native removal of an actually earned duel can publish this ticket.
    // It belongs to this live instance epoch, never a saved scenario step.
    struct Retired
    {
        ObjectGuid actor;
        unsigned epoch = 0;
        std::uint64_t health = 0;
        std::uint64_t maximum = 0;
    };
    void Reset(InstanceMap* map, State& state);
    void Bind(InstanceMap* map, State& state, Creature* actor, unsigned epoch);
    void Remove(InstanceMap* map, State& state, Creature* actor);
    State* Find(Creature* actor, CreatureAI const* expected);
    void Remember(Creature* actor, CreatureAI const* expected,
        std::array<ObjectGuid, 12> const& roster, std::array<bool, 12> const& deaths);
    bool Restore(Creature* actor, CreatureAI const* expected, SummonList& summons,
        std::array<ObjectGuid, 12>& roster, std::array<bool, 12>& deaths);
    bool Rebind(Creature* actor, CreatureAI const* expected, std::uint64_t& health);
    bool NeedsReset(Creature* actor, CreatureAI const* expected);
    void RetirePresentation(Creature* actor);
}
#endif
