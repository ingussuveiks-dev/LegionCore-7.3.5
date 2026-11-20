#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <list>

using uint32 = std::uint32_t;
using int32 = std::int32_t;
using ObjectGuid = unsigned;
constexpr unsigned CURRENT_GENERIC_SPELL = 1;
constexpr unsigned SPELL_STATE_FINISHED = 4;

struct SpellInfo { uint32 Id = 234728; };
struct Spell
{
    SpellInfo const* GetSpellInfo() const { return &info; }
    unsigned getState() const { return state; }
    SpellInfo info;
    unsigned state = 2;
};

struct Object
{
    float GetPositionX() const { return x; }
    float GetPositionY() const { return y; }
    float GetPositionZ() const { return z; }
    float GetExactDist2d(float otherX, float otherY) const { return std::hypot(x - otherX, y - otherY); }
    bool IsInMap(Object const* other) const { return other->map; }
    bool InSamePhase(Object const* other) const { return other->phase; }

    float x = 0, y = 0, z = 0;
    bool map = true, phase = true;
};

struct Player;
struct Unit : Object
{
    Player* GetCharmerOrOwnerPlayerOrPlayerItself() const { return owner; }
    ObjectGuid GetGUID() const { return guid; }
    bool IsAlive() const { return alive; }

    Player* owner = nullptr;
    unsigned guid = 1;
    bool alive = true;
};

struct AreaTrigger : Object
{
    int32 GetDuration() const { return duration; }
    unsigned GetRealEntry() const { return entry; }
    unsigned GetCasterGUID() const { return caster; }

    int32 duration = 10000;
    unsigned entry = 9255, caster = 1;
};

struct Aura
{
    int32 GetDuration() const { return duration; }
    void SetDuration(int32 value) { duration = value; }
    int32 duration = 2000;
};

struct Player : Unit
{
    Player() { owner = this; }
    bool HasPendingTeleport() const { return transfer; }
    void GetAreaObjectList(std::list<AreaTrigger*>& result, uint32 spell)
    {
        assert(spell == 132950);
        result = flares;
    }
    Aura* GetAura(uint32 spell, ObjectGuid caster)
    {
        assert(spell == 231980 && caster == 42);
        return hasShield ? &shield : nullptr;
    }
    void RemoveAurasDueToSpell(uint32 spell, ObjectGuid caster)
    {
        assert(caster == 42);
        if (spell == 231980)
            hasShield = false;
        else
            assert(spell == 232007);
    }

    bool transfer = false, hasShield = false;
    Aura shield;
    std::list<AreaTrigger*> flares;
};

struct Creature : Unit
{
    Creature() { guid = 42; }
    Spell const* GetCurrentSpell(unsigned slot) const
    {
        assert(slot == CURRENT_GENERIC_SPELL);
        return cast;
    }
    Unit* getVictim() const { return victim; }
    float GetHealthPct() const { return healthPercent; }
    void GetAreaObjectList(std::list<AreaTrigger*>& result, uint32 spell)
    {
        assert(spell == 231982);
        result = barriers;
    }
    void CastSpell(Creature*, uint32 spell, bool triggered)
    {
        assert(spell == 242394 && triggered);
        ++frostAnnouncements;
    }
    void AddAura(uint32 spell, Player* player, void*, unsigned, int32 duration, int32 maxDuration)
    {
        assert(spell == 231980 && duration == 2000 && maxDuration == 2000);
        player->hasShield = true;
        player->shield.duration = duration;
    }

    std::list<AreaTrigger*> barriers;
    Spell* cast = nullptr;
    Unit* victim = nullptr;
    unsigned frostAnnouncements = 0;
    float healthPercent = 100.0f;
};

struct ObjectAccessor
{
    static Player* GetPlayer(Creature const&, ObjectGuid guid)
    {
        return current && guid == current->guid ? current : nullptr;
    }
    static Player* current;
};
Player* ObjectAccessor::current = nullptr;

struct PlayerGuid
{
    bool IsEmpty() const { return !value; }
    operator ObjectGuid() const { return value; }
    PlayerGuid& operator=(ObjectGuid guid) { value = guid; return *this; }
    ObjectGuid value = 0;
};

struct SmartAI
{
    void EnterEvadeMode() { ++evades; }
    unsigned evades = 0;
};

struct XylemAI : SmartAI
{
    enum class Phase { FirstFrost, Arcane, Intermission, Frost };
    explicit XylemAI(Creature& boss) : me(&boss) {}

    bool IsInIntermission() const { return _phase == Phase::Intermission; }
    bool IsEncounterActive() const { return active; }
    Player* GetAttemptPlayer() const
    {
        Player* player = ObjectAccessor::GetPlayer(*me, _player);
        return player && player->IsAlive() && player->phase && player->map ? player : nullptr;
    }
    bool CanHitInIntermission(Unit* target) const;
    bool IsCastingAnnihilation() const;
    bool IsProtected(Unit const& target);
    bool CanReveal(Player& player);
    void UpdateEncounter(uint32 diff);

    void StartIntermission(Player&)
    {
        ++starts;
        _phase = Phase::Intermission;
    }
    void ClearIntermission(Player*) { ++clears; }
    void ResumeFight(Player&)
    {
        ++resumes;
        _phase = Phase::Arcane;
    }

    Creature* me;
    PlayerGuid _player;
    Phase _phase = Phase::FirstFrost;
    uint32 _phaseRemaining = 90000;
    unsigned starts = 0, clears = 0, resumes = 0;
    bool active = true;
};

#include "XylemIntermissions.inc"

int main()
{
    Creature boss;
    Player player;
    Unit pet, unrelated;
    pet.owner = &player;
    ObjectAccessor::current = &player;
    XylemAI ai(boss);

    ai.UpdateEncounter(30000);
    assert(ai.starts == 0 && ai._phaseRemaining == 90000);
    boss.victim = &pet;
    ai.UpdateEncounter(89999);
    assert(ai.starts == 0 && ai._phaseRemaining == 1 && boss.frostAnnouncements == 1);
    boss.healthPercent = 10.5f;
    ai.UpdateEncounter(1);
    assert(ai.starts == 0);
    boss.healthPercent = 40.0f;
    ai.UpdateEncounter(1);
    assert(ai.starts == 1 && ai.IsInIntermission());

    Spell cast;
    boss.cast = &cast;
    assert(ai.IsCastingAnnihilation());
    cast.state = SPELL_STATE_FINISHED;
    assert(!ai.IsCastingAnnihilation());
    cast.state = 2;
    assert(ai.CanHitInIntermission(&player) && ai.CanHitInIntermission(&pet));
    assert(!ai.CanHitInIntermission(&unrelated) && !ai.CanHitInIntermission(nullptr));
    pet.map = false;
    assert(!ai.CanHitInIntermission(&pet));
    pet.map = true;
    pet.phase = false;
    assert(!ai.CanHitInIntermission(&pet));

    AreaTrigger barrier;
    boss.barriers = { &barrier };
    player.x = 8.0f;
    assert(ai.IsProtected(player));
    player.x = 8.01f;
    assert(!ai.IsProtected(player));
    player.x = 4.3f;
    barrier.duration = 5000;
    assert(ai.IsProtected(player));
    player.x = 4.5f;
    assert(!ai.IsProtected(player));
    player.x = 6.0f;
    barrier.duration = 10000;
    ai.UpdateEncounter(1);
    assert(player.hasShield && ai.resumes == 0);
    player.shield.duration = 400;
    ai.UpdateEncounter(1);
    assert(player.shield.duration == 2000);
    barrier.duration = 0;
    ai.UpdateEncounter(1);
    assert(!player.hasShield);
    barrier.duration = 10000;
    player.z = 4.5f;
    assert(!ai.IsProtected(player));

    player.x = 4.0f;
    player.z = 0;
    assert(ai.CanReveal(player));
    player.transfer = true;
    assert(!ai.CanReveal(player));
    player.transfer = false;
    player.x = 25.0f;
    assert(!ai.CanReveal(player));
    AreaTrigger flare;
    flare.entry = 510;
    flare.x = 10.0f;
    player.flares = { &flare };
    assert(ai.CanReveal(player));
    flare.caster = 9;
    assert(!ai.CanReveal(player));
    flare.caster = 1;
    flare.phase = false;
    assert(!ai.CanReveal(player));
    flare.phase = true;
    flare.duration = -1;
    assert(!ai.CanReveal(player));
    flare.duration = 20000;
    flare.x = 10.01f;
    assert(!ai.CanReveal(player));
    flare.x = 10.0f;
    ai.UpdateEncounter(1);
    assert(ai.resumes == 1 && !ai.IsInIntermission());

    ai._phase = XylemAI::Phase::Intermission;
    boss.cast = nullptr;
    ai.UpdateEncounter(1);
    assert(ai.evades == 1 && ai.clears == 1 && ai.resumes == 1);
    boss.cast = &cast;
    player.alive = false;
    ai.UpdateEncounter(1);
    assert(ai.evades == 2 && ai.clears == 2 && ai.resumes == 1);
    ai.active = false;
    ai.UpdateEncounter(1);
    assert(ai.clears == 3 && !ai.IsInIntermission());
    std::cout << "PASS Xylem timer, surrender guard, owner and pet scope, shrinking shield, real proximity and Flare, and cast/attempt cleanup.\n";
}
