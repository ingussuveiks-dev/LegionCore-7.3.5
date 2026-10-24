#define MARKSMANSHIP_INTERROGATION_TEST
#include "ConversationFarewellTest.cpp"

int main(int argc, char** argv)
{
    _set_abort_behavior(0, _CALL_REPORTFAULT);
    assert(argc == 2);
    store.identity = 2147;
    std::ifstream input(argv[1]);
    std::string text;
    while (std::getline(input, text))
    {
        std::istringstream row(text);
        std::string kind;
        row >> kind;
        if (kind == "actor")
        {
            ConversationActor value{};
            assert(row >> value.entry >> value.id >> value.actorId >> value.creatureId >> value.displayId
                >> value.unk1 >> value.unk2 >> value.unk3 >> value.duration);
            assert(value.entry == 2147);
            store.actors.push_back(value);
        }
        else
        {
            assert(kind == "line");
            ConversationData value{};
            assert(row >> value.entry >> value.id >> value.idx >> value.textId >> value.unk1 >> value.unk2);
            assert(value.entry == 2147);
            store.lines.push_back(value);
        }
    }
    assert(store.actors.size() == 3 && store.lines.size() == 3);
    Unit finder;
    finder.nearby.clear();
    auto repairedActors = store.actors;
#ifdef BASELINE
    store.actors[0].duration = 3550;
#endif
    Conversation repaired;
    repaired.entry = 2147;
    assert(repaired.Start(&finder));
    assert(repaired.deadline >= 20076 + 5000);
    assert(repaired.deadline == 27100 && repaired.lifetime == 27100);
    assert(repaired.creatures.empty() && repaired.actors.size() == 3 && repaired.lines.size() == 3);
    for (auto const& actor : store.actors)
    {
        auto const& payload = repaired.actors.at(actor.id);
        assert(payload.actorId == actor.actorId && payload.creatureId == actor.creatureId &&
            payload.displayId == actor.displayId && payload.unk1 == actor.unk1 &&
            payload.unk2 == actor.unk2 && payload.unk3 == actor.unk3);
    }
    for (auto const& line : store.lines)
    {
        auto const& payload = repaired.lines.at(line.idx);
        assert(payload.id == line.id && payload.textId == line.textId &&
            payload.unk1 == line.unk1 && payload.unk2 == line.unk2);
    }
    for (uint32 checkpoint : {15651u, 15700u, 20076u, 25076u, 27100u})
    {
        currentTime = 100 + checkpoint;
        repaired.Update(1);
        assert(!repaired.removed);
    }
    currentTime = 100 + 27101;
    repaired.Update(1);
    assert(repaired.removed);
    store.actors[0].duration = 3550;
    Conversation baseline;
    baseline.entry = 2147;
    assert(baseline.Start(&finder) && baseline.deadline == 15650);
    currentTime = 100 + 15651;
    baseline.Update(1);
    assert(baseline.removed && baseline.deadline < store.lines.back().textId);
    store.actors = repairedActors;
    Conversation scoped;
    scoped.entry = 2147;
    assert(scoped.Start(&finder));
    scoped.questExpired = true;
    scoped.Update(1);
    assert(scoped.removed);
    Conversation replay;
    replay.entry = 2147;
    replay.playing.clear();
    currentTime = 999999;
    assert(replay.Start(&finder));
    assert(!replay.CanNeverSee2(&finder) && replay.playing.at(42) == currentTime);
    currentTime += 27100;
    replay.Update(1);
    assert(!replay.removed);
    ++currentTime;
    replay.Update(1);
    assert(replay.removed);
    Unit npcCaster;
    npcCaster.player = false;
    npcCaster.guid = 106730;
    store.actors[0].duration = 3550;
    Conversation npcBaseline;
    npcBaseline.entry = 2147;
    npcBaseline.casterGUID = npcCaster.guid;
    npcBaseline.playing.clear();
    assert(npcBaseline.Start(&npcCaster));
    currentTime = 900000;
    assert(!npcBaseline.CanNeverSee2(&finder));
    currentTime += 15651;
    npcBaseline.Update(1);
    assert(!npcBaseline.removed && npcBaseline.CanNeverSee2(&finder));
    store.actors = repairedActors;
    Conversation npcConversation;
    npcConversation.entry = 2147;
    npcConversation.casterGUID = npcCaster.guid;
    npcConversation.playing.clear();
    assert(npcConversation.Start(&npcCaster));
    currentTime = 1000000;
    assert(!npcConversation.CanNeverSee2(&finder));
    Unit secondViewer;
    secondViewer.guid = 43;
    currentTime += 1000;
    assert(!npcConversation.CanNeverSee2(&secondViewer));
    currentTime = 1000000 + 20076;
    assert(!npcConversation.CanNeverSee2(&finder));
    currentTime = 1000000 + 27101;
    npcConversation.Update(1);
    assert(!npcConversation.removed);
    assert(npcConversation.CanNeverSee2(&finder));
    assert(!npcConversation.CanNeverSee2(&secondViewer));
    currentTime += 1000;
    assert(npcConversation.CanNeverSee2(&secondViewer));
    assert(npcConversation.CanNeverSee2(&npcCaster));
    Conversation missing;
    missing.entry = 2147;
    store.actors.clear();
    assert(!missing.Start(&finder));
    store.actors = repairedActors;
    store.lines.clear();
    assert(!missing.Start(&finder));
    std::cout << "PASS: native2147 actor/line payloads, 15650ms baseline truncation, 27100ms padded lifetime, player-caster expiry, NPC-caster independent viewer deadlines, replay and quest-scope cleanup; controlled lookup/store/clock, not client playback.\n";
}
