// Members of npc_grashAI. The upper Grash has a separate authored point route;
// neither a gossip click nor a finalized, interrupted spline opens his wall.
ObjectGuid _upperRecipient;
std::weak_ptr<uint8> _upperAttempt;
uint32 _upperSerial = 0, _upperPoint = 0, _upperElapsed = 0, _upperRetry = 0, _upperTries = 0;
bool _upperMoving = false, _upperArrived = false, _upperCast = false, _upperOpened = false;
bool _upperBusy = false, _upperExhausted = false, _upperPresent = false;

bool IsUpperGrash() const
{
    return me->GetEntry() == 105298 && me->GetDBTableGUIDLow() == 362092;
}

Scenario* UpperWallScenario() const
{
    if (!IsUpperGrash() || me->GetMapId() != 1600 || !me->IsInWorld() || me->IsPreDelete() || !me->IsAlive()) return nullptr;
    Scenario* scenario = sScenarioMgr->GetScenario(me->GetInstanceId());
    return scenario && scenario->GetScenarioId() == 1066 ? scenario : nullptr;
}

bool UpperWallPose() const
{
    float x = me->GetPositionX() + 17.96f, y = me->GetPositionY() - 802.05f, z = me->GetPositionZ() - 807.66f;
    // Actual center distance, without the ogre's 4.5-yard combat reach.
    return x*x + y*y + z*z <= 1.0f;
}

void CancelUpperWall()
{
    _upperMoving = _upperArrived = _upperCast = _upperOpened = _upperExhausted = _upperPresent = false;
    _upperPoint = _upperElapsed = _upperRetry = _upperTries = 0;
    _upperRecipient.Clear();
    _upperAttempt.reset();
    _gossipActions &= ~(1u << 4);
    Scenario* scenario = UpperWallScenario();
    if (scenario && scenario->GetCurrentStep() == 4 && !SharasdalGrash::Receipt(scenario, 47133))
        me->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
}

bool BindUpperWall(Player* player)
{
    Scenario* scenario = UpperWallScenario();
    if (!scenario || scenario->GetCurrentStep() != 4 || SharasdalGrash::Receipt(scenario, 47133) ||
        _upperSerial == uint32(-1)) return false;
    player->AddQuestDelayedEvent(40341, 0, []() {});
    QuestStatusData* status = player->getQuestStatus(40341);
    if (!status || !status->ScriptLifetime) return false;
    _upperRecipient = player->GetGUID();
    _upperAttempt = status->ScriptLifetime;
    // A new explicit request cannot consume the previous generator's callback.
    _upperPoint = ++_upperSerial;
    _upperMoving = true;
    _upperArrived = _upperCast = _upperOpened = _upperExhausted = _upperPresent = false;
    _upperElapsed = _upperRetry = _upperTries = 0;
    return true;
}

void UpperWallArrival(uint32 id)
{
    if (!_upperMoving || id != _upperPoint) return;
    _upperMoving = false;
    if (!UpperWallScenario() || !UpperWallPose())
    {
        TC_LOG_WARN("scripts", "Sharasdal upper Grash point finalized away from its wall; retaining explicit gossip retry");
        CancelUpperWall();
        return;
    }
    _upperArrived = true;
    _upperRetry = _upperTries = 0;
    CompleteUpperWall(0);
}

void CompleteUpperWall(uint32 diff)
{
    if (_upperBusy || (!_upperMoving && !_upperArrived)) return;
    Scenario* scenario = UpperWallScenario();
    if (!scenario) return;
    if (SharasdalGrash::Receipt(scenario, 47133))
    {
        _upperMoving = _upperArrived = false;
        return;
    }
    Player* player = ObjectAccessor::GetPlayer(*me, _upperRecipient);
    QuestStatusData* status = player ? player->getQuestStatus(40341) : nullptr;
    if (_upperAttempt.expired() || (player && (!status || status->ScriptLifetime != _upperAttempt.lock() ||
        player->GetQuestRewardStatus(40341) ||
        (player->GetQuestStatus(40341) != QUEST_STATUS_INCOMPLETE && player->GetQuestStatus(40341) != QUEST_STATUS_COMPLETE))))
    {
        TC_LOG_WARN("scripts", "Sharasdal upper wall cancelled after its original quest attempt ended");
        CancelUpperWall();
        return;
    }
    if (scenario->GetCurrentStep() != 4)
    {
        TC_LOG_WARN("scripts", "Sharasdal upper wall has no accepted criterion after its stage ended");
        CancelUpperWall();
        return;
    }
    if (_upperMoving)
    {
        // The original spawn is 164 yards away. Allow five minutes of unblocked
        // travel, pausing for wounds/root/casts; never restart the spline here.
        if (me->HasUnitState(UNIT_STATE_NOT_MOVE | UNIT_STATE_CASTING)) return;
        if (diff >= 300000 - _upperElapsed)
        {
            TC_LOG_WARN("scripts", "Sharasdal upper Grash did not finish its issued point; retaining explicit gossip retry");
            CancelUpperWall();
        }
        else _upperElapsed += diff;
        return;
    }
    if (!_upperOpened && me->HasUnitState(UNIT_STATE_NOT_MOVE | UNIT_STATE_CASTING)) return;
    bool present = SharasdalGrash::Eligible(player, me);
    if (_upperExhausted && present && !_upperPresent)
    {
        _upperExhausted = false;
        _upperRetry = _upperTries = 0;
    }
    _upperPresent = present;
    if (_upperExhausted) return;
    if (_upperRetry > diff) { _upperRetry -= diff; return; }
    struct BusyGuard { bool& flag; ~BusyGuard() { flag = false; } } guard{_upperBusy};
    _upperBusy = true;
    if (!_upperOpened && UpperWallPose() && !me->HasUnitState(UNIT_STATE_NOT_MOVE | UNIT_STATE_CASTING))
    {
        // Native209904 is instant/self-range with an enemy cone. Preserve the
        // ordinary cast checks; do not open a door after a rejected cast.
        if (!_upperCast) _upperCast = me->CastSpell(me, 209904, false) == SPELL_CAST_OK;
        if (_upperCast)
            if (GameObject* go = me->FindNearestGameObject(248514, 5.0f))
                if (go->GetDBTableGUIDLow() == 134298 && go->IsInWorld() && !go->IsPreDelete() &&
                    go->GetMap() == me->GetMap() && me->InSamePhase(go))
                    if (InstanceScript* instance = me->GetInstanceScript())
                    {
                        instance->HandleGameObject(go->GetGUID(), true, go);
                        _upperOpened = go->GetGoState() == GO_STATE_ACTIVE;
                        if (_upperOpened)
                            if (Creature* lady = me->FindNearestCreature(104856, 90.0f, true))
                                if (lady->GetDBTableGUIDLow() == 362095 && lady->IsInWorld() && !lady->IsPreDelete() &&
                                    lady->IsAlive() && me->InSamePhase(lady))
                                {
                                    AttackStart(lady);
                                    for (uint32 entry : {102826u, 104853u})
                                        if (Creature* ally = me->FindNearestCreature(entry, 40.0f, true))
                                            if (ally->GetDBTableGUIDLow() == (entry == 102826 ? 362093 : 362094) &&
                                                ally->IsInWorld() && !ally->IsPreDelete() && ally->IsAlive() && ally->AI() && me->InSamePhase(ally))
                                                ally->AI()->AttackStart(lady);
                                }
                    }
    }
    if (_upperOpened && present) player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 49819);
    if (SharasdalGrash::Receipt(scenario, 47133)) _upperArrived = false;
    else
    {
        _upperRetry = 1000;
        if (++_upperTries >= 30)
        {
            if (!_upperOpened)
            {
                TC_LOG_WARN("scripts", "Sharasdal upper wall cast/object unavailable after thirty attempts; retaining explicit gossip retry");
                CancelUpperWall();
            }
            else
            {
                _upperExhausted = true;
                TC_LOG_WARN("scripts", "Sharasdal opened upper wall could not reach its original recipient/native criterion after thirty attempts; retaining original-recipient re-entry retry");
            }
        }
    }
}
