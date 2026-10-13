#ifndef TRINITY_QUEST_OBJECT_SCOPE_H
#define TRINITY_QUEST_OBJECT_SCOPE_H

#include "ObjectGuid.h"
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

// One accepted quest attempt owns the scope. Summons hold weak references:
// descendants remain cancellable even after their immediate summoner despawns.
struct QuestObjectScope
{
    ObjectGuid Owner;
    std::atomic<bool> Cancelled{false};

    void Register(ObjectGuid const& guid)
    {
        std::lock_guard<std::mutex> lock(Mutex);
        if (!Cancelled)
            Objects.push_back(guid);
    }

    std::vector<ObjectGuid> Cancel()
    {
        std::lock_guard<std::mutex> lock(Mutex);
        Cancelled = true;
        std::vector<ObjectGuid> objects;
        objects.swap(Objects);
        return objects;
    }

private:
    // A player can leave the map while a surviving child creates descendants.
    // Never iterate a registry that another map's update thread can append to.
    std::mutex Mutex;
    std::vector<ObjectGuid> Objects;
};

#endif
