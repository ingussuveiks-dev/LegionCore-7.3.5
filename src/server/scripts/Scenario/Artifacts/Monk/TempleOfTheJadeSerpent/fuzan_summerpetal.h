#pragma once

namespace FuZanSummerpetal
{
bool Receipt(Scenario* scenario,uint32 id)
{
    if (!scenario) return false;
    auto progress=scenario->GetAchievementMgr().GetCriteriaProgressMap();
    auto found=progress->find(id);
    return found!=progress->end() && found->second.Counter;
}
}

struct npc_fuzan_summerpetal : SmartAI
{
    explicit npc_fuzan_summerpetal(Creature* creature) : SmartAI(creature) { }
    bool Scoped() const { return _scoped; }

    Scenario* World() const
    {
        if (!me->IsInWorld() || me->IsPreDelete() || me->GetMapId()!=1625) return nullptr;
        Scenario* scenario=sScenarioMgr->GetScenario(me->GetInstanceId());
        return scenario && scenario->GetScenarioId()==1137 ? scenario : nullptr;
    }

    bool Active(Player* player) const
    {
        return player && player->IsInWorld() && player->GetMap()==me->GetMap() &&
            player->getClass()==CLASS_MONK && !player->isGameMaster() && !player->GetQuestRewardStatus(42765) &&
            (player->GetQuestStatus(42765)==QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(42765)==QUEST_STATUS_COMPLETE);
    }

    bool Cancelled() const { return _cancelled || (_cancellation && _cancellation->load(std::memory_order_acquire)); }

    Player* Owner()
    {
        if (Cancelled()) return nullptr;
        Player* player=ObjectAccessor::GetPlayer(*me,_owner);
        if (!Active(player)) return nullptr;
        // A normal reconnect replaces Player/QuestStatusData. The old receipt
        // still distinguishes it from an explicitly cancelled/replaced quest.
        auto receipt=player->GetQuestCancellationReceipt(42765);
        if (!receipt || receipt->load(std::memory_order_acquire)) return nullptr;
        _cancellation=std::move(receipt);
        return player;
    }

    bool Contact(Player* player) const
    {
        return Active(player) && player->GetGUID()==_owner && player->CanContact() && player->IsAlive() &&
            !player->IsBeingTeleported() && player->InSamePhase(me) && player->IsWithinDistInMap(me,50.0f);
    }

    bool Capture(Unit* summoner)
    {
        Creature* boss=summoner ? summoner->ToCreature() : nullptr;
        TempSummon* summon=me->ToTempSummon();
        Scenario* scenario=World();
        if (!summon || me->GetEntry()!=109355 || me->GetDBTableGUIDLow() || !boss ||
            boss->GetEntry()!=109397 || boss->GetDBTableGUIDLow()!=373405 || boss->GetMap()!=me->GetMap() ||
            summon->GetSummonerGUID()!=boss->GetGUID() || boss->IsAlive() || !scenario ||
            (scenario->GetCurrentStep()!=3 && scenario->GetCurrentStep()!=4) ||
            !FuZanSummerpetal::Receipt(scenario,49140)) return false;
        if (_scoped) return _summoner==boss->GetGUID();
        _scoped=true;_summoner=boss->GetGUID();_spoken=FuZanSummerpetal::Receipt(scenario,49356);
        auto source=dynamic_cast<npc_fuzan_belphiar*>(boss->AI());
        if (!source) source=dynamic_cast<npc_fuzan_belphiar*>(boss->GetDisabledAI());
        _owner=source ? source->VictoryOwner() : ObjectGuid::Empty;
        _cancellation=source ? source->VictoryAttempt() : nullptr;
        if (_cancellation && _cancellation->load(std::memory_order_acquire)) { Abort();return true; }
        Player* player=ObjectAccessor::GetPlayer(*me,_owner);
        if (!_cancellation || !_owner)
        {
            TC_LOG_ERROR("scripts.ai","Fu Zan: Summerpetal could not bind the actual Belphiar victory recipient; no replacement player or credit selected");
            Abort();return true;
        }
        if (player && !Active(player)) { Abort();return true; }
        auto rows=sSmartScriptMgr->GetScript(109355,SMART_SCRIPT_TYPE_CREATURE);
        for (auto const& row:rows)
        {
            if (row.event_id==4) _credit=row;
            if (row.event_id==5) _king=row;
            if (row.event_id==6) _follow=row;
        }
        _actions=rows.size()==9 && _credit.GetEventType()==SMART_EVENT_TEXT_OVER &&
            _credit.GetActionType()==SMART_ACTION_UPDATE_ACHIEVEMENT_CRITERIA &&
            _credit.GetTargetType()==SMART_TARGET_CLOSEST_PLAYER && _credit.target.playerDistance.dist==50 &&
            _credit.event.textOver.textGroupID==2 && _credit.event.textOver.creatureEntry==109355 && _credit.link==5 &&
            _credit.action.achievementCriteria.type==92 && _credit.action.achievementCriteria.misc1==52609 &&
            _king.GetEventType()==SMART_EVENT_LINK && _king.GetActionType()==SMART_ACTION_SET_DATA &&
            _king.action.setData.field==1 && _king.action.setData.data==1 && _king.GetTargetType()==SMART_TARGET_CLOSEST_CREATURE &&
            _king.target.closest.entry==109610 && !_king.target.closest.dist && !_king.target.closest.dead &&
            _king.link==6 && _follow.GetEventType()==SMART_EVENT_LINK &&
            _follow.GetActionType()==SMART_ACTION_FOLLOW && _follow.GetTargetType()==SMART_TARGET_CLOSEST_PLAYER &&
            _follow.target.playerDistance.dist==50 && !_follow.link && !_follow.action.follow.credit && !_follow.action.follow.entry;
        if (!_cancellation || !_actions)
        {
            TC_LOG_ERROR("scripts.ai","Fu Zan: Summerpetal native continuation/quest receipt differs; original rows retained and no replacement progression issued");
            Abort();
        }
        return true;
    }

    void Abort()
    {
        _cancelled=true;
        me->DespawnOrUnsummon();
    }

    void InitializeAI() override
    {
        SmartAI::InitializeAI();
        if (TempSummon* summon=me->ToTempSummon()) Capture(summon->GetSummoner());
    }

    void Reset() override
    {
        _spoken=_spoken || FuZanSummerpetal::Receipt(World(),49356);
        _started=false;_contact=false;_retry=0;_tries=0;_parked=false;
        SmartAI::Reset();
    }

    void IsSummonedBy(Unit* summoner) override
    {
        if (!Capture(summoner)) { SmartAI::IsSummonedBy(summoner);return; }
        // Actual summon identity/recipient is captured now. Native speech0
        // starts only when that recipient can receive the authored dialogue.
    }

    Creature* King()
    {
        if (!_actions) return nullptr;
        std::unique_ptr<ObjectList> targets(GetScript()->GetTargets(_king));
        if (!targets || targets->size()!=1) return nullptr;
        Creature* actor=targets->front()->ToCreature();
        return actor && actor->GetEntry()==109610 && actor->GetDBTableGUIDLow()==373384 && actor->IsAlive() &&
            actor->IsAIEnabled && dynamic_cast<SmartAI*>(actor->AI()) ? actor : nullptr;
    }

    bool NativeRecipient()
    {
        if (!_actions || !Contact(Owner())) return false;
        std::unique_ptr<ObjectList> targets(GetScript()->GetTargets(_credit));
        return targets && targets->size()==1 && targets->front()->GetGUID()==_owner;
    }

    bool CanContinue()
    {
        Scenario* scenario=World();
        return _scoped && !Cancelled() && _actions && scenario && scenario->GetCurrentStep()==4 &&
            FuZanSummerpetal::Receipt(scenario,49356) &&
            (_spoken || GetScript()->HasRunEvent(4,SMART_EVENT_TEXT_OVER)) && NativeRecipient();
    }

    bool NativeKingReady() { return CanContinue() && King(); }
    bool NativeFollowReady()
    {
        Creature* king=CanContinue() ? King() : nullptr;
        return king && static_cast<SmartAI*>(king->AI())->GetScript()->HasRunEvent(0,SMART_EVENT_DATA_SET);
    }

    void Continue()
    {
        if (_following || !CanContinue()) return;
        Creature* king=King();
        if (!king) return;
        auto ai=static_cast<SmartAI*>(king->AI());
        if (!ai->GetScript()->HasRunEvent(0,SMART_EVENT_DATA_SET))
        {
            SmartScriptHolder event=_king;GetScript()->InitTimer(event);event.runOnce=false;
            GetScript()->ProcessEvent(event,Owner()); // Only original LINK5, never the credit root.
        }
        if (!CanContinue() || !ai->GetScript()->HasRunEvent(0,SMART_EVENT_DATA_SET)) return;
        // Replay only the original follow LINK if the King already received
        // his native data. Its original closest-player selector must still
        // resolve to the actual solo recipient; never follow a bystander.
        if (GetFollowGuid()!=_owner)
        {
            SmartScriptHolder event=_follow;GetScript()->InitTimer(event);event.runOnce=false;
            GetScript()->ProcessEvent(event,Owner());
        }
        _following=GetFollowGuid()==_owner;
    }

    void SetData(uint32 id,uint32 value) override
    {
        if (_scoped && (Cancelled() || (id==1 && value==1 && !FuZanSummerpetal::Receipt(World(),49356)))) return;
        SmartAI::SetData(id,value);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!_scoped) { SmartAI::UpdateAI(diff);return; }
        if (Cancelled()) { Abort();return; }
        Scenario* scenario=World();
        if (!scenario || !FuZanSummerpetal::Receipt(scenario,49140) || scenario->GetCurrentStep()<3) { Abort();return; }
        if (scenario->GetCurrentStep()==3)
        {
            if (!scenario->CanUpdateCriteria(49356))
            {
                if (!_routingError) TC_LOG_ERROR("scripts.ai","Fu Zan: Summerpetal native49356 routing unavailable; dialogue remains pending without fabricated credit");
                _routingError=true;_contact=false;return;
            }
            Player* owner=Owner();
            if (!Contact(owner) || !NativeRecipient()) { _contact=false;return; }
            if (diff>1000) _contact=false; // Never spend an inactive-grid catch-up delta on newly visible speech.
            if (!_started)
            {
                _started=true;_contact=true;
                SmartAI::IsSummonedBy(ObjectAccessor::GetUnit(*me,_summoner));
                return;
            }
            if (!_contact) { _contact=true;return; }
            SmartAI::UpdateAI(diff);
            _spoken=FuZanSummerpetal::Receipt(World(),49356);
            if (Cancelled()) { Abort();return; }
            Continue();
            return;
        }
        _spoken=_spoken || FuZanSummerpetal::Receipt(scenario,49356);
        SmartAI::UpdateAI(diff); // Preserve native follow, farewell and despawn after the actual story.
        if (Cancelled()) { Abort();return; }
        if (_following || scenario->GetCurrentStep()!=4) return;
        bool contact=Contact(Owner());
        if (!contact) { _contact=false;return; }
        if (!_contact) { _contact=true;_parked=false;_tries=0;_retry=0; }
        if (_parked) return;
        if (_retry>diff) { _retry-=diff;return; }
        _retry=1000;
        Continue();
        if (!_following && ++_tries>=30) _parked=true;
    }

private:
    ObjectGuid _owner,_summoner;
    std::shared_ptr<std::atomic<bool> const> _cancellation;
    SmartScriptHolder _credit,_king,_follow;
    bool _scoped=false,_actions=false,_started=false,_spoken=false,_following=false,_contact=false;
    bool _cancelled=false,_routingError=false,_parked=false;
    uint32 _retry=0,_tries=0;
};

class condition_fuzan_summerpetal_continuation : public ConditionScript
{
public:
    condition_fuzan_summerpetal_continuation() : ConditionScript("condition_fuzan_summerpetal_continuation") { }
    bool OnConditionCheck(Condition* condition,ConditionSourceInfo& info) override
    {
        if (!condition || condition->SourceType!=CONDITION_SOURCE_TYPE_SMART_EVENT || condition->SourceEntry!=109355 ||
            condition->SourceId || (condition->SourceGroup!=6 && condition->SourceGroup!=7)) return false;
        Creature* actor=info.mConditionTargets[1] ? info.mConditionTargets[1]->ToCreature() : nullptr;
        auto ai=actor ? dynamic_cast<npc_fuzan_summerpetal*>(actor->AI()) : nullptr;
        if (!ai) return false;
        // Other summon contexts keep the authored native graph.
        if (!ai->Scoped()) return true;
        return condition->SourceGroup==6 ? ai->NativeKingReady() : ai->NativeFollowReady();
    }
};
