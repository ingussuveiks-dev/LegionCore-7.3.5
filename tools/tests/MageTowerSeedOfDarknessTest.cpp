#include <cassert>
#include <cstdint>
#include <iostream>

using uint32 = std::uint32_t;
constexpr uint32 SummonDarknessWithin = 233165;
constexpr unsigned AURA_REMOVE_BY_EXPIRE = 1;
using AuraEffectHandleModes = unsigned;
struct AuraEffect {};
struct CorruptingShadowsAI;
struct Player;
struct Unit
{
    bool IsPlayer() const { return player != nullptr; }
    Player* ToPlayer() const { return player; }
    Player* player = nullptr;
};

struct Player : Unit
{
    Player() { player = this; }
    bool HasPendingTeleport() const { return transferring; }
    void CastSpell(Player* target, uint32 spell, bool triggered, void*, void*, unsigned originalCaster);
    CorruptingShadowsAI* boss = nullptr;
    unsigned casts = 0;
    bool transferring = false, publish = true;
};

struct Creature : Unit
{
    bool IsAlive() const { return alive; }
    bool IsWithinDistInMap(Creature const*, float radius) const { return distance <= radius && inMap; }
    bool IsWithinLOSInMap(Creature const*) const { return inLOS; }
    unsigned GetGUID() const { return 42; }
    float distance = 1.0f;
    bool alive = true, inLOS = true, inMap = true;
};

struct CorruptingShadowsAI
{
    Player* GetSeedPlayer() const { return eligible; }
    void ReleaseSeeds(Player& player);
    Creature* me = nullptr;
    Player* eligible = nullptr;
    uint32 _seedPublished = 0;
};

void Player::CastSpell(Player* target, uint32 spell, bool triggered, void*, void*, unsigned originalCaster)
{
    assert(target == this && spell == SummonDarknessWithin && triggered && originalCaster == 42);
    ++casts;
    if (publish)
        ++boss->_seedPublished;
}

unsigned logs = 0;
#define TC_LOG_ERROR(category, message, count) do { assert((count) < 3); ++logs; } while (false)

namespace ClosingTheEye
{
    CorruptingShadowsAI* active = nullptr;
    CorruptingShadowsAI* GetCorruptingShadowsAI(Unit* source)
    {
        return active && source == active->me ? active : nullptr;
    }
}

struct AuraApplication
{
    unsigned GetRemoveMode() const { return mode; }
    unsigned mode = AURA_REMOVE_BY_EXPIRE;
};

struct spell_mage_tower_seed_expiry
{
    void ReleaseSeeds(AuraEffect const*, AuraEffectHandleModes);
    AuraApplication* GetTargetApplication() { return &application; }
    Unit* GetTarget() const { return auraTarget; }
    Unit* GetCaster() const { return caster; }
    AuraApplication application;
    Unit* caster = nullptr;
    Unit* auraTarget = nullptr;
    bool _released = false;
};

#include "SeedOfDarkness.inc"

int main()
{
    Creature source, seed;
    Player owner, stranger;
    auto ai = CorruptingShadowsAI{&source, &owner, 0};
    owner.boss = &ai;
    stranger.boss = &ai;
    ClosingTheEye::active = &ai;

    spell_mage_tower_seed_expiry aura;
    aura.caster = &source;
    aura.auraTarget = &owner;
    aura.application.mode = 0;
    aura.ReleaseSeeds(nullptr, 0);
    assert(owner.casts == 0);
    aura.application.mode = AURA_REMOVE_BY_EXPIRE;
    aura.ReleaseSeeds(nullptr, 0);
    aura.ReleaseSeeds(nullptr, 0);
    assert(owner.casts == 3 && ai._seedPublished == 3 && logs == 0);

    aura._released = false;
    aura.auraTarget = &stranger;
    aura.ReleaseSeeds(nullptr, 0);
    assert(stranger.casts == 0);
    aura._released = false;
    aura.auraTarget = &owner;
    owner.transferring = true;
    aura.ReleaseSeeds(nullptr, 0);
    assert(owner.casts == 3);
    owner.transferring = false;
    owner.publish = false;
    ai.ReleaseSeeds(owner);
    assert(owner.casts == 6 && logs == 1);
    ai.eligible = nullptr;
    ai.ReleaseSeeds(owner);
    assert(owner.casts == 6);

    assert(CanMergeDarkness(seed, source));
    seed.distance = 3.01f;
    assert(!CanMergeDarkness(seed, source));
    seed.distance = 3.0f;
    seed.inLOS = false;
    assert(!CanMergeDarkness(seed, source));
    seed.inLOS = true;
    seed.inMap = false;
    assert(!CanMergeDarkness(seed, source));
    seed.inMap = true;
    seed.alive = false;
    assert(!CanMergeDarkness(seed, source));
    std::cout << "PASS Seed expiry, exactly-once release, owner and transfer guards, original caster, failed publication and merge contact.\n";
}
