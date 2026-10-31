// Members of npc_erunak_stonespeakerAI. The lower guide's actual encounter
// entry is a per-player quest objective; its speech/combat opening is shared.
struct LowerEntry
{
    std::weak_ptr<uint8> attempt;
    uint32 retry = 0, tries = 0;
    bool exhausted = false, present = false;
};
std::map<ObjectGuid, LowerEntry> _lowerEntries;
bool _lowerEntryBusy = false, _lowerCapacityWarned = false;

Scenario* LowerEntryWorld() const
{
    if (me->GetEntry() != 102826 || me->GetDBTableGUIDLow() != 362063 || me->GetMapId() != 1600 ||
        !me->IsInWorld() || me->IsPreDelete() || !me->IsAlive()) return nullptr;
    Scenario* scenario = sScenarioMgr->GetScenario(me->GetInstanceId());
    return scenario && scenario->GetScenarioId() == 1066 && scenario->GetCurrentStep() <= 1 ? scenario : nullptr;
}

bool LowerEntryPlayer(Player* player, float range) const
{
    return player && player->IsInWorld() && player->CanContact() && player->IsAlive() &&
        !player->isGameMaster() && !player->IsBeingTeleported() && player->getClass() == CLASS_SHAMAN &&
        player->GetMap() == me->GetMap() && player->GetInstanceId() == me->GetInstanceId() &&
        player->InSamePhase(me) && player->canSeeOrDetect(me) && player->IsWithinLOSInMap(me) &&
        player->IsWithinDistInMap(me, range) && !player->GetQuestRewardStatus(40341) &&
        (player->GetQuestStatus(40341) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(40341) == QUEST_STATUS_COMPLETE);
}

bool LowerEntryAttempt(Player* player, std::weak_ptr<uint8> const& attempt) const
{
    QuestStatusData* status = player ? player->getQuestStatus(40341) : nullptr;
    return !attempt.expired() && status && status->ScriptLifetime == attempt.lock() &&
        !player->GetQuestRewardStatus(40341) &&
        (player->GetQuestStatus(40341) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(40341) == QUEST_STATUS_COMPLETE);
}

bool LowerCreditAllowed(Player* player) const
{
    // KilledMonsterCredit also dispatches achievement events before its quest
    // loop. Do not repeat it while the native quest admission is known to fail.
    Quest const* quest = sQuestDataStore->GetQuestTemplate(40341);
    return quest && player->GetQuestStatus(40341) == QUEST_STATUS_INCOMPLETE &&
        quest->HasSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST) &&
        (!player->GetGroup() || !player->GetGroup()->isRaidGroup() || quest->IsAllowedInRaid(player->GetMap()->GetDifficultyID()));
}

void BeginLowerIntro(Player* player)
{
    if (_introDone) return;
    player->AddQuestDelayedEvent(40341, 0, []() {});
    QuestStatusData* status = player->getQuestStatus(40341);
    if (!status || !status->ScriptLifetime) return;
    ObjectGuid original = player->GetGUID();
    std::weak_ptr<uint8> attempt = status->ScriptLifetime;
    _introDone = firstbattle = true;
    _functions.AddDelayedEvent(3000, [this, original, attempt]()
    {
        Player* owner = ObjectAccessor::GetPlayer(*me, original);
        if (!LowerEntryWorld() || !LowerEntryAttempt(owner, attempt) || !LowerEntryPlayer(owner, 100.0f))
        {
            // No new progression has been attached to the shared opening.
            // A cancelled opening can be offered again to a valid visitor.
            _introDone = firstbattle = false;
            return;
        }
        Talk(0);
        if (Creature* target = me->FindNearestCreature(102792, 70.0f, true))
            if ((target->GetDBTableGUIDLow() == 362064 || target->GetDBTableGUIDLow() == 362067) &&
                target->GetMap() == me->GetMap() && target->IsInWorld() && !target->IsPreDelete() &&
                target->IsAlive() && target->InSamePhase(me))
                if (CreatureAI* ai = me->AI()) ai->AttackStart(target);
    });
}

void AcceptLowerEntry(Player* player)
{
    if (_lowerEntryBusy || !LowerEntryWorld() || !LowerEntryPlayer(player, 20.0f)) return;
    if (player->GetQuestObjectiveData(40341, 105805) >= 1)
    {
        _lowerEntries.erase(player->GetGUID());
        BeginLowerIntro(player);
        return;
    }
    auto found = _lowerEntries.find(player->GetGUID());
    if (found == _lowerEntries.end())
    {
        if (_lowerEntries.size() >= 40)
        {
            if (!_lowerCapacityWarned)
                TC_LOG_WARN("scripts", "Sharasdal lower Erunak entry reached forty pending original visitors");
            _lowerCapacityWarned = true;
            return;
        }
        player->AddQuestDelayedEvent(40341, 0, []() {});
        QuestStatusData* status = player->getQuestStatus(40341);
        if (!status || !status->ScriptLifetime) return;
        _lowerEntries.emplace(player->GetGUID(), LowerEntry{status->ScriptLifetime});
    }
    CompleteLowerEntries(0);
}

void CompleteLowerEntries(uint32 diff)
{
    if (_lowerEntryBusy || _lowerEntries.empty()) return;
    if (!LowerEntryWorld())
    {
        _lowerEntries.clear();
        return;
    }
    struct BusyGuard { bool& flag; ~BusyGuard() { flag = false; } } guard{_lowerEntryBusy};
    _lowerEntryBusy = true;
    for (auto it = _lowerEntries.begin(); it != _lowerEntries.end();)
    {
        ObjectGuid original = it->first;
        LowerEntry& pending = it->second;
        Player* player = ObjectAccessor::GetPlayer(*me, original);
        if (pending.attempt.expired() || (player && !LowerEntryAttempt(player, pending.attempt)))
        {
            it = _lowerEntries.erase(it);
            continue;
        }
        bool present = LowerEntryPlayer(player, 20.0f);
        if (pending.exhausted && present && !pending.present)
        {
            pending.exhausted = false;
            pending.retry = pending.tries = 0;
        }
        pending.present = present;
        if (pending.exhausted) { ++it; continue; }
        if (pending.retry > diff) { pending.retry -= diff; ++it; continue; }
        std::weak_ptr<uint8> attempt = pending.attempt;
        if (present && player->GetQuestObjectiveData(40341, 105805) < 1 && LowerCreditAllowed(player))
            player->KilledMonsterCredit(105805);
        // Native credit can synchronously change the quest or its attempt.
        player = ObjectAccessor::GetPlayer(*me, original);
        if (LowerEntryWorld() && LowerEntryAttempt(player, attempt) && LowerEntryPlayer(player, 20.0f) &&
            player->GetQuestObjectiveData(40341, 105805) >= 1)
        {
            it = _lowerEntries.erase(it);
            BeginLowerIntro(player);
            continue;
        }
        pending.retry = 1000;
        if (++pending.tries >= 30)
        {
            pending.exhausted = true;
            TC_LOG_WARN("scripts", "Sharasdal lower Erunak original visitor entry unavailable after thirty attempts");
        }
        ++it;
    }
}
