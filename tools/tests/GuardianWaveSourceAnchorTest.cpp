// Keep the earlier independent-DEATH/own-species regression and its original
// counterfactual separate from this new geometry/admission regression.
#define main GuardianWaveIntegrityMain
#include "GuardianWaveDeathTest.cpp"
#undef main

int main(int argc, char** argv)
{
    if (argc != 7)
        return 1;
    assert(GuardianWaveIntegrityMain(5, argv) == 0);
    auto actions = Actions(argv[2]);
    auto before = Rules(argv[5]), after = Rules(argv[6]);
    assert(after.size() == before.size() + 9);
    unsigned counterfactuals = 0, repaired = 0, fullRuns = 0;
    for (uint32 step = 2; step <= 5; ++step)
    {
        uint32 list = step == 2 ? 101742 : 10174198 + step;
        std::vector<uint32> wave;
        for (auto const& action : actions)
            if (action.source_type == 9 && action.entryOrGuid == list && action.actionType == 12)
                wave.push_back(action.parameter);
        assert(!wave.empty());
        std::set<uint32> species(wave.begin(), wave.end());
        for (uint32 actorEntry : species)
            for (uint32 killerType = 0; killerType < 3; ++killerType)
                for (bool castFailure : {false, true})
                {
                    for (bool fixed : {false, true})
                    {
                        Reset(step); conditionMgr.rules = fixed ? after : before;
                        failCast = castFailure;
                        Unit player; player.player = true; player.x = -40;
                        Unit owned; owned.owner = &player; owned.x = -40;
                        owned.entry = killerType == 1 ? 1 : 2; // controlled pet/guardian identity
                        Unit* killer = killerType ? &owned : &player;
                        Unit dying, remaining; dying.entry = actorEntry;
                        remaining.entry = *species.begin(); remaining.x = 80;
                        arena = {&player, &dying, &remaining};
                        auto script = Script(actions, dying);
                        Kill(&dying, killer, script);
                        assert(!script.errors && remaining.alive);
                        if (!fixed)
                        {
                            assert(criterionCalls == 1 && scenario.step == step + 1);
                            ++counterfactuals; // old killer->enemy120, source->enemy80
                            continue;
                        }
                        assert(!criterionCalls && scenario.step == step);
                        player.x = owned.x = 40; // legitimate last kill, within native50 recipient range
                        auto last = Script(actions, remaining);
                        Kill(&remaining, killer, last);
                        assert(!last.errors && criterionCalls == 1 && criterionRecipient == &player);
                        assert(criterionInvoker == killer && criterionAsset == 48104 + step);
                        Kill(&remaining, killer, last); assert(criterionCalls == 1);
                        ++repaired;
                    }
                }

        // Run every original species order with the repaired SQL, displaced
        // owner/pet/guardian and both immediate/deferred native stage updates.
        std::sort(wave.begin(), wave.end());
        do
        {
            for (uint32 killerType = 0; killerType < 3; ++killerType)
                for (bool castFailure : {false, true})
                    for (bool synchronous : {false, true})
                    {
                        Reset(step); conditionMgr.rules = after;
                        failCast = castFailure; advance = synchronous;
                        Unit player; player.player = true; player.x = -40;
                        Unit owned; owned.owner = &player; owned.x = -40;
                        Unit* killer = killerType ? &owned : &player;
                        std::vector<Unit> mobs(wave.size()); arena = {&player};
                        for (size_t i = 0; i < wave.size(); ++i)
                        { mobs[i].entry = wave[i]; arena.push_back(&mobs[i]); }
                        for (size_t i = 0; i < mobs.size(); ++i)
                        {
                            auto script = Script(actions, mobs[i]); Kill(&mobs[i], killer, script);
                            assert(!script.errors && criterionCalls == (i + 1 == mobs.size() ? 1u : 0u));
                        }
                        assert(criterionRecipient == &player && criterionInvoker == killer);
                        ++fullRuns;
                    }
        } while (std::next_permutation(wave.begin(), wave.end()));
    }
    // Changing the spatial anchor must not turn a null killer into admission.
    for (uint32 entry : {101384u, 101388u, 101389u})
        for (uint32 mode = 0; mode < 6; ++mode)
        {
            Reset(4); conditionMgr.rules = after;
            Unit player; player.player = true; Unit actor; actor.entry = entry;
            Unit blocker; blocker.entry = 101388; arena = {&player, &actor};
            Unit* killer = &player;
            if (mode == 0) killer = nullptr;
            if (mode == 1) player.x = 51;
            if (mode == 2) player.alive = false;
            if (mode >= 3)
            {
                blocker.x = 101;
                if (mode == 4) { blocker.x = 0; blocker.map = &other; }
                if (mode == 5) { blocker.x = 0; blocker.phase = 2; }
                arena.push_back(&blocker);
            }
            auto script = Script(actions, actor); Kill(&actor, killer, script);
            assert(!script.errors && criterionCalls == (mode >= 3 ? 1u : 0u));
        }
    std::printf("PASS Guardian source-anchor native regression: %u explicit old premature-credit counterfactuals paired with %u repaired last-death checks; %u full wave permutations/owner-pet-guardian/cast/stage runs. Native Unit-invoker admission, 100yd source bounds, map/phase and50yd recipient retained; client traversal separate.\n", counterfactuals, repaired, fullRuns);
    return 0;
}
