#pragma once

#include "EventObject.h"
#include "SpellAuras.h"

namespace FuZanWaveEntry
{
class Instance;
inline Instance* Find(Player* player);
inline bool Owns(Aura* aura,ObjectGuid const& marker);
inline void Stamp(Aura* aura,ObjectGuid const& marker);

inline bool Source(EventObject* event)
{
    return event && event->GetEntry()==103 && event->GetDBTableGUIDLow()==400106 && event->GetMapId()==1625;
}

// One native LFG1298 participant. State belongs to the live instance, so an
// ordinary Player replacement cannot turn an old task into a new claimant.
class Instance : public InstanceScript, public FuZanVictoryRecipient
{
public:
    explicit Instance(InstanceMap* map) : InstanceScript(map) { }

    void RecordFuZanVictory(Creature* source,ObjectGuid const& owner,std::shared_ptr<QuestCancellationReceipt const> const& attempt) override
    {
        auto scenario=World();
        if(_victoryRecorded || !source || source->GetMap()!=instance || source->GetEntry()!=109397 ||
            source->GetDBTableGUIDLow()!=373405 || source->IsAlive() || !owner || !attempt || !scenario || scenario->GetCurrentStep()!=2)return;
        _owner=owner;_attempt=attempt;_victoryRecorded=true;
    }
    void OnPlayerEnter(Player* player) override
    {
        if(_victoryRecorded && !Cancelled() && Active(player) && player->GetGUID()==_owner)
        {
            if(!player->AdoptQuestCancellationReceipt(42765,_attempt)){_cancelled=true;return;}
            if(_claimed && !_retired)
            {
                _pending|=6;
                for(uint32 role=1;role<3;++role)if(!(_preexisting&(1u<<role)))Stamp(player->GetAura(AuraId(role),_owner),_marker);
            }
            _contact=false;
        }
    }

    Scenario* World() const
    {
        if(instance->GetId()!=1625)return nullptr;
        auto scenario=sScenarioMgr->GetScenario(instance->GetInstanceId());
        return scenario && scenario->GetScenarioId()==1137 ? scenario : nullptr;
    }
    bool Active(Player* player) const
    {
        return player && player->IsInWorld() && player->GetMap()==instance && player->getClass()==CLASS_MONK &&
            !player->isGameMaster() && !player->GetQuestRewardStatus(42765) &&
            (player->GetQuestStatus(42765)==QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(42765)==QUEST_STATUS_COMPLETE);
    }
    bool Cancelled() const { return _cancelled || (_attempt && _attempt->load(std::memory_order_acquire)); }
    ObjectGuid Marker() const { return _marker; }
    Player* Owner() const
    {
        return ObjectAccessor::GetObjectInMap(_owner,instance,static_cast<Player*>(nullptr));
    }
    bool Catalog()
    {
        if(_catalogChecked)return _catalogValid;
        _catalogChecked=true;
        auto rows=sSmartScriptMgr->GetScript(103,SMART_SCRIPT_TYPE_EVENTOBJECT);
        if(rows.size()==5)
        {
            for(auto const& row:rows)if(row.event_id<5)_rows[row.event_id]=row;
            auto const& root=_rows[0];auto const& data=_rows[2];
            _catalogValid=root.event_id==0 && root.GetEventType()==SMART_EVENT_EVENTOBJECT_ONTRIGGER && root.link==1 &&
                root.GetActionType()==SMART_ACTION_UPDATE_ACHIEVEMENT_CRITERIA && root.action.achievementCriteria.type==92 &&
                root.action.achievementCriteria.misc1==52477 && root.GetTargetType()==SMART_TARGET_ACTION_INVOKER &&
                data.GetEventType()==SMART_EVENT_LINK && data.link==3 && data.GetActionType()==SMART_ACTION_SET_DATA &&
                data.action.setData.field==1 && data.action.setData.data==1 && data.GetTargetType()==SMART_TARGET_CLOSEST_CREATURE &&
                data.target.closest.entry==109465 && !data.target.closest.dist && !data.target.closest.dead;
            for(uint32 id:{1u,3u,4u})
            {
                auto const& row=_rows[id];uint32 spell=id==1?217817:id==3?218883:218882;
                _catalogValid=_catalogValid && row.event_id==id && row.GetEventType()==SMART_EVENT_LINK &&
                    row.GetActionType()==SMART_ACTION_INVOKER_CAST && row.GetTargetType()==SMART_TARGET_ACTION_INVOKER &&
                    row.action.cast.spell==spell && !row.action.cast.flags && row.link==(id==4?0:id+1);
            }
        }
        if(!_catalogValid)TC_LOG_ERROR("scripts.ai","Fu Zan: wave-entry native graph differs; entry remains pending without replacement progression");
        return _catalogValid;
    }
    bool CanEnter(Player* player,EventObject* event)
    {
        auto scenario=World();
        if(!Source(event) || event->GetMap()!=instance || !Active(player) || !player->IsAlive() || !player->CanContact() ||
            player->IsBeingTeleported() || !player->InSamePhase(event) || !scenario || !Catalog() || Cancelled() || _retired)return false;
        if(!_victoryRecorded || player->GetGUID()!=_owner)return false;
        if(!_claimed)return scenario->GetCurrentStep()==4 && FuZanSummerpetal::Receipt(scenario,49356) && scenario->CanUpdateCriteria(49176) && Receivers(player);
        return player->GetGUID()==_owner && _rootAcknowledged && scenario->GetCurrentStep()>=5 && (_pending || !_dataSent);
    }
    void Enter(Player* player,EventObject* event)
    {
        if(!CanEnter(player,event))return;
        if(_claimed){_source=event->GetGUID();_tries=0;_parked=false;_timer=200;return;}
        if(!player->AdoptQuestCancellationReceipt(42765,_attempt))return;
        _source=_marker=event->GetGUID();
        if(!_attempt || _attempt->load(std::memory_order_acquire))return;
        _claimed=true;
        for(uint32 role=0;role<3;++role)if(player->GetAura(AuraId(role),player->GetGUID()))_preexisting|=1u<<role;
        Credit(player,event);
    }
    bool Allow(Player* player) const
    {
        auto scenario=World();
        return _claimed && !Cancelled() && !_retired && Active(player) && player->GetGUID()==_owner && _rootAcknowledged &&
            scenario && scenario->GetCurrentStep()>=5 && FuZanSummerpetal::Receipt(scenario,49176);
    }
    bool Spell(Player* player,uint32 id,ObjectGuid const& parent,ObjectGuid const& cast)
    {
        if(!Allow(player))return false;
        if(id==217817 || id==218883 || id==218882)
        {
            uint32 role=id==217817?0:id==218883?1:2;
            if(_issuing!=id && (cast.IsEmpty() || _tickets[role]!=cast))return false;
            _tickets[role]=cast;return true;
        }
        if(id==217659 || id==217667)return parent==_tickets[id==217659?1:2] && !parent.IsEmpty();
        if(id==217666 || id==217669)
            return Owns(player->GetAura(id==217666?217659:217667,player->GetGUID()),_marker);
        return false;
    }
    bool Story(Player* player,Conversation* story)
    {
        if(!Allow(player) || !story || story->GetEntry()!=2779 || story->GetCasterGUID()!=_owner || story->GetMap()!=instance)return false;
        _story=story->GetGUID();_storyDone=true;_pending&=~1u;player->BindQuestObject(42765,story);
        CleanupReceiver(player);return true;
    }
    void Retire(Player* player)
    {
        if(_claimed && player && player->GetGUID()==_owner){_retired=true;_pending=0;_issuing=0;}
    }
    void OnPlayerLeave(Player* player) override
    {
        if(_claimed && player && player->GetGUID()==_owner){Cleanup(player);if(!_retired)_pending|=6;_contact=false;}
    }
    void Update(uint32 diff) override
    {
        if(!_victoryRecorded)return;
        if(Cancelled())
        {
            if(!_cleaned){Cleanup(Owner());_cleaned=true;}_pending=0;return;
        }
        if(_retired)return;
        auto player=Owner();
        if(!Active(player)){_contact=false;return;}
        if(!player->AdoptQuestCancellationReceipt(42765,_attempt)){_cancelled=true;return;}
        auto current=player->GetQuestCancellationReceipt(42765);
        if(!current || current->load(std::memory_order_acquire))return;
        if(!_claimed)return;
        if(!player->CanContact() || !player->IsAlive() || player->IsBeingTeleported()){_contact=false;return;}
        if(!_contact){_contact=true;_tries=0;_parked=false;_timer=200;if(_rootAcknowledged)_pending|=6;}
        if(_timer>diff){_timer-=diff;return;}_timer=500;
        auto source=instance->GetEventObject(_source);
        if(!Source(source) || !player->InSamePhase(source))return;
        if(!_rootAcknowledged){Credit(player,source);return;}
        if(_storyDone)CleanupReceiver(player);
        if(!Allow(player) || !_pending || _parked)return;
        for(uint32 role=0;role<3;++role)
        {
            uint32 bit=1u<<role;if(!(_pending&bit))continue;
            if(role==0 && _storyDone){_pending&=~bit;continue;}
            auto aura=player->GetAura(AuraId(role),_owner);
            if(role && aura && Minion(player,role)){_pending&=~bit;continue;}
            if(role && Owns(aura,_marker))player->RemoveAura(aura); // Retry the original cast, never a direct summon.
            if(!Allow(player))break;
            _issuing=_rows[role==0?1:role==1?3:4].action.cast.spell;
            player->CastSpell(player,_issuing,false);
            _issuing=0;
        }
        if(_pending && ++_tries>=10)
        {
            _parked=true;
            TC_LOG_ERROR("scripts.ai","Fu Zan: wave-entry native casts remain unacknowledged after bounded retries; same attempt retained for source re-entry");
        }
    }
private:
    static uint32 AuraId(uint32 role){return role==0?217817:role==1?217659:217667;}
    bool Receivers(Player* player) const
    {
        // Yu'lon is also the next defense's real actor. Do not consume entry
        // while that actor or the original conversation speaker is absent.
        for(auto const& expected:{std::make_pair(109391u,373396u),std::make_pair(109465u,373425u)})
        {
            auto actor=player->FindNearestCreature(expected.first,player->GetVisibilityRange(),true);
            if(!actor || actor->GetDBTableGUIDLow()!=expected.second || actor->GetMap()!=instance || !actor->InSamePhase(player) ||
                !actor->IsAIEnabled || !dynamic_cast<SmartAI*>(actor->AI()))return false;
        }
        return true;
    }
    void CleanupReceiver(Player* player)
    {
        if(_dataSent || !_storyDone || !Allow(player))return;
        auto source=instance->GetEventObject(_source);if(!Source(source))return;
        // Keep the actual required speaker until the native conversation
        // exists. Independent companions and genuine progression do not wait.
        Dispatch(2,player,source);_dataSent=true;
    }
    bool Minion(Player* player,uint32 role) const
    {
        uint32 entry=role==1?109350:109352;uint32 spell=role==1?217666:217669;
        for(auto const& guid:*player->GetSummonList(entry))
            if(auto actor=ObjectAccessor::GetCreature(*player,guid))
                if(actor->IsAlive() && actor->GetEntry()==entry && actor->ToTempSummon() &&
                    actor->ToTempSummon()->GetSummonerGUID()==_owner && actor->GetUInt32Value(UNIT_FIELD_CREATED_BY_SPELL)==spell)return true;
        return false;
    }
    void Dispatch(uint32 id,Player* player,EventObject* event)
    {
        SmartScript script;script.OnInitialize(event,nullptr);auto row=_rows[id];row.link=0;row.runOnce=false;
        script.InitTimer(row);script.ProcessEvent(row,player,event->GetEntry());
    }
    void Credit(Player* player,EventObject* event)
    {
        auto scenario=World();
        if(Cancelled() || !Active(player) || !scenario)return;
        if(scenario->GetCurrentStep()==4 && scenario->CanUpdateCriteria(49176))Dispatch(0,player,event);
        if(Cancelled()){Cleanup(player);_cleaned=true;return;}
        _rootAcknowledged=scenario->GetCurrentStep()==5 && FuZanSummerpetal::Receipt(scenario,49176);
        if(!_rootAcknowledged)return;
        _pending=7;_timer=200;_contact=true;
    }
    void Cleanup(Player* player)
    {
        if(!player || player->GetGUID()!=_owner)return;
        for(uint32 role=0;role<3;++role)
            if(auto aura=player->GetAura(AuraId(role),_owner))if(Owns(aura,_marker))player->RemoveAura(aura);
        if(auto story=ObjectAccessor::GetConversation(*player,_story))if(story->GetCasterGUID()==_owner)story->Remove();
    }
    std::array<SmartScriptHolder,5> _rows;
    std::array<ObjectGuid,3> _tickets;
    ObjectGuid _owner,_source,_marker,_story;
    std::shared_ptr<QuestCancellationReceipt const> _attempt;
    uint32 _pending=0,_preexisting=0,_issuing=0,_timer=0,_tries=0;
    bool _catalogChecked=false,_catalogValid=false,_victoryRecorded=false,_claimed=false,_cancelled=false,_cleaned=false;
    bool _rootAcknowledged=false,_storyDone=false,_dataSent=false,_contact=false,_parked=false,_retired=false;
};

inline Instance* Find(Player* player)
{
    return player && player->GetMapId()==1625 ? dynamic_cast<Instance*>(player->GetInstanceScript()) : nullptr;
}
}

class eventobject_fuzan_wave_entry : public EventObjectScript
{
public:
    eventobject_fuzan_wave_entry() : EventObjectScript("eventobject_fuzan_wave_entry") { }
    bool IsTriggerMeets(Player* player,EventObject* event) override
    {
        if(!FuZanWaveEntry::Source(event))return true;
        auto state=FuZanWaveEntry::Find(player);return state && state->CanEnter(player,event);
    }
    bool OnTrigger(Player* player,EventObject* event,bool enter) override
    {
        if(!player->IsAlive())return false;
        if(FuZanWaveEntry::Source(event))
        {
            if(enter)if(auto state=FuZanWaveEntry::Find(player))state->Enter(player,event);
            return true;
        }
        SmartScript script;script.OnInitialize(event,nullptr);
        script.ProcessEventsFor(enter?SMART_EVENT_EVENTOBJECT_ONTRIGGER:SMART_EVENT_EVENTOBJECT_OFFTRIGGER,player,event->GetEntry());
        return true;
    }
};

class spell_fuzan_wave_attempt : public SpellScript
{
    PrepareSpellScript(spell_fuzan_wave_attempt);
    SpellCastResult Check()
    {
        auto player=GetCaster()->ToPlayer();if(!player)return SPELL_CAST_OK;
        auto state=FuZanWaveEntry::Find(player);
        return state && state->Spell(player,GetSpellInfo()->Id,GetSpell()->m_castGuid[1],GetSpell()->m_castGuid[0]) ? SPELL_CAST_OK : SPELL_FAILED_BAD_TARGETS;
    }
    void Register() override { OnCheckCast+=SpellCheckCastFn(spell_fuzan_wave_attempt::Check); }
};

class aura_fuzan_wave_controller : public AuraScript
{
    PrepareAuraScript(aura_fuzan_wave_controller);
public:
    ObjectGuid marker;
private:
    void Apply(AuraEffect const*,AuraEffectHandleModes)
    {
        auto player=GetUnitOwner() ? GetUnitOwner()->ToPlayer() : nullptr;
        if(!player)return;
        auto state=FuZanWaveEntry::Find(player);
        if(!state || !state->Allow(player)){PreventDefaultAction();GetAura()->Remove();return;}
        marker=state->Marker();
    }
    void Register() override
    {
        OnEffectApply+=AuraEffectApplyFn(aura_fuzan_wave_controller::Apply,EFFECT_ALL,SPELL_AURA_ANY,AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
    }
};

namespace FuZanWaveEntry
{
inline bool Owns(Aura* aura,ObjectGuid const& marker)
{
    if(!aura || marker.IsEmpty())return false;
    for(auto script:aura->m_loadedScripts)if(auto owned=dynamic_cast<aura_fuzan_wave_controller*>(script))return owned->marker==marker;
    return false;
}
inline void Stamp(Aura* aura,ObjectGuid const& marker)
{
    if(aura)for(auto script:aura->m_loadedScripts)if(auto owned=dynamic_cast<aura_fuzan_wave_controller*>(script))owned->marker=marker;
}
}

class spell_fuzan_wave_conversation : public SpellScript
{
    PrepareSpellScript(spell_fuzan_wave_conversation);
    SpellCastResult Check()
    {
        auto player=GetCaster()->ToPlayer();if(!player)return SPELL_CAST_OK;
        auto state=FuZanWaveEntry::Find(player);
        return state && state->Spell(player,217817,GetSpell()->m_castGuid[1],GetSpell()->m_castGuid[0]) ? SPELL_CAST_OK : SPELL_FAILED_BAD_TARGETS;
    }
    void Create(SpellEffIndex effect)
    {
        if(!GetCaster()->IsPlayer())return;
        PreventHitDefaultEffect(effect);
        auto player=GetCaster()->ToPlayer();auto state=FuZanWaveEntry::Find(player);
        if(!state || !state->Allow(player))return;
        auto story=new Conversation();Position position=player->GetPosition();
        if(GetExplTargetDest() && GetHitDest())position=*GetHitDest();
        if(!story->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(),2779,player,GetSpellInfo(),position)){delete story;return;}
        if(!state->Story(player,story))story->Remove();
    }
    void Register() override
    {
        OnCheckCast+=SpellCheckCastFn(spell_fuzan_wave_conversation::Check);
        OnEffectHit+=SpellEffectFn(spell_fuzan_wave_conversation::Create,EFFECT_0,SPELL_EFFECT_SUMMON_CONVERSATION);
    }
};

class spell_fuzan_wave_retrieval : public SpellScript
{
    PrepareSpellScript(spell_fuzan_wave_retrieval);
    void Retire(SpellEffIndex)
    {
        auto player=GetCaster()->ToPlayer();
        if(auto state=FuZanWaveEntry::Find(player))state->Retire(player);
    }
    void Register() override
    {
        // Native217814 effect4 still removes217659; effect5 removes217667.
        // Merely stop our retries; retain every original retrieval effect.
        OnEffectHitTarget+=SpellEffectFn(spell_fuzan_wave_retrieval::Retire,EFFECT_4,SPELL_EFFECT_REMOVE_AURA);
    }
};
