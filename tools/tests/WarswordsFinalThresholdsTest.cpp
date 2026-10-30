#include <algorithm>
#include <limits>
#include <functional>
#include "Harness.inc"
struct FinalBoss : CreatureAI
{
    Creature* me;
    std::vector<uint32> texts;
    std::function<void(uint32)> onTalk;
    explicit FinalBoss(Creature* actor) : me(actor) { introDone = false; }
#include "Fields.inc"
    void Talk(uint32 identity) { texts.push_back(identity); if (onTalk) onTalk(identity); }
    void RemoveIntroConversation() { }
#include "Reset.inc"
#include "EncounterReady.inc"
#include "DamageTaken.inc"
#include "AfterScaling.inc"
};
#include "Deal.inc"
struct Fixture
{
    Creature actor;
    FinalBoss ai{&actor};
    Fixture() { scenarioManager = {}; scenarioManager.scenario.step = 5; actor.entry = 98602; actor.active = &ai; ai.Reset(); ai.introDone = true; }
};
int main()
{
#ifdef BASELINE
    {
        Fixture fixture; Player player;
        assert(player.Deal(&fixture.actor, 200000) >= 1000 && !fixture.actor.alive && fixture.ai.texts.empty() && fixture.actor.motion.points.empty());
    }
    {
        Fixture fixture; Player player; fixture.actor.health = 10000;
        fixture.ai.onTalk = [&](uint32 identity) { if (identity == 0 && fixture.ai.texts.size() == 1) { uint32 damage = 1; fixture.ai.DamageTaken(&player, damage, 0); } };
        uint32 damage = 1; fixture.ai.DamageTaken(&player, damage, 0);
        assert(fixture.ai.texts == (std::vector<uint32>{0, 0}));
    }
    std::cout << "BASELINE: pinned final full-health scaled lethal hit omits15/7 lines/movement; reentrant pre-hit speech duplicates before phase claim. Controlled services, not client combat.\n";
#elif defined(ADMISSION_BASELINE)
    Fixture fixture; Player player;
    scenarioManager.scenario.step = 0;
    assert(player.Deal(&fixture.actor, 200000) > 0 && !fixture.actor.alive && fixture.ai.texts.empty());
    std::cout << "BASELINE: pinned14774c59 actual post-scaled final handler permits stage0 lethal damage; no dialogue request is not damage admission. Controlled native scaling/death decision, not live client.\n";
#elif defined(COMMUNION_BASELINE)
    for (bool pending : {false, true})
    {
        Fixture fixture; Player player; fixture.ai.introDone = pending; fixture.ai.introPending = pending;
        assert(player.Deal(&fixture.actor, 200000) > 0 && !fixture.actor.alive);
    }
    std::cout << "BASELINE: pinneda617a973 scaled lethal damage kills final Vigfus before/during communion at stage5. Controlled native scaling/current-suspended dispatch, not live client.\n";
#else
    uint32 cases = 0;
    for (bool alliance : {false, true}) for (uint32 kind = 0; kind < 4; ++kind)
        for (bool suspended : {false, true}) for (uint64 targetMaximum : {100ull, 1000ull, 100000ull, 1000000ull})
            for (uint32 supplied : {1u, 149u, 150u, 849u, 850u, 851u, 930u, 931u, 100000u, 200000u})
    {
        Fixture fixture; Player player; player.alliance = alliance;
        Unit pet, guardian, charmed; pet.owner = guardian.owner = &player; charmed.charmer = &player;
        Unit* attacker = std::vector<Unit*>{&player, &pet, &guardian, &charmed}[kind];
        fixture.actor.stat.healthMax = targetMaximum;
        CreatureAI temporary;
        fixture.actor.active = suspended ? &temporary : &fixture.ai;
        fixture.actor.i_disabledAI = suspended ? &fixture.ai : nullptr;
        uint32 finalDamage = uint32(supplied / fixture.actor.GetHealthMultiplierForTarget(attacker));
        uint64 remaining = finalDamage >= targetMaximum ? 0 : targetMaximum - finalDamage;
        bool first = finalDamage && remaining < targetMaximum * 15 / 100;
        bool second = finalDamage && remaining < targetMaximum * 7 / 100;
        uint32 dealt = attacker->Deal(&fixture.actor, supplied);
        assert(dealt == finalDamage && fixture.ai.phase1 == first && fixture.ai.phase2 == second && player.credits == 0);
        std::vector<uint32> expected;
        if (first) expected.push_back(0);
        if (second) expected.push_back(1);
        assert(fixture.ai.texts == expected && fixture.actor.motion.points.size() == uint32(second));
        assert(fixture.actor.alive == (finalDamage < targetMaximum));
        ++cases;
    }
    for (uint32 supplied : {849u, 850u, 851u, 929u, 930u, 931u, 1000u, 2000u})
    {
        Fixture fixture; Player player; uint32 damage = supplied;
        fixture.ai.DamageTakenAfterScaling(&player, damage, 0);
        assert(damage == supplied && fixture.actor.health == 100000);
        assert(fixture.ai.phase1 == (supplied > 850) && fixture.ai.phase2 == (supplied > 930));
        auto texts = fixture.ai.texts; auto points = fixture.actor.motion.points;
        fixture.ai.DamageTakenAfterScaling(&player, damage, 0);
        assert(fixture.ai.texts == texts && fixture.actor.motion.points == points);
    }
    for (uint32 invalid = 0; invalid < 13; ++invalid)
    {
        Fixture fixture; Player player; uint32 damage = 2000; Unit* attacker = &player;
        if (invalid == 0) attacker = nullptr;
        if (invalid == 1) damage = 0;
        if (invalid == 2) fixture.actor.alive = false;
        if (invalid == 3) fixture.actor.entry = 98007;
        if (invalid == 4) fixture.actor.mapId = 1220;
        if (invalid == 5) fixture.actor.instance = 99;
        if (invalid == 6) scenarioManager.available = false;
        if (invalid == 7) scenarioManager.scenario.identity = 973;
        if (invalid >= 8) scenarioManager.scenario.step = invalid == 8 ? 0 : invalid == 9 ? 4 : invalid == 10 ? 6 : invalid == 11 ? 7 : 8;
        uint32 supplied = damage;
        fixture.ai.DamageTakenAfterScaling(attacker, damage, 0);
        assert(damage == (invalid >= 5 ? 0 : supplied) && fixture.ai.texts.empty() && fixture.actor.motion.points.empty());
    }
    for (uint32 resetLine : {0u, 1u})
    {
        Fixture fixture; Player player;
        fixture.ai.onTalk = [&](uint32 identity) { if (identity == resetLine) fixture.ai.Reset(); };
        uint32 damage = 2000; fixture.ai.DamageTakenAfterScaling(&player, damage, 0);
        assert(!fixture.ai.phase1 && !fixture.ai.phase2 && fixture.actor.motion.points.empty() && damage == 2000);
    }
    {
        Fixture fixture; Player player;
        fixture.ai.onTalk = [&](uint32) { uint32 damage = 2000; fixture.ai.DamageTakenAfterScaling(&player, damage, 0); };
        uint32 damage = 2000; fixture.ai.DamageTakenAfterScaling(&player, damage, 0);
        assert(fixture.ai.texts == (std::vector<uint32>{0, 1}) && fixture.actor.motion.points == (std::vector<uint32>{0}));
        fixture.ai.Reset(); fixture.ai.onTalk = {}; fixture.ai.DamageTakenAfterScaling(&player, damage, 0);
        assert(fixture.ai.texts == (std::vector<uint32>{0, 1, 0, 1}) && fixture.actor.motion.points == (std::vector<uint32>{0, 0}));
    }
    {
        Fixture fixture; Unit npc; uint32 damage = 2000; fixture.ai.DamageTakenAfterScaling(&npc, damage, 0);
        assert(fixture.ai.phase1 && fixture.ai.phase2 && damage == 2000);
    }
    uint32 admissionCases = 0;
    uint32 communionCases = 0;
    for (bool done : {false, true}) for (bool pending : {false, true})
        for (uint32 kind = 0; kind < 5; ++kind) for (bool alliance : {false, true}) for (bool suspended : {false, true})
    {
        Fixture fixture; Player player; player.alliance = alliance;
        Unit pet, guardian, charmed, npc; pet.owner = guardian.owner = &player; charmed.charmer = &player;
        Unit* attacker = std::vector<Unit*>{&player, &pet, &guardian, &charmed, &npc}[kind];
        fixture.ai.introDone = done; fixture.ai.introPending = pending;
        CreatureAI temporary; fixture.actor.active = suspended ? &temporary : &fixture.ai;
        fixture.actor.i_disabledAI = suspended ? &fixture.ai : nullptr;
        uint32 scaled = uint32(200000 / fixture.actor.GetHealthMultiplierForTarget(attacker));
        bool ready = done && !pending;
        assert(attacker->Deal(&fixture.actor, 200000) == (ready ? scaled : 0));
        assert(fixture.actor.alive == !ready && fixture.actor.health == 100000 && player.credits == 0);
        assert(fixture.ai.texts.size() == (ready ? 2u : 0u));
        ++communionCases;
    }
    for (uint32 step = 0; step < 9; ++step) for (uint32 kind = 0; kind < 5; ++kind)
        for (bool alliance : {false, true}) for (bool suspended : {false, true})
    {
        Fixture fixture; Player player; player.alliance = alliance;
        Unit pet, guardian, charmed, npc; pet.owner = guardian.owner = &player; charmed.charmer = &player;
        Unit* attacker = std::vector<Unit*>{&player, &pet, &guardian, &charmed, &npc}[kind];
        CreatureAI temporary;
        fixture.actor.active = suspended ? &temporary : &fixture.ai;
        fixture.actor.i_disabledAI = suspended ? &fixture.ai : nullptr;
        scenarioManager.scenario.step = step;
        uint32 scaled = uint32(200000 / fixture.actor.GetHealthMultiplierForTarget(attacker));
        uint32 dealt = attacker->Deal(&fixture.actor, 200000);
        assert(dealt == (step == 5 ? scaled : 0u) && fixture.actor.alive == (step != 5));
        assert(fixture.actor.health == 100000 && player.credits == 0);
        assert(fixture.ai.texts.size() == (step == 5 ? 2u : 0u));
        ++admissionCases;
    }
    {
        Fixture fixture; Player player;
        scenarioManager.scenario.step = 4;
        assert(player.Deal(&fixture.actor, 200000) == 0 && fixture.actor.alive);
        scenarioManager.scenario.step = 5;
        assert(player.Deal(&fixture.actor, 0) == 0 && fixture.actor.alive);
        uint32 scaledHit = uint32(1 / fixture.actor.GetHealthMultiplierForTarget(&player));
        assert(player.Deal(&fixture.actor, 1) == scaledHit && fixture.actor.alive);
        scenarioManager.scenario.step = 4;
        uint64 health = fixture.actor.health;
        assert(player.Deal(&fixture.actor, 200000) == 0 && fixture.actor.health == health);
        scenarioManager.scenario.step = 5;
        uint32 scaledLethal = uint32(200000 / fixture.actor.GetHealthMultiplierForTarget(&player));
        assert(player.Deal(&fixture.actor, 200000) == scaledLethal && !fixture.actor.alive);
    }
    std::cout << "PASS: " << communionCases << " all intro-state/scaled lethal/current-suspended/owner/faction paths reject damage until completed nonpending communion without credit, healing or permanent immunity.\n";
    std::cout << "PASS: " << admissionCases << " actual scaled dispatch stage/owner-label/faction-label/current-suspended paths reject wrong-stage lethal damage; stationary4->5 and reverse transitions re-evaluate live state without persistent immunity or health repair.\n";
    std::cout << "PASS: " << cases << " actual native scale/hook/current-suspended/owner-label/faction-label paths; projected strict15/7 boundaries, unchanged stage5 damage/health and no quest grant, once-before-speech claims, invalid scope and reset/reentrant guards. Movement/speech are requests, not lethal-hit cinematic survival or client visuals.\n";
#endif
}
