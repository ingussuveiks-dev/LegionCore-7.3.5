// Members of npc_erunak_stonespeakerAI. Upper362093 has no player credit:
// native ascent authorizes its approach, and an actually created1868 finishes it.
ObjectGuid _upperRecipient;
std::weak_ptr<uint8> _upperAttempt;
ObjectGuid _upperGuides[2];
uint8 _upperPaths = 0, _upperTalks = 0;
bool _upperPending = false, _upperWaiting = false, _upperBusy = false;
bool _upperExhausted = false, _upperPresent = false, _upperReady = false;
uint32 _upperRetry = 0, _upperTries = 0, _upperDelay = 0;

Scenario* UpperIntroWorld() const
{
    if (me->GetEntry() != 102826 || me->GetDBTableGUIDLow() != 362093 || me->GetMapId() != 1600 ||
        !me->IsInWorld() || me->IsPreDelete() || !me->IsAlive()) return nullptr;
    Scenario* scenario = sScenarioMgr->GetScenario(me->GetInstanceId());
    return scenario && scenario->GetScenarioId() == 1066 ? scenario : nullptr;
}

bool UpperIntroActor(Creature* actor, uint32 entry, uint64 spawn) const
{
    return actor && actor->GetEntry() == entry && actor->GetDBTableGUIDLow() == spawn &&
        actor->IsInWorld() && !actor->IsPreDelete() && actor->IsAlive() &&
        actor->GetMap() == me->GetMap() && actor->InSamePhase(me);
}

CreatureAI* UpperContentAI(Creature* actor) const
{
    return actor->GetDisabledAI() ? static_cast<CreatureAI*>(actor->GetDisabledAI()) : actor->AI();
}

void CancelUpperIntro()
{
    _upperPending = _upperWaiting = _upperExhausted = _upperPresent = _upperReady = false;
    _upperRetry = _upperTries = _upperDelay = 0;
    _upperRecipient.Clear();
    _upperAttempt.reset();
    // Preserve acknowledged paths/speech: abandoning a quest does not undo
    // movement already issued to the real companions or authorize a replay.
}

void AcceptUpperIntro(Player* player)
{
    Scenario* scenario = UpperIntroWorld();
    // LowerEntryPlayer/Attempt are the shared contact and quest-lifetime
    // policies; the distinct upper world/role/native step is checked here.
    if (_introDone || _upperBusy || !scenario || scenario->GetCurrentStep() != 3 ||
        !LowerEntryPlayer(player, 20.0f)) return;
    if (_upperPending)
    {
        if (!_upperAttempt.expired()) return;
        CancelUpperIntro();
    }
    player->AddQuestDelayedEvent(40341, 0, []() {});
    QuestStatusData* status = player->getQuestStatus(40341);
    if (!status || !status->ScriptLifetime) return;
    _upperRecipient = player->GetGUID();
    _upperAttempt = status->ScriptLifetime;
    _upperPending = true;
    CompleteUpperIntro(0);
}

void SetGUID(ObjectGuid const& guid, int32 id = 0) override
{
    if (id != 40341) return;
    // Re-entry can occur while an inactive grid has observed no absent tick.
    // Rearm only this same original visitor; neither branch grants progress.
    auto lower = _lowerEntries.find(guid);
    if (lower != _lowerEntries.end())
    {
        lower->second.exhausted = false;
        lower->second.retry = lower->second.tries = 0;
    }
    if (_upperPending && guid == _upperRecipient)
    {
        _upperExhausted = false;
        _upperRetry = _upperTries = 0;
    }
}

void ProbeErunakArrival()
{
    // A first visibility visit can occur while the native teleport/loading
    // guards still reject contact. Revisit actual nearby map players after
    // those guards settle; a pet or a spawn itself is never an entry receipt.
    if (LowerEntryWorld())
    {
        for (auto const& reference : me->GetMap()->GetPlayers())
            if (Player* player = reference.getSource())
                if (LowerEntryPlayer(player, 20.0f)) AcceptLowerEntry(player);
        return;
    }
    Scenario* scenario = UpperIntroWorld();
    if (_introDone || _upperPending || !scenario || scenario->GetCurrentStep() != 3) return;
    Player* candidate = nullptr;
    for (auto const& reference : me->GetMap()->GetPlayers())
        if (Player* player = reference.getSource())
            if (LowerEntryPlayer(player, 20.0f))
            {
                if (candidate) return; // Do not choose an unrelated replacement.
                candidate = player;
            }
    if (candidate) AcceptUpperIntro(candidate);
}

void CompleteUpperIntro(uint32 diff)
{
    if (!_upperPending || _upperBusy) return;
    Scenario* scenario = UpperIntroWorld();
    if (!scenario) { CancelUpperIntro(); return; }
    if (scenario->GetCurrentStep() < 3 || scenario->GetCurrentStep() > 4 ||
        (!_upperWaiting && scenario->GetCurrentStep() != 3))
    {
        // A late approach must never replace Grash's real stage4 wall route.
        _introDone = scenario->GetCurrentStep() > 3;
        CancelUpperIntro();
        return;
    }
    Player* player = ObjectAccessor::GetPlayer(*me, _upperRecipient);
    if (_upperAttempt.expired() || (player && !LowerEntryAttempt(player, _upperAttempt)))
    {
        CancelUpperIntro();
        return;
    }
    bool present = LowerEntryPlayer(player, _upperWaiting ? 100.0f : 20.0f);
    Creature* grash = nullptr;
    Creature* adel = nullptr;
    bool ready = present && SharasdalGrash::Receipt(scenario, 47131);
    if (ready && !_upperWaiting)
    {
        grash = me->FindNearestCreature(105298, 30.0f, true);
        adel = me->FindNearestCreature(104853, 30.0f, true);
        ready = UpperIntroActor(grash, 105298, 362092) && UpperIntroActor(adel, 104853, 362094) &&
            UpperContentAI(grash) && UpperContentAI(adel) &&
            !me->HasUnitState(UNIT_STATE_STUNNED | UNIT_STATE_ROOT | UNIT_STATE_CASTING) &&
            !grash->HasUnitState(UNIT_STATE_STUNNED | UNIT_STATE_ROOT | UNIT_STATE_CASTING) &&
            !adel->HasUnitState(UNIT_STATE_STUNNED | UNIT_STATE_ROOT | UNIT_STATE_CASTING);
    }
    else if (ready)
    {
        // The native conversation speaks only through Adelee0 and Erunak1;
        // Grash may already be following his independently earned next route.
        adel = player->FindNearestCreature(104853, player->GetVisibilityRange(), true);
        ready = UpperIntroActor(adel, 104853, 362094) &&
            player->FindNearestCreature(102826, player->GetVisibilityRange(), true) == me;
    }
    if (_upperExhausted && ((present && !_upperPresent) || (ready && !_upperReady)))
    {
        _upperExhausted = false;
        _upperRetry = _upperTries = 0;
    }
    _upperPresent = present;
    _upperReady = ready;
    if (_upperWaiting && _upperDelay)
    {
        _upperDelay = _upperDelay > diff ? _upperDelay - diff : 0;
        if (_upperDelay) return;
    }
    if (_upperExhausted) return;
    if (_upperRetry > diff) { _upperRetry -= diff; return; }
    struct BusyGuard { bool& flag; ~BusyGuard() { flag = false; } } guard{_upperBusy};
    _upperBusy = true;
    if (ready && !_upperWaiting)
    {
        if (_upperGuides[0] != grash->GetGUID()) { _upperPaths &= ~uint8(1); _upperTalks &= ~uint8(1); _upperGuides[0] = grash->GetGUID(); }
        if (_upperGuides[1] != adel->GetGUID()) { _upperPaths &= ~uint8(4); _upperTalks &= ~uint8(4); _upperGuides[1] = adel->GetGUID(); }
        Creature* actors[3] = {grash, me, adel};
        for (uint8 i = 0; i < 3; ++i)
        {
            uint8 const bit = uint8(1u << i);
            if (_upperPaths & bit) continue;
            if (SharasdalWall::Path(actors[i]) != 10993811)
            {
                if (i != 1 && !(_upperTalks & bit))
                {
                    _upperTalks |= bit;
                    UpperContentAI(actors[i])->Talk(8);
                }
                if (i == 0) actors[i]->GetMotionMaster()->MovePath(10993811, false);
                else actors[i]->GetMotionMaster()->MovePath(10993811, false, irand(-5, 5), irand(-5, 5));
            }
            if (SharasdalWall::Path(actors[i]) == 10993811) _upperPaths |= bit;
        }
        if (_upperPaths == 7)
        {
            _upperWaiting = true;
            _upperDelay = 5000;
            _upperRetry = _upperTries = 0;
            return;
        }
    }
    else if (ready)
    {
        SpellInfo const* spell = sSpellMgr->GetSpellInfo(209312);
        if (spell)
        {
            auto conversation = new Conversation;
            // Native209312 has only219/1868 and no cast visual. Keep the
            // authored NPC position, but use the original player creator:
            // native visibility/playback expiry must have a Player clock.
            if (conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(),
                1868, player, spell, *me))
            {
                auto a = conversation->GetCreature(0);
                auto e = conversation->GetCreature(1);
                player = ObjectAccessor::GetPlayer(*me, _upperRecipient);
                if (a && e && a->GuidLow == adel->GetGUID().GetLowPart() && a->GuidHight == adel->GetGUID().GetHighPart() &&
                    e->GuidLow == me->GetGUID().GetLowPart() && e->GuidHight == me->GetGUID().GetHighPart() &&
                    LowerEntryAttempt(player, _upperAttempt) && LowerEntryPlayer(player, 100.0f))
                {
                    player->BindQuestObject(40341, conversation);
                    _introDone = true;
                    CancelUpperIntro();
                    return;
                }
                conversation->Remove(); // Map owns a successful creation.
            }
            else delete conversation;
        }
    }
    _upperRetry = 1000;
    if (++_upperTries >= 30)
    {
        _upperExhausted = true;
        TC_LOG_WARN("scripts", "Sharasdal upper Erunak original ascent/companions/path or conversation unavailable after thirty attempts");
    }
}
