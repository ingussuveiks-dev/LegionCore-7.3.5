#ifndef LEGION_VANTUS_RUNE_LIFECYCLE_H
#define LEGION_VANTUS_RUNE_LIFECYCLE_H

#include "InstanceScript.h"
#include "Unit.h"
#include <array>

namespace LegionVantus
{
struct Encounter
{
    uint32 Rune;
    uint32 Effect;
    uint32 Boss;
};

template<std::size_t N>
Encounter const* Find(uint32 rune, std::array<Encounter, N> const& entries)
{
    for (auto const& entry : entries)
        if (entry.Rune == rune)
            return &entry;
    return nullptr;
}

template<std::size_t N>
void Cleanup(Unit* owner, uint32 rune, std::array<Encounter, N> const& entries)
{
    if (owner)
        if (auto entry = Find(rune, entries))
            owner->RemoveAurasDueToSpell(entry->Effect);
}

template<std::size_t N>
void Update(Unit* owner, uint32 rune, uint32 mapId, uint32 diff, uint32& timer,
    std::array<Encounter, N> const& entries)
{
    if (timer > diff)
    {
        timer -= diff;
        return;
    }
    timer = 1000;
    auto entry = Find(rune, entries);
    if (!owner || !entry)
        return;

    InstanceScript* instance = owner->IsInWorld() && !owner->IsPreDelete() && owner->GetMapId() == mapId
        ? owner->GetInstanceScript() : nullptr;
    if (instance && instance->GetBossState(entry->Boss) == IN_PROGRESS)
    {
        if (!owner->HasAura(entry->Effect))
            owner->CastSpell(owner, entry->Effect, false);
    }
    else
        owner->RemoveAurasDueToSpell(entry->Effect);
}
}
#endif
