#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <list>
#include <set>
#include <vector>
#include <iostream>
#include <utility>
using uint32=uint32_t;
using int32=int32_t;
using SpellEffIndex=uint32;
#include "CastFlags.inc"
enum { SMART_ACTION_CAST=11, SMART_ACTION_INVOKER_CAST=85, SMART_TARGET_POSITION=8, SMART_TARGET_NONE=0, SMART_TARGET_ACTION_INVOKER=7, SPELL_EFFECT_HANDLE_HIT_TARGET=1 };
enum {SMART_TARGET_ACTION_INVOKER_PLAYER=130};
enum SpellCastResult { SPELL_CAST_OK, SPELL_FAILED_BAD_TARGETS, SPELL_FAILED_SPELL_IN_PROGRESS };
enum { CLASS_HUNTER=3, QUEST_STATUS_INCOMPLETE=3, QUEST_STATUS_COMPLETE=1, EFFECT_0=0, EFFECT_1=1, SPELL_EFFECT_TELEPORT_L=252, SPELL_EFFECT_KILL_CREDIT=90 };
#define TC_LOG_DEBUG(...) ((void)0)
#define ENSURE_AI(type, pointer) (pointer)
struct Unit;
struct Player;
struct ObjectGuid { Unit* pointer=nullptr; static ObjectGuid Empty; };
ObjectGuid ObjectGuid::Empty;
struct WorldObject {
    bool world=true;
    virtual ~WorldObject()=default;
    virtual Unit* ToUnit(){return nullptr;}
    virtual Player* ToPlayer(){return nullptr;}
    bool IsPlayer(){return ToPlayer()!=nullptr;}
    bool IsUnit(){return ToUnit()!=nullptr;}
    bool IsCreature(){return !IsPlayer();}
    bool IsInWorld(){return world;}
    int GetMap(){return 1;}bool InSamePhase(WorldObject*){return true;}bool IsWithinDistInMap(WorldObject*,float){return true;}
    bool HasQuestObjectScope(){return false;}bool CanTargetQuestObject(WorldObject*){return true;}
    virtual ObjectGuid GetGUID(){return {};}
};
struct SmartAI { void SetCombatMove(bool){} };
struct Unit:WorldObject {
    uint32 casts=0,spell=0;
    uint32 entry=100397,map=1489,instance=42,scriptId=0;
    bool triggered=true;
    bool living=true;
    Unit* target=nullptr;
    SpellCastResult result=SPELL_CAST_OK;
    std::vector<std::function<void()>> delayed;
    Unit* ToUnit() override{return this;}
    Unit* ToCreature(){return this;}
    uint32 GetEntry(){return entry;}
    uint32 GetMapId(){return map;}
    uint32 GetInstanceId(){return instance;}
    uint32 GetScriptId(){return scriptId;}
    bool IsAlive(){return living;}
    ObjectGuid GetGUID() override{return {this};}
    void GetPosition(float& first,float& second,float& third){first=second=third=0;}
    SpellCastResult CastSpell(float,float,float,uint32,bool){return result;}
    SpellCastResult CastSpell(Unit* selected,uint32 identity,bool instant){++casts;spell=identity;target=selected;triggered=instant;return result;}
    void InterruptNonMeleeSpells(bool){}
    bool HasAura(uint32){return false;}
    SmartAI* AI(){static SmartAI ai;return &ai;}
    void AddDelayedEvent(uint32 delay,std::function<void()> callback){assert(delay==200);delayed.push_back(callback);}
    void Flush(){auto pending=std::move(delayed);delayed.clear();for(auto& callback:pending)callback();}
};
struct GroupReference { Player* player=nullptr; GroupReference* following=nullptr; Player* getSource(){return player;} GroupReference* next(){return following;} };
struct Group { GroupReference* first=nullptr; GroupReference* GetFirstMember(){return first;} };
struct WorldLocation { WorldLocation(uint32,float,float,float,float){} };
struct Player:Unit {
    uint32 credits=0;
    bool near=true,alive=true,corpse=false,talked=false;
    bool canDelay=false,pendingTeleport=false,teleporting=false,owned=true,phase=true,rewarded=false;
    uint32 classId=CLASS_HUNTER,quest=QUEST_STATUS_INCOMPLETE,m_teleport_options=0;
    WorldLocation m_teleport_dest{0,0,0,0,0};
    Group* currentGroup=nullptr;
    Player* ToPlayer() override{return this;}
    Group* GetGroup(){return currentGroup;}
    bool TalkedToCreature(uint32,ObjectGuid){return talked;}
    bool IsAtGroupRewardDistance(WorldObject*){return near;}
    bool IsAlive(){return alive;}
    uint32 getClass(){return classId;}
    uint32 GetQuestStatus(uint32 identity){assert(identity==40419);return quest;}
    bool GetQuestRewardStatus(uint32 identity){assert(identity==40419);return rewarded;}
    bool HasItemCount(uint32 identity,uint32 amount,bool bank){assert(identity==128826&&amount==1&&bank);return owned;}
    bool InSamePhase(Unit*){return phase;}
    bool IsBeingTeleported(){return teleporting;}
    bool IsBeingTeleportedFar(){return teleporting;}
    bool IsCanDelayTeleport(){return canDelay;}
    void SetCanDelayTeleport(bool state){canDelay=state;}
    bool IsHasDelayedTeleport(){return pendingTeleport;}
    void SetDelayedTeleportFlag(bool state){pendingTeleport=state;}
    void SetSemaphoreTeleportFar(bool state){teleporting=state;}
#include "DelayScope.inc"
    bool isWatchingMovie(){return false;}
    bool TeleportToFixture(uint32 mapid,float x,float y,float z,float orientation,uint32 options){
#include "TeleportDelay.inc"
        world=false;return true;
    }
    WorldObject* GetCorpse(){return corpse?this:nullptr;}
    void KilledMonsterCredit(uint32 identity,ObjectGuid){assert(identity==102548);++credits;}
    void RewardPlayerAndGroupAtEvent(uint32,WorldObject*);
};
#include "GroupCredit.inc"
struct Effect { int32 MiscValue=102548; };
struct SpellInfo { Effect effect; Effect const* GetEffect(uint32,uint32) const{return &effect;} };
struct SpellManager { SpellInfo info; SpellInfo const* GetSpellInfo(uint32){return &info;} } spellManager;
auto sSpellMgr=&spellManager;
Unit* GetScriptCreditCaster(Unit* invoker,SpellInfo const*){return invoker;}
Player* GetScriptCreditPlayer(Unit* invoker){return invoker?invoker->ToPlayer():nullptr;}
struct Spell {
    uint32 effectHandleMode=SPELL_EFFECT_HANDLE_HIT_TARGET,m_diffMode=0;
    Unit* unitTarget=nullptr;
    SpellInfo const* m_spellInfo=&spellManager.info;
    uint32 teleports=0;
    bool rejectTeleport=false;
    void EffectKillCredit(SpellEffIndex);
    void EffectTeleportUnits(SpellEffIndex){++teleports;if(!rejectTeleport)if(auto* player=unitTarget?unitTarget->ToPlayer():nullptr)player->TeleportToFixture(1220,-837,4330.1f,745,6.09993f,0);}
};
#include "CreditEffect.inc"
struct ObjectAccessor { static Unit* GetUnit(Unit&,ObjectGuid guid){return guid.pointer&&guid.pointer->world?guid.pointer:nullptr;}static WorldObject* GetWorldObject(Unit&,ObjectGuid guid){return guid.pointer;} };
struct GameObject { SpellCastResult CastSpell(Unit*,uint32){return SPELL_CAST_OK;} };
using ObjectList=std::list<WorldObject*>;
struct SmartScriptHolder {
    bool runOnce=false;
    uint32 type=11,targetType=7;
    struct {struct {uint32 spell=205040,flags=0;} cast;} action;
    struct {float x=0,y=0,z=0;struct{uint32 dist=0;}playerDistance;} target;
    uint32 GetTargetType()const{return targetType;}
};
struct SmartScript {
 // This older scope has no linked-cast opt-in; unexpected use must fail.
 template<class... Args>void QueueLinkedCastRetry(Args const&...){assert(false);}
 template<class... Args>uint32 BeginLinkedCastAttempt(Args const&...){assert(false);return 0;}
 template<class... Args>bool FinishLinkedCastAttempt(Args const&...){assert(false);return false;}
 void UpdateLinkedCastRetries(uint32){}
 void ResetLinkedCastRetries(){}

    Unit* me=nullptr;
    Unit* invoker=nullptr;
    GameObject* go=nullptr;
    uint32 recalc=0;
    Unit* GetLastInvoker(){return invoker;}
    WorldObject* GetBaseObject(){return me;}
    bool IsUnit(WorldObject* object){return object&&object->ToUnit();}
    void RecalcTimer(SmartScriptHolder&,uint32 low,uint32 high){assert(low==500&&high==500);++recalc;}
    ObjectList* GetTargets(SmartScriptHolder const& e,Unit* selected){
        Unit* trigger=selected?selected:GetLastInvoker();auto* l=new ObjectList;
        switch(e.GetTargetType()){
#include "InvokerTarget.inc"
        }
        return l;
    }
    void ProcessAction(SmartScriptHolder& e,Unit* unit){
        uint32 var0=0,var1=0;bool bvar=false;auto spell=nullptr;auto gob=nullptr;
        switch(e.type){
#include "CastAction.inc"
#include "InvokerCast.inc"
        }
    }
};
namespace ArtifactAcquisition {
#include "Active.inc"
}
struct Scenario {uint32 id=972,step=8;uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;}};
struct ScenarioManager {Scenario scenario;bool present=true;Scenario* GetScenario(uint32){return present?&scenario:nullptr;}} scenarioManager;
auto sScenarioMgr=&scenarioManager;
struct SpellScript;
struct CheckHook {
    std::function<SpellCastResult(SpellScript*)> callback;
    CheckHook& operator+=(CheckHook const& other){callback=other.callback;return *this;}
};
struct EffectHook {
    struct Entry {uint32 effect,type;std::function<void(SpellScript*,SpellEffIndex)> callback;};
    std::vector<Entry> entries;
    EffectHook& operator+=(Entry const& entry){entries.push_back(entry);return *this;}
};
struct SpellScript {
    Unit* caster=nullptr;Unit* explicitTarget=nullptr;Player* hitPlayer=nullptr;Spell* spell=nullptr;
    bool prevented[2]={false,false};
    CheckHook OnCheckCast;EffectHook OnEffectHitTarget;
    Unit* GetCaster(){return caster;}
    Unit* GetExplTargetUnit(){return explicitTarget;}
    Player* GetHitPlayer(){return hitPlayer;}
    Spell* GetSpell(){return spell;}
    void PreventHitDefaultEffect(uint32 effect){prevented[effect]=true;}
    virtual void Register(){}
    virtual bool Load(){return true;}
    void HitEffect(uint32 index){
        for(auto& handler:OnEffectHitTarget.entries)if(handler.effect==index)handler.callback(this,index);
        if(!prevented[index]){if(index==0)spell->EffectTeleportUnits(index);else spell->EffectKillCredit(index);}
    }
};
template<class Script> CheckHook MakeCheck(SpellCastResult(Script::*callback)()){
    return {[callback](SpellScript* base){return (static_cast<Script*>(base)->*callback)();}};
}
template<class Script> EffectHook::Entry MakeEffect(void(Script::*callback)(SpellEffIndex),uint32 effect,uint32 type){
    assert((effect==0&&type==252)||(effect==1&&type==90));
    return {effect,type,[callback](SpellScript* base,SpellEffIndex index){(static_cast<Script*>(base)->*callback)(index);}};
}
#define PrepareSpellScript(name) public:
#define SpellCheckCastFn(callback) MakeCheck(&callback)
#define SpellEffectFn(callback,effect,type) MakeEffect(&callback,effect,type)
#include "ReturnState.inc"
void CheckTransferLifecycle(){
    Unit vereesa;Player original;Spell baseline;baseline.unitTarget=&original;
    baseline.EffectTeleportUnits(0);baseline.EffectKillCredit(1);
    assert(!original.world&&!original.credits);
    for(uint32 stage=0;stage<=14;++stage)for(uint32 status:{uint32(QUEST_STATUS_INCOMPLETE),uint32(QUEST_STATUS_COMPLETE)}){
        Player eligible;eligible.quest=status;scenarioManager.scenario.step=stage;
        assert(MarksmanshipReturn::Current(&eligible,&vereesa)==(stage==8));
    }
    scenarioManager={};
    for(bool inheritedDelay:{false,true})for(uint32 status:{uint32(QUEST_STATUS_INCOMPLETE),uint32(QUEST_STATUS_COMPLETE)}){
        Player owner;owner.canDelay=inheritedDelay;owner.quest=status;Spell travel;travel.unitTarget=&owner;
        spell_marksmanship_return hook;hook.caster=&vereesa;hook.explicitTarget=&owner;hook.hitPlayer=&owner;hook.spell=&travel;hook.Register();assert(hook.Load());
        assert(hook.OnCheckCast.callback(&hook)==SPELL_CAST_OK);
        hook.HitEffect(0);hook.HitEffect(1);
        assert(owner.world&&owner.pendingTeleport&&owner.teleporting&&owner.credits==1&&travel.teleports==1);
        assert(owner.canDelay==inheritedDelay&&hook.prevented[0]&&!hook.prevented[1]);
    }
    for(uint32 invalid=0;invalid<20;++invalid){
        scenarioManager={};vereesa.world=true;vereesa.living=true;vereesa.map=1489;vereesa.entry=100397;vereesa.scriptId=0;Player owner;Spell travel;travel.unitTarget=&owner;
        Unit pet;
        spell_marksmanship_return hook;hook.caster=&vereesa;hook.explicitTarget=&owner;hook.hitPlayer=&owner;hook.spell=&travel;hook.Register();assert(hook.Load());
        assert(hook.OnCheckCast.callback(&hook)==SPELL_CAST_OK);
        switch(invalid){case 0:owner.owned=false;break;case 1:owner.quest=0;break;case 2:owner.rewarded=true;break;case 3:owner.instance=99;break;case 4:owner.phase=false;break;case 5:owner.alive=false;break;case 6:owner.classId=1;break;case 7:owner.map=1220;break;case 8:scenarioManager.present=false;break;case 9:scenarioManager.scenario.step=7;break;case 10:owner.teleporting=true;break;case 11:hook.explicitTarget=nullptr;hook.hitPlayer=nullptr;break;case 12:owner.world=false;break;case 13:scenarioManager.scenario.id=973;break;case 14:vereesa.world=false;break;case 15:vereesa.living=false;break;case 16:vereesa.map=1220;break;case 17:vereesa.scriptId=1;break;case 18:vereesa.entry=123;break;case 19:hook.explicitTarget=&pet;hook.hitPlayer=nullptr;break;}
        assert(hook.OnCheckCast.callback(&hook)==SPELL_FAILED_BAD_TARGETS);
        hook.HitEffect(0);hook.HitEffect(1);
        assert(!travel.teleports&&!owner.pendingTeleport&&!owner.credits&&!owner.canDelay&&hook.prevented[0]&&hook.prevented[1]);
    }
    scenarioManager={};vereesa.world=true;vereesa.living=true;vereesa.map=1489;vereesa.entry=100397;vereesa.scriptId=0;Player owner;Spell rejected;rejected.unitTarget=&owner;rejected.rejectTeleport=true;
    spell_marksmanship_return hook;hook.caster=&vereesa;hook.explicitTarget=&owner;hook.hitPlayer=&owner;hook.spell=&rejected;hook.Register();assert(hook.Load());
    hook.HitEffect(0);hook.HitEffect(1);assert(rejected.teleports==1&&!owner.pendingTeleport&&!owner.credits&&!owner.canDelay&&hook.prevented[1]);
    for(uint32 alternate=0;alternate<3;++alternate){
        Player other;Unit custom;custom.scriptId=alternate==0?1:0;custom.entry=alternate==1?123:100397;
        spell_marksmanship_return untouched;untouched.caster=alternate==2?static_cast<Unit*>(&other):&custom;
        untouched.Register();assert(untouched.Load());untouched.Hit(0);untouched.Hit(1);
        assert(untouched.OnCheckCast.callback(&untouched)==SPELL_CAST_OK&&!untouched.prevented[0]&&!untouched.prevented[1]);
    }
}
int main(int argc,char** argv){
    _set_abort_behavior(0,_CALL_REPORTFAULT);assert(argc==2);
    SmartScriptHolder native;std::ifstream input(argv[1]);assert(input>>native.type>>native.action.cast.spell>>native.action.cast.flags>>native.targetType);
    assert(native.type==11&&native.action.cast.spell==205040&&native.action.cast.flags==0&&native.targetType==7);
    Unit vereesa;Player owner;SmartScript script;script.me=&vereesa;script.invoker=&owner;
    auto original=native;original.type=85;script.ProcessAction(original,&owner);
    assert(!vereesa.casts&&!owner.casts&&owner.delayed.size()==1);owner.Flush();
    assert(owner.casts==1&&owner.target==&owner&&!owner.triggered&&!vereesa.casts);
    script.ProcessAction(native,&owner);assert(vereesa.casts==1&&vereesa.target==&owner&&!vereesa.triggered&&owner.casts==1&&owner.delayed.empty());
    vereesa.result=SPELL_FAILED_BAD_TARGETS;script.ProcessAction(native,&owner);assert(script.recalc==1);
    vereesa.result=SPELL_CAST_OK;script.ProcessAction(native,&owner);assert(vereesa.casts==3&&vereesa.target==&owner);
    script.invoker=nullptr;script.ProcessAction(native,nullptr);assert(vereesa.casts==3);
    script.me=nullptr;script.ProcessAction(native,&owner);assert(vereesa.casts==3&&owner.casts==1);
    Spell effect;effect.unitTarget=&owner;effect.EffectKillCredit(1);assert(owner.credits==1);
    effect.unitTarget=&vereesa;effect.EffectKillCredit(1);assert(owner.credits==1);
    effect.unitTarget=nullptr;effect.EffectKillCredit(1);
    Player nearby,remote,offline,ghost;remote.near=false;offline.world=false;ghost.alive=false;ghost.corpse=true;
    GroupReference fifth{&ghost},fourth{&offline,&fifth},third{&remote,&fourth},second{&nearby,&third},first{&owner,&second};
    Group group{&first};owner.currentGroup=&group;effect.unitTarget=&owner;effect.EffectKillCredit(1);
    assert(owner.credits==2&&nearby.credits==1&&!remote.credits&&!offline.credits&&!ghost.credits);
    owner.talked=true;effect.EffectKillCredit(1);assert(owner.credits==2&&nearby.credits==1);
    owner.talked=false;owner.world=false;effect.EffectKillCredit(1);assert(owner.credits==2&&nearby.credits==1);
    CheckTransferLifecycle();
    std::cout<<"PASS: actual scoped hooks/native far-teleport deferral preserve effect90 before map removal, restore inherited delay state, reject changed context/rejected transfers and leave other casters untouched\n";
    std::cout<<"PASS: actual SmartAI invoker target/cast bodies reproduce old player self-cast; repaired native205040 is cast normally by Vereesa on the player, with failure retry and missing actor/invoker safety\n";
    std::cout<<"PASS: actual effect90/group-credit bodies retain target-player and native group eligibility, not an invented personal-only award; actual far-map transfer, scene and client renderer not exercised\n";
}
