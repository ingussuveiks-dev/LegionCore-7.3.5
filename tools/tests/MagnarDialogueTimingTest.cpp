#define MARKSMANSHIP_INTERROGATION_TEST
#include "ConversationFarewellTest.cpp"

int main(int argc, char** argv)
{
    assert(argc == 3);
    std::ifstream input(argv[1]);
    std::string text;
    std::vector<ConversationCreature> definitions;
    std::vector<ConversationData> lines;
    while (std::getline(input, text))
    {
        std::istringstream row(text);
        std::string kind;
        row >> kind;
        if (kind == "creature")
        {
            ConversationCreature actor{};
            assert(row >> actor.entry >> actor.id >> actor.creatureId >> actor.creatureGuid
                >> actor.unk1 >> actor.unk2 >> actor.duration);
            definitions.push_back(actor);
        }
        else
        {
            assert(kind == "line");
            ConversationData line{};
            assert(row >> line.entry >> line.id >> line.idx >> line.textId >> line.unk1 >> line.unk2);
            lines.push_back(line);
        }
    }
    std::ifstream audio(argv[2]);
    std::map<uint32, std::pair<uint32, uint32>> durations;
    uint32 lineId, voiceMs, paddingMs;
    while (audio >> lineId >> voiceMs >> paddingMs)
        assert(durations.emplace(lineId, std::make_pair(voiceMs, paddingMs)).second);
    assert(definitions.size() == 4 && lines.size() == 7 && durations.size() == 7);
    Creature boss{96034}, companion{96468};
    Unit caster;
    caster.player = false;
    caster.guid = 96034;
    caster.nearby = {{96034, &boss}, {96468, &companion}};
    uint32 overlaps = 0, boundaries = 0;
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
        Conversation conversation;
        conversation.entry = identity;
        conversation.casterGUID = caster.guid;
        assert(conversation.Start(&caster));
        uint32 sequentialEnd = 0;
        for (auto const& line : store.lines)
        {
            auto const& payload = conversation.lines.at(line.idx);
            auto const timing = durations.at(line.id);
            assert(payload.id == line.id && payload.textId == line.textId
                && payload.unk1 == line.unk1 && payload.unk2 == line.unk2);
            if (line.idx)
            {
                ++boundaries;
                overlaps += payload.textId < sequentialEnd;
#ifndef TIMING_BASELINE
                assert(payload.textId == sequentialEnd);
#endif
            }
            sequentialEnd = payload.textId + timing.first + timing.second;
        }
#ifndef TIMING_BASELINE
        assert(sequentialEnd == (identity == 498 ? 29533u : 48286u));
        assert(conversation.deadline >= sequentialEnd + 5000);
#endif
    }
    assert(boundaries == 5);
#ifdef TIMING_BASELINE
    assert(overlaps == 5);
    std::cout << "BASELINE: five actual native line-payload boundaries precede the previous full26972 voice/gap end; not a captured client audio overlap.\n";
#else
    assert(!overlaps);
    std::cout << "PASS: five native serialized line starts follow complete26972 voices plus native gaps; original line/actor/index/camera/packed flags and35300/54550 visibility budgets retained. Actual dialogue/actor/stage/client playback remains separate.\n";
#endif
}
