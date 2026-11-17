// Members of the Tuure instance. The native route and Jace's one-shot follow
// remain independent of acknowledging the optional virtual-actor taunt2039.
bool routeTauntPending = false, routeTauntDone = false, routeTauntBusy = false;
bool routeTauntExhausted = false, routeTauntReady = false;
uint32 routeTauntRetry = 0, routeTauntTries = 0;

void ResetRouteTaunt()
{
    routeTauntPending = routeTauntDone = routeTauntExhausted = routeTauntReady = false;
    routeTauntRetry = routeTauntTries = 0;
}

void RearmRouteTaunt()
{
    if (routeTauntPending && !routeTauntDone)
    {
        routeTauntExhausted = false;
        routeTauntRetry = routeTauntTries = 0;
    }
}

void CompleteRouteTaunt(uint32 diff)
{
    if (!routeTauntPending || routeTauntDone || routeTauntBusy) return;
    auto boros = GetBoros();
    auto player = TuureFirstFight::Find(instance, routeRecipient);
    auto status = player ? player->getQuestStatus(41993) : nullptr;
    auto attempt = routeAttempt.lock();
    bool ready = attempt && status && status->ScriptLifetime == attempt && boros &&
        boros->GetGUID() == routeActor && !boros->HasUnitState(UNIT_STATE_STUNNED) &&
        boros->GetCharmerGUID().IsEmpty() && TuureFirstFight::Eligible(player, boros, 2, 2);
    if (routeTauntExhausted && ready && !routeTauntReady) RearmRouteTaunt();
    routeTauntReady = ready;
    if (!ready || routeTauntExhausted) return;
    if (routeTauntRetry > diff) { routeTauntRetry -= diff; return; }
    routeTauntRetry = 1000;
    struct BusyGuard { bool& flag; ~BusyGuard() { flag = false; } } guard{routeTauntBusy};
    routeTauntBusy = true;
    auto guid = boros->GetGUID();
    std::unique_ptr<Conversation> story(new Conversation);
    if (story->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), 2039, boros, nullptr, *boros))
    {
        auto created = story.release(); // Successful native creation is map-owned.
        player = TuureFirstFight::Find(instance, routeRecipient);
        status = player ? player->getQuestStatus(41993) : nullptr;
        boros = GetBoros();
        if (routeTauntPending && routeAttempt.lock() == attempt && status && status->ScriptLifetime == attempt &&
            boros && boros->GetGUID() == guid && routeActor == guid && TuureFirstFight::Eligible(player, boros, 2, 2))
        {
            player->BindQuestObject(41993, created);
            routeTauntDone = true;
            routeTauntPending = false;
        }
        else created->Remove();
        return;
    }
    if (routeAttempt.lock() != attempt || !routeTauntPending) return;
    if (++routeTauntTries >= 30)
    {
        routeTauntExhausted = true;
        TC_LOG_WARN("scripts", "Tuure route: taunt2039 unavailable after thirty attempts; retain original-recipient re-entry retry without delaying progression");
    }
}
