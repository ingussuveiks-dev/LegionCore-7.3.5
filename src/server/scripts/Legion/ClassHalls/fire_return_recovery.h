#pragma once

namespace FireReturnRecovery
{
struct State
{
    uint32 instance = 0;
    uint32 timer = 0;
    uint32 elapsed = 0;
    bool stopped = false;
    bool waiting = false;
};

bool Earned(Player* player)
{
    if (!player || !player->IsInWorld() || !player->CanContact() || !player->IsAlive() ||
        player->IsBeingTeleported() || player->GetMapId() != 1480)
        return false;
    Scenario* scenario = sScenarioMgr->GetScenario(player->GetInstanceId());
    if (!scenario || scenario->GetScenarioId() != 957 || scenario->GetCurrentStep() < 4 || scenario->GetCurrentStep() > 5)
        return false;
    auto progress = scenario->GetAchievementMgr().GetCriteriaProgressMap();
    auto defeated = progress->find(45965); // Native47380, actual957/3 death leaf.
    return defeated != progress->end() && defeated->second.Counter;
}

bool IntroScoped()
{
    auto conditions = sConditionMgr->GetConditionsForSmartEvent(99763, 0, SMART_SCRIPT_TYPE_CREATURE);
    if (conditions.size() != 2)
        return false;
    bool map = false, step = false;
    for (Condition const* condition : conditions)
    {
        if (!condition || condition->SourceType != CONDITION_SOURCE_TYPE_SMART_EVENT || condition->SourceGroup != 1 ||
            condition->SourceEntry != 99763 || condition->SourceId || condition->ElseGroup || condition->ConditionTarget != 1 || condition->NegativeCondition ||
            condition->ReferenceId || condition->ScriptId || condition->ConditionValue3)
            return false;
        if (condition->ConditionType == CONDITION_MAPID && condition->ConditionValue1 == 1480 && !condition->ConditionValue2)
            map = true;
        else if (condition->ConditionType == CONDITION_SCENARION_STEP && condition->ConditionValue1 == 957 && condition->ConditionValue2 == 3)
            step = true;
        else
            return false;
    }
    return map && step;
}

bool Near(float value, float expected) { return std::fabs(value - expected) < 0.02f; }

bool NativeSummon(SmartScriptHolder& summon)
{
    CreatureTemplate const* boss = sObjectMgr->GetCreatureTemplate(99615);
    if (!boss || boss->ScriptID || boss->AIName != "SmartAI")
        return false;
    uint32 found = 0;
    for (auto const& row : sSmartScriptMgr->GetScript(99615, SMART_SCRIPT_TYPE_CREATURE))
        if (row.event_id == 23)
        {
            ++found;
            summon = row;
        }
    return found == 1 && summon.entryOrGuid == 99615 && summon.GetScriptType() == SMART_SCRIPT_TYPE_CREATURE &&
        summon.GetEventType() == SMART_EVENT_TEXT_OVER && !summon.link &&
        summon.event.event_chance == 100 && !summon.event.event_phase_mask &&
        summon.event.event_flags == SMART_EVENT_FLAG_NOT_REPEATABLE &&
        summon.event.raw.param1 == 6 && summon.event.raw.param2 == 99615 &&
        !summon.event.raw.param3 && !summon.event.raw.param4 && !summon.event.raw.param5 &&
        summon.GetActionType() == SMART_ACTION_SUMMON_CREATURE && summon.action.raw.param1 == 99763 &&
        summon.action.raw.param2 == 8 && !summon.action.raw.param3 && !summon.action.raw.param4 &&
        !summon.action.raw.param5 && !summon.action.raw.param6 && summon.GetTargetType() == SMART_TARGET_POSITION &&
        !summon.target.raw.param1 && !summon.target.raw.param2 && !summon.target.raw.param3 && !summon.target.raw.param4 &&
        Near(summon.target.x, 4657.79f) && Near(summon.target.y, 2785.37f) &&
        Near(summon.target.z, 361.19f) && Near(summon.target.o, 5.38595f);
}

bool NativeAethas()
{
    uint32 scriptId = sObjectMgr->GetScriptId("npc_fire_artifact_aethas");
    CreatureTemplate const* actor = sObjectMgr->GetCreatureTemplate(99763);
    if (!scriptId || !actor || actor->ScriptID != scriptId || actor->AIName != "SmartAI")
        return false;
    struct Row { uint32 id, event, link, action, first, second, target, targetFirst, targetSecond, eventFirst, eventSecond; };
    Row const expected[] = {
        {0,60,1,11,12980,0,1,0,0,0,0}, {1,61,2,11,196561,0,1,0,0,0,0},
        {2,61,0,11,196347,0,19,99615,50,0,0}, {3,38,0,11,196381,0,1,0,0,2,2},
        {4,38,5,28,0,0,1,0,0,3,3}, {5,61,6,53,1,99763,1,0,0,0,0},
        {6,61,0,1,0,0,1,0,0,0,0}, {7,40,8,1,1,0,1,0,0,1,99763},
        {8,61,9,11,223908,0,1,0,0,0,0}, {9,61,10,12,99861,8,8,0,0,0,0},
        {10,61,0,11,196520,0,1,0,0,0,0}
    };
    auto rows = sSmartScriptMgr->GetScript(99763, SMART_SCRIPT_TYPE_CREATURE);
    if (rows.size() != 11 || !IntroScoped())
        return false;
    for (Row const& required : expected)
    {
        uint32 found = 0;
        for (auto const& row : rows)
            if (row.event_id == required.id)
            {
                ++found;
                if (row.entryOrGuid != 99763 || row.GetScriptType() != SMART_SCRIPT_TYPE_CREATURE ||
                    row.GetEventType() != required.event || row.link != required.link ||
                    row.event.event_chance != 100 || row.event.event_phase_mask ||
                    row.event.event_flags != SMART_EVENT_FLAG_NOT_REPEATABLE ||
                    row.event.raw.param1 != required.eventFirst || row.event.raw.param2 != required.eventSecond ||
                    row.event.raw.param3 || row.event.raw.param4 || row.event.raw.param5 ||
                    row.GetActionType() != required.action || row.action.raw.param1 != required.first || row.action.raw.param2 != required.second ||
                    row.action.raw.param3 || row.action.raw.param4 || row.action.raw.param5 || row.action.raw.param6 ||
                    row.GetTargetType() != required.target || row.target.raw.param1 != required.targetFirst ||
                    row.target.raw.param2 != required.targetSecond || row.target.raw.param3 || row.target.raw.param4)
                    return false;
                if (required.id == 9 && (!Near(row.target.x, 4625.47f) || !Near(row.target.y, 2769.47f) ||
                    !Near(row.target.z, 361.248f) || !Near(row.target.o, 0.f)))
                    return false;
                if (required.id != 0 && !sConditionMgr->GetConditionsForSmartEvent(99763, required.id, SMART_SCRIPT_TYPE_CREATURE).empty())
                    return false;
            }
        if (found != 1)
            return false;
    }
    return true;
}

Creature* Find(Player* player, uint32 entry, SmartScriptHolder const& summon, bool alive = true)
{
    std::list<Creature*> actors;
    player->GetCreatureListWithEntryInGrid(actors, entry, 160.f);
    for (Creature* actor : actors)
        if (actor && actor->IsInWorld() && actor->IsAlive() == alive && !actor->IsQuestObjectExpired() &&
            actor->GetMap() == player->GetMap() && player->InSamePhase(actor) &&
            actor->IsWithinDist3d(summon.target.x, summon.target.y, summon.target.z, 70.f) &&
            (!actor->HasQuestObjectScope() || actor->CanTargetQuestObject(player)))
            return actor;
    return nullptr;
}

bool NativeActor(Creature* actor, bool smart)
{
    if (!actor || actor->GetCreatureTemplate()->AIName != (smart ? "SmartAI" : ""))
        return false;
    uint32 scriptId = actor->GetEntry() == 99763 ? sObjectMgr->GetScriptId("npc_fire_artifact_aethas") : 0;
    if (actor->GetScriptId() != scriptId || (actor->GetEntry() == 99763 && !scriptId))
        return false;
    return !actor->GetDBTableGUIDLow() || sSmartScriptMgr->GetScript(-int32(actor->GetDBTableGUIDLow()), SMART_SCRIPT_TYPE_CREATURE).empty();
}
}

struct npc_fire_artifact_aethas : SmartAI
{
    explicit npc_fire_artifact_aethas(Creature* actor) : SmartAI(actor) { }
    bool speechPending = false;
    uint32 epoch = 0;

    void Reset() override
    {
        ++epoch;
        speechPending = false;
        SmartAI::Reset();
    }

    void SetData(uint32 id, uint32 value) override
    {
        uint32 current = epoch;
        bool first = id == 3 && value == 3 && !GetScript()->HasRunEvent(4, SMART_EVENT_DATA_SET);
        SmartAI::SetData(id, value);
        if (first && current == epoch)
            speechPending = GetScript()->HasRunEvent(4, SMART_EVENT_DATA_SET) &&
                !HasEscortState(SMART_ESCORT_ESCORTING) && !GetScript()->HasRunEvent(7, SMART_EVENT_WAYPOINT_REACHED);
    }

    bool Recover(Player* player, SmartScriptHolder const& summon)
    {
        if (!FireReturnRecovery::Earned(player) || !FireReturnRecovery::NativeAethas())
            return false;
        if (!GetScript()->HasRunEvent(4, SMART_EVENT_DATA_SET))
            SetData(3, 3);
        if (!GetScript()->HasRunEvent(4, SMART_EVENT_DATA_SET))
            return false;
        if (GetScript()->HasRunEvent(7, SMART_EVENT_WAYPOINT_REACHED))
        {
            if (Creature* portal = FireReturnRecovery::Find(player, 99861, summon))
                return FireReturnRecovery::NativeActor(portal, false);
            // The real waypoint was reached, but native action12 can fail.
            // Retry only its original portal summon, never its speech/story.
            for (auto const& row : sSmartScriptMgr->GetScript(99763, SMART_SCRIPT_TYPE_CREATURE))
                if (row.event_id == 9)
                    return me->SummonCreature(row.action.summonCreature.creature, row.target.x, row.target.y, row.target.z,
                        row.target.o, static_cast<TempSummonType>(row.action.summonCreature.type), row.action.summonCreature.duration) != nullptr;
            return false;
        }
        if (!HasEscortState(SMART_ESCORT_ESCORTING) && !me->isInCombat() && StartPath(true, 99763, false, nullptr) && speechPending)
        {
            // A rejected native WP_START returned before LINK6. Deliver its
            // pending line once only after the same native path accepts.
            speechPending = false;
            for (auto row : sSmartScriptMgr->GetScript(99763, SMART_SCRIPT_TYPE_CREATURE))
                if (row.event_id == 6)
                    GetScript()->ProcessAction(row);
        }
        return false; // Wait for actual movement/waypoint and the real portal.
    }
};

namespace FireReturnRecovery
{

bool Attempt(Player* player, SmartScriptHolder const& summon)
{
    if (Creature* portal = Find(player, 99861, summon))
        return NativeActor(portal, false);
    Creature* actor = Find(player, 99763, summon);
    if (!actor)
    {
        // Keep the original source when its corpse is present. An earned
        // re-entry can outlive that corpse; recover only the same native actor.
        WorldObject* source = Find(player, 99615, summon, false);
        if (source && !NativeActor(source->ToCreature(), true))
            return false;
        if (!source)
            source = player;
        actor = source->SummonCreature(summon.action.summonCreature.creature, summon.target.x, summon.target.y,
            summon.target.z, summon.target.o, static_cast<TempSummonType>(summon.action.summonCreature.type), summon.action.summonCreature.duration);
    }
    if (!Earned(player) || !actor || !NativeActor(actor, true) || !actor->IsAIEnabled || !actor->AI() ||
        !actor->IsInWorld() || actor->GetMap() != player->GetMap() || !player->InSamePhase(actor))
        return false;
    auto ai = dynamic_cast<npc_fire_artifact_aethas*>(actor->AI());
    return ai && ai->Recover(player, summon);
}

void Tick(Player* player, uint32 diff, State& state)
{
    if (!Earned(player))
        return;
    if (state.instance != player->GetInstanceId())
    {
        state = State{};
        state.instance = player->GetInstanceId();
    }
    if (state.stopped)
        return;
    if (state.waiting)
        state.elapsed += diff >= 30000 - state.elapsed ? 30000 - state.elapsed : diff;
    if (state.timer > diff)
    {
        state.timer -= diff;
        return;
    }
    state.timer = 500;
    SmartScriptHolder summon;
    if (!NativeSummon(summon) || !NativeAethas())
    {
        state.stopped = true;
        TC_LOG_WARN("scripts", "Fire artifact return: native Aethas data/scope differs for player %u; recovery withheld", player->GetGUIDLow());
        return;
    }
    if (!player->IsWithinDist3d(summon.target.x, summon.target.y, summon.target.z, 100.f))
    {
        state.waiting = false;
        state.elapsed = 0;
        return;
    }
    if (Attempt(player, summon))
    {
        state.elapsed = 0;
        state.waiting = false;
        return;
    }
    state.waiting = true;
    if (state.elapsed >= 30000)
    {
        state.stopped = true;
        TC_LOG_WARN("scripts", "Fire artifact return: native return unavailable after 30000ms for player %u; retain earned scenario and retry on re-entry", player->GetGUIDLow());
    }
}

std::mutex stateLock;
std::unordered_map<ObjectGuid, State> states;
void Forget(Player* player)
{
    if (!player)
        return;
    std::lock_guard<std::mutex> guard(stateLock);
    states.erase(player->GetGUID());
}
void Update(Player* player, uint32 diff)
{
    if (!player)
        return;
    if (!Earned(player))
    {
        Forget(player);
        return;
    }
    State state;
    {
        std::lock_guard<std::mutex> guard(stateLock);
        auto found = states.find(player->GetGUID());
        if (found != states.end())
            state = found->second;
    }
    Tick(player, diff, state);
    if (!Earned(player))
    {
        Forget(player);
        return;
    }
    std::lock_guard<std::mutex> guard(stateLock);
    states[player->GetGUID()] = state;
}
}

class player_fire_artifact_return : public PlayerScript
{
public:
    player_fire_artifact_return() : PlayerScript("player_fire_artifact_return") { }
    void OnUpdate(Player* player, uint32 diff) override { FireReturnRecovery::Update(player, diff); }
    void OnLogout(Player* player) override { FireReturnRecovery::Forget(player); }
    void OnMapChanged(Player* player) override { FireReturnRecovery::Forget(player); }
};
