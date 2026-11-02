#pragma once

#include "chen_rescue.h"

namespace MonkJorvinax
{
constexpr uint32 ArrivalData = 94350;

inline bool Source(Creature* actor)
{
    return actor && actor->GetMapId() == 1014 && actor->GetEntry() == 98217 && actor->GetDBTableGUIDLow() == 362203;
}

inline bool Pose(Creature* actor)
{
    auto data = sWaypointMgr->GetPathScript(9859006);
    if (!actor || !data || data->size() != 9 || !actor->movespline || !actor->movespline->Finalized()) return false;
    auto const& last = data->back();
    return actor->IsWithinDist3d(last.x, last.y, last.z, 0.5f, false);
}

struct State
{
    explicit State(MonkChenRescue::State* owner) : context(owner) { }
    MonkChenRescue::State* context;
    ObjectGuid source, killer;
    std::vector<MonkChenRescue::Member> members;
    uint64 epoch = 0;
    uint32 lastStep = 0, creditAt = 0, voiceAt = 0;
    bool pending = false, earned = false, creditWaiting = false;
    bool arrivalSeen = false, voiceWaiting = false, voiceDone = false, warned = false;

    Scenario* Progress() const { return context ? context->Progress() : nullptr; }
    bool Leaf() const
    {
        Scenario* scenario = Progress();
        if (!scenario) return false;
        auto const* progress = scenario->GetAchievementMgr().GetCriteriaProgressMap();
        auto found = progress->find(29071);
        return found != progress->end() && found->second.Counter;
    }
    void Cancel()
    {
        ++epoch; pending = false; members.clear(); voiceDone = true;
    }
    Player* Ready(MonkChenRescue::Member const& member, Creature* anchor = nullptr) const
    {
        Player* player = context->Retained(member);
        if (!player || !player->IsInWorld() || !player->CanContact() || player->isGameMaster() || player->HasPendingTeleport()) return nullptr;
        Creature* corpse = context->map->GetCreature(source);
        if (corpse && corpse->IsInWorld() && !corpse->IsPreDelete() && !player->InSamePhase(corpse)) return nullptr;
        return !anchor || (player->InSamePhase(anchor) && !anchor->ShouldHideFor(player->GetGUID()) &&
            anchor->IsWithinDistInMap(player, 100.0f)) ? player : nullptr;
    }
    void Death(Creature* actor, Unit* attacker)
    {
        Scenario* scenario = Progress();
        if (pending || earned || !scenario || scenario->GetCurrentStep() > 6 || !Source(actor) ||
            actor->GetMap() != context->map || !actor->IsInWorld() || actor->IsPreDelete() || actor->IsAlive() ||
            context->map->GetCreature(actor->GetGUID()) != actor || source == actor->GetGUID()) return;
        source = actor->GetGUID(); killer = attacker ? attacker->GetGUID() : ObjectGuid(); members.clear();
        if (scenario->GetCurrentStep() == 6 && Leaf()) { earned = true; context->genuineBossDefeat = true; }
        for (auto const& reference : context->map->GetPlayers())
            if (Player* player = reference.getSource())
            {
                if (player->GetMap() != context->map || !player->IsInWorld() || !player->CanContact() || player->isGameMaster() ||
                    player->HasPendingTeleport() || player->getClass() != CLASS_MONK || !player->InSamePhase(actor) ||
                    actor->ShouldHideFor(player->GetGUID()) || player->GetQuestRewardStatus(MonkChenRescue::QuestId) ||
                    player->GetQuestStatus(MonkChenRescue::QuestId) != QUEST_STATUS_INCOMPLETE ||
                    player->FindQuestSlot(MonkChenRescue::QuestId) >= MAX_QUEST_LOG_SIZE) continue;
                player->AddQuestDelayedEvent(MonkChenRescue::QuestId, 0, [] { });
                auto status = player->getQuestStatus(MonkChenRescue::QuestId);
                if (status && status->ScriptLifetime) members.push_back({player->GetGUID(), status->ScriptLifetime});
            }
        if (members.empty()) return;
        Player* owner = attacker ? attacker->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr;
        if (owner)
            for (auto member = members.begin(); member != members.end(); ++member)
                if (member->player == owner->GetGUID()) { std::iter_swap(members.begin(), member); break; }
        ++epoch; lastStep = scenario->GetCurrentStep(); pending = !earned; creditWaiting = false;
        Update(0);
    }
    Creature* Guide() const
    {
        Creature* actor = context && context->map ? context->map->GetCreature(context->hait) : nullptr;
        return MonkChenRoutes::Source(actor) && actor->GetMap() == context->map && actor->IsInWorld() &&
            !actor->IsPreDelete() && actor->IsAlive() ? actor : nullptr;
    }
    bool BoundRoute() const
    {
        return context->routeIssued && context->arrivalDone && !context->handoffPending && !context->handoffPlayer.IsEmpty() &&
            std::any_of(context->members.begin(), context->members.end(), [this](MonkChenRescue::Member const& member)
            { return member.player == context->handoffPlayer && context->Retained(member) != nullptr; });
    }
    void ObserveArrival()
    {
        Creature* actor = Guide();
        if (arrivalSeen || !Progress() || !BoundRoute() || !actor || actor->GetCurrentWaypointID() != 8 || !Pose(actor)) return;
        auto movement = actor->GetMotionMaster()->GetMotionSlot(MOTION_SLOT_IDLE);
        auto path = MonkChenArrival::Path(actor);
        if (!movement || movement->GetMovementGeneratorType() == IDLE_MOTION_TYPE || (path && path->GetPathId() == 9859006))
        {
            arrivalSeen = true; voiceWaiting = false;
            actor->SetHomePosition(actor->GetPositionX(), actor->GetPositionY(), actor->GetPositionZ(), actor->GetOrientation());
        }
    }
    void Arrival(ObjectGuid value)
    {
        Creature* actor = Guide();
        auto path = MonkChenArrival::Path(actor);
        if (!BoundRoute() || arrivalSeen || !actor || value != actor->GetGUID() || !path || path->GetPathId() != 9859006 ||
            path->GetCurrentNode() != 8 || actor->GetMotionMaster()->empty() || actor->GetMotionMaster()->top() != path || !Pose(actor)) return;
        arrivalSeen = true; voiceWaiting = false;
        Update(0);
    }
    void Update(uint32 /*diff*/)
    {
        ObserveArrival();
        if (!pending && !earned && !source.IsEmpty())
            if (Creature* actor = context->map->GetCreature(source))
                if (Source(actor) && actor->IsAlive()) source.Clear();
        if (pending)
        {
            Scenario* scenario = Progress();
            if (!scenario || scenario->GetCurrentStep() < lastStep || scenario->GetCurrentStep() > 6 ||
                std::none_of(members.begin(), members.end(), [this](MonkChenRescue::Member const& member) { return context->Retained(member) != nullptr; }))
            { Cancel(); return; }
            lastStep = scenario->GetCurrentStep();
            if (lastStep < 5) return;
            if (creditWaiting && GetMSTimeDiffToNow(creditAt) < 2000) return;
            creditWaiting = true; creditAt = getMSTime();
            uint64 generation = epoch;
            auto original = members;
            for (auto const& member : original)
                if (Player* player = Ready(member))
                {
                    if (scenario->GetCurrentStep() == 5 && !Leaf())
                    {
                        Unit* reference = killer.IsEmpty() ? nullptr : ObjectAccessor::GetUnit(*player, killer);
                        if (reference && reference->GetMap() != context->map) reference = nullptr;
                        player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 46951, 0, 0, reference, true);
                    }
                    if (!pending || epoch != generation) return;
                    scenario = Progress(); player = Ready(member);
                    if (!scenario || scenario->GetCurrentStep() < lastStep || scenario->GetCurrentStep() > 6)
                    { Cancel(); return; }
                    lastStep = scenario->GetCurrentStep();
                    if (lastStep == 6 && Leaf() && player)
                    { pending = false; earned = true; context->genuineBossDefeat = true; break; }
                }
            if (pending && !warned)
                TC_LOG_WARN("scripts", "Monk Jorvinax: genuine98217 death46951 not acknowledged; retaining original quest attempts");
            warned = pending;
        }
        if (!earned || voiceDone) return;
        Scenario* scenario = Progress();
        if (!scenario || scenario->GetCurrentStep() != 6 || !Leaf() ||
            std::none_of(members.begin(), members.end(), [this](MonkChenRescue::Member const& member) { return context->Retained(member) != nullptr; }))
        { voiceDone = true; return; }
        Creature* actor = Guide();
        if (!arrivalSeen || !actor || (voiceWaiting && GetMSTimeDiffToNow(voiceAt) < 2000)) return;
        voiceWaiting = true; voiceAt = getMSTime();
        for (auto const& member : members)
            if (Player* player = Ready(member, actor))
            { voiceDone = true; sCreatureTextMgr->SendChat(actor, 15, player->GetGUID()); return; }
    }
};
}
