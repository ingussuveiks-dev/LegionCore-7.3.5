// Members of the original Taran Zhu AI. Only the opening applied-heal receipt
// moves here; later combat wounds and the original movement callbacks remain.
ObjectGuid _introHealer;
std::weak_ptr<uint8> _introHealAttempt;
uint32 _introHealTimer = 0;
uint8 _introHealTries = 0;
bool _introHealBusy = false;

Scenario* IntroHealScenario() const
{
    if (!me->IsInWorld() || !me->IsAlive() || me->IsPreDelete() || me->HasQuestObjectScope() ||
        me->GetEntry() != 101881 || me->GetDBTableGUIDLow() != 362837 || me->GetMapId() != 1541)
        return nullptr;
    Scenario* scenario = sScenarioMgr->GetScenario(me->GetInstanceId());
    return scenario && scenario->GetScenarioId() == 1007 ? scenario : nullptr;
}

bool IntroHealPlayer(Player* player) const
{
    if (!player || !player->IsInWorld() || !player->IsAlive() || !player->CanContact() ||
        player->isGameMaster() || player->HasPendingTeleport() || player->GetMap() != me->GetMap() ||
        !player->InSamePhase(me) || player->getClass() != CLASS_MONK || player->GetQuestRewardStatus(41003))
        return false;
    QuestStatus status = player->GetQuestStatus(41003);
    return status == QUEST_STATUS_INCOMPLETE || status == QUEST_STATUS_COMPLETE;
}

bool IntroHealReceipt(Scenario* scenario) const
{
    auto const* progress = scenario ? scenario->GetAchievementMgr().GetCriteriaProgressMap() : nullptr;
    if (!progress) return false;
    auto found = progress->find(46576);
    return found != progress->end() && found->second.Counter >= 1;
}

void CancelIntroHeal()
{
    _introHealer.Clear();
    _introHealAttempt.reset();
    _introHealTimer = _introHealTries = 0;
}

void OnHealApplied(Unit* who, uint32 /*amount*/, int32 /*gain*/) override
{
    ObserveWoundHeal();
    Scenario* scenario = IntroHealScenario();
    Player* player = who ? who->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr;
    if (_introHealBusy || _introHealDone || !scenario || scenario->GetCurrentStep() != 0 ||
        me->GetHealthPct() < 95 || !IntroHealPlayer(player))
        return;
    // An unrelated later healer must not replace the accepted first healer.
    // Retaken quests can start only from another real applied heal.
    if (!_introHealer.IsEmpty() && !_introHealAttempt.expired()) return;
    player->AddQuestDelayedEvent(41003, 0, []() { });
    if (QuestStatusData* status = player->getQuestStatus(41003))
    {
        _introHealer = player->GetGUID();
        _introHealAttempt = status->ScriptLifetime;
        _introHealTimer = _introHealTries = 0;
    }
}

void UpdateLifecycle(uint32 diff) override
{
    RecoverWound();
    // Preserve the former Unit timer clock, including stun and suspended AI.
    _presentationFunctions.Update(diff);
    if (_introHealBusy || _introHealer.IsEmpty()) return;
    if (_introHealTimer > diff) { _introHealTimer -= diff; return; }
    Player* player = ObjectAccessor::GetPlayer(*me, _introHealer);
    QuestStatusData* status = player ? player->getQuestStatus(41003) : nullptr;
    Scenario* scenario = IntroHealScenario();
    if (_introHealAttempt.expired() || !status || status->ScriptLifetime != _introHealAttempt.lock() ||
        !IntroHealPlayer(player) || !scenario || scenario->GetCurrentStep() > 1 || me->GetHealthPct() < 95)
    {
        CancelIntroHeal();
        return;
    }
    _introHealBusy = true;
    struct BusyGuard { bool& flag; ~BusyGuard() { flag = false; } } guard{_introHealBusy};
    if (scenario->GetCurrentStep() == 0 && !IntroHealReceipt(scenario))
        player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 48371);
    // Delivery may synchronously abandon the quest, replace its attempt or
    // change maps. Re-resolve the original owner before the old route starts.
    player = ObjectAccessor::GetPlayer(*me, _introHealer);
    status = player ? player->getQuestStatus(41003) : nullptr;
    scenario = IntroHealScenario();
    if (_introHealAttempt.expired() || !status || status->ScriptLifetime != _introHealAttempt.lock() ||
        !IntroHealPlayer(player) || !scenario || scenario->GetCurrentStep() > 1)
    {
        CancelIntroHeal();
        return;
    }
    if (scenario->GetCurrentStep() == 1 && IntroHealReceipt(scenario))
    {
        _introHealDone = true;
        CancelIntroHeal();
        Talk(2);
        me->SetByteValue(UNIT_FIELD_BYTES_1, 0, 0);
        me->SetAnimKitId(0);
        me->GetMotionMaster()->MovePoint(1, -1020.62f, -3135.73f, 26.91f);
        return;
    }
    if (++_introHealTries >= 30)
    {
        TC_LOG_WARN("scripts", "Sheilun Taran Zhu: applied opening heal48371 rejected after thirty attempts; retaining real-heal retry");
        CancelIntroHeal();
    }
    else _introHealTimer = 1000;
}
