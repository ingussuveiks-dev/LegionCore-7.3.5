#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <vector>

using uint32 = std::uint32_t;
using int32 = std::int32_t;
using ObjectGuid = uint32;
using AuraEffectHandleModes = unsigned;
constexpr uint32 AgathaDarkFuryDamage = 243111;
constexpr uint32 AgathaDarkFuryShield = 243113;
constexpr uint32 AgathaDarkFuryChannel = 243114;
constexpr uint32 AgathaShadowShield = 243027;
constexpr uint32 AgathaSummonSmoldering = 230163;
constexpr uint32 AgathaSummonUmbral = 230165;
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
struct Position {};
uint32 waveRoll = 1;
uint32 urand(uint32 low, uint32 high) { assert(waveRoll >= low && waveRoll <= high); return waveRoll; }

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
    unsigned GetGUID() const { return guid; }
    uint32 GetInstanceId() const { return 57; }
    int32 GetPower(unsigned power) const { assert(power == POWER_ENERGY); return energy; }
    void SetPower(unsigned power, int32 amount) { assert(power == POWER_ENERGY); energy = amount; }
    bool HasUnitState(unsigned state) const { assert(state == UNIT_STATE_CASTING); return casting; }
    bool HasAura(uint32 aura, unsigned source) const
    {
        return (aura == AgathaDarkFuryChannel && source == GetGUID() && channelAura) ||
            (aura == AgathaShadowShield && umbralSources.count(source));
    }
    bool IsAlive() const { return alive; }
    void DespawnOrUnsummon() { despawned = true; }
    Position GetPosition() const { return {}; }
    void MovePositionToFirstCollision(Position&, float distance, float) const { assert(distance == 3.0f); }
    SpellCastResult CastSpell(Position const&, uint32 spell, bool triggered)
    {
        assert(triggered && (spell == AgathaSummonUmbral || spell == AgathaSummonSmoldering));
        waveCasts.push_back(spell);
        return SPELL_CAST_OK;
    }
    SpellCastResult CastSpell(Unit* target, uint32 spell, bool triggered)
    {
        if (spell == AgathaShadowShield)
        {
            assert(alive && triggered && target != this);
            target->umbralSources.insert(GetGUID());
            return SPELL_CAST_OK;
        }
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
        if (spell == AgathaShadowShield)
        {
            umbralSources.erase(source);
            return;
        }
        assert(source == GetGUID());
        removed.insert(spell);
    }
    Spell* channel = nullptr;
    int32 energy = 0;
    unsigned attempts = 0;
    bool casting = false;
    bool channelAura = false;
    unsigned guid = 42;
    bool alive = true;
    bool despawned = false;
    std::set<ObjectGuid> umbralSources;
    std::vector<uint32> waveCasts;
    std::set<std::pair<unsigned, unsigned>> immune;
    std::set<uint32> removed;
};

using Creature = Unit;
struct ObjectAccessor
{
    static Creature* GetCreature(Unit const&, ObjectGuid guid)
    {
        auto const it = summons.find(guid);
        return it == summons.end() ? nullptr : it->second;
    }
    static std::map<ObjectGuid, Creature*> summons;
};
std::map<ObjectGuid, Creature*> ObjectAccessor::summons;

struct AgathaAI
{
    bool GetArenaPlayer() const { return player; }
    void UpdateDarkFury(uint32 diff);
    void DarkFuryStarted();
    void UpdateMixedImpWave(uint32 diff);
    void RefreshUmbralShield();
    void RetireUmbral(Creature const& imp);
    void ClearUmbrals();
    Unit* me = nullptr;
    bool player = true;
    AgathaPosition _position = AgathaPosition::Center;
    AgathaDarkFuryCharge _darkFuryCharge;
    uint32 _darkFuryRetry = 0;
    uint32 _darkFuryAttempts = 0;
    uint32 _pendingTranslocate = 0;
    bool _mixedWavesStarted = false;
    uint32 _nextMixedWave = 0;
    std::set<ObjectGuid> _umbrals;
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

    assert(encounter._mixedWavesStarted && encounter._nextMixedWave == 12000);
    encounter._position = AgathaPosition::Center;
    encounter.UpdateMixedImpWave(11999);
    assert(boss.waveCasts.empty());
    encounter.UpdateMixedImpWave(1);
    assert((boss.waveCasts == std::vector<uint32>{230165,230165,230165}));
    boss.waveCasts.clear();
    encounter._position = AgathaPosition::Upper;
    encounter.UpdateMixedImpWave(54999);
    assert(boss.waveCasts.empty());
    encounter.UpdateMixedImpWave(1);
    assert((boss.waveCasts == std::vector<uint32>{230165,230163,230163}));
    boss.waveCasts.clear();
    encounter._position = AgathaPosition::Lower;
    waveRoll = 2;
    encounter.UpdateMixedImpWave(55000);
    assert((boss.waveCasts == std::vector<uint32>{230165,230165,230163}));

    Creature umbral1, umbral2, umbral3, foreign;
    umbral1.guid = 100;
    umbral2.guid = 200;
    umbral3.guid = 300;
    foreign.guid = 900;
    ObjectAccessor::summons = {{100,&umbral1},{200,&umbral2},{300,&umbral3},{900,&foreign}};
    encounter._umbrals = {100,200,300};
    encounter.RefreshUmbralShield();
    assert(boss.umbralSources == std::set<ObjectGuid>{100});
    umbral1.alive = false;
    encounter.RetireUmbral(umbral1);
    assert(boss.umbralSources == std::set<ObjectGuid>{200});
    encounter.RetireUmbral(umbral1);
    encounter.RetireUmbral(foreign);
    assert(boss.umbralSources == std::set<ObjectGuid>{200});
    encounter.RetireUmbral(umbral3);
    assert(boss.umbralSources == std::set<ObjectGuid>{200});
    encounter.RetireUmbral(umbral2);
    assert(boss.umbralSources.empty());
    encounter._umbrals = {200};
    boss.umbralSources = {200,900};
    encounter.ClearUmbrals();
    assert(boss.umbralSources == std::set<ObjectGuid>{900} && umbral2.despawned && !foreign.despawned);

    std::cout << "PASS Dark Fury active channel ticks, interrupt immunity scoped to the shield, and cleanup of shield and stacks.\n";
    std::cout << "PASS Dark Fury energy cost, initial and phase-specific cycles, real aura receipt and bounded retries.\n";
    std::cout << "PASS three original or mixed imps after native Dark Fury, shield transfer on death and owned reset cleanup.\n";
}
