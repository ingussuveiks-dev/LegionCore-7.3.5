#ifndef TRINITY_QUEST_OBJECT_SCOPE_H
#define TRINITY_QUEST_OBJECT_SCOPE_H

#include "ObjectGuid.h"
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

class QuestCancellationReceipt
{
public:
    bool load(std::memory_order order=std::memory_order_seq_cst) const { return _cancelled.load(order); }
private:
    friend struct QuestObjectScope;
    bool Link(std::shared_ptr<QuestCancellationReceipt const> const& previous) const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if(_cancelled.load(std::memory_order_acquire))return false;
        for(auto it=_mirrors.begin();it!=_mirrors.end();)
        {
            auto current=it->lock();
            if(!current)it=_mirrors.erase(it);
            else{if(current==previous)return true;++it;}
        }
        _mirrors.push_back(previous);return true;
    }
    void Collect(std::vector<std::shared_ptr<QuestCancellationReceipt const>>& pending) const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        for(auto const& weak:_mirrors)if(auto receipt=weak.lock())pending.push_back(std::move(receipt));
    }
    void Cancel() const
    {
        if(_cancelled.exchange(true,std::memory_order_acq_rel))return;
        std::vector<std::shared_ptr<QuestCancellationReceipt const>> pending;
        Collect(pending);
        for(size_t index=0;index<pending.size();++index)
            if(!pending[index]->_cancelled.exchange(true,std::memory_order_acq_rel))pending[index]->Collect(pending);
    }
    mutable std::atomic<bool> _cancelled{false};
    mutable std::mutex _mutex;
    mutable std::vector<std::weak_ptr<QuestCancellationReceipt const>> _mirrors;
};

// One accepted quest attempt owns the scope. Summons hold weak references:
// descendants remain cancellable even after their immediate summoner despawns.
struct QuestObjectScope
{
    ObjectGuid Owner;
    std::atomic<bool> Cancelled{false};

    // A deferred story can distinguish explicit cancellation from logout
    // without keeping this scope (or its ordinary children) alive.
    std::shared_ptr<QuestCancellationReceipt const> CancellationReceipt() const
    {
        std::lock_guard<std::mutex> lock(Mutex);
        return _cancellationReceipt;
    }

    // Reattach the live instance's receipt before a re-entering player can
    // process another abandon/retake. Readers already handed the new local
    // receipt are mirrored too, without retaining either scope or children.
    bool AdoptCancellationReceipt(std::shared_ptr<QuestCancellationReceipt const> const& receipt)
    {
        std::lock_guard<std::mutex> lock(Mutex);
        if(Cancelled || !receipt || receipt->load(std::memory_order_acquire))return false;
        if(_cancellationReceipt==receipt)return true;
        if(!receipt->Link(_cancellationReceipt))return false;
        _cancellationReceipt=receipt;return !receipt->load(std::memory_order_acquire);
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
        _cancellationReceipt->Cancel();
        Cancelled = true;
        std::vector<ObjectGuid> objects;
        objects.swap(Objects);
        return objects;
    }

private:
    // A player can leave the map while a surviving child creates descendants.
    // Never iterate a registry that another map's update thread can append to.
    mutable std::mutex Mutex;
    std::vector<ObjectGuid> Objects;
    std::shared_ptr<QuestCancellationReceipt const> _cancellationReceipt = std::make_shared<QuestCancellationReceipt>();
};

#endif
