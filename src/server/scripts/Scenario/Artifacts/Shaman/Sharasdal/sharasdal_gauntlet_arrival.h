// Members of npc_lady_zitrinAI. Only the early, real DB illusion receives
// gauntlet arrival. Her actual boss counterpart keeps its combat/death path.
ObjectGuid _gauntletRecipient;
std::weak_ptr<uint8> _gauntletAttempt;
bool _gauntletPending = false, _gauntletEarned = false, _gauntletBusy = false;
bool _gauntletExhausted = false, _gauntletPresent = false;
uint32 _gauntletRetry = 0, _gauntletTries = 0;

bool IsGauntletIllusion() const
{
    return me->GetEntry() == 104856 && me->GetDBTableGUIDLow() == 362091;
}

Scenario* GauntletWorld() const
{
    if (!IsGauntletIllusion() || me->GetMapId() != 1600 || !me->IsInWorld() || me->IsPreDelete() || !me->IsAlive()) return nullptr;
    Scenario* scenario = sScenarioMgr->GetScenario(me->GetInstanceId());
    return scenario && scenario->GetScenarioId() == 1066 ? scenario : nullptr;
}

bool GauntletPlayer(Player* player) const
{
    return player && player->IsInWorld() && player->CanContact() && player->IsAlive() &&
        !player->isGameMaster() && !player->IsBeingTeleported() && player->getClass() == CLASS_SHAMAN &&
        player->GetMap() == me->GetMap() && player->GetInstanceId() == me->GetInstanceId() &&
        player->InSamePhase(me) && player->canSeeOrDetect(me) && player->IsWithinLOSInMap(me) &&
        player->IsWithinDistInMap(me, 40.0f) && !player->GetQuestRewardStatus(40341) &&
        (player->GetQuestStatus(40341) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(40341) == QUEST_STATUS_COMPLETE);
}

void CancelGauntletArrival()
{
    _gauntletPending = _gauntletExhausted = _gauntletPresent = false;
    _gauntletRetry = _gauntletTries = 0;
    _gauntletRecipient.Clear();
    _gauntletAttempt.reset();
}

void AcceptGauntletArrival(Player* player)
{
    Scenario* scenario = GauntletWorld();
    if (_introDone || _gauntletPending || _gauntletBusy || !scenario || scenario->GetCurrentStep() != 3 ||
        !SharasdalGrash::Receipt(scenario, 47131) || !GauntletPlayer(player)) return;
    player->AddQuestDelayedEvent(40341, 0, []() {});
    QuestStatusData* status = player->getQuestStatus(40341);
    if (!status || !status->ScriptLifetime) return;
    _gauntletRecipient = player->GetGUID();
    _gauntletAttempt = status->ScriptLifetime;
    _gauntletPending = true;
    _gauntletRetry = _gauntletTries = 0;
    CompleteGauntletArrival(0);
}

void SetGUID(ObjectGuid const& guid, int32 id = 0) override
{
    if (!_gauntletPending) return;
    bool rearm = id == 40341 && guid == _gauntletRecipient;
    if (_gauntletEarned && id == 105298)
        if (Creature* grash = me->GetMap()->GetCreature(guid))
            rearm = grash->GetEntry() == 105298 && grash->GetDBTableGUIDLow() == 362092;
    if (rearm)
    {
        _gauntletExhausted = false;
        _gauntletRetry = _gauntletTries = 0;
    }
}

void CompleteGauntletArrival(uint32 diff)
{
    if (!_gauntletPending || _gauntletBusy) return;
    Scenario* scenario = GauntletWorld();
    if (!scenario) return;
    Player* player = ObjectAccessor::GetPlayer(*me, _gauntletRecipient);
    bool present = GauntletPlayer(player);
    if (_gauntletExhausted && present && !_gauntletPresent)
    {
        _gauntletExhausted = false;
        _gauntletRetry = _gauntletTries = 0;
    }
    _gauntletPresent = present;
    if (_gauntletExhausted) return;
    if (_gauntletRetry > diff) { _gauntletRetry -= diff; return; }
    struct BusyGuard { bool& flag; ~BusyGuard() { flag = false; } } guard{_gauntletBusy};
    _gauntletBusy = true;
    if (!_gauntletEarned)
    {
        QuestStatusData* status = player ? player->getQuestStatus(40341) : nullptr;
        if (_gauntletAttempt.expired() || (player && (!status || status->ScriptLifetime != _gauntletAttempt.lock() ||
            player->GetQuestRewardStatus(40341) ||
            (player->GetQuestStatus(40341) != QUEST_STATUS_INCOMPLETE && player->GetQuestStatus(40341) != QUEST_STATUS_COMPLETE))))
        {
            CancelGauntletArrival();
            return;
        }
        if (!SharasdalGrash::Receipt(scenario, 47378) &&
            (scenario->GetCurrentStep() != 3 || !SharasdalGrash::Receipt(scenario, 47131)))
        {
            TC_LOG_WARN("scripts", "Sharasdal gauntlet arrival lost its native stage/prerequisite before an accepted receipt");
            CancelGauntletArrival();
            return;
        }
        if (present && !SharasdalGrash::Receipt(scenario, 47378))
            player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 50314);
        if (SharasdalGrash::Receipt(scenario, 47378))
        {
            _gauntletEarned = _introDone = true;
            intro = false;
            _gauntletRetry = _gauntletTries = 0;
        }
    }
    if (_gauntletEarned)
    {
        // After the actual native receipt, only NPC presentation remains.
        // It may finish without its original player; it grants no new progress.
        if (!SharasdalGrash::Receipt(scenario, 47378))
        {
            CancelGauntletArrival();
            return;
        }
        if (scenario->GetCurrentStep() > 4)
        {
            _gauntletPending = false;
            me->DespawnOrUnsummon(1000);
            return;
        }
        if (scenario->GetCurrentStep() == 4)
            if (Creature* grash = me->FindNearestCreature(105298, 200.0f, true))
                if (grash->GetDBTableGUIDLow() == 362092 && grash->GetMap() == me->GetMap() && grash->IsInWorld() &&
                    !grash->IsPreDelete() && grash->IsAlive() && (grash->AI() || grash->GetDisabledAI()) && me->InSamePhase(grash))
                {
                    _gauntletPending = false;
                    // Native charm suspends the content AI. Keep this command
                    // on that original queue, once, rather than a charm AI.
                    auto guide = grash->GetDisabledAI() ? grash->GetDisabledAI() : grash->AI();
                    guide->DoAction(3);
                    me->GetMotionMaster()->MovePoint(0, 10.37f, 803.32f, 806.75f);
                    me->DespawnOrUnsummon(1000);
                    return;
                }
    }
    _gauntletRetry = 1000;
    if (++_gauntletTries >= 30)
    {
        _gauntletExhausted = true;
        TC_LOG_WARN("scripts", "Sharasdal gauntlet original arrival/native criterion or upper Grash unavailable after thirty attempts; retaining recipient/actor re-entry retry");
    }
}

void UpdateLifecycle(uint32 diff) override
{
    CompleteGauntletArrival(diff);
}
