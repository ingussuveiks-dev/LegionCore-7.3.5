#include <algorithm>
#define MARKSMANSHIP_INTERROGATION_TEST
#include "ConversationFarewellTest.cpp"

int main(int argc, char** argv)
{
    _set_abort_behavior(0, _CALL_REPORTFAULT);
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
    for (auto& context : definitions) std::sort(context.second.lines.begin(), context.second.lines.end(), [](ConversationData const& a, ConversationData const& b) { return a.idx < b.idx; });
    assert(definitions.size() == 2 && durations.size() == 10);
    Creature garona{108015}, shaw{98094};
    Unit speaker; speaker.player = true;
    speaker.nearby = {{108015, &garona}, {98094, &shaw}};
    uint32 shortenedBoundaries = 0, lostPadding = 0, afterDeadline = 0;
    for (uint32 entry : {2361u, 2377u})
    {
        store = definitions.at(entry); store.identity = entry;
        Conversation conversation;
        conversation.entry = entry; conversation.casterGUID = speaker.guid; conversation.playing.clear();
        currentTime = 100;
        assert(conversation.Start(&speaker));
        assert(conversation.lines.size() == store.lines.size() && conversation.creatures.size() == store.creatures.size() && conversation.actors.empty());
        for (auto const& actor : store.creatures)
        {
            auto const& packed = conversation.creatures.at(actor.id);
            assert(packed.GuidLow == actor.creatureId && packed.GuidHight == 1 && packed.unk1 == actor.unk1 && packed.unk2 == actor.unk2);
        }
        uint32 sequence = 0;
        for (auto const& line : store.lines)
        {
            auto const& packed = conversation.lines.at(line.idx);
            auto duration = durations.at(line.id);
            assert(packed.id == line.id && packed.textId == line.textId && packed.unk1 == line.unk1 && packed.unk2 == line.unk2);
            shortenedBoundaries += packed.textId != sequence;
            afterDeadline += packed.textId >= conversation.deadline;
#ifndef KINGSLAYERS_VOICE_BASELINE
            assert(packed.textId == sequence);
#endif
            sequence += duration.first + duration.second;
        }
        lostPadding += conversation.deadline < sequence + 5000;
#ifdef KINGSLAYERS_VOICE_BASELINE
        assert(conversation.deadline == (entry == 2361 ? 16700u : 19600u));
        // Drive the actual player-caster removal clock beyond its short budget.
        Unit viewer;
        assert(!conversation.CanNeverSee2(&viewer));
        currentTime += conversation.deadline + 1; conversation.Update(1);
        assert(conversation.removed && conversation.CanNeverSee2(&viewer));
#else
        assert(sequence == (entry == 2361 ? 35840u : 42679u));
        assert(conversation.deadline == sequence + 5000 && conversation.lifetime == sequence + 5000);
        Unit viewer, late; late.guid = 43;
        assert(!conversation.CanNeverSee2(&viewer));
        currentTime = 1100; assert(!conversation.CanNeverSee2(&late));
        for (auto const& line : store.lines)
        {
            auto duration = durations.at(line.id);
            for (uint32 point : {line.textId, line.textId + duration.first, line.textId + duration.first + duration.second})
            {
                currentTime = 100 + point; conversation.Update(1);
                assert(!conversation.removed && !conversation.CanNeverSee2(&viewer));
            }
        }
        currentTime = 100 + conversation.deadline; conversation.Update(1);
        assert(!conversation.removed && !conversation.CanNeverSee2(&viewer));
        currentTime++; conversation.Update(1);
        assert(conversation.removed && conversation.CanNeverSee2(&viewer) && !conversation.CanNeverSee2(&late));
        // Native Player ownership retires this shared object on the caster's
        // clock even if another group viewer started later. Keep that limit visible.
        for (uint32 missing : {108015u, 98094u})
        {
            if (entry == 2377 && missing == 98094) continue;
            Unit unavailable = speaker; unavailable.nearby.erase(missing);
            Conversation refused; refused.entry = entry; assert(!refused.Start(&unavailable));
        }
        Conversation cancelled; cancelled.entry = entry; currentTime = 100;
        assert(cancelled.Start(&speaker)); cancelled.questExpired = true; cancelled.Update(1); assert(cancelled.removed);
#endif
    }
#ifdef KINGSLAYERS_VOICE_BASELINE
    assert(shortenedBoundaries == 8 && lostPadding == 2 && afterDeadline == 3);
    std::cout << "COUNTERFACTUAL original2361/2377 native payloads:8 cumulative voice/gap boundaries differ;16700/19600 lifetimes precede3 last-line starts. Actual player-clock removal reproduced; no captured client/audio claim.\n";
#else
    assert(!shortenedBoundaries && !lostPadding && !afterDeadline);
    std::cout << "PASS actual native factory/dynamic payload/player visibility/update under ASAN:10 original roles/IDs/flags, complete26972 voices/native gaps and5000 padding;40840/47679 lifetimes, normal expiry, cancellation and missing-actor refusal. Player-caster clock, late-group-viewer and cross-caller/client limits remain explicit.\n";
#endif
}
