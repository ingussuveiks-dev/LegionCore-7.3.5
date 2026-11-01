namespace FuZanTempleIntro
{
class Actor;
Actor* Find(Player* player);
Actor* Content(Creature* actor);
void RetireStory(Creature* actor, ObjectGuid const& story, uint32 epoch);

Scenario* World(Creature* actor)
{
    if (!actor || !actor->IsInWorld() || actor->IsPreDelete() || !actor->IsAlive() ||
        actor->GetEntry() != 109413 || actor->GetDBTableGUIDLow() != 373392 || actor->GetMapId() != 1625)
        return nullptr;
    Scenario* scenario = sScenarioMgr->GetScenario(actor->GetInstanceId());
    return scenario && scenario->GetScenarioId() == 1137 ? scenario : nullptr;
}

bool Receipt(Scenario* scenario)
{
    if (!scenario) return false;
    auto progress = scenario->GetAchievementMgr().GetCriteriaProgressMap();
    auto found = progress->find(49115);
    return found != progress->end() && found->second.Counter;
}

struct Story : Conversation
{
    ObjectGuid source;
    uint32 epoch;
    explicit Story(ObjectGuid identity, uint32 attempt) : source(identity), epoch(attempt) { }
    bool Played() const
    {
        auto started = playing.find(GetCasterGUID());
        uint32 deadline = GetUInt32Value(CONVERSATION_FIELD_LAST_LINE_END_TIME);
        return deadline && started != playing.end() && getMSTime() - started->second > deadline;
    }
    void Update(uint32 diff) override;
};

class Actor : public SmartAI
{
public:
    explicit Actor(Creature* actor) : SmartAI(actor) { }

    bool Scoped() const
    {
        return me->GetEntry() == 109413 && me->GetDBTableGUIDLow() == 373392 && me->GetMapId() == 1625;
    }

    Player* Owner() const { return ObjectAccessor::GetPlayer(*me, _owner); }

    bool Current(Player* player) const
    {
        auto lease = _attempt.lock();
        QuestStatusData* status = player ? player->getQuestStatus(42765) : nullptr;
        return lease && status && status->ScriptLifetime == lease && player->GetGUID() == _owner &&
            player->IsInWorld() && player->CanContact() && !player->isGameMaster() &&
            player->GetMap() == me->GetMap() && player->getClass() == CLASS_MONK && player->InSamePhase(me) &&
            !player->GetQuestRewardStatus(42765) &&
            (player->GetQuestStatus(42765) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(42765) == QUEST_STATUS_COMPLETE);
    }

    bool Contact(Player* player, float range) const
    {
        return Current(player) && player->IsAlive() && !player->IsBeingTeleported() &&
            player->IsWithinDistInMap(me, range);
    }

    void Reset() override
    {
        Cancel();
        SmartAI::Reset();
    }

    void MoveInLineOfSight(Unit* who) override
    {
        if (!Scoped()) { if (_state != Idle) Cancel(); SmartAI::MoveInLineOfSight(who); return; }
        Scenario* scenario = World(me);
        Player* player = who ? who->ToPlayer() : nullptr;
        if (!scenario || Receipt(scenario) || scenario->GetCurrentStep() != 0 || !player ||
            !player->IsAlive() || !player->CanContact() || player->isGameMaster() || player->IsBeingTeleported() ||
            player->GetMap() != me->GetMap() || !player->InSamePhase(me) ||
            !me->IsWithinDistInMap(player, 16.0f) || !me->IsWithinLOSInMap(player) ||
            !sConditionMgr->IsObjectMeetingSmartEventConditions(109413, 0, 0, player, me)) return;
        if (_state != Idle)
        {
            if (!Current(Owner())) Cancel();
            else
            {
                if (_state == Exhausted && player->GetGUID() == _owner)
                {
                    _state = _played ? Ending : Starting;
                    _tries = _age = 0;
                    _replays = 0;
                    _retry = 200;
                    Activate();
                }
                return;
            }
        }
        player->AddQuestDelayedEvent(42765, 0, []() { });
        auto status = player->getQuestStatus(42765);
        if (!status || !status->ScriptLifetime) return;
        ++_epoch;
        _owner = player->GetGUID();
        _attempt = status->ScriptLifetime;
        _state = Starting;
        _retry = 200; // Original SmartAI invoker-cast delay.
        _tries = _age = 0;
        _replays = 0;
        Activate();
    }

    void UpdateAI(uint32 diff) override
    {
        if (!Scoped()) { if (_state != Idle) Cancel(); SmartAI::UpdateAI(diff); return; }
        if (_state == Idle || _state == Exhausted) return;
        Scenario* scenario = World(me);
        if (!scenario || !Current(Owner())) { Cancel(); return; }
        if (Receipt(scenario)) { Cancel(); return; }
        if (scenario->GetCurrentStep() != 0) { Cancel(); return; }
        _age += std::min(diff, 120000u);
        if (_age >= 120000)
        {
            Exhaust("Fu Zan: intro wait exhausted; original player may re-enter to retry");
            return;
        }
        // Do not run the obsolete, unacknowledged source9 timer for this
        // original actor. Other instances of the shared entry delegate above.
        if (!_cast.IsEmpty())
        {
            if (_castWait > diff) { _castWait -= diff; return; }
            if (_state != Playing && !_reported)
            {
                _reported = true;
                TC_LOG_ERROR("scripts.ai", "Fu Zan: native intro cast acknowledgement timed out for player %u; bounded retry pending", uint32(_owner.GetCounter()));
            }
            _cast.Clear();
            _reserved = false;
        }
        if (_state == Playing)
        {
            if (!ObjectAccessor::GetConversation(*me, _story))
            {
                if (++_replays > 3) { Exhaust("Fu Zan: interrupted intro replay budget exhausted"); return; }
                TC_LOG_ERROR("scripts.ai", "Fu Zan: accepted intro conversation disappeared before completion for player %u; bounded replay pending", uint32(_owner.GetCounter()));
                ++_epoch;
                _story.Clear(); _cast.Clear();
                _reserved = false;
                _state = Starting;
                _retry = 1000;
            }
            return;
        }
        if (_retry > diff) { _retry -= diff; return; }
        _retry = 1000;
        Player* player = Owner();
        float range = _state == Starting ? 16.0f : 50.0f;
        if (!Contact(player, range) || (_state == Starting && !me->IsWithinLOSInMap(player))) return;
        if (++_tries > 30)
        {
            Exhaust("Fu Zan: native intro cast retries exhausted; no completion bypass");
            return;
        }
        _issuing = true;
        uint32 spell = _state == Starting ? 217775 : 217774;
        SpellCastResult result = player->CastSpell(player, spell, false);
        _issuing = false;
        if (result != SPELL_CAST_OK || (_state == Starting && _cast.IsEmpty()))
        {
            if (!_reported)
            {
                _reported = true;
                TC_LOG_ERROR("scripts.ai", "Fu Zan: native intro spell %u failed or was not acknowledged for player %u; bounded retry pending", spell, player->GetGUIDLow());
            }
        }
    }

    bool Ticket(Player* player, ObjectGuid const& cast, bool ending, uint32& epoch)
    {
        if (!_issuing || cast.IsEmpty() || !Contact(player, ending ? 50.0f : 16.0f) ||
            !World(me) || Receipt(World(me)) || World(me)->GetCurrentStep() != 0 ||
            _state != (ending ? Ending : Starting) || !_cast.IsEmpty()) return false;
        _cast = cast;
        _castWait = 2000;
        _reserved = false;
        epoch = _epoch;
        return true;
    }

    bool Valid(Player* player, ObjectGuid const& cast, uint32 epoch, bool ending) const
    {
        Scenario* scenario = World(me);
        return epoch == _epoch && !cast.IsEmpty() && cast == _cast && scenario &&
            !Receipt(scenario) && scenario->GetCurrentStep() == 0 &&
            Contact(player, ending ? 50.0f : 16.0f) &&
            (ending ? _state == Ending && _played : _state == Starting || _state == Playing);
    }

    bool Reserve(Player* player, ObjectGuid const& cast, uint32 epoch)
    {
        if (_reserved || !Valid(player, cast, epoch, true)) return false;
        _reserved = true; // Before the native effect can re-enter.
        return true;
    }

    bool Created(Player* player, ObjectGuid const& cast, uint32 epoch, Story* story)
    {
        if (!story || !Valid(player, cast, epoch, false) || _state != Starting ||
            !_story.IsEmpty() || story->GetCasterGUID() != _owner) return false;
        _story = story->GetGUID();
        _state = Playing;
        _tries = _age = 0;
        _reported = false;
        return true;
    }

    bool Owns(Story const* story) const
    {
        return story && story->source == me->GetGUID() && story->epoch == _epoch &&
            story->GetGUID() == _story && story->GetCasterGUID() == _owner && Current(Owner());
    }

    void Played(Story* story)
    {
        if (_state != Playing || !Owns(story) || !story->Played()) return;
        _played = true;
        _state = Ending;
        _retry = 1;
        _tries = _age = 0;
        _story.Clear();
        story->Remove();
    }

    void CastEnded(Player* player, ObjectGuid const& cast, uint32 epoch, bool ending)
    {
        if (epoch != _epoch || cast != _cast || !Current(player)) return;
        if (ending && Receipt(World(me))) { Cancel(); return; }
        if (!ending && _state == Playing)
        {
            _cast.Clear();
            _reserved = false;
        }
        // AfterCast may precede a delayed hit. Its full cast ticket remains
        // bounded until the actual creation/event receipt or timeout.
    }

private:
    enum State { Idle, Starting, Playing, Ending, Exhausted };
    State _state = Idle;
    ObjectGuid _owner, _story, _cast;
    std::weak_ptr<uint8> _attempt;
    uint32 _epoch = 0, _retry = 0, _age = 0, _tries = 0, _castWait = 0, _replays = 0;
    bool _issuing = false, _reserved = false, _played = false, _reported = false, _activeOwned = false;

    void Activate()
    {
        if (!me->isActiveObject()) { me->setActive(true); _activeOwned = true; }
    }
    void Deactivate()
    {
        if (_activeOwned && me->IsInWorld()) me->setActive(false);
        _activeOwned = false;
    }
    void Cancel()
    {
        ObjectGuid story = _story;
        uint32 epoch = _epoch++;
        _state = Idle;
        _story.Clear(); _cast.Clear();
        _owner.Clear(); _attempt.reset();
        _issuing = _reserved = _played = _reported = false;
        _retry = _age = _tries = _castWait = _replays = 0;
        if (me->IsInWorld()) RetireStory(me, story, epoch);
        Deactivate();
    }
    void Exhaust(char const* message)
    {
        TC_LOG_ERROR("scripts.ai", "%s (player %u)", message, uint32(_owner.GetCounter()));
        ObjectGuid story = _story;
        uint32 epoch = _epoch++;
        _state = Exhausted;
        _story.Clear(); _cast.Clear();
        _issuing = _reserved = false;
        RetireStory(me, story, epoch);
        Deactivate();
    }
};

Actor* Find(Player* player)
{
    if (!player || !player->IsInWorld() || player->GetMapId() != 1625) return nullptr;
    Creature* actor = player->FindNearestCreature(109413, 100.0f, true);
    return Content(actor);
}

Actor* Content(Creature* actor)
{
    if (!World(actor) || !actor->IsAIEnabled) return nullptr;
    CreatureAI* content = actor->GetDisabledAI() ? static_cast<CreatureAI*>(actor->GetDisabledAI()) : actor->AI();
    return dynamic_cast<Actor*>(content);
}

void RetireStory(Creature* actor, ObjectGuid const& guid, uint32 epoch)
{
    if (guid.IsEmpty()) return;
    auto story = dynamic_cast<Story*>(ObjectAccessor::GetConversation(*actor, guid));
    if (story && story->source == actor->GetGUID() && story->epoch == epoch) story->Remove();
}

void Story::Update(uint32 /*diff*/)
{
    if (!IsInWorld()) return;
    Player* player = ObjectAccessor::GetPlayer(*this, GetCasterGUID());
    Actor* actor = Content(ObjectAccessor::GetCreature(*this, source));
    if (IsQuestObjectExpired() || !actor || !actor->Owns(this) || !InSamePhase(player)) { Remove(); return; }
    // The native visibility clock must actually contain the original player.
    // No unhandled client line notification or bare30s timer is a receipt.
    if (Played()) actor->Played(this);
}
}

struct npc_fuzan_temple_intro : FuZanTempleIntro::Actor
{
    explicit npc_fuzan_temple_intro(Creature* creature) : Actor(creature) { }
};

class spell_fuzan_temple_intro : public SpellScript
{
    PrepareSpellScript(spell_fuzan_temple_intro);
    ObjectGuid _cast;
    uint32 _epoch = 0;
    bool _ticket = false;

    bool Validate(SpellInfo const* info) override
    {
        return info->Id == 217775 && info->Effects[EFFECT_0] && info->Effects[EFFECT_1] &&
            info->Effects[EFFECT_0]->Effect == SPELL_EFFECT_SUMMON_CONVERSATION && info->Effects[EFFECT_0]->MiscValue == 2770 &&
            info->Effects[EFFECT_1]->Effect == SPELL_EFFECT_APPLY_AURA && info->Effects[EFFECT_1]->ApplyAuraName == SPELL_AURA_DUMMY;
    }
    SpellCastResult Check()
    {
        if (GetCaster()->GetMapId() != 1625) return SPELL_CAST_OK;
        Player* player = GetCaster()->ToPlayer();
        auto actor = FuZanTempleIntro::Find(player);
        if (!actor) return SPELL_FAILED_BAD_TARGETS;
        if (!_ticket)
        {
            _cast = GetSpell()->m_castGuid[0];
            _ticket = actor->Ticket(player, _cast, false, _epoch);
        }
        return _ticket && actor->Valid(player, _cast, _epoch, false) ? SPELL_CAST_OK : SPELL_FAILED_BAD_TARGETS;
    }
    void Create(SpellEffIndex effect)
    {
        if (GetCaster()->GetMapId() != 1625) return;
        PreventHitDefaultEffect(effect);
        Player* player = GetCaster()->ToPlayer();
        auto actor = FuZanTempleIntro::Find(player);
        if (!_ticket || !actor || !actor->Valid(player, _cast, _epoch, false)) return;
        Creature* source = player->FindNearestCreature(109413, 100.0f, true);
        if (!source) return;
        Creature* guide = player->FindNearestCreature(109353, player->GetVisibilityRange(), true);
        if (!guide || guide->GetDBTableGUIDLow() != 373394 || guide->GetMap() != source->GetMap() || !guide->InSamePhase(player)) return;
        ObjectGuid sourceGuid = source->GetGUID();
        ObjectGuid guideGuid = guide->GetGUID();
        auto story = new FuZanTempleIntro::Story(sourceGuid, _epoch);
        Position position = player->GetPosition();
        if (GetExplTargetDest() && GetHitDest()) position = *GetHitDest();
        if (!story->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), 2770, player, GetSpellInfo(), position))
        { delete story; return; }
        auto nativeSource = story->GetCreature(2);
        auto nativeGuide = story->GetCreature(0);
        bool exact = nativeSource && nativeGuide && nativeSource->GuidLow == sourceGuid.GetLowPart() && nativeSource->GuidHight == sourceGuid.GetHighPart() &&
            nativeGuide->GuidLow == guideGuid.GetLowPart() && nativeGuide->GuidHight == guideGuid.GetHighPart();
        player = ObjectAccessor::GetPlayer(*story, story->GetCasterGUID());
        actor = FuZanTempleIntro::Find(player);
        if (!exact || !actor || !actor->Created(player, _cast, _epoch, story)) { story->Remove(); return; }
        player->BindQuestObject(42765, story);
    }
    void Finished()
    {
        if (auto actor = FuZanTempleIntro::Find(GetCaster()->ToPlayer())) actor->CastEnded(GetCaster()->ToPlayer(), _cast, _epoch, false);
    }
    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_fuzan_temple_intro::Check);
        OnEffectHit += SpellEffectFn(spell_fuzan_temple_intro::Create, EFFECT_0, SPELL_EFFECT_SUMMON_CONVERSATION);
        AfterCast += SpellCastFn(spell_fuzan_temple_intro::Finished);
    }
};

class spell_fuzan_temple_complete : public SpellScript
{
    PrepareSpellScript(spell_fuzan_temple_complete);
    ObjectGuid _cast;
    uint32 _epoch = 0;
    bool _ticket = false;
    bool Validate(SpellInfo const* info) override
    {
        return info->Id == 217774 && info->Effects[EFFECT_0] &&
            info->Effects[EFFECT_0]->Effect == SPELL_EFFECT_SEND_EVENT && info->Effects[EFFECT_0]->MiscValue == 52411;
    }
    SpellCastResult Check()
    {
        if (GetCaster()->GetMapId() != 1625) return SPELL_CAST_OK;
        Player* player = GetCaster()->ToPlayer();
        auto actor = FuZanTempleIntro::Find(player);
        if (!actor) return SPELL_FAILED_BAD_TARGETS;
        if (!_ticket) { _cast = GetSpell()->m_castGuid[0]; _ticket = actor->Ticket(player, _cast, true, _epoch); }
        return _ticket && actor->Valid(player, _cast, _epoch, true) ? SPELL_CAST_OK : SPELL_FAILED_BAD_TARGETS;
    }
    void Guard(SpellEffIndex effect)
    {
        if (GetCaster()->GetMapId() != 1625) return;
        Player* player = GetCaster()->ToPlayer();
        auto actor = FuZanTempleIntro::Find(player);
        if (!_ticket || !actor || !actor->Reserve(player, _cast, _epoch)) PreventHitDefaultEffect(effect);
    }
    void Finished()
    {
        if (auto actor = FuZanTempleIntro::Find(GetCaster()->ToPlayer())) actor->CastEnded(GetCaster()->ToPlayer(), _cast, _epoch, true);
    }
    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_fuzan_temple_complete::Check);
        OnEffectHit += SpellEffectFn(spell_fuzan_temple_complete::Guard, EFFECT_0, SPELL_EFFECT_SEND_EVENT);
        AfterCast += SpellCastFn(spell_fuzan_temple_complete::Finished);
    }
};
