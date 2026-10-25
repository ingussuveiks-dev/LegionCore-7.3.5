#include "Harness.inc"
struct Magnar : CreatureAI
{
    Creature* me;
    explicit Magnar(Creature* actor) : me(actor) { wave = 1; count_dies = 0; first_phase = true; Reset(); }
#include "Fields.inc"
#include "Reset.inc"
#include "Eligible.inc"
#include "Before.inc"
#include "After.inc"
};
#include "Deal.inc"
struct Fixture
{
    Creature actor;
    Magnar ai{&actor};
    Player owner, peer, bystander;
    Fixture()
    {
        scenarioManager = {}; instanceMap.players.clear();
        actor.active = &ai; ai.first_phase = false;
        owner.distance = 25; peer.distance = 5; bystander.distance = 1; bystander.playerClass = 2;
        actor.nearby = &bystander;
        instanceMap.players = {{&bystander}, {nullptr}, {&peer}, {&owner}};
    }
};
int main()
{
#ifdef BASELINE
    for (bool firstPhase : {false, true})
    {
        Fixture fixture; fixture.ai.first_phase = firstPhase;
        assert(fixture.owner.Deal(&fixture.actor, 20) >= 1000 && !fixture.actor.alive && !fixture.actor.conversations);
    }
    {
        Fixture fixture; fixture.actor.scalable = false; fixture.actor.nearby = &fixture.bystander;
        fixture.owner.Deal(&fixture.actor, 100000); fixture.owner.Deal(&fixture.actor, 100000);
        assert(fixture.actor.conversations == 2 && fixture.bystander.credits == 2 && !fixture.owner.credits);
    }
    std::cout << "BASELINE: pinned675d6f7a loses both phases to an actual scaled lethal hit; unscaled repeated finale credits unrelated nearest bystander twice. Controlled services, not client combat.\n";
#else
    for (bool alliance : {false, true}) for (uint32 status : {1u, 3u}) for (uint32 kind = 0; kind < 5; ++kind)
        for (bool suspended : {false, true}) for (bool firstPhase : {false, true})
    {
        Fixture fixture; fixture.owner.alliance = alliance; fixture.owner.status = status; fixture.ai.first_phase = firstPhase;
        Unit pet, guardian, charmed, ally; CreatureAI temporary;
        pet.owner = guardian.owner = &fixture.owner; charmed.charmer = &fixture.owner;
        Unit* attacker = kind == 0 ? static_cast<Unit*>(&fixture.owner) : kind == 1 ? &pet : kind == 2 ? &guardian : kind == 3 ? &charmed : &ally;
        if (suspended) { fixture.actor.active = &temporary; fixture.actor.i_disabledAI = &fixture.ai; }
        assert(attacker->Deal(&fixture.actor, 20) == 0 && fixture.actor.alive);
        assert(fixture.bystander.credits == 0);
        if (firstPhase) { assert(!fixture.actor.conversations && !fixture.owner.credits && !fixture.peer.credits && !fixture.ai.finaleStarted); }
        else
        {
            assert(fixture.actor.conversations == 1 && fixture.actor.despawns == 1 && fixture.actor.faction == 35 && fixture.actor.removed == 1);
            assert(fixture.ai.finaleStarted && fixture.ai.stop_combat && scenarioManager.scenario.step == 5);
            assert(fixture.owner.credits == uint32(kind != 4) && fixture.peer.credits == uint32(kind == 4));
            attacker->Deal(&fixture.actor, 1); assert(fixture.actor.conversations == 1);
        }
    }
    for (uint32 invalid = 0; invalid < 16; ++invalid)
    {
        Fixture fixture; instanceMap.players = {{&fixture.owner}};
        if (invalid == 0) fixture.owner.world = false;
        if (invalid == 1) fixture.owner.contact = false;
        if (invalid == 2) fixture.owner.playerClass = 2;
        if (invalid == 3) fixture.owner.map = &unrelatedMap;
        if (invalid == 4) fixture.owner.instance = 43;
        if (invalid == 5) fixture.owner.phase = false;
        if (invalid == 6) fixture.owner.distance = 101;
        if (invalid == 7) fixture.owner.rewarded = true;
        if (invalid == 8) fixture.owner.status = 0;
        if (invalid == 9) fixture.owner.status = 5;
        if (invalid == 10) fixture.owner.status = 6;
        if (invalid == 11) scenarioManager.available = false;
        if (invalid == 12) scenarioManager.scenario.identity = 944;
        if (invalid == 13) scenarioManager.scenario.step = 3;
        if (invalid == 14) scenarioManager.scenario.step = 5;
        if (invalid == 15) fixture.actor.map = nullptr;
        assert(fixture.owner.Deal(&fixture.actor, 20) == 0 && fixture.actor.alive);
        assert(!fixture.actor.conversations && !fixture.actor.despawns && !fixture.ai.finaleStarted && !fixture.owner.credits);
    }
    {
        Fixture fixture; instanceMap.players.clear(); Unit ally;
        assert(ally.Deal(&fixture.actor, 20) == 0 && !fixture.ai.finaleStarted);
        instanceMap.players = {{&fixture.peer}};
        assert(ally.Deal(&fixture.actor, 20) == 0 && fixture.peer.credits == 1 && fixture.actor.conversations == 1);
    }
    {
        Fixture fixture; fixture.owner.alive = false;
        Unit pet; pet.owner = &fixture.owner;
        assert(pet.Deal(&fixture.actor, 20) == 0 && fixture.owner.credits == 1);
    }
    {
        Fixture fixture; fixture.owner.available = false;
        instanceMap.players = {{&fixture.peer}};
        Unit guardian; guardian.owner = &fixture.owner;
        assert(guardian.Deal(&fixture.actor, 20) == 0 && !fixture.owner.credits && fixture.peer.credits == 1);
    }
    {
        Fixture fixture; fixture.actor.onConversation = [&]() { uint32 damage = 2000; fixture.ai.DamageTakenAfterScaling(&fixture.owner, damage, 0); assert(!damage); };
        fixture.owner.Deal(&fixture.actor, 20);
        assert(fixture.actor.conversations == 1 && fixture.owner.credits == 1 && fixture.actor.despawns == 1);
        fixture.ai.Reset(); assert(!fixture.ai.finaleStarted && !fixture.ai.stop_combat);
    }
    {
        Fixture fixture;
        assert(fixture.owner.Deal(&fixture.actor, 5) > 0 && fixture.actor.alive && !fixture.actor.conversations && fixture.actor.GetHealth(&fixture.owner) == 500);
        assert(fixture.owner.Deal(&fixture.actor, 5) == 0 && fixture.owner.credits == 1);
    }
    for (uint32 variant : {0u, 1u})
    {
        Fixture fixture;
        if (variant == 0) fixture.actor.scalable = false; else fixture.actor.statsAvailable = false;
        assert(fixture.owner.Deal(&fixture.actor, variant == 0 ? 100000 : 2000) == 0 && fixture.actor.alive && fixture.owner.credits == 1);
    }
    for (uint32 variant : {0u, 1u})
    {
        Fixture fixture;
        if (variant == 0) fixture.actor.entry = 96035; else fixture.actor.mapId = 1220;
        assert(fixture.owner.Deal(&fixture.actor, 20) > 0 && !fixture.actor.alive && !fixture.actor.conversations);
    }
    {
        Fixture fixture; uint32 damage = 0;
        fixture.ai.DamageTakenAfterScaling(nullptr, damage, 0); assert(!fixture.actor.conversations);
        damage = 100000; fixture.ai.DamageTakenAfterScaling(nullptr, damage, 0);
        assert(!damage && fixture.peer.credits == 1 && fixture.actor.conversations == 1);
    }
    std::cout << "PASS: actual native scale/health/owner/post-hook order protects80 faction/status/killer/suspended/phase combinations;16 ineligible contexts stay alive without finishing. Owner preference, eligible shared-instance fallback, missing recipient retry, reentrant-hit guard, partial/unscaled/missing-stats/null-source and unrelated actors checked. Shared scenario advancement controlled; full waves/reset/conversation/client remain separate.\n";
#endif
}
