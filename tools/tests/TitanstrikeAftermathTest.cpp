#define MARKSMANSHIP_INTERROGATION_TEST
#include "ConversationFarewellTest.cpp"

int main(int argc, char** argv)
{
    _set_abort_behavior(0, _CALL_REPORTFAULT);
    assert(argc == 2);
    std::map<uint32, Store> definitions;
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
            definitions[value.entry].identity = value.entry;
            definitions[value.entry].actors.push_back(value);
        }
        else
        {
            assert(kind == "line");
            ConversationData value{};
            assert(row >> value.entry >> value.id >> value.idx >> value.textId >> value.unk1 >> value.unk2);
            definitions[value.entry].identity = value.entry;
            definitions[value.entry].lines.push_back(value);
        }
    }
    assert(definitions.size() == 2);
    Unit viewer;
    viewer.nearby.clear();
    for (uint32 identity : {2167u, 2173u})
    {
        store = definitions.at(identity);
        bool aftermath = identity == 2167;
        uint32 originalDuration = aftermath ? 9400 : 3700;
        uint32 originalDeadline = aftermath ? 33200 : 12400;
        uint32 requiredEnd = aftermath ? 67398 : 56364;
        uint32 repairedDeadline = aftermath ? 73800 : 63700;
        assert(store.lines.size() == 5 && store.actors.size() == (aftermath ? 3u : 2u));
#ifdef BASELINE
        store.actors[0].duration = originalDuration;
#endif
        for (bool npcCaster : {false, true})
        {
            Unit caster;
            caster.player = !npcCaster;
            caster.guid = npcCaster ? 103154 : 42;
            Conversation repaired;
            repaired.entry = identity;
            repaired.casterGUID = caster.guid;
            repaired.playing.clear();
            assert(repaired.Start(&caster));
            assert(repaired.deadline >= requiredEnd + 5000);
            assert(repaired.deadline == repairedDeadline && repaired.lifetime == repairedDeadline);
            assert(repaired.actors.size() == store.actors.size() && repaired.lines.size() == 5 && repaired.creatures.empty());
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
            currentTime = 1000000;
            assert(!repaired.CanNeverSee2(&viewer));
            for (uint32 checkpoint : {originalDeadline + 1, store.lines.back().textId, requiredEnd, requiredEnd + 5000, repairedDeadline})
            {
                currentTime = 1000000 + checkpoint;
                repaired.Update(1);
                assert(!repaired.removed && !repaired.CanNeverSee2(&viewer));
            }
            ++currentTime;
            repaired.Update(1);
            assert(repaired.removed == !npcCaster && repaired.CanNeverSee2(&viewer));
            if (npcCaster)
            {
                Unit lateViewer;
                lateViewer.guid = 43;
                assert(!repaired.CanNeverSee2(&lateViewer));
            }
            repaired.questExpired = true;
            repaired.Update(1);
            assert(repaired.removed);
        }
        store.actors[0].duration = originalDuration;
        Conversation baseline;
        baseline.entry = identity;
        baseline.playing.clear();
        assert(baseline.Start(&viewer) && baseline.deadline == originalDeadline);
        currentTime = 2000000;
        assert(!baseline.CanNeverSee2(&viewer));
        currentTime += originalDeadline + 1;
        baseline.Update(1);
        assert(baseline.removed && baseline.CanNeverSee2(&viewer) && baseline.deadline < store.lines.back().textId);
        store = definitions.at(identity);
        store.actors.clear();
        Conversation missing;
        missing.entry = identity;
        assert(!missing.Start(&viewer));
        store = definitions.at(identity);
        store.lines.clear();
        assert(!missing.Start(&viewer));
    }
    std::cout << "PASS: native2167/2173 payloads and player/NPC-caster deadlines reproduce33200/12400 truncation;73800/63700 retain conservative voice/gap/padding and exact boundaries, scope cleanup, missing definitions and native late-viewer behavior; controlled store/clock, not live playback.\n";
}
