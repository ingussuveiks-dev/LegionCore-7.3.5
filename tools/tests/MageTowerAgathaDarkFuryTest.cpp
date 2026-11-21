#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <set>

using uint32 = std::uint32_t;
using int32 = std::int32_t;
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
constexpr unsigned POWER_ENERGY = 3;
constexpr unsigned UNIT_STATE_CASTING = 2;
enum SpellCastResult { SPELL_CAST_OK, SPELL_FAILED_NOT_READY };
enum class AgathaPosition { Center, Upper, Lower };
struct AuraEffect {};

#include "AgathaDarkFuryCharge.inc"

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
    uint32 GetInstanceId() const { return 57; }
    int32 GetPower(unsigned power) const { assert(power == POWER_ENERGY); return energy; }
    void SetPower(unsigned power, int32 amount) { assert(power == POWER_ENERGY); energy = amount; }
    bool HasUnitState(unsigned state) const { assert(state == UNIT_STATE_CASTING); return casting; }
    bool HasAura(uint32 aura, unsigned guid) const { return aura == AgathaDarkFuryChannel && guid == GetGUID() && channelAura; }
    SpellCastResult CastSpell(Unit* target, uint32 spell, bool triggered)
    {
        assert(target == this && spell == AgathaDarkFuryChannel && !triggered && energy == 100);
        ++attempts;
        return SPELL_FAILED_NOT_READY;
    }
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
    int32 energy = 0;
    unsigned attempts = 0;
    bool casting = false;
    bool channelAura = false;
    std::set<std::pair<unsigned, unsigned>> immune;
    std::set<uint32> removed;
};

struct AgathaAI
{
    bool GetArenaPlayer() const { return player; }
    void UpdateDarkFury(uint32 diff);
    void DarkFuryStarted();
    Unit* me = nullptr;
    bool player = true;
    AgathaPosition _position = AgathaPosition::Center;
    AgathaDarkFuryCharge _darkFuryCharge;
    uint32 _darkFuryRetry = 0;
    uint32 _darkFuryAttempts = 0;
    uint32 _pendingTranslocate = 0;
};
unsigned warnings = 0;
void TC_LOG_WARN(char const*, char const*, uint32 instance, uint32 result)
{
    assert(instance == 57 && result == SPELL_FAILED_NOT_READY);
    ++warnings;
}
Unit* expected = nullptr;
AgathaAI encounter;
AgathaAI* GetAgathaAI(Unit* source) { return source && source == expected ? &encounter : nullptr; }

struct spell_mage_tower_agatha_dark_fury_channel
{
    void Start(AuraEffect const*, AuraEffectHandleModes);
    void OnTick(AuraEffect const*);
    void Stop(AuraEffect const*, AuraEffectHandleModes);
    Unit* GetCaster() const { return auraCaster; }
    Unit* GetTarget() const { return auraTarget ? auraTarget : auraCaster; }
    void PreventDefaultAction() { ++preventions; }

    Unit* auraCaster = nullptr;
    Unit* auraTarget = nullptr;
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

    encounter.me = &boss;
    encounter.UpdateDarkFury(49999);
    assert(boss.energy == 99 && boss.attempts == 0);
    boss.casting = true;
    encounter.UpdateDarkFury(1);
    assert(boss.energy == 100 && boss.attempts == 0);
    boss.casting = false;
    encounter._pendingTranslocate = 242987;
    encounter.UpdateDarkFury(1);
    assert(boss.attempts == 0);
    encounter._pendingTranslocate = 0;
    encounter.UpdateDarkFury(1);
    assert(boss.attempts == 1);
    encounter.UpdateDarkFury(499);
    assert(boss.attempts == 1);
    encounter.UpdateDarkFury(1);
    assert(boss.attempts == 2);

    aura.auraTarget = &stranger;
    aura.Start(nullptr, 0);
    assert(encounter._darkFuryAttempts == 2);
    aura.auraTarget = &boss;
    aura.Start(nullptr, 0);
    assert(encounter._darkFuryAttempts == 0);
    encounter.UpdateDarkFury(51099);
    assert(boss.energy == 99 && boss.attempts == 2);
    boss.channelAura = true;
    encounter.UpdateDarkFury(1);
    assert(boss.energy == 100 && boss.attempts == 2);

    encounter._position = AgathaPosition::Upper;
    aura.Start(nullptr, 0);
    boss.channelAura = false;
    encounter.UpdateDarkFury(67999);
    assert(boss.energy == 99 && boss.attempts == 2);
    encounter.UpdateDarkFury(1);
    assert(boss.attempts == 3);
    for (unsigned i = 0; i < 11; ++i)
        encounter.UpdateDarkFury(500);
    assert(warnings == 1 && boss.attempts == 14);

    std::cout << "PASS Dark Fury active channel ticks, interrupt immunity scoped to the shield, and cleanup of shield and stacks.\n";
    std::cout << "PASS Dark Fury energy cost, initial and phase-specific cycles, real aura receipt and bounded retries.\n";
}
