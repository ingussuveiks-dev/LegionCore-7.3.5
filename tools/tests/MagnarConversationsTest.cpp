#define MARKSMANSHIP_INTERROGATION_TEST
#include "ConversationFarewellTest.cpp"

int main(int argc, char** argv)
{
    _set_abort_behavior(0, _CALL_REPORTFAULT);
    assert(argc == 2);
    std::vector<ConversationCreature> definitions;
    std::vector<ConversationData> lines;
    std::ifstream input(argv[1]);
    std::string text;
    while (std::getline(input, text))
    {
        std::istringstream row(text);
        std::string kind;
        row >> kind;
        if (kind == "creature")
        {
            ConversationCreature value{};
            assert(row >> value.entry >> value.id >> value.creatureId >> value.creatureGuid
                >> value.unk1 >> value.unk2 >> value.duration);
            definitions.push_back(value);
        }
        else
        {
            assert(kind == "line");
            ConversationData value{};
            assert(row >> value.entry >> value.id >> value.idx >> value.textId >> value.unk1 >> value.unk2);
            lines.push_back(value);
        }
    }
    assert(definitions.size() == 4 && lines.size() == 7);
    Creature boss{96034};
    Creature companion{96468};
    Unit caster;
    caster.player = false;
    caster.guid = 96034;
    caster.nearby = {{96034, &boss}, {96468, &companion}};
    Unit viewer;
    Unit secondViewer;
    secondViewer.guid = 43;
    for (uint32 identity : {498u, 504u})
    {
        store.identity = identity;
        store.creatures.clear();
        store.lines.clear();
        for (auto const& actor : definitions)
            if (actor.entry == identity)
                store.creatures.push_back(actor);
        for (auto const& line : lines)
            if (line.entry == identity)
                store.lines.push_back(line);
        uint32 baselineDuration = identity == 498 ? 4300 : 7550;
        uint32 baselineDeadline = identity == 498 ? 13600 : 20100;
        uint32 fixedDeadline = identity == 498 ? 35300 : 54550;
        uint32 requiredEnd = identity == 498 ? 29533 : 48286;
        assert(store.creatures.size() == 2 && store.lines.size() == (identity == 498 ? 3 : 4));
        auto repaired = store.creatures;
        store.creatures[0].duration = baselineDuration;
        Conversation baseline;
        baseline.entry = identity;
        baseline.casterGUID = caster.guid;
        baseline.playing.clear();
        assert(baseline.Start(&caster) && baseline.deadline == baselineDeadline);
        currentTime = 100;
        assert(!baseline.CanNeverSee2(&viewer));
        currentTime += baselineDeadline;
        assert(!baseline.CanNeverSee2(&viewer));
        ++currentTime;
        baseline.Update(1);
        assert(!baseline.removed && baseline.CanNeverSee2(&viewer));
        assert(baseline.deadline < requiredEnd);
        if (identity == 504)
            assert(baseline.deadline < store.lines.back().textId);
        store.creatures = repaired;
#ifdef BASELINE
        if (identity == BASELINE)
            store.creatures[0].duration = baselineDuration;
#endif
        Conversation fixed;
        fixed.entry = identity;
        fixed.casterGUID = caster.guid;
        fixed.playing.clear();
        assert(fixed.Start(&caster));
        assert(fixed.deadline >= requiredEnd + 5000);
        assert(fixed.deadline == fixedDeadline && fixed.lifetime == fixedDeadline);
        assert(fixed.actors.empty() && fixed.creatures.size() == 2 && fixed.lines.size() == store.lines.size());
        assert(fixed.creatures.at(0).GuidLow == 96034 && fixed.creatures.at(1).GuidLow == 96468);
        for (auto const& actor : store.creatures)
        {
            auto const& payload = fixed.creatures.at(actor.id);
            assert(payload.unk1 == actor.unk1 && payload.unk2 == actor.unk2 && payload.GuidHight == 1);
        }
        for (auto const& line : store.lines)
        {
            auto const& payload = fixed.lines.at(line.idx);
            assert(payload.id == line.id && payload.textId == line.textId
                && payload.unk1 == line.unk1 && payload.unk2 == line.unk2);
        }
        currentTime = 100000;
        assert(!fixed.CanNeverSee2(&viewer));
        currentTime += 1000;
        assert(!fixed.CanNeverSee2(&secondViewer));
        for (uint32 checkpoint : {baselineDeadline + 1, requiredEnd, requiredEnd + 5000, fixedDeadline})
        {
            currentTime = 100000 + checkpoint;
            fixed.Update(1);
            assert(!fixed.removed && !fixed.CanNeverSee2(&viewer));
        }
        ++currentTime;
        fixed.Update(1);
        assert(!fixed.removed && fixed.CanNeverSee2(&viewer) && !fixed.CanNeverSee2(&secondViewer));
        currentTime += 1000;
        assert(fixed.CanNeverSee2(&secondViewer) && fixed.CanNeverSee2(&caster));
        fixed.questExpired = true;
        fixed.Update(1);
        assert(fixed.removed);
        for (uint32 actor : {96034u, 96468u})
        {
            Unit unavailable = caster;
            unavailable.nearby.erase(actor);
            Conversation missing;
            missing.entry = identity;
            assert(!missing.Start(&unavailable));
            Conversation retry;
            retry.entry = identity;
            assert(retry.Start(&caster) && retry.deadline == fixedDeadline);
        }
        Conversation missing;
        missing.entry = identity;
        store.creatures.clear();
        assert(!missing.Start(&caster));
        store.creatures = repaired;
        store.lines.clear();
        assert(!missing.Start(&caster));
    }
    std::cout << "PASS: actual498/504 creature/line factory-budget/visibility/update bodies reproduce13601/20101ms per-player truncation.35300/54550ms retain29533/48286ms native conservative voice/gap budgets plus5000ms. Seven line/four actor payloads, late viewers, missing actors/retry and scope cleanup preserved. Controlled map/clock/remove; existing6000ms boss despawn, rapid dialogue ordering, NPC-caster persistence and client playback are not repaired or simulated.\n";
}
