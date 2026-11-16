// Members of the original lower/upper Erunak AI. End combat without the
// generic home/reset/aura cleanup: his wound and narrative queues must survive.
bool _evadeChasePending = false;

void CompleteEvadeMovement()
{
    if (!_evadeChasePending) return;
    auto motion = me->GetMotionMaster();
    if (me->isInCombat() || motion->GetMotionSlotType(MOTION_SLOT_ACTIVE) != CHASE_MOTION_TYPE)
    {
        _evadeChasePending = false;
        return;
    }
    // Fear/distraction owns the controlled slot until its native expiry. Do
    // not remove it to reach the obsolete chase underneath it.
    if (motion->GetCurrentMovementGeneratorType() != CHASE_MOTION_TYPE) return;
    _evadeChasePending = false;
    motion->MovementExpired(); // Reset/initialize the uncovered authored route.
}

void EnterEvadeMode() override
{
    if (!me->IsAlive()) return;
    me->DeleteThreatList();
    me->ClearSaveThreatTarget();
    me->CombatStop(true);
    me->SetCannotReachTarget(false);
    _evadeChasePending = me->GetMotionMaster()->GetMotionSlotType(MOTION_SLOT_ACTIVE) == CHASE_MOTION_TYPE;
    CompleteEvadeMovement();
    // Real Brute deaths and all progression remain owned by the instance.
}
