#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
using uint32 = uint32_t;
using Difficulty = unsigned;
unsigned errors = 0;
#define TC_LOG_DEBUG(...) ((void)0)
#define TC_LOG_ERROR(...) (++errors)
#include "SmartEventFlags.inc"
struct Map { Difficulty difficulty; Difficulty GetDifficultyID() const { return difficulty; } };
struct WorldObject { Map map; uint32 GetEntry() const { return 14285; } Map* GetMap() { return &map; } };
struct AreaTriggerEntry { uint32 ID = 0; };
struct SmartScriptHolder {
    struct { uint32 event_flags = 0; } event;
    std::vector<Difficulty> Difficulties;
};
using SmartAIEventList = std::vector<SmartScriptHolder>;
struct SmartScript {
    SmartAIEventList mEvents;
    void FillScript(SmartAIEventList, WorldObject*, AreaTriggerEntry const*);
};
#include "FillScript.inc"
bool DeprecatedFlagsValid(SmartScriptHolder const& e) {
#include "DeprecatedFlags.inc"
    return true;
}
SmartAIEventList Load(char const* file) {
    std::ifstream input(file); assert(input);
    SmartAIEventList events; std::string line;
    while (std::getline(input, line)) {
        std::istringstream row(line); unsigned id, flags; std::string difficulties;
        row >> id >> flags >> difficulties;
        SmartScriptHolder event; event.event.event_flags = flags;
        if (difficulties != "-") {
            std::istringstream parts(difficulties); std::string part;
            while (std::getline(parts, part, ',')) event.Difficulties.push_back(std::stoul(part));
        }
        events.push_back(event);
    }
    assert(events.size() == 4); return events;
}
int main(int argc, char** argv) {
    assert(argc == 3);
    auto old = Load(argv[1]), repaired = Load(argv[2]);
    WorldObject actor{{0}}; SmartScript before;
    before.FillScript(old, &actor, nullptr);
    assert(before.mEvents.empty() && errors == 1); // Reproduce the logged error.
    for (Difficulty difficulty : {0u, 1u, 3u}) {
        actor.map.difficulty = difficulty; SmartScript after;
        for (auto const& event : repaired) assert(DeprecatedFlagsValid(event));
        after.FillScript(repaired, &actor, nullptr);
        assert(after.mEvents.size() == 4);
    }
    assert(errors == 1);
    assert(repaired[0].event.event_flags == 0);
    for (unsigned id = 1; id < 4; ++id) assert(repaired[id].event.event_flags == SMART_EVENT_FLAG_NOT_REPEATABLE);
    // An unrelated, intentionally restricted event must still be filtered.
    SmartScript restricted; actor.map.difficulty = 0;
    restricted.FillScript({old.front()}, &actor, nullptr);
    assert(restricted.mEvents.empty() && errors == 2);
    SmartScriptHolder invalid; invalid.event.event_flags = SMART_EVENT_FLAG_DIFFICULTY_0_DEPRECATED;
    assert(!DeprecatedFlagsValid(invalid) && errors == 3);
    std::cout << "PASS: production FillScript reproduces old difficulty-0 failure, loads all repaired events, preserves nonrepeatable flags and unrelated difficulty validation.\n";
}
