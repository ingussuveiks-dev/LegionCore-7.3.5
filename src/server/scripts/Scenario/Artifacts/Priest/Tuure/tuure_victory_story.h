// Optional2009 presentation has its own original quest lease. The earned
// sixteen-second unlock remains independent after questAttempt is released.
ObjectGuid victoryStoryOwner;
std::weak_ptr<uint8> victoryStoryAttempt;
bool victoryStoryPending = false, victoryStoryDone = false, victoryStoryBusy = false;
bool victoryStoryExhausted = false, victoryStoryReady = false;
uint32 victoryStoryRetry = 0, victoryStoryTries = 0;

void ResetVictoryStory()
{
    victoryStoryOwner.Clear();
    victoryStoryAttempt.reset();
    victoryStoryPending = victoryStoryDone = victoryStoryExhausted = victoryStoryReady = false;
    victoryStoryRetry = victoryStoryTries = 0;
}

void RearmVictoryStory()
{
    if (victoryStoryPending && !victoryStoryDone)
    {
        victoryStoryExhausted = false;
        victoryStoryRetry = victoryStoryTries = 0;
    }
}

void CompleteVictoryStory(uint32 diff)
{
    if (!victoryStoryPending || victoryStoryDone || victoryStoryBusy) return;
    auto scenario = sScenarioMgr->GetScenario(instance->GetInstanceId());
    auto attempt = victoryStoryAttempt.lock();
    auto player = TuureFirstFight::Find(instance, victoryStoryOwner);
    auto status = player ? player->getQuestStatus(41993) : nullptr;
    if (!attempt || (player && (!status || status->ScriptLifetime != attempt)) ||
        !scenario || scenario->GetScenarioId() != 1085 || scenario->GetCurrentStep() != 2 ||
        !TuureFirstFight::HasVictory(scenario) || firstRoute != FirstRoute::Waiting)
    {
        ResetVictoryStory(); // Never replay first-victory speech into the next route.
        return;
    }
    auto boros = GetBoros();
    bool eligible = status && status->ScriptLifetime == attempt && boros &&
        !boros->HasUnitState(UNIT_STATE_STUNNED) && boros->GetCharmerGUID().IsEmpty() &&
        TuureFirstFight::Eligible(player, boros, 2, 2);
    auto jace = instance->GetCreature(jaceGuid);
    bool ready = eligible && jace && jace->IsInWorld() && !jace->IsPreDelete() && jace->IsAlive() &&
        jace->GetEntry() == 106137 && jace->GetDBTableGUIDLow() == 361432 &&
        jace->GetMap() == instance && jace->InSamePhase(boros) &&
        jace->IsWithinDistInMap(boros, boros->GetVisibilityRange());
    if (victoryStoryExhausted && ready && !victoryStoryReady) RearmVictoryStory();
    victoryStoryReady = ready;
    if (!eligible || victoryStoryExhausted) return;
    if (victoryStoryRetry > diff) { victoryStoryRetry -= diff; return; }
    victoryStoryRetry = 1000;
    struct BusyGuard { bool& flag; ~BusyGuard() { flag = false; } } guard{victoryStoryBusy};
    victoryStoryBusy = true;
    if (ready)
    {
        auto guid = boros->GetGUID();
        // Same original Boros/Jace roles as2007; the native third player
        // placeholder of2009 is serialized unchanged by Conversation itself.
        std::unique_ptr<RouteMiddleConversation> story(new RouteMiddleConversation(boros, jace));
        if (story->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), 2009, boros, nullptr, *boros))
        {
            auto created = story.release();
            player = TuureFirstFight::Find(instance, victoryStoryOwner);
            status = player ? player->getQuestStatus(41993) : nullptr;
            boros = GetBoros();
            if (victoryStoryPending && victoryStoryAttempt.lock() == attempt && status && status->ScriptLifetime == attempt &&
                firstRoute == FirstRoute::Waiting && boros && boros->GetGUID() == guid && TuureFirstFight::Eligible(player, boros, 2, 2))
            {
                player->BindQuestObject(41993, created);
                victoryStoryDone = true;
                victoryStoryPending = false;
            }
            else created->Remove();
            return;
        }
    }
    if (victoryStoryAttempt.lock() != attempt || !victoryStoryPending) return;
    if (++victoryStoryTries >= 30)
    {
        victoryStoryExhausted = true;
        TC_LOG_WARN("scripts", "Tuure first fight: dialogue2009 unavailable after thirty attempts; retain original-recipient/actor re-entry retry without delaying earned unlock");
    }
}
