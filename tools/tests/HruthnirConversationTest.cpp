#define MARKSMANSHIP_INTERROGATION_TEST
#include "ConversationFarewellTest.cpp"

int main(int argc, char** argv)
{
    _set_abort_behavior(0, _CALL_REPORTFAULT);
    assert(argc == 2);
    store.identity = 497;
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
            assert(value.entry == 497);
            store.creatures.push_back(value);
        }
        else
        {
            assert(kind == "line");
            ConversationData value{};
            assert(row >> value.entry >> value.id >> value.idx >> value.textId >> value.unk1 >> value.unk2);
            assert(value.entry == 497);
            store.lines.push_back(value);
        }
    }
    assert(store.creatures.size() == 3 && store.lines.size() == 10 && store.actors.empty());
    Creature companion{96468};
    Creature boss{96034};
    Unit caster;
    caster.player = false;
    caster.guid = 96468;
    caster.nearby = {{96468, &companion}, {96034, &boss}};
    auto repairedCreatures = store.creatures;
    store.creatures[0].duration = 0;
    Unit viewer;
    Conversation baseline;
    baseline.entry = 497;
    baseline.casterGUID = caster.guid;
    baseline.playing.clear();
    assert(baseline.Start(&caster) && baseline.deadline == 35000);
    currentTime = 100;
    assert(!baseline.CanNeverSee2(&viewer));
    currentTime += 35000;
    assert(!baseline.CanNeverSee2(&viewer));
    ++currentTime;
    baseline.Update(1);
    assert(!baseline.removed && baseline.CanNeverSee2(&viewer));
    assert(baseline.deadline < store.lines[8].textId && baseline.deadline < store.lines[9].textId);
    store.creatures = repairedCreatures;
#ifdef BASELINE
    store.creatures[0].duration = 0;
#endif
    Conversation repaired;
    repaired.entry = 497;
    repaired.casterGUID = caster.guid;
    repaired.playing.clear();
    assert(repaired.Start(&caster));
    assert(repaired.deadline >= 67992 + 5000);
    assert(repaired.deadline == 73000 && repaired.lifetime == 73000);
    assert(repaired.actors.empty() && repaired.creatures.size() == 3 && repaired.lines.size() == 10);
    assert(repaired.creatures.at(0).GuidLow == 96468 && repaired.creatures.at(1).GuidLow == 96034);
    assert(repaired.creatures.at(2).GuidLow == 0xFFFFFFFFFF);
    for (auto const& actor : store.creatures)
    {
        auto const& payload = repaired.creatures.at(actor.id);
        assert(payload.unk1 == actor.unk1 && payload.unk2 == actor.unk2 && payload.GuidHight == 1);
    }
    for (auto const& line : store.lines)
    {
        auto const& payload = repaired.lines.at(line.idx);
        assert(payload.id == line.id && payload.textId == line.textId
            && payload.unk1 == line.unk1 && payload.unk2 == line.unk2);
    }
    currentTime = 100000;
    assert(!repaired.CanNeverSee2(&viewer));
    Unit secondViewer;
    secondViewer.guid = 43;
    currentTime += 1000;
    assert(!repaired.CanNeverSee2(&secondViewer));
    for (uint32 checkpoint : {35001u, 40000u, 41000u, 46672u, 67992u, 72992u, 73000u})
    {
        currentTime = 100000 + checkpoint;
        repaired.Update(1);
        assert(!repaired.removed && !repaired.CanNeverSee2(&viewer));
    }
    ++currentTime;
    repaired.Update(1);
    assert(!repaired.removed && repaired.CanNeverSee2(&viewer));
    assert(!repaired.CanNeverSee2(&secondViewer));
    currentTime += 1000;
    assert(repaired.CanNeverSee2(&secondViewer));
    assert(repaired.CanNeverSee2(&caster) && !repaired.playing.count(caster.guid));
    repaired.questExpired = true;
    repaired.Update(1);
    assert(repaired.removed);
    for (uint32 identity : {96468u, 96034u})
    {
        Unit unavailable = caster;
        unavailable.nearby.erase(identity);
        Conversation rejected;
        rejected.entry = 497;
        assert(!rejected.Start(&unavailable));
        Conversation retry;
        retry.entry = 497;
        assert(retry.Start(&caster) && retry.deadline == 73000);
    }
    Conversation missing;
    missing.entry = 497;
    store.creatures.clear();
    assert(!missing.Start(&caster));
    store.creatures = repairedCreatures;
    store.lines.clear();
    assert(!missing.Start(&caster));
    std::cout << "PASS: actual497 generation/lifetime/visibility/update bodies reproduce35001ms per-player cutoff before40000/41000 lines;73000ms retains46672ms absolute and67992ms conservative sequential native voice/gap budgets plus5000ms. All3 actor/10 line payloads, player sentinel, independent late viewer, missing actor retry and scope cleanup preserved. NPC-caster object persistence remains unchanged; controlled lookup/clock/remove, not client playback.\n";
}
