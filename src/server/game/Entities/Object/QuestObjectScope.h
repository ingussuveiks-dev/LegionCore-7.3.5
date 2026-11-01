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

    // A deferred story can distinguish explicit cancellation from logout
    // without keeping this scope (or its ordinary children) alive.
    std::shared_ptr<std::atomic<bool> const> CancellationReceipt() const
    {
        return _cancellationReceipt;
    }

    void Register(ObjectGuid const& guid)
    {
        std::lock_guard<std::mutex> lock(Mutex);
        if (!Cancelled)
            Objects.push_back(guid);
    }

    std::vector<ObjectGuid> Cancel()
    {
        std::lock_guard<std::mutex> lock(Mutex);
        _cancellationReceipt->store(true, std::memory_order_release);
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
    std::shared_ptr<std::atomic<bool>> _cancellationReceipt = std::make_shared<std::atomic<bool>>(false);
};

#endif
