#include "WarswordsFinalIntroTest.cpp"
#include <atomic>
#include <map>
#include <mutex>

using uint64 = uint64_t;
using FunctionList = std::multimap<uint64, std::function<void()>>;
#define TC_COMMON_API
#include "NativeFunctionProcessor.inc"
#include "NativeFunctionMethods.inc"

void TestNativeBlessingTimer()
{
    for (uint32 stalled : {0u, 31000u, 100000u})
    {
        FunctionProcessor processor;
        uint32 casts = 0, engagements = 0;
        processor.AddDelayedEvent(24000, [&]() {
            ++casts;
            processor.AddDelayedEvent(7000, [&]() { ++engagements; });
        });
        processor.Update(stalled ? stalled : 23999);
        if (!stalled) { assert(!casts && !engagements); processor.Update(1); }
        assert(casts == 1 && !engagements && processor.SizeQueue() == 1);
        processor.Update(6999); assert(!engagements);
        processor.Update(1); assert(engagements == 1 && processor.Empty() && !processor.SizeQueue());
        processor.Update(100000); assert(engagements == 1 && casts == 1);
    }
}

void Tick(Fixture& fixture, uint32 duration)
{
    while (duration)
    {
        uint32 elapsed = std::min(duration, 1000u);
        fixture.actor.Advance(elapsed);
        duration -= elapsed;
    }
}

void Finish(Fixture& fixture)
{
    Tick(fixture, 24000);
    Tick(fixture, 7000);
}

void InvalidateHelya(Fixture& fixture, uint32 invalid)
{
    if (invalid == 0) ObjectAccessor::creatures.clear();
    if (invalid == 1) fixture.helya.world = false;
    if (invalid == 2) fixture.helya.predelete = true;
    if (invalid == 3) fixture.helya.alive = false;
    if (invalid == 4) fixture.helya.map = &otherMap;
    if (invalid == 5) fixture.helya.phase = false;
    if (invalid == 6) fixture.helya.entry = 98007;
    if (invalid == 7) fixture.helya.distanceFromActor = 100.1f;
    if (invalid == 8) fixture.helya.map = nullptr;
}

int main()
{
    TestNativeBlessingTimer();
    {
        Fixture fixture;
        uint64_t castAt = 0;
        fixture.helya.onCast = [&]() { castAt = fixture.actor.elapsed; };
        fixture.actor.onElapsed = [&](uint32 diff) {
            fixture.ai.TickVisual(diff);
            if (castAt && fixture.actor.elapsed >= castAt + 7000) fixture.actor.aura = false;
        };
        fixture.Start(); fixture.Start();
        assert(fixture.owner.acceptedConversations == 1 && !fixture.helya.casts && !fixture.actor.aura && fixture.ai.introPending);
        for (uint32 deadline : {7000u, 8000u, 12150u, 23999u})
        {
            Tick(fixture, uint32(deadline - fixture.actor.elapsed));
            fixture.ai.TickCombat(0); fixture.ai.AttackStart(&fixture.owner);
            assert(!fixture.helya.casts && !fixture.ai.attacks && fixture.actor.bubble && fixture.ai.introPending);
            assert(!fixture.ai.victimChecks && !fixture.ai.combatTicks);
        }
        Tick(fixture, 1);
        assert(castAt == 24000 && fixture.helya.casts == 1 && fixture.actor.aura && !fixture.helya.aura);
        assert(fixture.actor.auraCaster == fixture.helya.guid && !fixture.ai.IsEncounterReady());
        Tick(fixture, 6999); assert(fixture.actor.aura && fixture.actor.bubble && !fixture.ai.attacks);
        Tick(fixture, 1);
        assert(!fixture.actor.aura && !fixture.actor.bubble && fixture.ai.IsEncounterReady());
        assert(fixture.ai.attacks == 1 && fixture.ai.attacked == &fixture.owner && !fixture.ai.introPending);
        Tick(fixture, 50000); assert(fixture.ai.attacks == 1 && fixture.helya.casts == 1 && fixture.owner.acceptedConversations == 1);
        fixture.ai.TickCombat(1); assert(fixture.ai.victimChecks == 1 && fixture.ai.combatTicks == 1);
    }
    {
        Fixture fixture; fixture.Start();
        fixture.actor.Advance(31000);
        assert(fixture.helya.casts == 1 && !fixture.ai.attacks && fixture.actor.callbacks.size() == 1 && fixture.actor.bubble);
        Tick(fixture, 6999); assert(!fixture.ai.attacks);
        Tick(fixture, 1); assert(fixture.ai.attacks == 1 && fixture.actor.elapsed == 38000);
    }
    for (uint32 invalid = 0; invalid < 19; ++invalid)
    {
        Fixture fixture; Invalidate(fixture, invalid, false); fixture.Start();
        assert(!fixture.owner.conversations && !fixture.helya.casts && fixture.actor.callbacks.empty() && !fixture.ai.introDone);
    }
    for (uint32 stage = 0; stage < 8; ++stage) for (uint32 status = 0; status < 7; ++status)
    {
        Fixture fixture; scenarioManager.scenario.step = stage; fixture.owner.status = status; fixture.Start();
        assert(fixture.ai.introPending == (stage == 5 && (status == 1 || status == 3)));
        assert(!fixture.helya.casts);
    }
    for (bool afterCast : {false, true}) for (uint32 invalid = 0; invalid < 21; ++invalid)
    {
        Fixture fixture; fixture.Start(); auto conversation = Conversation::objects.back();
        Tick(fixture, afterCast ? 24000 : 12150);
        Invalidate(fixture, invalid, true); Tick(fixture, afterCast ? 7000 : 11850);
        assert(!fixture.ai.attacks && fixture.actor.bubble && !fixture.ai.introDone && !fixture.ai.introPending);
        assert(!conversation->world && !fixture.ai.introConversation && Conversation::removed == 1);
        assert(fixture.helya.casts == uint32(afterCast));
    }
    for (uint32 invalid = 0; invalid < 8; ++invalid)
    {
        Fixture fixture; InvalidateHelya(fixture, invalid);
        if (invalid == 0) fixture.actor.helya = nullptr;
        fixture.Start(); fixture.Start();
        assert(!fixture.owner.conversations && !fixture.ai.introDone && !fixture.helya.casts && warningRequests == 1);
    }
    for (bool afterCast : {false, true}) for (uint32 invalid = 0; invalid < 9; ++invalid)
    {
        Fixture fixture; fixture.Start(); auto conversation = Conversation::objects.back();
        if (afterCast) Tick(fixture, 24000);
        InvalidateHelya(fixture, invalid);
        Creature replacement; replacement.entry = 99043; replacement.guid = 100;
        fixture.actor.helya = &replacement; ObjectAccessor::creatures[replacement.guid] = &replacement;
        Tick(fixture, afterCast ? 7000 : 24000);
        assert(!fixture.ai.attacks && !fixture.ai.introPending && !fixture.ai.introDone && fixture.actor.bubble);
        assert(!conversation->world && !fixture.ai.introConversation && !replacement.casts);
    }
    for (bool accepted : {false, true})
    {
        Fixture fixture; fixture.helya.castWorks = accepted; fixture.helya.auraWorks = false;
        fixture.Start(); Tick(fixture, 24000);
        assert(fixture.helya.casts == 1 && !fixture.ai.introDone && !fixture.ai.introPending && !fixture.ai.attacks && fixture.actor.bubble);
        assert(Conversation::removed == 1 && fixture.actor.callbacks.empty() && warningRequests == 1);
        fixture.Start(); assert(fixture.owner.conversations == 1);
        fixture.ai.TickVisual(999); fixture.Start(); assert(fixture.owner.conversations == 1);
        fixture.ai.TickVisual(1); fixture.Start();
        fixture.helya.castWorks = fixture.helya.auraWorks = true;
        Finish(fixture);
        assert(fixture.owner.acceptedConversations == 2 && fixture.helya.casts == 2 && fixture.ai.attacks == 1 && recoveryRequests == 1);
    }
    for (uint32 caster : {98u, 99u, 100u})
    {
        Fixture fixture; fixture.actor.aura = true; fixture.actor.auraCaster = caster;
        fixture.Start(); assert(!fixture.helya.casts);
        Tick(fixture, 24000); assert(fixture.helya.casts == 1 && fixture.actor.auraCaster == fixture.helya.guid);
        Tick(fixture, 6999); assert(!fixture.ai.attacks);
        Tick(fixture, 1); assert(fixture.ai.attacks == 1);
    }
    for (bool accepted : {false, true})
    {
        Fixture fixture; fixture.helya.castWorks = accepted; fixture.helya.auraWorks = false;
        fixture.actor.aura = true; fixture.actor.auraCaster = 100;
        fixture.Start(); Tick(fixture, 24000); assert(!fixture.ai.attacks && !fixture.ai.introPending && fixture.actor.bubble);
    }
    {
        Fixture fixture; fixture.helya.castWorks = false;
        fixture.actor.aura = true; fixture.actor.auraCaster = fixture.helya.guid;
        fixture.Start(); Tick(fixture, 24000);
        assert(!fixture.ai.attacks && !fixture.ai.introPending && fixture.actor.bubble && fixture.helya.casts == 1);
    }
    {
        Fixture fixture; fixture.owner.conversationWorks = false;
        for (uint32 repeat = 0; repeat < 4; ++repeat)
        {
            fixture.Start(); fixture.ai.TickVisual(1000);
            assert(!fixture.ai.introDone && !fixture.ai.introPending && fixture.actor.callbacks.empty() && !fixture.helya.casts);
        }
        assert(Conversation::deleted == 4 && Conversation::objects.empty() && warningRequests == 1);
        fixture.owner.conversationWorks = true; fixture.Start(); Finish(fixture);
        assert(fixture.ai.attacks == 1 && fixture.owner.acceptedConversations == 1 && recoveryRequests == 1);
    }
    for (bool reset : {false, true})
    {
        Fixture fixture;
        fixture.owner.onConversation = [&]() { if (reset) fixture.ai.Reset(); else fixture.owner.status = 0; };
        fixture.Start();
        assert(!fixture.ai.introDone && !fixture.ai.introPending && fixture.actor.callbacks.empty() && Conversation::removed == 1 && !fixture.helya.casts);
    }
    {
        Fixture fixture;
        fixture.owner.onConversation = [&]() { if (fixture.owner.conversations == 1) { fixture.ai.Reset(); fixture.Start(); } };
        fixture.Start(); Finish(fixture);
        assert(fixture.owner.acceptedConversations == 2 && Conversation::removed == 1 && fixture.ai.attacks == 1 && fixture.helya.casts == 1);
    }
    for (uint32 reentry = 0; reentry < 5; ++reentry)
    {
        Fixture fixture;
        fixture.helya.onCast = [&]() {
            if (reentry == 0) fixture.Start();
            if (reentry == 1) fixture.ai.Reset();
            if (reentry == 2) fixture.owner.status = 0;
            if (reentry == 3) fixture.helya.world = false;
            if (reentry == 4) { fixture.ai.Reset(); fixture.Start(); }
        };
        fixture.Start(); Tick(fixture, 24000);
        assert(fixture.helya.casts == 1 && !fixture.ai.attacks);
        if (reentry == 0) { Tick(fixture, 7000); assert(fixture.ai.attacks == 1); }
        else if (reentry == 4) { fixture.helya.onCast = {}; Finish(fixture); assert(fixture.ai.attacks == 1 && fixture.helya.casts == 2); }
        else assert(!fixture.ai.introDone && !fixture.ai.introPending && fixture.actor.callbacks.empty());
    }
    for (uint32 deadline : {0u, 8000u, 12150u, 23999u, 24000u, 30999u})
    {
        Fixture fixture; fixture.Start(); Tick(fixture, deadline);
        auto obsolete = fixture.actor.callbacks.front(); fixture.actor.callbacks.clear();
        auto original = Conversation::objects.back();
        auto foreign = new Conversation; foreign->world = true;
        fixture.ai.Reset(); assert(!original->world && foreign->world && !fixture.ai.introConversation);
        fixture.Start(); obsolete(); assert(!fixture.ai.attacks && fixture.ai.introPending);
        Finish(fixture); assert(fixture.ai.attacks == 1 && foreign->world);
        auto conversation = Conversation::objects.back();
        fixture.ai.Reset(); assert(!conversation->world && foreign->world && fixture.ai.introDone);
        fixture.actor.bubble = true; fixture.ai.UpdateIntro(1); assert(!fixture.actor.bubble);
    }
    for (uint32 missing = 0; missing < 4; ++missing)
    {
        Fixture fixture; fixture.Start(); auto conversation = Conversation::objects.back();
        if (missing == 0) conversation->world = false;
        if (missing == 1) conversation->predelete = true;
        if (missing == 2) conversation->map = &otherMap;
        if (missing == 3) fixture.actor.map = nullptr;
        fixture.ai.Reset(); assert(!fixture.ai.introConversation && !Conversation::removed);
    }
    {
        Fixture fixture; fixture.Start(); auto original = Conversation::objects.back();
        original->onRemove = [&]() { assert(!fixture.ai.introConversation); fixture.Start(); };
        fixture.ai.Reset(); auto replacement = Conversation::objects.back();
        assert(!original->world && replacement->world && fixture.ai.introConversation == replacement->GetGUID());
        Finish(fixture); assert(fixture.ai.attacks == 1 && replacement->world);
    }
    for (bool afterCast : {false, true})
    {
        Fixture fixture; fixture.Start(); if (afterCast) Tick(fixture, 24000);
        fixture.actor.alive = false; fixture.ai.UpdateLifecycle(1);
        assert(!fixture.actor.bubble && !fixture.ai.introConversation && Conversation::removed == 1);
        Tick(fixture, 31000); assert(!fixture.ai.attacks && !fixture.ai.introPending && !fixture.ai.introDone && Conversation::removed == 1);
    }
    {
        Fixture fixture; Unit pet; fixture.ai.MoveInLineOfSight(&pet); fixture.ai.MoveInLineOfSight(nullptr);
        assert(!fixture.owner.conversations && !fixture.helya.casts);
        fixture.ai.UpdateIntro(1); Finish(fixture);
        assert(fixture.ai.attacked == &fixture.owner && !fixture.intruder.conversations);
    }
    for (uint32 invalid = 0; invalid < 4; ++invalid)
    {
        Fixture fixture;
        if (invalid == 0) fixture.actor.sightless = true;
        if (invalid == 1) fixture.actor.aggressive = false;
        if (invalid == 2) fixture.owner.flight = true;
        if (invalid == 3) fixture.owner.detectable = false;
        fixture.ai.UpdateIntro(1); assert(!fixture.owner.conversations);
    }
    {
        Fixture fixture; fixture.actor.helya = nullptr;
        fixture.ai.UpdateIntro(1); assert(!fixture.owner.conversations && warningRequests == 1);
        fixture.actor.helya = &fixture.helya;
        fixture.ai.TickVisual(999); fixture.ai.UpdateIntro(999); assert(!fixture.owner.conversations);
        fixture.ai.TickVisual(1); fixture.ai.UpdateIntro(1); assert(fixture.owner.acceptedConversations == 1 && !fixture.helya.casts);
        Finish(fixture); assert(fixture.ai.attacks == 1 && recoveryRequests == 1);
    }
    for (uint32 scope = 0; scope < 2; ++scope)
    {
        Fixture fixture; fixture.ai.introDone = true; fixture.actor.alive = false;
        if (scope) fixture.actor.mapId = 1220; else fixture.actor.entry = 98007;
        fixture.ai.Reset(); fixture.ai.UpdateIntro(1); fixture.ai.UpdateLifecycle(1);
        assert(fixture.actor.bubble);
    }
    std::cout << "PASS: extracted native intro creates843 without early195062, waits24000 then acknowledges a fresh normal Helya-to-Vigfus cast and waits7000 from actual acknowledgement. Actual FunctionProcessor header/methods additionally retain the nested7000ms wait after normal and31s/100s stalled ticks.56 stage/status,19 initial and42 delayed invalid-player contexts, original Helya identity/live/map/phase/range, failure/1s retry, reset/death/reentrant cleanup and exactly-once combat covered. World/cast services controlled; not client rendering/audio or full-chain certification.\n";
}
