#include "Harness.inc"
struct Magnar : CreatureAI
{
    Creature* me;
    explicit Magnar(Creature* actor) : me(actor) { wave = 1; count_dies = 0; first_phase = true; Reset(); }
#include "Fields.inc"
#include "Reset.inc"
#include "Eligible.inc"
#include "Before.inc"
#include "Conversation.inc"
#include "Handoff.inc"
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
        conversationDataStore.Reset();
        actor.active = &ai; ai.first_phase = false;
        owner.distance = 25; peer.distance = 5; bystander.distance = 1; bystander.playerClass = 2;
        actor.nearby = &bystander;
        instanceMap.players = {{&bystander}, {nullptr}, {&peer}, {&owner}};
    }
    ~Fixture() { Conversation::Cleanup(); }
    void Finish()
    {
#if !defined(BASELINE) && !defined(CREATION_BASELINE) && !defined(HANDOFF_BASELINE)
        ai.UpdateFinale(34064);
        ai.UpdateFinale(6000);
#endif
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
#elif defined(CREATION_BASELINE)
    {
        Fixture fixture;
        fixture.actor.conversationWorks = false;
        uint32 before = Conversation::failedDeletes;
        assert(fixture.owner.Deal(&fixture.actor, 20) == 0);
        assert(!fixture.actor.conversations && fixture.actor.conversationAttempts == 1);
        assert(Conversation::failedDeletes == before + 1 && Conversation::allocations.empty());
        assert(fixture.owner.credits == 1 && fixture.actor.despawns == 1 && fixture.ai.finaleStarted);
        fixture.actor.conversationWorks = true;
        fixture.owner.Deal(&fixture.actor, 20);
        assert(!fixture.actor.conversations && fixture.actor.conversationAttempts == 1);
    }
    std::cout << "BASELINE: pinned65f7a845/native void Unit factory deletes failed504 but Magnar still credits45784, despawns and permanently consumes finale retry. Controlled factory result, not a live grid failure.\n";
#elif defined(HANDOFF_BASELINE)
    Fixture fixture;
    assert(fixture.owner.Deal(&fixture.actor, 20) == 0);
    assert(fixture.actor.conversations == 1 && fixture.owner.credits == 1 && scenarioManager.scenario.step == 5 && fixture.actor.despawns == 1);
    std::cout << "BASELINE: pinnedf91fbfc7 credits45784/opens native final-stage phase immediately on acknowledged504, while its last dialogue starts34064ms later; Creature already owns6000ms removal. Controlled native handler/services, not an observed early client pickup.\n";
#else
    {
        Fixture fixture;
        fixture.owner.Deal(&fixture.actor, 20);
        fixture.ai.UpdateFinale(34063);
        assert(!fixture.owner.credits && !fixture.actor.despawns && fixture.ai.finaleTimer == 1 && scenarioManager.scenario.step == 4);
        fixture.ai.UpdateFinale(1);
        assert(fixture.owner.credits == 1 && !fixture.actor.despawns && fixture.ai.finaleDeparting && scenarioManager.scenario.step == 5);
        fixture.ai.UpdateFinale(5999);
        assert(!fixture.actor.despawns);
        fixture.ai.UpdateFinale(1);
        fixture.ai.UpdateFinale(100000);
        assert(fixture.actor.despawns == 1 && fixture.owner.credits == 1 && !fixture.actor.retiredConversations);
    }
    for (uint32 invalid = 0; invalid < 8; ++invalid)
    {
        Fixture fixture;
        if (invalid == 0) conversationDataStore.available = false;
        if (invalid == 1) conversationDataStore.lines.pop_back();
        if (invalid == 2) conversationDataStore.lines.push_back(conversationDataStore.lines.back());
        if (invalid == 3) conversationDataStore.lines[3].id = 1343;
        if (invalid == 4) conversationDataStore.lines[3].idx = 2;
        if (invalid == 5) conversationDataStore.lines[3].textId = 23424;
        if (invalid == 6) conversationDataStore.lines[3].unk1 = 1;
        if (invalid == 7) conversationDataStore.lines[3].unk2 = 0;
        fixture.owner.Deal(&fixture.actor, 20);
        assert(!fixture.ai.finaleStarted && !fixture.owner.credits && !fixture.actor.conversationAttempts && !fixture.actor.despawns);
        conversationDataStore.Reset();
        fixture.owner.Deal(&fixture.actor, 20); fixture.Finish();
        assert(fixture.owner.credits == 1 && fixture.actor.despawns == 1);
    }
    for (bool departing : {false, true})
    {
        Fixture fixture;
        fixture.owner.Deal(&fixture.actor, 20);
        if (departing) fixture.ai.UpdateFinale(34064);
        fixture.actor.flags |= 0x40000000;
        fixture.ai.Reset();
        assert(!fixture.ai.finalePending && !fixture.ai.finaleDeparting && fixture.actor.retiredConversations == 1 && fixture.actor.faction == 14 && fixture.actor.flags == (UNIT_FLAG_IMMUNE_TO_NPC | 0x40000000));
        fixture.ai.UpdateFinale(100000);
        assert(!fixture.actor.despawns && fixture.owner.credits == uint32(departing));
        scenarioManager.scenario.step = 4;
        fixture.owner.Deal(&fixture.actor, 20); fixture.Finish();
        assert(fixture.owner.credits == 1 + uint32(departing) && fixture.actor.conversations == 2 && fixture.actor.despawns == 1);
    }
    {
        Fixture fixture;
        auto* unrelated = new Conversation;
        assert(unrelated->CreateConversation(100, 504, &fixture.actor, nullptr, fixture.actor));
        fixture.owner.Deal(&fixture.actor, 20);
        fixture.ai.Reset();
        assert(fixture.actor.retiredConversations == 1 && !unrelated->removed);
    }
    {
        Fixture fixture;
        fixture.owner.Deal(&fixture.actor, 20);
        fixture.owner.world = fixture.peer.world = false;
        fixture.ai.UpdateFinale(100000);
        assert(fixture.ai.finalePending && !fixture.owner.credits && !fixture.actor.despawns);
        fixture.owner.world = true;
        fixture.ai.UpdateFinale(1);
        assert(fixture.owner.credits == 1 && !fixture.actor.despawns);
        fixture.ai.UpdateFinale(6000); assert(fixture.actor.despawns == 1);
    }
    {
        Fixture fixture;
        fixture.owner.Deal(&fixture.actor, 20);
        fixture.owner.available = false;
        fixture.Finish();
        assert(!fixture.owner.credits && fixture.peer.credits == 1);
    }
    {
        Fixture fixture;
        fixture.owner.creditWorks = false;
        fixture.owner.Deal(&fixture.actor, 20); fixture.Finish();
        assert(fixture.ai.finalePending && !fixture.actor.despawns && scenarioManager.scenario.step == 4);
        fixture.owner.creditWorks = true;
        fixture.ai.UpdateFinale(1); fixture.ai.UpdateFinale(6000);
        assert(fixture.actor.despawns == 1 && scenarioManager.scenario.step == 5);
    }
    {
        Fixture fixture;
        fixture.owner.onCredit = [&]() { fixture.ai.Reset(); };
        fixture.owner.Deal(&fixture.actor, 20); fixture.Finish();
        assert(fixture.owner.credits == 1 && !fixture.ai.finaleDeparting && !fixture.actor.despawns && fixture.actor.retiredConversations == 1);
    }
    {
        Fixture fixture;
        conversationDataStore.lines[3].textId = 36000;
        fixture.owner.Deal(&fixture.actor, 20);
        conversationDataStore.lines[3].textId = 37000;
        fixture.ai.UpdateFinale(35999); assert(!fixture.owner.credits);
        fixture.ai.UpdateFinale(1); assert(fixture.owner.credits == 1);
    }
    for (uint32 invalid = 0; invalid < 15; ++invalid)
    {
        Fixture fixture; instanceMap.players = {{&fixture.owner}};
        fixture.owner.Deal(&fixture.actor, 20);
        Player original = fixture.owner;
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
        if (invalid == 14) { fixture.owner.available = false; instanceMap.players.clear(); }
        fixture.ai.UpdateFinale(100000);
        assert(fixture.ai.finalePending && !fixture.owner.credits && !fixture.actor.despawns);
        fixture.owner = original; scenarioManager = {}; instanceMap.players = {{&fixture.owner}};
        fixture.ai.UpdateFinale(1); fixture.ai.UpdateFinale(6000);
        assert(fixture.owner.credits == 1 && fixture.actor.despawns == 1);
    }
    for (bool duringCredit : {false, true}) for (uint32 invalid = 0; invalid < 7; ++invalid)
    {
        Fixture fixture;
        auto revoke = [&]() {
            if (invalid == 0) fixture.actor.world = false;
            if (invalid == 1) fixture.actor.alive = false;
            if (invalid == 2) fixture.actor.despawning = true;
            if (invalid == 3) fixture.actor.entry = 96035;
            if (invalid == 4) fixture.actor.mapId = 1220;
            if (invalid == 5) fixture.actor.map = nullptr;
            if (invalid == 6) fixture.actor.instance = 43;
        };
        fixture.owner.Deal(&fixture.actor, 20);
        if (duringCredit) fixture.owner.onCredit = revoke; else revoke();
        fixture.ai.UpdateFinale(100000);
        assert(!fixture.actor.despawns && fixture.ai.finalePending && fixture.owner.credits == uint32(duringCredit));
        fixture.actor.world = fixture.actor.alive = true; fixture.actor.despawning = false;
        fixture.actor.entry = 96034; fixture.actor.mapId = 1495; fixture.actor.map = &instanceMap; fixture.actor.instance = 42;
        fixture.owner.onCredit = nullptr;
        fixture.ai.UpdateFinale(duringCredit ? 1 : 34064); fixture.ai.UpdateFinale(6000);
        assert(fixture.owner.credits == 1 && fixture.actor.despawns == 1);
    }
    {
        Fixture fixture;
        fixture.owner.Deal(&fixture.actor, 20);
        scenarioManager.scenario.step = 5;
        fixture.Finish();
        assert(!fixture.owner.credits && !fixture.peer.credits && fixture.actor.despawns == 1);
    }
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
            assert(!fixture.actor.despawns && !fixture.owner.credits && !fixture.peer.credits && scenarioManager.scenario.step == 4);
            fixture.Finish();
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
        assert(ally.Deal(&fixture.actor, 20) == 0); fixture.Finish();
        assert(fixture.peer.credits == 1 && fixture.actor.conversations == 1);
    }
    {
        Fixture fixture; fixture.owner.alive = false;
        Unit pet; pet.owner = &fixture.owner;
        assert(pet.Deal(&fixture.actor, 20) == 0); fixture.Finish(); assert(fixture.owner.credits == 1);
    }
    {
        Fixture fixture; fixture.owner.available = false;
        instanceMap.players = {{&fixture.peer}};
        Unit guardian; guardian.owner = &fixture.owner;
        assert(guardian.Deal(&fixture.actor, 20) == 0); fixture.Finish(); assert(!fixture.owner.credits && fixture.peer.credits == 1);
    }
    {
        Fixture fixture; fixture.actor.onConversation = [&]() { uint32 damage = 2000; fixture.ai.DamageTakenAfterScaling(&fixture.owner, damage, 0); assert(!damage); };
        fixture.owner.Deal(&fixture.actor, 20);
        fixture.Finish();
        assert(fixture.actor.conversations == 1 && fixture.owner.credits == 1 && fixture.actor.despawns == 1);
        fixture.ai.Reset(); assert(!fixture.ai.finaleStarted && !fixture.ai.stop_combat);
    }
    {
        Fixture fixture;
        assert(fixture.owner.Deal(&fixture.actor, 5) > 0 && fixture.actor.alive && !fixture.actor.conversations && fixture.actor.GetHealth(&fixture.owner) == 500);
        assert(fixture.owner.Deal(&fixture.actor, 5) == 0); fixture.Finish(); assert(fixture.owner.credits == 1);
    }
    for (uint32 variant : {0u, 1u})
    {
        Fixture fixture;
        if (variant == 0) fixture.actor.scalable = false; else fixture.actor.statsAvailable = false;
        assert(fixture.owner.Deal(&fixture.actor, variant == 0 ? 100000 : 2000) == 0); fixture.Finish(); assert(fixture.actor.alive && fixture.owner.credits == 1);
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
        fixture.Finish();
        assert(!damage && fixture.peer.credits == 1 && fixture.actor.conversations == 1);
    }
    for (bool alliance : {false, true}) for (uint32 status : {1u, 3u}) for (uint32 kind = 0; kind < 5; ++kind)
    {
        Fixture fixture; fixture.owner.alliance = alliance; fixture.owner.status = status;
        Unit pet, guardian, charmed, ally;
        pet.owner = guardian.owner = &fixture.owner; charmed.charmer = &fixture.owner;
        Unit* attacker = kind == 0 ? static_cast<Unit*>(&fixture.owner) : kind == 1 ? &pet : kind == 2 ? &guardian : kind == 3 ? &charmed : &ally;
        fixture.actor.conversationWorks = false;
        uint32 warnings = warningCount;
        uint32 deletes = Conversation::failedDeletes;
        for (uint32 attempt = 0; attempt < 2; ++attempt)
        {
            assert(attacker->Deal(&fixture.actor, 20) == 0 && fixture.actor.alive);
            assert(!fixture.ai.finaleStarted && !fixture.ai.stop_combat && !fixture.actor.conversations);
            assert(!fixture.owner.credits && !fixture.peer.credits && !fixture.actor.despawns);
            assert(!fixture.actor.removed && !fixture.actor.flags && fixture.actor.faction == 14);
            assert(scenarioManager.scenario.step == 4 && Conversation::allocations.empty());
        }
        assert(warningCount == warnings + 2 && Conversation::failedDeletes == deletes + 2);
        fixture.actor.conversationWorks = true;
        attacker->Deal(&fixture.actor, 20);
        assert(fixture.ai.finaleStarted && fixture.actor.conversations == 1 && fixture.actor.conversationAttempts == 3);
        fixture.Finish();
        assert(fixture.owner.credits + fixture.peer.credits == 1 && fixture.actor.despawns == 1);
    }
    for (uint32 invalid = 0; invalid < 22; ++invalid)
    {
        Fixture fixture;
        fixture.actor.onConversation = [&]() {
            if (invalid == 0) fixture.ai.Reset();
            if (invalid == 1) fixture.actor.world = false;
            if (invalid == 2) fixture.actor.alive = false;
            if (invalid == 3) fixture.actor.despawning = true;
            if (invalid == 4) fixture.actor.entry = 96035;
            if (invalid == 5) fixture.actor.mapId = 1220;
            if (invalid == 6) fixture.actor.map = &unrelatedMap;
            if (invalid == 7) fixture.actor.instance = 43;
            if (invalid == 8) fixture.owner.status = 0;
            if (invalid == 9) fixture.owner.contact = false;
            if (invalid == 10) fixture.owner.phase = false;
            if (invalid == 11) scenarioManager.scenario.step = 5;
            if (invalid == 12) scenarioManager.available = false;
            if (invalid == 13) fixture.ai.first_phase = true;
            if (invalid == 14) fixture.ai.finaleStarted = false;
            if (invalid == 15) fixture.owner.world = false;
            if (invalid == 16) fixture.owner.playerClass = 2;
            if (invalid == 17) fixture.owner.map = &unrelatedMap;
            if (invalid == 18) fixture.owner.instance = 43;
            if (invalid == 19) fixture.owner.distance = 101;
            if (invalid == 20) fixture.owner.rewarded = true;
            if (invalid == 21) scenarioManager.scenario.identity = 944;
        };
        fixture.owner.Deal(&fixture.actor, 20);
        assert(fixture.actor.conversations == 1 && fixture.actor.retiredConversations == 1);
        assert(!fixture.ai.finaleStarted && !fixture.owner.credits && !fixture.actor.despawns && !fixture.actor.removed);
    }
    {
        Fixture fixture;
        fixture.actor.onConversation = [&]() {
            fixture.actor.onConversation = nullptr;
            fixture.ai.Reset();
            fixture.owner.Deal(&fixture.actor, 20);
        };
        fixture.owner.Deal(&fixture.actor, 20);
        assert(fixture.actor.conversations == 2 && fixture.actor.retiredConversations == 1);
        fixture.Finish();
        assert(fixture.ai.finaleStarted && fixture.owner.credits == 1 && fixture.actor.despawns == 1);
    }
    {
        Fixture fixture;
        fixture.actor.conversationWorks = false;
        fixture.actor.onCreationFailure = [&]() {
            fixture.actor.onCreationFailure = nullptr;
            fixture.actor.conversationWorks = true;
            fixture.ai.Reset();
            fixture.owner.Deal(&fixture.actor, 20);
        };
        uint32 deletes = Conversation::failedDeletes;
        fixture.owner.Deal(&fixture.actor, 20);
        assert(fixture.actor.conversationAttempts == 2 && fixture.actor.conversations == 1 && !fixture.actor.retiredConversations);
        fixture.Finish();
        assert(Conversation::failedDeletes == deletes + 1 && fixture.ai.finaleStarted && fixture.owner.credits == 1 && fixture.actor.despawns == 1);
    }
    std::cout << "PASS: data-derived34064ms handoff retains stage4 and boss through Magnar's speech;6000ms removal starts only after observed stage5. Eight malformed-data contexts remain retryable, pending/departing Reset cancels only owned conversation/timers and restores duel admission; missing recipient, failed progression, late owner fallback and reentrant credit Reset covered. Normal departure does not remove Hruthnir's remaining conversation. No client traversal/VFX certificate.\n";
    std::cout << "PASS:20 faction/status/killer contexts reject two failed504 attempts without flags/faction/credit/despawn, delete failed allocations with diagnostics and succeed once on later lethal retry.22 post-creation revoked contexts retire only that object; old successful or failed Reset epoch cannot clear a newer successful claim. Actual factory result/actor lookup controlled; failed creation still retries on a later lethal hit.\n";
    std::cout << "PASS: actual native scale/health/owner/post-hook order protects80 faction/status/killer/suspended/phase combinations;16 ineligible contexts stay alive without finishing. Owner preference, eligible shared-instance fallback, missing recipient retry, reentrant-hit guard, partial/unscaled/missing-stats/null-source and unrelated actors checked. Shared scenario advancement controlled; full waves/reset/conversation/client remain separate.\n";
#endif
}
