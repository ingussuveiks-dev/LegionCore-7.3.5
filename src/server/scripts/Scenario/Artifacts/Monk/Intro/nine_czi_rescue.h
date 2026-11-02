#pragma once

#include "chen_rescue.h"
#include "MoveSpline.h"

namespace MonkNineRescue
{
constexpr uint32 RequestData = 94361;
inline bool Source(Creature* actor)
{
    return actor && actor->GetMapId() == 1014 && actor->GetEntry() == 97774 && actor->GetDBTableGUIDLow() == 362181;
}
inline bool Pose(Creature* actor)
{
    return actor && actor->movespline && actor->movespline->Finalized() &&
        actor->IsWithinDist3d(3848.98f, 1945.78f, 931.85f, 0.5f, false);
}
struct State
{
    explicit State(MonkChenRescue::State* owner) : context(owner) { }
    MonkChenRescue::State* context;
    Scenario* origin = nullptr;
    ObjectGuid source, lastDeath;
    std::vector<MonkChenRescue::Member> members;
    MonkChenRescue::Member greeting;
    MovementGenerator* point = nullptr; // identity only, never dereferenced after retirement
    uint32 splineBefore = 0, splineId = 0;
    uint64 epoch = 0;
    uint32 lastStep = 0, startedAt = 0, arrivedAt = 0, retryAt = 0;
    bool pending = false, started = false, credited = false, moved = false, arrived = false, ready = false;
    bool busy = false, retrying = false, warned = false, spoken = false, routeWarned = false;

    Creature* Actor() const
    {
        auto actor = context->map ? context->map->GetCreature(source) : nullptr;
        return Source(actor) && actor->GetMap() == context->map && actor->IsInWorld() &&
            !actor->IsPreDelete() && actor->IsAlive() ? actor : nullptr;
    }
    bool Leaf() const
    {
        auto progress = context->Progress();
        if (!progress) return false;
        auto const* leaves = progress->GetAchievementMgr().GetCriteriaProgressMap();
        auto found = leaves->find(29046);
        return found != leaves->end() && found->second.Counter;
    }
    Player* Current(MonkChenRescue::Member const& member, Creature* actor) const
    {
        auto player = context->Retained(member);
        return player && actor && player->IsInWorld() && player->IsAlive() && player->CanContact() &&
            !player->isGameMaster() && !player->HasPendingTeleport() && player->InSamePhase(actor) &&
            !actor->ShouldHideFor(player->GetGUID()) && actor->IsWithinDistInMap(player, 100.0f) ? player : nullptr;
    }
    void Cancel()
    {
        ++epoch; origin = nullptr; members.clear(); point = nullptr;
        if (ready) if (auto actor = Actor()) actor->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
        pending = started = credited = moved = arrived = ready = retrying = warned = spoken = routeWarned = false;
    }
    bool Valid(uint64 generation)
    {
        if (epoch != generation) return false;
        auto progress = context->Progress();
        if (!progress || progress != origin || progress->GetCurrentStep() < lastStep ||
            (credited && !Leaf()) || std::none_of(members.begin(), members.end(),
                [this](MonkChenRescue::Member const& member) { return context->Retained(member) != nullptr; }))
        { Cancel(); return false; }
        lastStep = progress->GetCurrentStep();
        return true;
    }
    void Observe(Creature* actor)
    {
        if (!actor || actor->GetMap() != context->map) return;
        if (actor->GetEntry() == 97968 && actor->GetDBTableGUIDLow() == 362187 && actor->IsAlive() && !pending && !started)
            lastDeath.Clear();
        if (!Source(actor)) return;
        if ((pending || started) && std::none_of(members.begin(), members.end(),
            [this](MonkChenRescue::Member const& member) { return context->Retained(member) != nullptr; })) Cancel();
        if (source.IsEmpty() || (!pending && !started)) source = actor->GetGUID();
    }
    void Request(ObjectGuid playerGuid)
    {
        auto progress = context->Progress(); auto actor = Actor();
        auto player = ObjectAccessor::FindPlayer(playerGuid);
        if (busy || !progress || progress->GetCurrentStep() > 1 || !actor || !player ||
            !actor->IsWithinDistInMap(player, 30.0f) || !player->IsWithinLOSInMap(actor) || !player->canSeeOrDetect(actor)) return;
        auto status = player->getQuestStatus(MonkChenRescue::QuestId);
        if (!status) return;
        player->AddQuestDelayedEvent(MonkChenRescue::QuestId, 0, [] { });
        MonkChenRescue::Member member{playerGuid, status->ScriptLifetime};
        if (!Current(member, actor) || context->Retained(greeting)) return;
        greeting = member;
        MonkChenRoutes::Dispatch dispatch(busy);
        sCreatureTextMgr->SendChat(actor, 1);
    }
    void Death(Creature* corpse, Unit* killer)
    {
        auto progress = context->Progress();
        if (busy || pending || started || !progress || progress->GetCurrentStep() > 1 || !corpse ||
            corpse->GetMap() != context->map || !corpse->IsInWorld() || corpse->IsPreDelete() || corpse->IsAlive() ||
            corpse->GetEntry() != 97968 || corpse->GetDBTableGUIDLow() != 362187 ||
            context->map->GetCreature(corpse->GetGUID()) != corpse || lastDeath == corpse->GetGUID()) return;
        lastDeath = corpse->GetGUID();
        members.clear();
        for (auto const& reference : context->map->GetPlayers())
            if (auto player = reference.getSource())
            {
                if (player->GetMap() != context->map || !player->IsInWorld() || !player->IsAlive() || !player->CanContact() ||
                    player->isGameMaster() || player->HasPendingTeleport() || player->getClass() != CLASS_MONK ||
                    !player->InSamePhase(corpse) || !corpse->IsWithinDistInMap(player, 100.0f) ||
                    corpse->ShouldHideFor(player->GetGUID()) || player->GetQuestRewardStatus(MonkChenRescue::QuestId) ||
                    player->GetQuestStatus(MonkChenRescue::QuestId) != QUEST_STATUS_INCOMPLETE ||
                    player->FindQuestSlot(MonkChenRescue::QuestId) >= MAX_QUEST_LOG_SIZE) continue;
                player->AddQuestDelayedEvent(MonkChenRescue::QuestId, 0, [] { });
                auto status = player->getQuestStatus(MonkChenRescue::QuestId);
                if (status && status->ScriptLifetime) members.push_back({player->GetGUID(), status->ScriptLifetime});
            }
        if (members.empty()) return;
        if (auto owner = killer ? killer->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr)
            for (auto member = members.begin(); member != members.end(); ++member)
                if (member->player == owner->GetGUID()) { std::iter_swap(members.begin(), member); break; }
        origin = progress; lastStep = progress->GetCurrentStep();
        ++epoch; pending = true;
        Update(0);
    }
    void Update(uint32 /*diff*/)
    {
        if (busy || (!pending && !started)) return;
        uint64 generation = epoch;
        if (!Valid(generation)) return;
        auto actor = Actor();
        if (!actor) return;
        if (ready) { actor->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK); return; }
        MonkChenRoutes::Dispatch dispatch(busy);
        auto valid = [this, generation, actor]() { return Valid(generation) && Actor() == actor; };
        if (pending)
        {
            if (lastStep != 1) return; // The native first tree must really finish.
            bool contact = std::any_of(members.begin(), members.end(),
                [this, actor](MonkChenRescue::Member const& member) { return Current(member, actor) != nullptr; });
            if (!contact) return;
            pending = false; started = true; startedAt = getMSTime();
            actor->CastSpell(actor, 193248);
            if (!valid()) return;
            sCreatureTextMgr->SendChat(actor, 3);
            if (!valid()) return;
            actor->GetMotionMaster()->MoveJump(3814.16f, 2009.03f, 935.15f, actor->GetOrientation(), 5.0f, 5.0f);
            if (!valid()) return;
        }
        if (!credited && GetMSTimeDiffToNow(startedAt) >= 3000)
        {
            if (retrying && GetMSTimeDiffToNow(retryAt) < 2000) return;
            retrying = true; retryAt = getMSTime();
            if (!spoken) { spoken = true; sCreatureTextMgr->SendChat(actor, 4); }
            if (!valid()) return;
            auto original = members;
            for (auto const& member : original)
                if (auto player = Current(member, actor))
                {
                    if (lastStep == 1 && !Leaf()) player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 46734);
                    if (!valid()) return;
                    if (Leaf() && lastStep >= 2) { credited = true; retrying = false; break; }
                }
            if (!credited)
            {
                if (!warned) TC_LOG_WARN("scripts", "Monk Nine: genuine Vizznak death46734 not acknowledged; retaining original quest attempts");
                warned = true; return;
            }
        }
        if (!credited || GetMSTimeDiffToNow(startedAt) < 7000) return;
        if (!moved)
        {
            if (actor->GetTransport()) return;
            if (!point)
            {
                if (retrying && GetMSTimeDiffToNow(retryAt) < 2000) return;
                retrying = true; retryAt = getMSTime();
                splineBefore = actor->movespline->GetId();
                actor->GetMotionMaster()->MovePoint(1, 3848.98f, 1945.78f, 931.85f);
                if (!valid()) return;
                point = actor->GetMotionMaster()->GetMotionSlot(MOTION_SLOT_ACTIVE);
                if (!point || point->GetMovementGeneratorType() != POINT_MOTION_TYPE)
                {
                    point = nullptr;
                    if (!routeWarned) TC_LOG_WARN("scripts", "Monk Nine: original point1 was not installed; retaining genuine rescue");
                    routeWarned = true; return;
                }
            }
            if (actor->GetMotionMaster()->GetMotionSlot(MOTION_SLOT_ACTIVE) != point)
            { point = nullptr; return; }
            if (actor->movespline->GetId() == splineBefore) return; // Native root/cast deferral has not launched it.
            splineId = actor->movespline->GetId(); moved = true; retrying = false;
        }
        // A clock, another point callback, a teleport or a replacement spline
        // cannot stand in for completion of the actual issued native point.
        if (!arrived && actor->movespline->GetId() == splineId && Pose(actor))
        {
            arrived = true; arrivedAt = getMSTime();
            actor->SetHomePosition(actor->GetPositionX(), actor->GetPositionY(), actor->GetPositionZ(), actor->GetOrientation());
            sCreatureTextMgr->SendChat(actor, 5);
            if (!valid()) return;
        }
        if (!arrived || ready || GetMSTimeDiffToNow(arrivedAt) < 4000) return;
        ready = true;
        auto original = members;
        for (auto const& member : original)
            if (auto player = Current(member, actor)) { sCreatureTextMgr->SendChat(actor, 6, player->GetGUID()); break; }
        if (!valid()) return;
        actor->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
        actor->CastSpell(actor, 193721);
    }
};
}
