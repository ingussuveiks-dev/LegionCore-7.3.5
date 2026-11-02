#pragma once

#include "chen_rescue.h"

namespace MonkNineArrival
{
constexpr uint32 RequestData = 94360;
inline bool Source(Creature* actor)
{
    return actor && actor->GetMapId() == 1014 && actor->GetEntry() == 97774 && actor->GetDBTableGUIDLow() == 362252;
}

// A real visit to the destination actor owns this native46902 receipt. The
// taxi request/clock is not an arrival; earlier native trees must finish first.
struct State
{
    explicit State(MonkChenRescue::State* owner) : context(owner) { }
    MonkChenRescue::State* context;
    Scenario* origin = nullptr; // identity only, never dereferenced after replacement
    ObjectGuid source;
    MonkChenRescue::Member member;
    uint64 epoch = 0;
    uint32 startedAt = 0, retryAt = 0, lastStep = 0;
    bool pending = false, started = false, finished = false, busy = false, retrying = false, warned = false;

    Creature* Actor() const
    {
        auto actor = context->map ? context->map->GetCreature(source) : nullptr;
        return Source(actor) && actor->GetMap() == context->map && actor->IsInWorld() &&
            !actor->IsPreDelete() && actor->IsAlive() && actor->GetPositionY() <= 1736.0f ? actor : nullptr;
    }
    bool Leaf() const
    {
        auto progress = context->Progress();
        if (!progress) return false;
        auto const* leaves = progress->GetAchievementMgr().GetCriteriaProgressMap();
        auto found = leaves->find(29069);
        return found != leaves->end() && found->second.Counter;
    }
    Player* Current(MonkChenRescue::Member const& original, Creature* actor) const
    {
        auto player = context->Retained(original);
        return player && actor && player->IsInWorld() && player->IsAlive() && player->CanContact() &&
            !player->isGameMaster() && !player->HasPendingTeleport() && player->InSamePhase(actor) &&
            !actor->ShouldHideFor(player->GetGUID()) && player->canSeeOrDetect(actor) ? player : nullptr;
    }
    Player* Contact(MonkChenRescue::Member const& original, Creature* actor) const
    {
        auto player = Current(original, actor);
        return player && actor->IsWithinDistInMap(player, 7.0f) && player->IsWithinLOSInMap(actor) ? player : nullptr;
    }
    void Observe(Creature* actor)
    {
        if (!Source(actor) || actor->GetMap() != context->map) return;
        if ((pending || started) && !context->Retained(member)) Cancel();
        if (!pending && !started) source = actor->GetGUID();
    }
    void Cancel()
    {
        ++epoch; origin = nullptr; member = {}; pending = started = finished = retrying = warned = false;
    }
    void Request(ObjectGuid playerGuid)
    {
        if (busy) return;
        if ((pending || started) && !context->Retained(member)) Cancel();
        if (pending || started) { Update(0); return; }
        auto progress = context->Progress();
        auto actor = Actor();
        auto player = ObjectAccessor::FindPlayer(playerGuid);
        if (!progress || progress->GetCurrentStep() > 2 || !actor || !player) return;
        // Establish a genuine quest attempt before retaining a weak reference;
        // nothing is consumed by an early/ineligible visit or rejected event.
        auto status = player->getQuestStatus(MonkChenRescue::QuestId);
        if (!status) return;
        player->AddQuestDelayedEvent(MonkChenRescue::QuestId, 0, [] { });
        MonkChenRescue::Member candidate{playerGuid, status->ScriptLifetime};
        if (!Contact(candidate, actor)) return;
        member = candidate; origin = progress; lastStep = progress->GetCurrentStep(); pending = true; ++epoch;
        Update(0);
    }
    void Update(uint32 /*diff*/)
    {
        if (busy || (!pending && !started) || finished) return;
        auto progress = context->Progress();
        if (!progress || progress != origin || !context->Retained(member) || progress->GetCurrentStep() < lastStep ||
            (started && (progress->GetCurrentStep() < 3 || !Leaf())))
        { Cancel(); return; }
        lastStep = progress->GetCurrentStep();
        auto actor = Actor();
        if (!actor || actor->ShouldHideFor(member.player)) return;
        MonkChenRescue::Member const original = member;
        ObjectGuid const actorGuid = source;
        uint64 const generation = epoch;
        auto valid = [this, actor, actorGuid, generation, &original]()
        {
            auto scenario = context->Progress();
            return epoch == generation && source == actorGuid && Actor() == actor && scenario && scenario == origin &&
                (!started || (scenario->GetCurrentStep() >= 3 && Leaf())) &&
                context->Retained(original) && !actor->ShouldHideFor(original.player);
        };
        MonkChenRoutes::Dispatch dispatch(busy);
        if (pending)
        {
            if (progress->GetCurrentStep() < 2) return;
            if (progress->GetCurrentStep() != 2) { Cancel(); return; }
            Player* player = Contact(original, actor);
            if (!player || (retrying && GetMSTimeDiffToNow(retryAt) < 2000)) return;
            retrying = true; retryAt = getMSTime();
            player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 46902);
            if (!valid()) return;
            progress = context->Progress();
            if (progress->GetCurrentStep() < 3 || !Leaf())
            {
                if (!warned) { warned = true; TC_LOG_WARN("scripts", "Monk Nine Czi: native46902 arrival not acknowledged; retaining original participant for retry"); }
                return;
            }
            pending = false; started = true; startedAt = getMSTime();
            if (Current(original, actor)) sCreatureTextMgr->SendChat(actor, 7, original.player);
            if (!valid()) return;
        }
        if (GetMSTimeDiffToNow(startedAt) < 4000) return;
        // Preserve the original4s callback and its native3s delayed despawn.
        // The latter starts when the callback actually runs, even in a late frame.
        finished = true;
        if (Current(original, actor)) sCreatureTextMgr->SendChat(actor, 8, original.player);
        if (valid()) actor->DespawnOrUnsummon(3000);
    }
};
}
