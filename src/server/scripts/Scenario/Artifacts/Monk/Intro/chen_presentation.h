#pragma once

#include "chen_rescue.h"

namespace MonkChenPresentation
{
// The accepted native arrival owns these original3/7/11/16s actions. Losing
// Chen's AI must not retire them or bind them to another nearby player/actor.
struct State
{
    explicit State(MonkChenRescue::State* owner) : context(owner) { }
    MonkChenRescue::State* context;
    uint64 epoch = 0;
    uint8 issued = 0;
    bool updating = false;

    Player* Owner(MonkChenRescue::Member const& member) const
    {
        auto scenario = context->Progress();
        return context->started && context->arrivalDone && context->EscortStage(scenario) && context->EscortLeaf() &&
            context->handoffPlayer == member.player ? context->Current(member) : nullptr;
    }

    Creature* Guide(Creature* actor, Player* player) const
    {
        auto guide = context->map->GetCreature(context->hait);
        return MonkChenRoutes::Source(guide) && guide->GetMap() == context->map && guide->IsInWorld() &&
            !guide->IsPreDelete() && guide->IsAlive() && actor->FindNearestCreature(97778, 50.0f, true) == guide &&
            player->InSamePhase(guide) && !guide->ShouldHideFor(player->GetGUID()) ? guide : nullptr;
    }

    void Update(uint32 /*diff*/, bool departure = false)
    {
        if (updating) return;
        if (epoch != context->epoch) { epoch = context->epoch; issued = 0; }
        if (issued == 15) return;
        auto found = std::find_if(context->members.begin(), context->members.end(), [this](MonkChenRescue::Member const& member)
            { return member.player == context->handoffPlayer; });
        if (found == context->members.end()) return;
        MonkChenRescue::Member const original = *found;
        Creature* actor = context->Actor();
        Player* player = Owner(original);
        if (!actor || !player) return;
        uint64 generation = context->epoch;
        ObjectGuid const actorGuid = context->chen;
        auto valid = [this, generation, actor, actorGuid, &original]() -> bool
            { return context->epoch == generation && context->chen == actorGuid && context->Actor() == actor && Owner(original); };
        MonkChenRoutes::Dispatch dispatch(updating);
        uint32 elapsed = GetMSTimeDiffToNow(context->handoffStartedAt);
        if (departure)
        {
            if (!(issued & 8) && elapsed >= 16000)
            {
                // Reserve the original native command, not an arrival receipt.
                // No waypoint/scenario credit is inferred from MovePoint.
                issued |= 8;
                actor->GetMotionMaster()->MovePoint(0, 3837.48f, 1729.83f, 946.42f);
            }
            return;
        }
        // These were optional, once-only native callbacks. Do not replay
        // greetings after the existing12/13 handoff has already begun.
        if (context->handoffAnnounced) issued |= 7;
        if (!(issued & 1) && elapsed >= 3000)
        {
            issued |= 1;
            if (Creature* guide = Guide(actor, player))
                sCreatureTextMgr->SendChat(guide, 10);
        }
        if (!valid()) return;
        if (!(issued & 2) && elapsed >= 7000)
        { issued |= 2; sCreatureTextMgr->SendChat(actor, 8, original.player); }
        if (!valid()) return;
        if (!(issued & 4) && elapsed >= 11000)
        {
            issued |= 4;
            if (Creature* guide = Guide(actor, Owner(original)))
                sCreatureTextMgr->SendChat(guide, 11, original.player);
        }
        if (!valid()) return;
    }
};
}
