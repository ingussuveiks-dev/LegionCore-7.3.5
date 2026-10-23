#ifndef LEGION_TITANSTRIKE_RUNE_WAVE_H
#define LEGION_TITANSTRIKE_RUNE_WAVE_H

#include <array>
#include <cstddef>
#include <cstdint>

namespace TitanstrikeRunes
{
struct Enemy
{
    uint32_t entry;
    float x, y, z;
    int parent; // final wolves belong to the two trackers, not the player
};

inline std::array<Enemy, 5> Layout(unsigned wave)
{
    Enemy guard{106335, 4888.34f, 315.806f, -36.3445f, -1};
    if (wave == 0)
        return {{guard, {}, {}, {}, {}}};
    if (wave == 1)
        return {{{106335, 4887.54f, 312.818f, -36.3468f, -1},
                 {106335, 4888.9f, 318.578f, -36.3436f, -1}, {}, {}, {}}};
    return {{{106341, 4887.52f, 312.828f, -36.3467f, -1},
             {106341, 4888.94f, 318.653f, -36.3437f, -1}, guard,
             {106344, 4886.52f, 312.828f, -36.3467f, 0},
             {106344, 4887.94f, 318.653f, -36.3437f, 1}}};
}

enum class Life { Missing, Alive, Dead };

// Count actual successfully spawned actors, independent of the killer, range,
// summon order and transient SmartAI death notifications. Missing actors retry;
// they are never counted as defeated. Each completed wave is emitted once.
template<class Guid> class Wave
{
public:
    unsigned index = 0;
    bool started = false, armed = false;
    std::array<Guid, 5> guids{};
    std::array<bool, 5> dead{};

    void Start() { *this = Wave(); started = true; }
    void Arm(unsigned number) { if (started && index < 3 && number == index + 1) armed = true; }
    std::size_t Count() const { return index == 0 ? 1 : index == 1 ? 2 : 5; }

    template<class Spawn, class Lookup> bool Update(Spawn spawn, Lookup lookup)
    {
        if (!started || !armed || index >= 3)
            return false;
        auto layout = Layout(index);
        bool complete = true;
        for (std::size_t i = 0; i < Count(); ++i)
        {
            if (dead[i])
                continue;
            if (guids[i] != Guid())
            {
                auto state = lookup(guids[i]);
                if (state == Life::Dead)
                {
                    dead[i] = true;
                    continue;
                }
                if (state == Life::Missing)
                    guids[i] = Guid();
            }
            if (guids[i] == Guid())
            {
                Guid parent{};
                if (layout[i].parent >= 0)
                {
                    parent = guids[layout[i].parent];
                    if (parent == Guid())
                    {
                        complete = false;
                        continue;
                    }
                }
                guids[i] = spawn(layout[i], parent);
            }
            complete = false; // even a failed summon must block completion
        }
        if (!complete)
            return false;
        ++index;
        armed = false;
        guids = {};
        dead = {};
        return true;
    }
};
}
#endif
