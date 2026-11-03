#pragma once

#include "chen_rescue.h"
#include "ScriptedCreature.h"

namespace MonkChenPresentation
{
// Genuine accepted rescue/arrival receipts own the original NPC actions.
// Losing Chen's AI must not retire them or bind them to another player/actor.
struct State
{
    explicit State(MonkChenRescue::State* owner) : context(owner) { }
    MonkChenRescue::State* context;
    uint64 epoch = 0;
    uint8 issued = 0;
    bool updating = false;
    uint8 rescueIssued = 0;
    bool rescueUpdating = false;
    uint32 rescueSpokenAt = 0;

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
        if (epoch != context->epoch) { epoch = context->epoch; issued = 0; rescueIssued = 0; }
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

    bool Student(Creature* student, Creature* actor, ObjectGuid const& owner) const
    {
        if (!student || student->GetEntry() != 98074 || student->GetMap() != context->map ||
            !student->IsInWorld() || student->IsPreDelete() || !student->IsAlive() ||
            !student->GetCharmerOrOwnerGUID().IsEmpty() || !student->InSamePhase(actor) ||
            student->ShouldHideFor(owner) || !actor->IsWithinDistInMap(student, 70.0f, false)) return false;
        uint64 spawn = student->GetDBTableGUIDLow();
        return spawn == 362259 || spawn == 362263 || spawn == 362264 || spawn == 362265;
    }

    void UpdateRescue(uint32 /*diff*/)
    {
        if (rescueUpdating) return;
        if (epoch != context->epoch) { epoch = context->epoch; issued = 0; rescueIssued = 0; }
        if ((rescueIssued & 3) == 3 || !context->started) return;
        // Do not replay early scolding/follow attempts after the acknowledged
        // ordinary escort has already begun. Missing actors are not replaced.
        if (context->routeIssued || context->arrivalDone) { rescueIssued |= 2; return; }
        auto progress = context->Progress();
        Creature* actor = context->Actor();
        if (!progress || progress->GetCurrentStep() != 4 || !context->Leaf() || !actor) return;
        auto members = context->members;
        for (auto const& original : members)
            // These original callbacks address NPCs, not a player. A retained
            // genuine attempt still owns them during a transient UI/contact
            // gate; actor phase/private ownership and quest lifetime remain.
            if (context->Retained(original) && !actor->ShouldHideFor(original.player))
            {
                uint64 generation = context->epoch;
                ObjectGuid const actorGuid = context->chen;
                auto valid = [this, generation, actor, actorGuid, &original]() -> bool
                {
                    auto scenario = context->Progress();
                    return context->epoch == generation && !context->routeIssued && !context->arrivalDone && context->chen == actorGuid && context->Actor() == actor &&
                        scenario && scenario->GetCurrentStep() == 4 && context->Leaf() && context->Retained(original) &&
                        !actor->ShouldHideFor(original.player);
                };
                MonkChenRoutes::Dispatch dispatch(rescueUpdating);
                uint32 elapsed = GetMSTimeDiffToNow(context->escortStartedAt);
                if (!(rescueIssued & 1) && elapsed >= 4443)
                { rescueIssued |= 1; rescueSpokenAt = getMSTime(); sCreatureTextMgr->SendChat(actor, 4); }
                if (!valid()) return;
                if (!(rescueIssued & 2) && elapsed >= 5000)
                {
                    rescueIssued |= 2;
                    std::list<Creature*> nearby;
                    GetCreatureListWithEntryInGrid(nearby, actor, 98074, 70.0f);
                    std::vector<ObjectGuid> students;
                    for (auto student : nearby)
                        if (Student(student, actor, original.player) &&
                            std::find(students.begin(), students.end(), student->GetGUID()) == students.end())
                            students.push_back(student->GetGUID());
                    for (auto const& guid : students)
                    {
                        if (!valid()) return;
                        Creature* student = context->map->GetCreature(guid);
                        if (!Student(student, actor, original.player)) continue;
                        student->GetMotionMaster()->MoveFollow(actor, urand(1, 4), float(-PET_FOLLOW_ANGLE), MOTION_SLOT_IDLE);
                        if (!valid()) return;
                        student = context->map->GetCreature(guid);
                        if (Student(student, actor, original.player)) sCreatureTextMgr->SendChat(student, 0);
                        if (!valid()) return;
                    }
                }
                return;
            }
    }

    void UpdateEscortSpeech(uint32 /*diff*/)
    {
        if (rescueUpdating) return;
        if (epoch != context->epoch) { epoch = context->epoch; issued = 0; rescueIssued = 0; }
        if ((rescueIssued & 4) || !context->started || !context->escortSpoken) return;
        // Optional speech must never hold the essential native8s route. Use
        // the actual previous issue time when a late frame coalesces timers.
        if (context->arrivalDone) { rescueIssued |= 4; return; }
        if ((rescueIssued & 1) && GetMSTimeDiffToNow(rescueSpokenAt) < 2125) return;
        auto scenario = context->Progress();
        Creature* actor = context->Actor();
        if (!scenario || scenario->GetCurrentStep() != 4 || !context->Leaf() || !actor) return;
        auto original = context->members;
        for (auto const& member : original)
            if (Player* player = context->Current(member))
            {
                rescueIssued |= 4;
                MonkChenRoutes::Dispatch dispatch(rescueUpdating);
                sCreatureTextMgr->SendChat(actor, 5, player->GetGUID());
                return;
            }
    }
};
}
