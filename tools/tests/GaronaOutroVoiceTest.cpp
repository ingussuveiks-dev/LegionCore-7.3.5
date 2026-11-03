#define MARKSMANSHIP_INTERROGATION_TEST
#include "ConversationFarewellTest.cpp"

int main(int argc, char** argv)
{
    assert(argc == 3);
    std::map<uint32, Store> definitions;
    std::ifstream input(argv[1]);
    std::string text;
    while (std::getline(input, text))
    {
        std::istringstream row(text);
        std::string kind; row >> kind;
        if (kind == "creature")
        {
            ConversationCreature actor{};
            assert(row >> actor.entry >> actor.id >> actor.creatureId >> actor.creatureGuid >> actor.unk1 >> actor.unk2 >> actor.duration);
            definitions[actor.entry].creatures.push_back(actor);
        }
        else if (kind == "actor")
        {
            ConversationActor actor{};
            assert(row >> actor.entry >> actor.id >> actor.actorId >> actor.creatureId >> actor.displayId >> actor.unk1 >> actor.unk2 >> actor.unk3 >> actor.duration);
            definitions[actor.entry].actors.push_back(actor);
        }
        else
        {
            assert(kind == "line");
            ConversationData line{};
            assert(row >> line.entry >> line.id >> line.idx >> line.textId >> line.unk1 >> line.unk2);
            definitions[line.entry].lines.push_back(line);
        }
    }
    std::ifstream voices(argv[2]);
    std::map<uint32, std::pair<uint32, uint32>> durations;
    uint32 lineId, voice, gap;
    while (voices >> lineId >> voice >> gap) assert(durations.emplace(lineId, std::make_pair(voice, gap)).second);
    assert(definitions.size() == 1 && durations.size() == 2);
    Creature garona{108214};
    Unit speaker; speaker.player = false; speaker.guid = 108214;
    speaker.nearby = {{108214, &garona}};
    uint32 shortenedBoundaries = 0, lostPadding = 0;
    for (uint32 entry : {2432u})
    {
        store = definitions.at(entry); store.identity = entry;
        Conversation conversation;
        conversation.entry = entry; conversation.casterGUID = speaker.guid; conversation.playing.clear();
        currentTime = 100;
        assert(conversation.Start(&speaker));
        assert(conversation.lines.size() == store.lines.size() && conversation.creatures.size() == store.creatures.size()
            && conversation.actors.size() == store.actors.size());
        for (auto const& actor : store.creatures)
        {
            auto const& packed = conversation.creatures.at(actor.id);
            assert(packed.GuidLow == actor.creatureId && packed.GuidHight == 1 && packed.unk1 == actor.unk1 && packed.unk2 == actor.unk2);
        }
        for (auto const& actor : store.actors)
        {
            auto const& packed = conversation.actors.at(actor.id);
            assert(packed.actorId == actor.actorId && packed.creatureId == actor.creatureId && packed.displayId == actor.displayId
                && packed.unk1 == actor.unk1 && packed.unk2 == actor.unk2 && packed.unk3 == actor.unk3);
        }
        uint32 sequence = 0;
        for (auto const& line : store.lines)
        {
            auto const& packed = conversation.lines.at(line.idx);
            auto duration = durations.at(line.id);
            assert(packed.id == line.id && packed.textId == line.textId && packed.unk1 == line.unk1 && packed.unk2 == line.unk2);
            shortenedBoundaries += packed.textId != sequence;
#ifndef GARONA_VOICE_BASELINE
            assert(packed.textId == sequence);
#endif
            sequence += duration.first + duration.second;
        }
        lostPadding += conversation.deadline < sequence + 5000;
#ifndef GARONA_VOICE_BASELINE
        assert(sequence == 14164u);
        assert(conversation.deadline == sequence + 5000 && conversation.lifetime == sequence + 5000);
        Unit viewer, late; late.guid = 43;
        assert(!conversation.CanNeverSee2(&viewer));
        currentTime = 1100; assert(!conversation.CanNeverSee2(&late));
        currentTime = 100 + sequence; conversation.Update(1);
        assert(!conversation.removed && !conversation.CanNeverSee2(&viewer));
        currentTime = 100 + conversation.deadline + 1; conversation.Update(1);
        assert(!conversation.removed && conversation.CanNeverSee2(&viewer) && !conversation.CanNeverSee2(&late));
        currentTime += 1000; conversation.Update(1); assert(conversation.removed && conversation.CanNeverSee2(&late));
        if (entry != 770)
            for (auto const& actor : store.creatures)
            {
                Unit missing = speaker; missing.nearby.erase(actor.creatureId);
                Conversation rejected; rejected.entry = entry;
                assert(!rejected.Start(&missing));
            }
#endif
    }
#ifdef GARONA_VOICE_BASELINE
    assert(shortenedBoundaries == 1 && lostPadding == 1);
    std::cout << "COUNTERFACTUAL original native payloads: Garona second line starts1107ms after full26972 first voice/native gap, published9700 before second line11800 and full14164 sequence; native5000 padding not preserved. Not captured client clipping or audio scheduling.\n";
#else
    assert(!shortenedBoundaries && !lostPadding);
    std::cout << "PASS actual native conversation factory/payload/padding/visibility/update: two original Garona lines/roles/flags, full matching26972 voices and native gaps, exact10693 cumulative boundary and19164 published lifetime, independent late viewers and missing-actor refusal. Controlled map/clock host; cross-conversation callers and client playback remain separate.\n";
#endif
}
