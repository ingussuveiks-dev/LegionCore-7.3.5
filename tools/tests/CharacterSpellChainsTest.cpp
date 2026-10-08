#include <cstdint>
#include <iostream>
#include <vector>

using uint32 = uint32_t;
enum SpellEffIndex { EFFECT_0 };
constexpr uint32 SPELL_DK_DEATH_COIL_DAMAGE = 47632;
constexpr uint32 SPELL_DK_UNHOLY_VIGOR = 196263;
constexpr uint32 SPELL_DK_UNHOLY = 137007;
struct Player;
struct Unit;
struct Cast { Unit* target; uint32 spell; };
struct Unit
{
    bool unholy = false;
    std::vector<Cast> casts;
    virtual Player* ToPlayer() { return nullptr; }
    bool HasAura(uint32 id) const { return id == SPELL_DK_UNHOLY && unholy; }
    void CastSpell(Unit* target, uint32 spell, bool) { casts.push_back({target, spell}); }
};
struct Pet : Unit {};
struct Player : Unit
{
    Pet* pet = nullptr;
    Player* ToPlayer() override { return this; }
    Pet* GetPet() { return pet; }
};
struct Script
{
    Unit* testCaster = nullptr;
    Unit* testTarget = nullptr;
    bool prevented = false;
    Unit* GetCaster() { return testCaster; }
    Unit* GetHitUnit() { return testTarget; }
    void PreventHitDefaultEffect(SpellEffIndex) { prevented = true; }
#include "DeathCoilDummy.inc"
};

// Spell::HandleEffects calls script handlers before the default EffectDummy.
// The installed spell_dummy_trigger row casts 47632 at HIT_TARGET when the
// script leaves the default enabled. Keep this fallback in the fixture so
// the pre-fix production handler produces two damage casts and fails.
void Dispatch(Script& script)
{
    script.HandleDummy(EFFECT_0);
    if (!script.prevented && script.testCaster && script.testTarget)
        script.testCaster->CastSpell(script.testTarget, SPELL_DK_DEATH_COIL_DAMAGE, true);
}
#define CHECK(expression) do { if (!(expression)) { std::cerr << "Failed: " #expression "\n"; return 1; } } while (false)

int main()
{
    Unit target;
    Player player;
    Pet pet;
    player.unholy = true;
    player.pet = &pet;
    Script withPet{&player, &target};
    Dispatch(withPet);
    CHECK(withPet.prevented);
    CHECK(player.casts.size() == 2);
    CHECK(player.casts[0].spell == SPELL_DK_DEATH_COIL_DAMAGE && player.casts[0].target == &target);
    CHECK(player.casts[1].spell == SPELL_DK_UNHOLY_VIGOR && player.casts[1].target == &pet);

    player.casts.clear();
    player.pet = nullptr;
    Script noPet{&player, &target};
    Dispatch(noPet);
    CHECK(player.casts.size() == 1 && player.casts[0].spell == SPELL_DK_DEATH_COIL_DAMAGE);

    player.casts.clear();
    player.pet = &pet;
    player.unholy = false;
    Script otherSpec{&player, &target};
    Dispatch(otherSpec);
    CHECK(player.casts.size() == 1);

    Unit npc;
    npc.unholy = true;
    Script nonPlayer{&npc, &target};
    Dispatch(nonPlayer);
    CHECK(npc.casts.size() == 1);

    player.casts.clear();
    Script noTarget{&player, nullptr};
    Dispatch(noTarget);
    CHECK(player.casts.empty());
    Script noCaster{nullptr, &target};
    Dispatch(noCaster);
    std::cout << "Death Coil: single damage cast, pet energy, no-pet, other-spec and null-target cases passed\n";
}
