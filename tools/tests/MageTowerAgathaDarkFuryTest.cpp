#include <cassert>
#include <cstdint>
#include <iostream>
#include <set>

using uint32 = std::uint32_t;
using AuraEffectHandleModes = unsigned;
constexpr uint32 AgathaDarkFuryDamage = 243111;
constexpr uint32 AgathaDarkFuryShield = 243113;
constexpr uint32 AgathaDarkFuryChannel = 243114;
constexpr unsigned CURRENT_CHANNELED_SPELL = 2;
constexpr unsigned SPELL_STATE_CASTING = 2;
constexpr unsigned IMMUNITY_EFFECT = 0;
constexpr unsigned IMMUNITY_MECHANIC = 1;
constexpr unsigned SPELL_EFFECT_INTERRUPT_CAST = 68;
constexpr unsigned MECHANIC_INTERRUPT = 26;
struct AuraEffect {};

struct SpellInfo { uint32 Id = AgathaDarkFuryChannel; };
struct Spell
{
    unsigned getState() const { return state; }
    SpellInfo const* GetSpellInfo() const { return &info; }
    unsigned state = SPELL_STATE_CASTING;
    SpellInfo info;
};

struct Unit
{
    Spell const* GetCurrentSpell(unsigned slot) const
    {
        assert(slot == CURRENT_CHANNELED_SPELL);
        return channel;
    }
    unsigned GetGUID() const { return 42; }
    void ApplySpellImmune(uint32 source, unsigned type, unsigned value, bool apply)
    {
        assert(source == AgathaDarkFuryShield);
        auto const pair = std::make_pair(type, value);
        if (apply)
            immune.insert(pair);
        else
            immune.erase(pair);
    }
    void RemoveAurasDueToSpell(uint32 spell, unsigned source)
    {
        assert(source == GetGUID());
        removed.insert(spell);
    }
    Spell* channel = nullptr;
    std::set<std::pair<unsigned, unsigned>> immune;
    std::set<uint32> removed;
};

struct AgathaAI { bool GetArenaPlayer() const { return player; } bool player = true; };
Unit* expected = nullptr;
AgathaAI encounter;
AgathaAI* GetAgathaAI(Unit* source) { return source && source == expected ? &encounter : nullptr; }

struct spell_mage_tower_agatha_dark_fury_channel
{
    void OnTick(AuraEffect const*);
    void Stop(AuraEffect const*, AuraEffectHandleModes);
    Unit* GetCaster() const { return auraCaster; }
    Unit* GetTarget() const { return auraCaster; }
    void PreventDefaultAction() { ++preventions; }

    Unit* auraCaster = nullptr;
    unsigned preventions = 0;
};

struct spell_mage_tower_agatha_dark_fury_shield
{
    void SetInterruptImmunity(bool apply);
    Unit* GetTarget() const { return shieldOwner; }
    Unit* shieldOwner = nullptr;
};

#include "AgathaDarkFury.inc"

int main()
{
    Unit boss, stranger;
    expected = &boss;
    Spell channel;
    boss.channel = &channel;
    spell_mage_tower_agatha_dark_fury_channel aura;
    aura.auraCaster = &boss;
    aura.OnTick(nullptr);
    assert(aura.preventions == 0);

    encounter.player = false;
    aura.OnTick(nullptr);
    assert(aura.preventions == 1);
    encounter.player = true;
    ++channel.state;
    aura.OnTick(nullptr);
    assert(aura.preventions == 2);
    --channel.state;
    channel.info.Id = AgathaDarkFuryDamage;
    aura.OnTick(nullptr);
    assert(aura.preventions == 3);
    aura.auraCaster = &stranger;
    aura.OnTick(nullptr);
    assert(aura.preventions == 4);
    aura.auraCaster = &boss;
    channel.info.Id = AgathaDarkFuryChannel;

    spell_mage_tower_agatha_dark_fury_shield shield{&boss};
    shield.SetInterruptImmunity(true);
    assert(boss.immune.count({IMMUNITY_EFFECT, SPELL_EFFECT_INTERRUPT_CAST}));
    assert(boss.immune.count({IMMUNITY_MECHANIC, MECHANIC_INTERRUPT}));
    shield.SetInterruptImmunity(false);
    assert(boss.immune.empty());
    aura.OnTick(nullptr);
    assert(aura.preventions == 4);
    aura.Stop(nullptr, 0);
    assert(boss.removed.size() == 2 && boss.removed.count(AgathaDarkFuryShield) && boss.removed.count(AgathaDarkFuryDamage));
    std::cout << "PASS Dark Fury active channel ticks, interrupt immunity scoped to the shield, and cleanup of shield and stacks.\n";
}
