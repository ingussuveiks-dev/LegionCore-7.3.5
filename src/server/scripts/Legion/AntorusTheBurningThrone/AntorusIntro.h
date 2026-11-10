#ifndef ANTORUS_INTRO_H
#define ANTORUS_INTRO_H

#include "Conversation.h"
#include "EventObject.h"
#include "InstanceScript.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include <functional>
#include <map>
#include <set>

// EventObjectScript objects are shared by every object with the ScriptName.
// The raid owns the progress and the delayed work, including failed attempts.
class AntorusIntroInstance : public InstanceScript
{
public:
    explicit AntorusIntroInstance(InstanceMap* map) : InstanceScript(map) { }

    bool IntroPlayed(uint32 entry) const
    {
        auto itr = introEvents.find(entry);
        return itr != introEvents.end() && itr->second.done;
    }

    bool BeginIntro(uint32 entry, Player* player, EventObject* source)
    {
        auto& event = introEvents[entry];
        if (event.done)
            return false;
        event.triggers.emplace(source->GetGUID(), player->GetGUID());
        if (event.pending)
            return false;
        event.pending = true;
        return true;
    }

    bool IntroStarted(uint32 entry) const { return introEvents.at(entry).started; }
    void SetIntroStarted(uint32 entry) { introEvents.at(entry).started = true; }

    void FinishIntro(uint32 entry, bool success)
    {
        auto& event = introEvents.at(entry);
        event.pending = false;
        if (success)
        {
            event.done = true;
            event.triggers.clear();
            return;
        }

        // The native EO dispatcher records admission after OnTrigger returns;
        // its return value does not undo either affected-player set.
        auto triggers = std::move(event.triggers);
        event.triggers.clear();
        AddDelayedEvent(1, [this, triggers]()
        {
            for (auto const& trigger : triggers)
                if (EventObject* source = instance->GetEventObject(trigger.first))
                {
                    source->affectedPlayers.erase(trigger.second);
                    source->affectedPermamentPlayers.erase(trigger.second);
                }
        });
    }

    void QueueIntroPlayer(uint32 entry, Player* player, uint32 delay,
        std::function<void(Player*)> action, std::function<void()> failure = {})
    {
        ObjectGuid guid = player->GetGUID();
        AddDelayedEvent(delay, [this, entry, guid, action, failure]()
        {
            Player* player = ObjectAccessor::FindPlayer(instance, guid);
            if (!player || !player->IsInWorld() || !player->IsAlive() || player->GetMap() != instance)
            {
                if (failure)
                    failure();
                FinishIntro(entry, false);
                return;
            }
            action(player);
        });
    }

    void QueueIntroWorld(uint32 entry, uint32 delay, std::function<bool()> action,
        std::function<void()> failure = {})
    {
        AddDelayedEvent(delay, [this, entry, action, failure]()
        {
            bool success = action();
            if (!success && failure)
                failure();
            FinishIntro(entry, success);
        });
    }

    static bool StartConversation(Player* player, uint32 id)
    {
        Conversation* conversation = new Conversation;
        if (conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), id, player, nullptr, *player))
            return true;
        delete conversation;
        return false;
    }

private:
    struct IntroEvent
    {
        bool pending = false;
        bool done = false;
        bool started = false;
        std::set<std::pair<ObjectGuid, ObjectGuid>> triggers;
    };
    std::map<uint32, IntroEvent> introEvents;
};

#endif
