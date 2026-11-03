#pragma once

#include <algorithm>
#include "chen_arrival.h"
#include "chen_routes.h"

namespace MonkChenRescue
{
constexpr uint32 QuestId = 12103;
constexpr uint32 StatusData = 94340, ClaimData = 94341, RequestData = 94342;

inline bool Source(Creature* actor)
{
    return actor && actor->GetMapId() == 1014 && actor->GetEntry() == 103148 && actor->GetDBTableGUIDLow() == 362201;
}

struct Member
{
    ObjectGuid player;
    std::weak_ptr<uint8> attempt;
};

// The real private-instance death, rather than nearest-player SetData, owns
// this receipt. Earlier prerequisite trees cannot consume its native event.
struct State
{
    explicit State(Map* owner) : map(owner) { }
    Map* map;
    ObjectGuid chen, source, lastDeath, claim;
    std::vector<Member> members;
    uint64 epoch = 0;
    uint32 retry = 0, lastStep = 0;
    bool pending = false, started = false, warned = false;
    ObjectGuid arrivalClaim, arrivalPlayer;
    bool routeIssued = false, arrivalSeen = false, arrivalDone = false, arrivalWarned = false;
    uint32 arrivalRetryStart = 0;
    bool arrivalWaiting = false;
    ObjectGuid hait, handoffPlayer;
    uint32 escortStartedAt = 0, handoffStartedAt = 0, routeRetryAt = 0;
    bool routeRetrying = false, routeBusy = false, handoffPending = false;
    bool escortSpoken = false, escortFollowed = false, handoffAnnounced = false, handoffSpoken = false;
    bool escortWarned = false, handoffWarned = false;
    bool genuineBossDefeat = false;

    Scenario* Progress() const
    {
        if (!map || map->IsMapUnload() || map->GetId() != 1014) return nullptr;
        Scenario* scenario = sScenarioMgr->GetScenario(map->GetInstanceId());
        return scenario && scenario->GetScenarioId() == 943 ? scenario : nullptr;
    }

    bool Leaf() const
    {
        Scenario* scenario = Progress();
        if (!scenario) return false;
        auto const* progress = scenario->GetAchievementMgr().GetCriteriaProgressMap();
        auto found = progress->find(29047);
        return found != progress->end() && found->second.Counter;
    }

    Player* Retained(Member const& member) const
    {
        Player* player = ObjectAccessor::FindPlayer(member.player);
        auto status = player ? player->getQuestStatus(QuestId) : nullptr;
        auto attempt = member.attempt.lock();
        return player && status && attempt && status->ScriptLifetime == attempt && player->GetMap() == map &&
            player->getClass() == CLASS_MONK && !player->GetQuestRewardStatus(QuestId) &&
            player->GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE && player->FindQuestSlot(QuestId) < MAX_QUEST_LOG_SIZE ? player : nullptr;
    }

    Creature* Actor() const
    {
        Creature* actor = map ? map->GetCreature(chen) : nullptr;
        return Source(actor) && actor->GetMap() == map && actor->IsInWorld() && !actor->IsPreDelete() && actor->IsAlive() ? actor : nullptr;
    }

    UnitAI* Content() const
    {
        Creature* actor = Actor();
        return actor ? (actor->GetDisabledAI() ? actor->GetDisabledAI() : actor->GetAI()) : nullptr;
    }

    Player* Current(Member const& member) const
    {
        Player* player = Retained(member);
        Creature* actor = Actor();
        Creature* corpse = map ? map->GetCreature(source) : nullptr;
        return player && actor && player->IsInWorld() && player->CanContact() && !player->isGameMaster() &&
            !player->HasPendingTeleport() && player->InSamePhase(actor) && !actor->ShouldHideFor(player->GetGUID()) &&
            actor->IsWithinDistInMap(player, 100.0f) &&
            (!corpse || !corpse->IsInWorld() || corpse->IsPreDelete() || player->InSamePhase(corpse)) ? player : nullptr;
    }

    void Cancel()
    {
        ++epoch; pending = false; claim.Clear(); members.clear(); retry = 0;
        arrivalClaim.Clear(); arrivalPlayer.Clear(); arrivalDone = started;
        arrivalWaiting = false;
        routeRetrying = false; handoffPending = false;
        genuineBossDefeat = false;
    }

    void Observe(Creature* actor)
    {
        if (!actor || actor->GetMap() != map) return;
        if (Source(actor) && !started && (chen.IsEmpty() || !pending)) chen = actor->GetGUID();
        if (MonkChenRoutes::Source(actor) && hait.IsEmpty()) hait = actor->GetGUID();
        if (actor->GetEntry() == 97811 && actor->GetDBTableGUIDLow() == 362229 && actor->IsAlive() && !pending)
            lastDeath.Clear();
    }

    void Death(Creature* actor, Unit* killer)
    {
        Scenario* scenario = Progress();
        if (pending || started || !scenario || scenario->GetCurrentStep() > 3 || !actor ||
            actor->GetMap() != map || !actor->IsInWorld() || actor->IsPreDelete() || actor->IsAlive() ||
            actor->GetEntry() != 97811 || actor->GetDBTableGUIDLow() != 362229 ||
            map->GetCreature(actor->GetGUID()) != actor || lastDeath == actor->GetGUID()) return;
        lastDeath = source = actor->GetGUID(); members.clear();
        for (auto const& reference : map->GetPlayers())
            if (Player* player = reference.getSource())
            {
                if (player->GetMap() != map || !player->IsInWorld() || !player->CanContact() || player->isGameMaster() ||
                    player->HasPendingTeleport() || player->getClass() != CLASS_MONK || !player->InSamePhase(actor) ||
                    player->GetQuestRewardStatus(QuestId) || player->GetQuestStatus(QuestId) != QUEST_STATUS_INCOMPLETE ||
                    player->FindQuestSlot(QuestId) >= MAX_QUEST_LOG_SIZE) continue;
                player->AddQuestDelayedEvent(QuestId, 0, [] { });
                auto status = player->getQuestStatus(QuestId);
                if (status && status->ScriptLifetime) members.push_back({player->GetGUID(), status->ScriptLifetime});
            }
        if (members.empty()) return;
        Player* owner = killer ? killer->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr;
        if (owner)
            for (auto member = members.begin(); member != members.end(); ++member)
                if (member->player == owner->GetGUID()) { std::iter_swap(members.begin(), member); break; }
        ++epoch; retry = 0; lastStep = scenario->GetCurrentStep(); pending = true; warned = false;
        Update(0);
    }

    void Request(ObjectGuid actor)
    {
        if (actor == chen) Update(0);
    }

    void Update(uint32 diff)
    {
        if (!pending) { UpdateArrival(diff); UpdateRoutes(); return; }
        Scenario* scenario = Progress();
        if (!scenario || scenario->GetCurrentStep() < lastStep || scenario->GetCurrentStep() > 4 ||
            std::none_of(members.begin(), members.end(), [this](Member const& member) { return Retained(member) != nullptr; }))
        { Cancel(); return; }
        lastStep = scenario->GetCurrentStep();
        if (lastStep < 3) return; // Native46902 must first admit the rescue tree.
        if (retry > diff) { retry -= diff; return; }
        retry = 2000;
        UnitAI* ai = Content();
        if (!ai || ai->GetData(StatusData) != 1) return;
        uint64 generation = epoch;
        auto original = members; // Native event may re-enter and retire this state.
        for (auto const& member : original)
            if (Player* player = Current(member))
            {
                if (scenario->GetCurrentStep() == 3 && !Leaf())
                    player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 46956);
                if (!pending || epoch != generation) return;
                scenario = Progress(); player = Current(member); ai = Content();
                if (!scenario || scenario->GetCurrentStep() < lastStep || scenario->GetCurrentStep() > 4)
                { Cancel(); return; }
                lastStep = scenario->GetCurrentStep();
                if (scenario && scenario->GetCurrentStep() == 4 && Leaf() && player && ai && ai->GetData(StatusData) == 1)
                {
                    // Reserve before native SetData/Talk can re-enter. The
                    // actual AI acknowledges before queuing its original path.
                    claim = chen;
                    ai->SetData(1, 1);
                    if (!pending || epoch != generation) return;
                    ai = Content();
                    if (ai && ai->GetData(StatusData) == 2)
                    { pending = false; started = true; escortStartedAt = getMSTime(); claim.Clear(); return; }
                    claim.Clear();
                }
            }
        if (!warned)
            TC_LOG_WARN("scripts", "Monk Chen: genuine Morvat death46956/escort not acknowledged; retaining original quest attempts");
        warned = true;
    }

    bool EscortLeaf() const
    {
        Scenario* scenario = Progress();
        if (!scenario) return false;
        auto const* progress = scenario->GetAchievementMgr().GetCriteriaProgressMap();
        auto found = progress->find(29070);
        return found != progress->end() && found->second.Counter;
    }

    void Routed(ObjectGuid value)
    {
        Scenario* scenario = Progress();
        if (started && !arrivalDone && value == chen && scenario && scenario->GetCurrentStep() == 4 && Leaf() &&
            std::any_of(members.begin(), members.end(), [this](Member const& member) { return Retained(member) != nullptr; }) &&
            MonkChenArrival::Issued(Actor())) routeIssued = true;
    }

    void Arrived(ObjectGuid value)
    {
        if (!started || !routeIssued || arrivalDone || arrivalSeen || value != chen || !MonkChenArrival::Callback(Actor())) return;
        arrivalSeen = true; arrivalWaiting = false;
        UpdateArrival(0);
    }

    void UpdateArrival(uint32 /*diff*/)
    {
        if (!started || arrivalDone) return;
        Scenario* scenario = Progress();
        if (!scenario || scenario->GetCurrentStep() < 4 || scenario->GetCurrentStep() < lastStep ||
            (scenario->GetCurrentStep() > 5 && !EscortStage(scenario)) ||
            std::none_of(members.begin(), members.end(), [this](Member const& member) { return Retained(member) != nullptr; }))
        { Cancel(); return; }
        lastStep = scenario->GetCurrentStep();
        if (!arrivalSeen && routeIssued && MonkChenArrival::Recorded(Actor()))
        { arrivalSeen = true; arrivalWaiting = false; }
        if (!arrivalSeen) return;
        // A callback can happen during Map::Update; the later instance diff
        // also includes time BEFORE that receipt. Measure the full retry from
        // the real attempt instead of spending that same frame twice.
        if (arrivalWaiting && GetMSTimeDiffToNow(arrivalRetryStart) < 2000) return;
        arrivalWaiting = true; arrivalRetryStart = getMSTime();
        CreatureAI* ai = dynamic_cast<CreatureAI*>(Content());
        if (!ai || ai->GetData(MonkChenArrival::StatusData) != 1 || !MonkChenArrival::Pose(Actor())) return;
        uint64 generation = epoch;
        auto original = members;
        for (auto const& member : original)
            if (Player* player = Current(member))
            {
                if (scenario->GetCurrentStep() == 4 && !EscortLeaf())
                    player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 46947);
                if (!arrivalSeen || arrivalDone || epoch != generation) return;
                scenario = Progress(); player = Current(member); ai = dynamic_cast<CreatureAI*>(Content());
                if (!scenario || scenario->GetCurrentStep() < 4 || scenario->GetCurrentStep() < lastStep ||
                    (scenario->GetCurrentStep() > 5 && !EscortStage(scenario)))
                { Cancel(); return; }
                lastStep = scenario->GetCurrentStep();
                if (EscortStage(scenario) && EscortLeaf() && player && ai &&
                    ai->GetData(MonkChenArrival::StatusData) == 1 && MonkChenArrival::Pose(Actor()))
                {
                    arrivalClaim = chen; arrivalPlayer = player->GetGUID();
                    ai->MovementInform(WAYPOINT_MOTION_TYPE, 5);
                    if (!arrivalSeen || arrivalDone || epoch != generation) return;
                    ai = dynamic_cast<CreatureAI*>(Content());
                    if (ai && ai->GetData(MonkChenArrival::StatusData) == 2)
                    {
                        arrivalDone = true; handoffStartedAt = getMSTime(); handoffPlayer = member.player; handoffPending = true;
                        arrivalClaim.Clear(); arrivalPlayer.Clear(); return;
                    }
                    arrivalClaim.Clear(); arrivalPlayer.Clear();
                }
            }
        if (!arrivalWarned)
            TC_LOG_WARN("scripts", "Monk Chen: real9859005 last-node arrival46947/outro not acknowledged; retaining original quest attempts");
        arrivalWarned = true;
    }

    void RouteRequest(ObjectGuid value)
    {
        if (value == chen) UpdateRoutes();
    }

    bool EscortStage(Scenario* scenario) const
    {
        if (!scenario) return false;
        if (scenario->GetCurrentStep() == 5) return true;
        if (scenario->GetCurrentStep() != 6 || !genuineBossDefeat) return false;
        auto const* progress = scenario->GetAchievementMgr().GetCriteriaProgressMap();
        auto found = progress->find(29071);
        return found != progress->end() && found->second.Counter;
    }

    void UpdateRoutes()
    {
        if (!started || members.empty() || routeBusy) return;
        if (routeIssued && arrivalDone && !handoffPending) return;
        Scenario* scenario = Progress();
        if (!scenario || scenario->GetCurrentStep() < 4 || scenario->GetCurrentStep() < lastStep ||
            (scenario->GetCurrentStep() > 5 && !EscortStage(scenario)) ||
            std::none_of(members.begin(), members.end(), [this](Member const& member) { return Retained(member) != nullptr; }))
        { Cancel(); return; }
        lastStep = scenario->GetCurrentStep();
        bool escort = !routeIssued && lastStep == 4 && Leaf() && GetMSTimeDiffToNow(escortStartedAt) >= 8000;
        bool handoff = handoffPending && EscortStage(scenario) && EscortLeaf() && GetMSTimeDiffToNow(handoffStartedAt) >= 15000;
        if (!escort && !handoff) return;
        if (routeRetrying && GetMSTimeDiffToNow(routeRetryAt) < 2000) return;
        routeRetrying = true; routeRetryAt = getMSTime();
        Creature* actor = Actor();
        if (!actor) return;
        uint64 generation = epoch;
        auto valid = [this, generation, actor, escort]() -> bool
        {
            if (epoch != generation) return false;
            Scenario* progress = Progress();
            if (!progress || (escort ? progress->GetCurrentStep() != 4 : !EscortStage(progress)) ||
                !(escort ? Leaf() : EscortLeaf()) ||
                std::none_of(members.begin(), members.end(), [this](Member const& member) { return Retained(member) != nullptr; }))
            { Cancel(); return false; }
            return Actor() == actor;
        };
        MonkChenRoutes::Dispatch dispatch(routeBusy);
        if (escort)
        {
            if (!MonkChenArrival::Data())
            {
                if (!escortWarned) TC_LOG_WARN("scripts", "Monk intro: original Chen9859005 route unavailable; retaining the genuine rescue handoff");
                escortWarned = true; return;
            }
            escortSpoken = true;
            if (!valid()) return;
            if (!escortFollowed)
            {
                escortFollowed = true;
                if (Creature* li = actor->FindNearestCreature(98074, 50.0f, true))
                    li->GetMotionMaster()->MoveFollow(actor, PET_FOLLOW_DIST, float(PET_FOLLOW_ANGLE), MOTION_SLOT_IDLE);
            }
            if (!valid()) return;
            actor->UpdateWaypointID(uint32(-1));
            actor->GetMotionMaster()->MovePath(9859005, false);
            if (!valid()) return;
            Routed(actor->GetGUID());
            if (!routeIssued && !escortWarned)
                TC_LOG_WARN("scripts", "Monk intro: original Chen9859005 route was not installed; retaining the genuine rescue handoff");
            escortWarned = !routeIssued;
            return;
        }
        auto owner = std::find_if(members.begin(), members.end(), [this](Member const& member) { return member.player == handoffPlayer; });
        if (owner == members.end() || !Retained(*owner)) { Cancel(); return; }
        Member const originalOwner = *owner;
        Creature* guide = map->GetCreature(hait);
        if (!MonkChenRoutes::Source(guide) || guide->GetMap() != map || !guide->IsInWorld() || guide->IsPreDelete() || !guide->IsAlive() ||
            actor->FindNearestCreature(97778, 50.0f, true) != guide) return;
        auto guideValid = [this, actor, guide, &valid, &originalOwner]() -> bool
        {
            return valid() && map->GetCreature(hait) == guide && MonkChenRoutes::Source(guide) && guide->GetMap() == map &&
                guide->IsInWorld() && !guide->IsPreDelete() && guide->IsAlive() &&
                actor->FindNearestCreature(97778, 50.0f, true) == guide && Retained(originalOwner);
        };
        if (!MonkChenRoutes::Data())
        {
            if (!handoffWarned) TC_LOG_WARN("scripts", "Monk intro: original Hait9859006 route unavailable; retaining the genuine Chen handoff");
            handoffWarned = true; return;
        }
        if (!handoffAnnounced)
        { handoffAnnounced = true; sCreatureTextMgr->SendChat(guide, 12, handoffPlayer); }
        if (!guideValid()) return;
        if (!handoffSpoken)
        { handoffSpoken = true; sCreatureTextMgr->SendChat(guide, 13, handoffPlayer); }
        if (!guideValid()) return;
        guide->UpdateWaypointID(uint32(-1));
        guide->GetMotionMaster()->MovePath(9859006, false);
        if (!guideValid()) return;
        if (MonkChenRoutes::Issued(guide)) handoffPending = false;
        else if (!handoffWarned)
        { handoffWarned = true; TC_LOG_WARN("scripts", "Monk intro: original Hait9859006 route was not installed; retaining the genuine Chen handoff"); }
    }
};
}
