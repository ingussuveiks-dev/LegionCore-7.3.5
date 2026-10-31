// Members of instance_sharasdal_InstanceMapScript. These in-process messages
// bind an original caller, acknowledge an installed path and retain its actual
// arrival. Only the native208580/-208580/209992 chain can award50336.
ObjectGuid ascentRecipient, ascentActor;
std::weak_ptr<uint8> ascentAttempt;
bool ascentBound = false, ascentIssued = false, ascentProgress = false, ascentArrived = false;
bool ascentBusy = false, ascentExhausted = false, ascentPresent = false, ascentScene = false;
bool ascentBindAccepted = false, ascentReentered = false, ascentFresh = false;
uint32 ascentRetry = 0, ascentTries = 0;

Scenario* AscentWorld() const
{
    if (instance->GetId() != 1600) return nullptr;
    Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId());
    return scenario && scenario->GetScenarioId() == 1066 ? scenario : nullptr;
}

Creature* AscentCreature(ObjectGuid const& guid, uint32 entry, uint64 spawn) const
{
    Creature* actor = instance->GetCreature(guid);
    return actor && actor->GetEntry() == entry && actor->GetDBTableGUIDLow() == spawn &&
        actor->GetMap() == instance && actor->IsInWorld() && !actor->IsPreDelete() && actor->IsAlive() ? actor : nullptr;
}

bool AscentQuest(Player* player) const
{
    return player && player->GetMap() == instance && player->getClass() == CLASS_SHAMAN &&
        !player->GetQuestRewardStatus(40341) &&
        (player->GetQuestStatus(40341) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(40341) == QUEST_STATUS_COMPLETE) &&
        player->GetQuestObjectiveData(40341, 105805) >= 1;
}

bool AscentPlayer(Player* player, Creature* anchor, float range) const
{
    return AscentQuest(player) && anchor && player->IsInWorld() && player->CanContact() && player->IsAlive() &&
        !player->isGameMaster() && !player->IsBeingTeleported() && player->GetInstanceId() == instance->GetInstanceId() &&
        player->InSamePhase(anchor) && player->canSeeOrDetect(anchor) && player->IsWithinLOSInMap(anchor) &&
        player->IsWithinDistInMap(anchor, range);
}

bool AscentSameAttempt(Player* player) const
{
    QuestStatusData* status = player ? player->getQuestStatus(40341) : nullptr;
    return AscentQuest(player) && !ascentAttempt.expired() && status && status->ScriptLifetime == ascentAttempt.lock();
}

WaypointMovementGenerator<Creature>* AscentPath(Creature* actor) const
{
    MovementGenerator* movement = actor->GetMotionMaster()->GetMotionSlot(MOTION_SLOT_IDLE);
    if (!movement || movement->GetMovementGeneratorType() != WAYPOINT_MOTION_TYPE) return nullptr;
    auto path = static_cast<WaypointMovementGenerator<Creature>*>(movement);
    return path->GetPathId() == 10993810 ? path : nullptr;
}

bool AscentEndPose(Creature* actor) const
{
    WaypointPath const* path = sWaypointMgr->GetPathScript(10993810);
    if (!path || path->size() != 6 || !actor->movespline->Finalized()) return false;
    // The authored route uses integer offsets[-5,5] in X/Y. Require the real
    // final vicinity as well as the issued/native arrival, without combat reach.
    auto const& end = path->back();
    float x = actor->GetPositionX() - end.x, y = actor->GetPositionY() - end.y, z = actor->GetPositionZ() - end.z;
    return x*x <= 36.0f && y*y <= 36.0f && z*z <= 1.0f;
}

void CancelAscent()
{
    ascentBound = ascentIssued = ascentProgress = ascentArrived = false;
    ascentExhausted = ascentPresent = ascentScene = ascentBindAccepted = ascentReentered = ascentFresh = false;
    ascentRetry = ascentTries = 0;
    ascentRecipient.Clear();
    ascentActor.Clear();
    ascentAttempt.reset();
}

ObjectGuid GetGuidData(uint32 id) const override
{
    return id == 208580 && ascentBindAccepted ? ascentRecipient : ObjectGuid::Empty;
}

void SetGuidData(uint32 id, ObjectGuid guid) override
{
    if (id == 208580) // Actual validated lower-Grash step2 gossip caller.
    {
        ascentBindAccepted = false;
        Scenario* scenario = AscentWorld();
        Creature* grash = AscentCreature(lowerGrash, 105298, 362066);
        Player* player = ObjectAccessor::FindPlayer(instance, guid);
        if (ascentBusy || !scenario || scenario->GetCurrentStep() != 2 || !AscentPlayer(player, grash, INTERACTION_DISTANCE)) return;
        if (ascentBound)
        {
            if (guid != ascentRecipient) return;
            if (AscentSameAttempt(player)) { ascentBindAccepted = true; return; }
            CancelAscent();
        }
        player->AddQuestDelayedEvent(40341, 0, []() {});
        QuestStatusData* status = player->getQuestStatus(40341);
        if (!status || !status->ScriptLifetime) return;
        ascentRecipient = guid;
        ascentAttempt = status->ScriptLifetime;
        ascentBound = ascentBindAccepted = ascentFresh = true;
        return;
    }
    // Keep physical receipts while the original caller is offline. A true
    // re-entry must revalidate the persisted quest before any camera cast.
    if (ascentBusy || !ascentBound) return;
    Scenario* scenario = AscentWorld();
    if (!scenario || scenario->GetCurrentStep() < 2 || scenario->GetCurrentStep() > 3) return;
    Creature* actor = AscentCreature(guid, 102826, 362063);
    if (!actor) return;
    if (id == 10993810) // Original Grash callback, after real MovePath installation.
    {
        auto path = AscentPath(actor);
        if (!path || path->GetCurrentNode() != 0 || (ascentIssued && guid != ascentActor)) return;
        if (ascentIssued) return;
        ascentActor = guid;
        ascentIssued = ascentFresh = true;
        return;
    }
    if (id == 50336 && ascentIssued && guid == ascentActor) // Actual Erunak final waypoint, not a credit message.
    {
        auto path = AscentPath(actor);
        if (!path || path->GetCurrentNode() != 5 || !AscentEndPose(actor)) return;
        ascentArrived = ascentFresh = true;
    }
}

void RearmAscent(Player* player)
{
    if (!player || !ascentBound || player->GetGUID() != ascentRecipient) return;
    ascentReentered = ascentFresh = true;
    ascentExhausted = false;
    ascentRetry = ascentTries = 0;
}

Creature* PollAscentArrival()
{
    Creature* actor = AscentCreature(ascentActor, 102826, 362063);
    if (!ascentIssued || ascentArrived || !actor) return actor;
    auto path = AscentPath(actor);
    MovementGenerator* movement = actor->GetMotionMaster()->GetMotionSlot(MOTION_SLOT_IDLE);
    if (!path && movement && movement->GetMovementGeneratorType() == WAYPOINT_MOTION_TYPE)
    {
        ascentIssued = ascentProgress = false;
        ascentActor.Clear();
        return nullptr;
    }
    if (path)
    {
        if (path->GetCurrentNode() > 0 && actor->GetCurrentWaypointID() < 5) ascentProgress = true;
        if (path->GetCurrentNode() == 5 && actor->GetCurrentWaypointID() == 5 && AscentEndPose(actor)) ascentArrived = true;
    }
    else if (ascentProgress && actor->GetCurrentWaypointID() == 5 && AscentEndPose(actor))
    {
        // Native OnArrived wrote the real node even when a temporary/null AI
        // missed its callback and the final update removed its IDLE generator.
        ascentArrived = true;
    }
    return actor;
}

void UpdateAscent(uint32 diff)
{
    if (!ascentBound || ascentBusy) return;
    if (ascentFresh) { ascentFresh = false; diff = 0; }
    Scenario* scenario = AscentWorld();
    bool received = false;
    if (scenario)
    {
        auto const* progress = scenario->GetAchievementMgr().GetCriteriaProgressMap();
        auto found = progress->find(47131);
        received = found != progress->end() && found->second.Counter;
    }
    if (!scenario || scenario->GetCurrentStep() < 2 || scenario->GetCurrentStep() > 3 || received)
    {
        CancelAscent();
        return;
    }
    Creature* actor = PollAscentArrival(); // Retain physical evidence even during the original caller's logout.
    Player* player = ObjectAccessor::FindPlayer(instance, ascentRecipient);
    if (ascentAttempt.expired())
    {
        if (!player) return; // Native instance lifetime owns this single absent caller's receipt.
        if (ascentReentered && (!player->CanContact() || player->IsBeingTeleported())) return;
        // A true map re-entry may recreate the login's in-memory quest token.
        // Persisted real105805 is required; a newly accepted empty quest cannot
        // inherit the earlier run. No objective or scenario counter is changed.
        if (!ascentReentered || !AscentQuest(player)) { CancelAscent(); return; }
        player->AddQuestDelayedEvent(40341, 0, []() {});
        QuestStatusData* status = player->getQuestStatus(40341);
        if (!status || !status->ScriptLifetime) { CancelAscent(); return; }
        ascentAttempt = status->ScriptLifetime;
    }
    ascentReentered = false;
    if (player && !AscentSameAttempt(player)) { CancelAscent(); return; }
    if (!ascentArrived || scenario->GetCurrentStep() == 2) return; // Wait for actual Kraliss/wall completion.
    bool present = actor && AscentPlayer(player, actor, 100.0f);
    if (ascentExhausted && present && !ascentPresent)
    {
        ascentExhausted = false;
        ascentRetry = ascentTries = 0;
    }
    ascentPresent = present;
    if (player && player->HasAura(208580))
    {
        ascentScene = true;
        ascentExhausted = false;
        ascentRetry = ascentTries = 0;
        return; // Actual scene underway; only real47131 completes this pending delivery.
    }
    if (ascentScene)
    {
        ascentScene = ascentExhausted = false;
        ascentRetry = ascentTries = 0; // Death/interruption without real ascent may retry for this same attempt.
    }
    if (ascentExhausted) return;
    if (ascentRetry > diff) { ascentRetry -= diff; return; }
    struct BusyGuard { bool& flag; ~BusyGuard() { flag = false; } } guard{ascentBusy};
    ascentBusy = true;
    if (present)
    {
        if (!actor->HasAura(83810)) actor->CastSpell(actor, 83810, false);
        SpellCastResult result = player->CastSpell(player, 208580, false);
        player = ObjectAccessor::FindPlayer(instance, ascentRecipient);
        if (!player || !AscentSameAttempt(player)) { CancelAscent(); return; }
        if (result == SPELL_CAST_OK && player->HasAura(208580))
        {
            ascentScene = true;
            ascentRetry = ascentTries = 0;
            return;
        }
    }
    ascentRetry = 1000;
    if (++ascentTries >= 30)
    {
        ascentExhausted = true;
        TC_LOG_WARN("scripts", "Sharasdal earned lower ascent cannot reach its original quest participant or native camera after thirty attempts; retaining original re-entry recovery");
    }
}
