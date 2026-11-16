// Members of instance_tuure_InstanceMapScript. Presentation is optional and
// never supplies or delays the native kill/arrival criteria.
bool routeMiddlePending = false, routeMiddleBusy = false, routeMiddleExhausted = false, routeMiddleReady = false;
uint32 routeMiddleRetry = 0, routeMiddleTries = 0;

class RouteMiddleConversation final : public Conversation
{
    ObjectGuid _boros, _jace;
public:
    RouteMiddleConversation(Creature* boros, Creature* jace) : _boros(boros->GetGUID()), _jace(jace->GetGUID()) { }
protected:
    Creature* FindCreatureActor(uint32, uint32 entry, WorldObject* searcher) override
    {
        ObjectGuid guid = entry == 106134 ? _boros : entry == 106137 ? _jace : ObjectGuid::Empty;
        auto actor = ObjectAccessor::GetCreature(*searcher, guid);
        return actor && actor->GetGUID() == guid && actor->GetEntry() == entry &&
            actor->GetDBTableGUIDLow() == (entry == 106134 ? 361419 : 361432) &&
            actor->IsInWorld() && !actor->IsPreDelete() && actor->IsAlive() &&
            actor->GetMap() == searcher->GetMap() && actor->InSamePhase(searcher) &&
            actor->IsWithinDistInMap(searcher, searcher->GetVisibilityRange()) ? actor : nullptr;
    }
};

void ResetRouteMiddle()
{
    routeMiddlePending = routeMiddleExhausted = routeMiddleReady = false;
    routeMiddleRetry = routeMiddleTries = 0;
}

void RearmRouteMiddle()
{
    if (!routeMiddle && routeMiddlePending)
    {
        routeMiddleExhausted = false;
        routeMiddleRetry = routeMiddleTries = 0;
    }
}

void OnPlayerEnter(Player* player) override
{
    InstanceScript::OnPlayerEnter(player);
    if (player && player->GetGUID() == routeRecipient && !routeAttempt.expired()) RearmRouteMiddle();
}

void CompleteRouteMiddle(uint32 diff)
{
    if (!routeMiddlePending || routeMiddle || routeMiddleBusy) return;
    auto boros = GetBoros();
    auto player = TuureFirstFight::Find(instance, routeRecipient);
    auto status = player ? player->getQuestStatus(41993) : nullptr;
    auto attempt = routeAttempt.lock();
    bool eligible = attempt && status && status->ScriptLifetime == attempt && TuureFirstFight::Eligible(player, boros, 2, 2);
    auto jace = instance->GetCreature(jaceGuid);
    bool ready = eligible && jace && jace->IsInWorld() && !jace->IsPreDelete() && jace->IsAlive() &&
        jace->GetEntry() == 106137 && jace->GetDBTableGUIDLow() == 361432 &&
        jace->GetMap() == instance && jace->InSamePhase(boros) &&
        jace->IsWithinDistInMap(boros, boros->GetVisibilityRange());
    // A genuine return to range/phase can occur without another grid create
    // or player-enter callback. Reopen one bounded attempt window on that edge.
    if (routeMiddleExhausted && ready && !routeMiddleReady) RearmRouteMiddle();
    routeMiddleReady = ready;
    if (!eligible || routeMiddleExhausted) return;
    if (routeMiddleRetry > diff) { routeMiddleRetry -= diff; return; }
    routeMiddleRetry = 1000;
    struct BusyGuard { bool& flag; ~BusyGuard() { flag = false; } } guard{routeMiddleBusy};
    routeMiddleBusy = true;
    if (ready)
    {
        auto borosGuidBefore = boros->GetGUID();
        std::unique_ptr<RouteMiddleConversation> story(new RouteMiddleConversation(boros, jace));
        if (story->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), 2007, boros, nullptr, *boros))
        {
            auto created = story.release(); // Native map owns a successful creation.
            player = TuureFirstFight::Find(instance, routeRecipient);
            status = player ? player->getQuestStatus(41993) : nullptr;
            boros = GetBoros();
            if (routeMiddlePending && routeAttempt.lock() == attempt && status && status->ScriptLifetime == attempt &&
                boros && boros->GetGUID() == borosGuidBefore && TuureFirstFight::Eligible(player, boros, 2, 2))
            {
                player->BindQuestObject(41993, created);
                routeMiddle = true;
                routeMiddlePending = false;
            }
            else created->Remove();
            return;
        }
    }
    if (routeAttempt.lock() != attempt || !routeMiddlePending) return;
    if (++routeMiddleTries >= 30)
    {
        routeMiddleExhausted = true;
        TC_LOG_WARN("scripts", "Tuure route: dialogue2007 unavailable after thirty attempts; retain original-recipient/actor re-entry retry without delaying progression");
    }
}
