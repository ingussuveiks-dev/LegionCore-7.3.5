#pragma once

#include "ChatTextBuilder.h"
#include "GameObjectAI.h"
#include "GridNotifiersImpl.h"
#include "MotionMaster.h"
#include "SpellAuras.h"

// Outdoor quest42868. Keep the native Type22 casts, personal3331 summon,
// four-unit power bar and217332 ->217331 completion; this supplies their
// missing interaction/lifetime controller. No final quest credit is issued here.
namespace FuZanBrewing
{
constexpr uint32 QuestId=42868, Parent=217302, Meter=217321, Complete=217332;
struct Ingredient { uint32 entry,spawn,child,request,added,text,voice; };
static Ingredient const Recipe[]{
    {251505,128078,217213,217245,217251,115346,3959},
    {251501,128068,217216,217278,217282,115347,4224},
    {251503,128077,217219,217286,217290,115392,5765},
    {251510,128081,217224,217294,217296,115348,4819}
};
class Actor;
inline Actor* Find(Player* player);
// The native main->controller->summon handoff is synchronous. This short
// thread-local scope authorizes only its destination adjustment; no Player or
// AI pointer is retained by a delayed event or shared between map threads.
static thread_local ObjectGuid StartingOwner;
struct StartScope
{
    ObjectGuid previous;
    explicit StartScope(ObjectGuid const& owner):previous(StartingOwner){StartingOwner=owner;}
    ~StartScope(){StartingOwner=previous;}
};
inline bool Active(Player* player,bool finishing=false)
{
    return player && player->IsInWorld() && player->GetMapId()==870 && player->getClass()==CLASS_MONK &&
        !player->GetQuestRewardStatus(QuestId) &&
        (player->GetQuestStatus(QuestId)==QUEST_STATUS_INCOMPLETE || (finishing && player->GetQuestStatus(QuestId)==QUEST_STATUS_COMPLETE));
}
inline bool Contact(Player* player,WorldObject* source,float range)
{
    return Active(player) && player->IsAlive() && player->CanContact() && !player->IsBeingTeleported() && source &&
        source->IsInWorld() && player->GetMap()==source->GetMap() && player->InSamePhase(source) &&
        !source->ShouldHideFor(player->GetGUID()) && player->canSeeOrDetect(source) &&
        player->IsWithinDistInMap(source,range) && player->IsWithinLOSInMap(source);
}
inline bool Fire(GameObject* go) { return go && go->GetMapId()==870 && go->GetEntry()==251490 && go->GetDBTableGUIDLow()==128074; }
inline Creature* Public(Player* player)
{
    if(!player)return nullptr;
    auto actor=player->FindNearestCreature(109187,60.0f,true);
    return actor && actor->GetDBTableGUIDLow()==366254 && actor->GetMapId()==870 && actor->InSamePhase(player) ? actor : nullptr;
}
inline int Index(GameObject* go)
{
    if(go && go->GetMapId()==870)for(int i=0;i<4;++i)
        if(go->GetEntry()==Recipe[i].entry && go->GetDBTableGUIDLow()==Recipe[i].spawn)return i;
    return -1;
}
inline int Child(uint32 spell) { for(int i=0;i<4;++i)if(Recipe[i].child==spell)return i;return -1; }
inline void Say(Creature* actor,Player* owner,uint32 id)
{
    auto text=sBroadcastTextStore.LookupEntry(id);
    if(!actor || !owner || !text || !sConditionMgr->IsPlayerMeetingCondition(owner,text->ConditionID))return;
    // PlayerDistWorker does not enforce a personal summon audience. Emit the
    // native localized text and its sound exactly once, to this owner only.
    Trinity::BroadcastTextBuilder builder(actor,CHAT_MSG_MONSTER_SAY,id,owner);
    Trinity::LocalizedPacketDo<Trinity::BroadcastTextBuilder> localized(builder);localized(owner);
    uint8 gender=actor->getGender();
    if(auto display=sCreatureDisplayInfoStore.LookupEntry(actor->GetDisplayId()))if(display->Gender!=GENDER_NONE)gender=display->Gender;
    auto sound=text->SoundEntriesID[gender==GENDER_FEMALE?1:0];
    if(sound)actor->PlayDirectSound(sound,owner);
}
inline void RemoveChildren(Player* player)
{
    for(auto const& item:Recipe)player->RemoveAurasDueToSpell(item.request);
    player->RemoveAurasDueToSpell(217247);
    player->RemoveAurasDueToSpell(Complete);
    player->RemoveAurasDueToSpell(Meter);
}

class Actor : public ScriptedAI
{
public:
    explicit Actor(Creature* creature):ScriptedAI(creature) { }
    ~Actor() override { Restore(); }
    enum Phase { Provisional,Approach,Intro,Between,Requested,Pending,Finishing,Returning,Done };
    Player* Owner() const { return ObjectAccessor::GetPlayer(*me,_owner); }
    ObjectGuid Guid() const { return me->GetGUID(); }
    void RestoreVisibility() { Restore(); }
    bool Current(Player* player) const
    {
        auto lease=_lease.lock();auto status=player?player->getQuestStatus(QuestId):nullptr;
        return _phase!=Provisional && _phase!=Done && me->IsInWorld() && me->IsAlive() && Active(player,_phase>=Finishing) && player->GetMap()==me->FindMap() &&
            player->GetGUID()==_owner && lease && status && status->ScriptLifetime==lease &&
            player->HasAura(Parent) && me->isSummon() && me->ToTempSummon()->GetSummonerGUID()==_owner;
    }
    bool Begin(Player* player,GameObject* source)
    {
        auto original=Public(player);
        if(_phase!=Provisional || !Fire(source) || !Contact(player,source,INTERACTION_DISTANCE) || !original ||
            !me->isSummon() || me->ToTempSummon()->GetSummonerGUID()!=player->GetGUID() ||
            !player->HasAura(Parent) || !player->HasAura(Meter) || !player->HasAura(Complete))return false;
        player->AddQuestDelayedEvent(QuestId,0,[]{});
        auto status=player->getQuestStatus(QuestId);if(!status || !status->ScriptLifetime)return false;
        _lease=status->ScriptLifetime;_owner=player->GetGUID();_fire=source->GetGUID();_public=original->GetGUID();
        _origin=original->GetPosition();
        source->GetContactPoint(original,_station.m_positionX,_station.m_positionY,_station.m_positionZ);
        original->UpdateGroundPositionZ(_station.m_positionX,_station.m_positionY,_station.m_positionZ);
        _ownsHide=!original->ShouldHideFor(_owner);
        if(_ownsHide){original->AddToHideList(_owner);original->UpdateObjectVisibility();}
        player->CastSpell(player,221291,true); // Native clear-four, no saved character edits.
        _phase=Approach;_timer=15000;_age=0;me->SetWalk(false);
        me->GetMotionMaster()->MovePoint(1,_station);
        return true;
    }
    bool Issue(Player* player,GameObject* source,int index)
    {
        if(index<0 || index>=4 || _phase!=Requested || !Current(player) || !Contact(player,source,INTERACTION_DISTANCE) ||
            !player->HasAura(Recipe[_step].request) || !player->HasAura(217247) || _issued || !source->isSpawned() ||
            source->HasFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_IN_USE|GO_FLAG_NOT_SELECTABLE))return false;
        // The native Type22 PlayerCondition ids are absent in build26972.
        // Preserve ConditionMgr's ordinary missing-record semantics.
        if(!sConditionMgr->IsPlayerMeetingCondition(player,source->GetGOInfo()->spellCaster.conditionID1))return false;
        _issued=++_serial;_ingredient=index;_source=source->GetGUID();_issueWait=1500;return true;
    }
    uint64 Ticket(Player* player,int index) const
    {
        if(!Current(player) || _phase!=Requested || !_issued || index!=_ingredient)return 0;
        auto source=me->GetMap()->GetGameObject(_source);
        return Index(source)==index && Contact(player,source,INTERACTION_DISTANCE) ? _issued : 0;
    }
    void Release(Player* player,int index,uint64 ticket)
    {
        if(!ticket || Ticket(player,index)!=ticket)return;
        _issued=0;_source.Clear();
        if(uint32(index)!=_step){Failure(player,false);return;}
        _phase=Pending;_credit=ticket;_timer=2500;_launching=ticket;
        auto result=player->CastSpell(player,Recipe[index].added,true);
        _launching=0;
        if(result!=SPELL_CAST_OK && _phase==Pending){_credit=0;_phase=Requested;}
    }
    uint64 Capture(Player* player,bool added,ObjectGuid const& cast,ObjectGuid const& parent)
    {
        if(!Current(player) || _phase!=Pending)return 0;
        if(added && _launching){_deliveryRoot=cast;return _launching;}
        // Native effect64 schedules the Credit cast ten milliseconds later.
        // Its parent GUID, rather than a temporary hit-hook scope, identifies
        // the originally issued Added even after a new attempt has begun.
        return !added && !_deliveryRoot.IsEmpty() && parent==_deliveryRoot ? _credit : 0;
    }
    uint64 CaptureReceipt(Player* player) const { return Current(player) && _phase==Pending ? _delivering : 0; }
    bool Delivery(Player* player,uint64 ticket) const
    {
        return ticket && Current(player) && player->IsAlive() && player->CanContact() && !player->IsBeingTeleported() &&
            player->IsWithinDistInMap(me,60.0f) && _phase==Pending && _credit==ticket;
    }
    void EnterDelivery(Player* player,uint64 ticket) { if(Delivery(player,ticket))_delivering=ticket; }
    void LeaveDelivery(uint64 ticket) { if(_delivering==ticket)_delivering=0; }
    void Receipt(Player* player,uint64 ticket,ObjectGuid const& target)
    {
        if(target!=Guid() || !Delivery(player,ticket))return;
        _credit=0;_delivering=0; // Consume before the reentrant native energize/proc.
        auto before=player->GetPower(POWER_ALTERNATE);
        if(before!=int32(_step)){Stop(player);return;}
        _phase=Between;
        me->CastSpell(player,217335,true); // Target92 is selected player, not summoner.
        if(player->GetPower(POWER_ALTERNATE)!=before+1){Stop(player);return;}
        if(before==3)
        {
            if(player->GetQuestStatus(QuestId)!=QUEST_STATUS_COMPLETE){Stop(player);return;}
            _phase=Finishing;Say(me,player,115359);_timer=5515;
        }
        else
        {
            static uint32 const text[]{115352,115353,115354,115355},voice[]{2833,2759,3533,3153};
            auto line=urand(0,3);Say(me,player,text[line]);_timer=voice[line];
        }
    }
    void MovementInform(uint32 type,uint32 id) override
    {
        if(type!=POINT_MOTION_TYPE)return;
        auto player=Owner();
        if(id==1 && _phase==Approach && Current(player) && me->GetExactDist(&_station)<2.0f)
        {
            auto fire=me->GetMap()->GetGameObject(_fire);
            if(!Fire(fire) || !me->IsWithinLOSInMap(fire)){Stop(player);return;}
            me->SetFacingToObject(fire);_phase=Intro;Say(me,player,115345);_timer=6069;
        }
        else if(id==2 && _phase==Returning && me->GetExactDist(&_origin)<2.0f)Stop(player);
    }
    void UpdateAI(uint32 diff) override
    {
        if(_phase==Done)return;
        if(_phase==Provisional)
        {
            // This template binding must not retire another script's raw
            // summon or a non-Monk using the native spell elsewhere.
            if(!me->isSummon() || me->GetUInt32Value(UNIT_FIELD_CREATED_BY_SPELL)!=217304)return;
            auto summoner=me->ToTempSummon()->GetSummoner();
            if(summoner && (!summoner->IsPlayer() || summoner->getClass()!=CLASS_MONK))return;
        }
        if(_phase==Provisional){if(_timer>diff)_timer-=diff;else Stop(nullptr);return;}
        auto player=Owner();
        if(!Current(player) || !player->IsAlive() || !player->CanContact() || player->IsBeingTeleported() ||
            !player->IsWithinDistInMap(me,60.0f)){Stop(player);return;}
        _age+=std::min(diff,300000u);if(_age>=300000){Stop(player);return;}
        if(_issued){if(_issueWait>diff)_issueWait-=diff;else{_issued=0;_source.Clear();}}
        if(_timer>diff){_timer-=diff;return;}_timer=0;
        switch(_phase)
        {
            case Approach:case Returning:Stop(player);break;
            case Intro:case Between:Request(player);break;
            case Requested:Failure(player,true);break;
            case Pending:
                // A receipt is required. Do not synthesize +1 when a cast or
                // target disappeared; invalidate all late delivery stamps.
                _credit=0;Failure(player,true);break;
            case Finishing:
                _phase=Returning;_timer=15000;me->GetMotionMaster()->MovePoint(2,_origin);break;
            default:break;
        }
    }
    void Stop(Player* player)
    {
        if(_phase==Done)return;
        bool current=Current(player);_phase=Done;_issued=_credit=_launching=_delivering=0;
        Restore();
        // Map threads own their Player. An old actor must not clean a new
        // attempt's auras or touch an owner who has already transferred maps.
        if(current && Find(player)==this){RemoveChildren(player);player->RemoveAurasDueToSpell(Parent);}
        me->DespawnOrUnsummon();
    }
private:
    void Request(Player* player)
    {
        auto power=player->GetPower(POWER_ALTERNATE);
        if(power<0 || power>=4){Stop(player);return;}
        _step=uint32(power);_phase=Requested;_timer=9000;
        Say(me,player,Recipe[_step].text);
        if(me->CastSpell(player,Recipe[_step].request,true)!=SPELL_CAST_OK ||
            player->CastSpell(player,217247,true)!=SPELL_CAST_OK || !player->HasAura(Recipe[_step].request) || !player->HasAura(217247))Stop(player);
    }
    void Failure(Player* player,bool timeout)
    {
        _issued=_credit=_launching=_delivering=0;_source.Clear();
        for(auto const& item:Recipe)player->RemoveAurasDueToSpell(item.request);
        player->RemoveAurasDueToSpell(217247);
        me->CastSpell(player,timeout?217337:217336,true);
        // Failure footage is unavailable. These native lower/clear spells and
        // native failure lines are an explicit implementation inference.
        static uint32 const wrong[]{115349,115350,115351},wv[]{3777,5460,6139};
        static uint32 const late[]{115356,115357,115358},lv[]{4875,5298,5966};
        auto line=urand(0,2);Say(me,player,timeout?late[line]:wrong[line]);_phase=Between;_timer=timeout?lv[line]:wv[line];
    }
    void Restore()
    {
        if(!_ownsHide)return;_ownsHide=false;
        if(auto map=me->FindMap())if(auto original=map->GetCreature(_public))
            if(original->GetEntry()==109187 && original->GetDBTableGUIDLow()==366254)
            {original->RemoveFromHideList(_owner);if(!map->IsMapUnload())original->UpdateObjectVisibility();}
    }
    Phase _phase=Provisional;
    ObjectGuid _owner,_fire,_public,_source,_deliveryRoot;
    std::weak_ptr<uint8> _lease;
    Position _origin,_station;
    uint32 _timer=5000,_age=0,_step=0,_issueWait=0;
    int _ingredient=-1;
    uint64 _serial=0,_issued=0,_credit=0,_launching=0,_delivering=0;
    bool _ownsHide=false;
};
inline Actor* Find(Player* player)
{
    if(!player || !player->IsInWorld())return nullptr;
    Actor* found=nullptr;
    for(auto const& guid:*player->GetSummonList(109224))
        if(auto creature=player->GetMap()->GetCreature(guid))if(creature->IsInWorld() && creature->IsAIEnabled && creature->isSummon() &&
            creature->GetUInt32Value(UNIT_FIELD_CREATED_BY_SPELL)==217304 &&
            creature->ToTempSummon()->GetSummonerGUID()==player->GetGUID())
            if(auto actor=dynamic_cast<Actor*>(creature->AI())){if(found)return nullptr;found=actor;}
    return found;
}
inline void Leave(Player* player)
{
    if(!player || !player->IsInWorld() || player->GetMapId()!=870 || player->getClass()!=CLASS_MONK)return;
    if(auto actor=Find(player))actor->Stop(player);
    if(player->HasAura(Parent)){RemoveChildren(player);player->RemoveAurasDueToSpell(Parent);}
}
}

class map_fuzan_brewing : public WorldMapScript
{
public:
    map_fuzan_brewing():WorldMapScript("map_fuzan_brewing",870){}
    void OnPlayerLeave(Map*,Player* player) override { FuZanBrewing::Leave(player); }
};
class player_fuzan_brewing : public PlayerScript
{
public:
    player_fuzan_brewing():PlayerScript("player_fuzan_brewing"){}
    void OnBeforeLogout(Player* player) override { FuZanBrewing::Leave(player); }
};

struct npc_fuzan_brewing : FuZanBrewing::Actor { explicit npc_fuzan_brewing(Creature* c):Actor(c){} };
struct npc_fuzan_brewing_questgiver : ScriptedAI
{
    explicit npc_fuzan_brewing_questgiver(Creature* c):ScriptedAI(c){}
    void sQuestAccept(Player* player,Quest const* quest) override
    {
        if(quest && quest->GetQuestId()==FuZanBrewing::QuestId && me->GetMapId()==870 && me->GetDBTableGUIDLow()==366254)
            FuZanBrewing::Say(me,player,115360);
    }
};
struct go_fuzan_brewing_fire : GameObjectAI
{
    explicit go_fuzan_brewing_fire(GameObject* g):GameObjectAI(g){}
    bool GossipUse(Player* player) override
    {
        using namespace FuZanBrewing;
        if(!FuZanBrewing::Fire(go))return false;
        if(!Contact(player,go,INTERACTION_DISTANCE) || !Public(player) || !go->isSpawned() ||
            go->HasFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_IN_USE|GO_FLAG_NOT_SELECTABLE))return true;
        if(auto actor=Find(player)){if(actor->Current(player))return true;actor->Stop(nullptr);return true;}
        if(player->HasAura(Parent)){player->RemoveAurasDueToSpell(Parent);return true;}
        if(player->HasAuraType(SPELL_AURA_ENABLE_ALT_POWER))return true;
        StartScope start(player->GetGUID());
        if(player->CastSpell(player,217297,true)==SPELL_CAST_OK)
            if(auto actor=Find(player))if(actor->Begin(player,go)){player->KillCreditGO(go->GetEntry(),go->GetGUID());return true;}
        RemoveChildren(player);player->RemoveAurasDueToSpell(Parent);return true;
    }
};
struct go_fuzan_brewing_ingredient : GameObjectAI
{
    explicit go_fuzan_brewing_ingredient(GameObject* g):GameObjectAI(g){}
    bool GossipUse(Player* player) override
    {
        auto index=FuZanBrewing::Index(go);if(index<0)return false;
        auto actor=FuZanBrewing::Find(player);
        return !actor || !actor->Issue(player,go,index); // Valid issuance retains native Type22 casting.
    }
};

class spell_fuzan_brewing_summon : public SpellScript
{
    PrepareSpellScript(spell_fuzan_brewing_summon);
    void Destination(SpellEffIndex)
    {
        auto player=GetCaster()->ToPlayer();auto original=FuZanBrewing::Public(player);
        if(original && FuZanBrewing::Active(player) && FuZanBrewing::StartingOwner==player->GetGUID() && GetHitDest())GetHitDest()->Relocate(original->GetPosition());
    }
    void Register() override { OnEffectLaunch+=SpellEffectFn(spell_fuzan_brewing_summon::Destination,EFFECT_0,SPELL_EFFECT_SUMMON); }
};
class spell_fuzan_brewing_ingredient : public SpellScript
{
    PrepareSpellScript(spell_fuzan_brewing_ingredient);
    ObjectGuid _actor;uint64 _ticket=0;
    bool Load() override { return FuZanBrewing::Active(GetCaster()->ToPlayer(),true); }
    SpellCastResult Check()
    {
        auto player=GetCaster()->ToPlayer();auto actor=FuZanBrewing::Find(player);
        if(!_ticket && actor){_ticket=actor->Ticket(player,FuZanBrewing::Child(GetId()));_actor=actor->Guid();}
        return actor && actor->Guid()==_actor && _ticket && actor->Ticket(player,FuZanBrewing::Child(GetId()))==_ticket ? SPELL_CAST_OK : SPELL_FAILED_BAD_TARGETS;
    }
    void Release()
    {
        auto player=GetCaster()->ToPlayer();auto actor=FuZanBrewing::Find(player);
        if(actor && actor->Guid()==_actor)actor->Release(player,FuZanBrewing::Child(GetId()),_ticket);
    }
    void Register() override
    {
        OnCheckCast+=SpellCheckCastFn(spell_fuzan_brewing_ingredient::Check);
        AfterCast+=SpellCastFn(spell_fuzan_brewing_ingredient::Release);
    }
};
class spell_fuzan_brewing_delivery : public SpellScript
{
    PrepareSpellScript(spell_fuzan_brewing_delivery);
    ObjectGuid _actor;uint64 _ticket=0;
    bool Load() override
    {
        auto player=GetCaster()->ToPlayer();auto actor=FuZanBrewing::Find(player);
        bool added=false;for(auto const& item:FuZanBrewing::Recipe)if(item.added==GetId())added=true;
        bool originalParent=false;
        auto const& parent=GetSpell()->m_castGuid[1];
        if(!added && player && player->getClass()==CLASS_MONK && parent.GetMapId()==870)
            for(auto const& item:FuZanBrewing::Recipe)if(parent.GetEntry()==item.added)originalParent=true;
        // Delegate unrelated contexts unchanged. A delayed native Credit from
        // our original map still needs its immutable guard after abandonment
        // or transfer, even though the owner is no longer locally eligible.
        if(!FuZanBrewing::Active(player,true) && !originalParent)return false;
        if(actor){_actor=actor->Guid();_ticket=actor->Capture(player,added,GetSpell()->m_castGuid[0],GetSpell()->m_castGuid[1]);}return true;
    }
    void Launch(SpellEffIndex effect)
    {
        auto player=GetCaster()->ToPlayer();auto actor=FuZanBrewing::Find(player);
        if(!actor || actor->Guid()!=_actor || !actor->Delivery(player,_ticket))PreventHitEffect(effect);
    }
    void Before()
    {
        auto player=GetCaster()->ToPlayer();auto actor=FuZanBrewing::Find(player);
        if(actor && actor->Guid()==_actor && actor->Delivery(player,_ticket)){actor->EnterDelivery(player,_ticket);return;}
        for(uint8 i=0;i<MAX_SPELL_EFFECTS;++i)PreventHitEffect(SpellEffIndex(i));
    }
    void After()
    {
        auto actor=FuZanBrewing::Find(GetCaster()->ToPlayer());if(actor && actor->Guid()==_actor)actor->LeaveDelivery(_ticket);
    }
    void Register() override
    {
        OnEffectLaunch+=SpellEffectFn(spell_fuzan_brewing_delivery::Launch,EFFECT_ALL,SPELL_EFFECT_ANY);
        OnEffectLaunchTarget+=SpellEffectFn(spell_fuzan_brewing_delivery::Launch,EFFECT_ALL,SPELL_EFFECT_ANY);
        BeforeHit+=SpellHitFn(spell_fuzan_brewing_delivery::Before);
        AfterHit+=SpellHitFn(spell_fuzan_brewing_delivery::After);
    }
};
class spell_fuzan_brewing_receipt : public SpellScript
{
    PrepareSpellScript(spell_fuzan_brewing_receipt);
    ObjectGuid _actor;uint64 _ticket=0;
    bool Load() override
    {
        auto player=GetCaster()->ToPlayer();if(auto actor=FuZanBrewing::Find(player)){_actor=actor->Guid();_ticket=actor->CaptureReceipt(player);}return true;
    }
    void Hit(SpellEffIndex)
    {
        auto player=GetCaster()->ToPlayer();auto actor=FuZanBrewing::Find(player);auto target=GetHitCreature();
        if(actor && actor->Guid()==_actor && target)actor->Receipt(player,_ticket,target->GetGUID());
    }
    void Register() override { OnEffectHitTarget+=SpellEffectFn(spell_fuzan_brewing_receipt::Hit,EFFECT_0,SPELL_EFFECT_SCRIPT_EFFECT); }
};
class aura_fuzan_brewing_controller : public AuraScript
{
    PrepareAuraScript(aura_fuzan_brewing_controller);
    std::weak_ptr<uint8> _lease;bool _bound=false;uint32 _timer=500;
    bool Load() override
    {
        auto owner=GetUnitOwner();return owner && owner->IsPlayer() && owner->getClass()==CLASS_MONK;
    }
    void Update(uint32 diff)
    {
        if(_timer>diff){_timer-=diff;return;}_timer=500;
        auto player=GetUnitOwner()?GetUnitOwner()->ToPlayer():nullptr;if(!player)return;
        auto status=player->getQuestStatus(FuZanBrewing::QuestId);
        if(!_bound && status && status->ScriptLifetime){_lease=status->ScriptLifetime;_bound=true;}
        auto lease=_lease.lock();auto actor=FuZanBrewing::Find(player);
        if(!FuZanBrewing::Active(player,true) || !player->IsAlive() || (_bound && (!lease || !status || lease!=status->ScriptLifetime)) ||
            !actor || !actor->Current(player))GetAura()->Remove();
    }
    void Remove(AuraEffect const*,AuraEffectHandleModes)
    {
        auto player=GetUnitOwner()?GetUnitOwner()->ToPlayer():nullptr;if(!player)return;
        auto current=player->GetAura(FuZanBrewing::Parent);
        if(current && current!=GetAura())return;
        // Release visibility before the native controller's delayed summon
        // despawn, so an immediate retry can acquire its own hide lease.
        if(auto actor=FuZanBrewing::Find(player))actor->Stop(nullptr);
        FuZanBrewing::RemoveChildren(player);
    }
    void BeforeRemove(AuraEffect const*,AuraEffectHandleModes)
    {
        auto player=GetUnitOwner()?GetUnitOwner()->ToPlayer():nullptr;if(!player)return;
        auto current=player->GetAura(FuZanBrewing::Parent);if(current && current!=GetAura())return;
        // AddObjectToRemoveList performs CleanupBeforeDelete immediately. An
        // AI destructor or AfterEffectRemove can therefore already lack a map.
        // Restore the owned public visibility before native Aura428 removes it.
        if(auto actor=FuZanBrewing::Find(player))actor->RestoreVisibility();
    }
    void Register() override
    {
        OnAuraUpdate+=AuraUpdateFn(aura_fuzan_brewing_controller::Update);
        OnEffectRemove+=AuraEffectRemoveFn(aura_fuzan_brewing_controller::BeforeRemove,EFFECT_0,SPELL_AURA_SUMMON_CONTROLLER,AURA_EFFECT_HANDLE_REAL);
        AfterEffectRemove+=AuraEffectRemoveFn(aura_fuzan_brewing_controller::Remove,EFFECT_0,SPELL_AURA_SUMMON_CONTROLLER,AURA_EFFECT_HANDLE_REAL);
    }
};
