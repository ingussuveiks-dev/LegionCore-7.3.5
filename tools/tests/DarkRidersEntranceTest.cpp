#include <array>
#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <list>
#include <set>
#include <vector>
using uint32=uint32_t;using uint8=uint8_t;using SMARTAI_TARGETS=uint32;
constexpr uint32 TARGET_FLAG_UNIT_MASK=1,TARGET_FLAG_CORPSE_MASK=2,TARGET_FLAG_GAMEOBJECT_MASK=4;
constexpr uint32 SMART_ACTION_TALK=1,SMART_ACTION_INVOKER_CAST=85,SMART_ACTION_CROSS_CAST=86,SMART_ACTION_NONE=0,SMART_EVENT_UPDATE_IC=0;
constexpr uint32 SMART_TARGET_SELF=1,SMART_TARGET_ACTION_INVOKER=7,SMART_TARGET_PLAYER_DISTANCE=18,SMART_EVENT_OOC_LOS=10,SMART_EVENT_TEXT_OVER=52,TARGET_UNIT_CASTER=1,CONDITION_TYPE_MASK=32;
constexpr uint32 SMARTCAST_INTERRUPT_PREVIOUS=1,SMARTCAST_TRIGGERED=2,SMARTCAST_AURA_NOT_PRESENT=32;
constexpr uint32 SMARTCAST_INVOKER_PLAYER=128,SMART_TARGET_ACTION_INVOKER_PLAYER=130;
constexpr uint32 SMART_ACTION_SET_INST_DATA=34,TRIGGERED_IGNORE_CAST_IN_PROGRESS=0x00000020,SPELL_FAILED_SPELL_IN_PROGRESS=123;
constexpr uint32 CURRENT_GENERIC_SPELL=1,CURRENT_CHANNELED_SPELL=2,CURRENT_AUTOREPEAT_SPELL=3,SPELL_STATE_FINISHED=3,SPELL_STATE_DELAYED=5,SPELL_ATTR2_NOT_RESET_AUTO_ACTIONS=2,SPELL_ATTR10_CAN_PARRY_ON_CAST=10;
struct ActiveInfo{bool HasAttribute(uint32){return false;}};
struct ActiveSpell{ActiveInfo info;ActiveInfo* m_spellInfo=&info;uint32 GetCastTime(){return 1500;}uint32 getState(){return 1;}};
#define TC_LOG_DEBUG(...) ((void)0)
#define TC_LOG_ERROR(...) assert(false)
struct InstanceScript{uint32 requests=0;void SetData(uint32 field,uint32 value){assert(field==10085002&&value==1);++requests;}};
struct Unit;struct Creature;struct Player;
bool NativePrepare(Unit*,bool);
using WorldObject=Unit;
struct ObjectGuid{Unit* value=nullptr;inline static constexpr std::nullptr_t Empty=nullptr;ObjectGuid(Unit* unit=nullptr):value(unit){}uint32 GetCounter(){return 0;}};
struct Unit{
 bool player=false;uint32 entry=1860,casts=0,spell=0;float distance=0;Unit* actualTarget=nullptr;std::vector<std::function<void()>> pending;
 bool casting=false;InstanceScript* instance=nullptr;InstanceScript* GetInstanceScript(){return instance;}
 ActiveSpell* m_currentSpells[4]{};bool IsNonMeleeSpellCast(bool,bool=false,bool=false,bool=false,bool=true)const;
 virtual ~Unit()=default;virtual Creature* ToCreature(){return nullptr;}virtual Player* ToPlayer(){return nullptr;}Unit* ToUnit(){return this;}
 ObjectGuid GetGUID(){return this;}uint32 GetGUIDLow(){return 0;}uint32 GetEntry(){return entry;}char const* GetName(){return "fixture";}
 bool isType(uint32 mask){return (player?16u:8u)&mask;}bool HasAura(uint32){return false;}void InterruptNonMeleeSpells(bool){}
 bool IsUnit(){return true;}Unit* ToGameObject(){return nullptr;}Unit* ToCorpse(){return nullptr;}
 bool IsInWorld(){return true;}int GetMap(){return 1;}bool InSamePhase(Unit*){return true;}bool IsWithinDistInMap(Unit*,float){return true;}
 bool HasQuestObjectScope(){return false;}bool CanTargetQuestObject(Unit*){return true;}
 void AddDelayedEvent(uint32 delay,std::function<void()> task){assert(delay==200);pending.push_back(task);}
 void Flush(){auto tasks=std::move(pending);for(auto& f:tasks)f();}
 void CastSpell(Unit* explicitTarget,uint32 id,bool triggered){
  if(!NativePrepare(this,triggered))return;
  struct Targets{Unit* object;WorldObject* GetObjectTarget(){return object;}void RemoveObjectTarget(){object=nullptr;}}m_targets{explicitTarget};
  uint32 neededTargets=0;
#include "EntranceExplicitTarget.inc"
  assert(!m_targets.object); // Even a dead/hostile base actor is redundant.
  Unit* m_caster=this;Unit* target=m_targets.object;bool checkIfValid=true;
  // These two companion spells have CASTER target1 on every native effect.
  switch(TARGET_UNIT_CASTER){
#include "EntranceCasterTarget.inc"
  }(void)checkIfValid;++casts;spell=id;actualTarget=target;
 }
};
#include "EntranceCurrentSpell.inc"
struct SpellAdmission{Unit* m_caster;uint32 _triggeredCastFlags;bool accepted=false;
 void SendCastResult(uint32 result){assert(result==123);}void finish(bool success){assert(!success);}
 void Prepare(){
#include "EntranceCastAdmission.inc"
 accepted=true;}
};
bool NativePrepare(Unit* caster,bool triggered){SpellAdmission spell{caster,triggered?TRIGGERED_IGNORE_CAST_IN_PROGRESS:0};spell.Prepare();return spell.accepted;}
struct Player:Unit{Player(){player=true;entry=0;}Player* ToPlayer()override{return this;}};
struct Creature:Unit{bool combat=false,los=true;Creature(){entry=101282;}Creature* ToCreature()override{return this;}
 bool isInCombat(){return combat;}bool IsWithinDistInMap(Unit* unit,float range){return unit&&unit->distance<=range;}
 bool IsWithinLOSInMap(Unit*){return los;}bool IsHostileTo(Unit*){return false;}
};
struct ObjectAccessor{static Unit* GetUnit(Unit&,ObjectGuid guid){return guid.value;}static Unit* GetWorldObject(Unit&,ObjectGuid guid){return guid.value;}};
struct SpellInfo{};struct Spells{SpellInfo info;SpellInfo* GetSpellInfo(uint32){return &info;}}spells;auto sSpellMgr=&spells;
Unit* GetScriptCreditCaster(Unit* invoker,SpellInfo const*){return invoker;} // Ordinary companion spells retain their caster.
Player* GetScriptCreditPlayer(Unit* invoker){return invoker?invoker->ToPlayer():nullptr;}
using ObjectList=std::list<Unit*>;
struct SmartScriptHolder{
 uint32 type=1,targetType=1;
 uint32 GetTargetType()const{return targetType;}struct{struct{uint32 dist=0;}playerDistance;}target;
 int entryOrGuid=-369378;uint32 GetEventType(){return 52;}
 struct{struct{uint32 textGroupID=0,duration=5000;}talk;
 struct{uint32 field=10085002,data=1;}setInstanceData;
 struct{uint32 spell=199626,flags=0,targetType=18,targetParam1=100,targetParam2=0,targetParam3=0;}cast;}action;
 struct{struct{uint32 noHostile=1,maxDist=2,cooldownMin=0,cooldownMax=0;}los;struct{uint32 textGroupID=0,creatureEntry=101282;}textOver;}event;
};
struct Texts{uint32 speaker=0;void SendChat(Creature* actor,uint8,ObjectGuid){speaker=actor->entry;}}texts;auto sCreatureTextMgr=&texts;
struct SmartScript{
 Creature* me;Unit* invoker=nullptr;std::vector<Unit*> players;uint32 mTalkerEntry=0,mLastTextID=0,mTextTimer=0;bool mUseTextTimer=false,consumed=false,requirePlayer=true;
 bool requestInstance=true;
 ObjectGuid mTextGUID;SmartScriptHolder e;
 bool IsUnit(Unit* u){return u;}bool IsPlayer(Unit* u){return u&&u->ToPlayer();}bool IsCreature(Unit* u){return u&&u->ToCreature();}
 Unit* GetLastInvoker(){return invoker;}Unit* GetBaseObject(){return me;}
 ObjectList* GetTargets(SmartScriptHolder const& row,Unit*){
  auto list=new ObjectList();if(row.targetType==1)list->push_back(me);if(row.targetType==7&&invoker)list->push_back(invoker);
  if(row.targetType==18)for(auto p:players)if(p->distance<=row.action.cast.targetParam1)list->push_back(p);return list;
 }
 template<class... Args>SmartScriptHolder CreateEvent(Args... args){std::array<uint32,sizeof...(args)> values{uint32(args)...};SmartScriptHolder row;row.targetType=values[14];row.action.cast.targetParam1=values[15];return row;}
 void RecalcTimer(SmartScriptHolder&,uint32,uint32){}
 void ProcessAction(SmartScriptHolder& row,Unit* unit=nullptr,uint32=0){if(unit)invoker=unit;auto& e=row;switch(e.type){
#include "EntranceTalk.inc"
#include "EntranceInvokerCast.inc"
#include "EntranceCrossCast.inc"
#include "RevilInstanceAction.inc"
 }}
 bool PlayerCondition(Unit* object){bool condMeets=false;uint32 ConditionValue1=16;switch(CONDITION_TYPE_MASK){
#include "EntrancePlayerCondition.inc"
 }return condMeets;}
 void Los(Unit* unit){if(consumed||(requirePlayer&&!PlayerCondition(unit)))return;switch(SMART_EVENT_OOC_LOS){
#include "EntranceLos.inc"
 }if(mUseTextTimer)consumed=true;}
 void ProcessEventsFor(uint32,Unit*,uint32 var0,uint32 var1){auto e=SmartScriptHolder{};e.type=requestInstance?34:85;e.targetType=requestInstance?1:7;Unit* unit=nullptr;switch(SMART_EVENT_TEXT_OVER){
#include "EntranceTextOver.inc"
 }}
 void Tick(uint32 diff){
#include "EntranceTimer.inc"
 }
};
int main(){
 Player owner,second;Creature revil,pet;InstanceScript instance;revil.instance=&instance;pet.entry=1860;SmartScript script{&revil};script.requestInstance=false;
 // Original event: a pet consumes LOS, TALK chooses the pet, and TEXT_OVER
 // cannot match101282. The player can arrive later without recovering it.
 script.requirePlayer=false;script.e.targetType=7;script.e.event.los.maxDist=2;pet.distance=1;
 script.Los(&pet);assert(script.consumed&&script.mTalkerEntry==1860);script.Tick(5001);pet.Flush();assert(!pet.casts&&!owner.casts);
 script.Los(&owner);assert(!owner.casts);
 // The20-yard recovery fired prematurely at the recorded entrance. Keep
 // the native stair-local2-yard introduction, Player gate and SELF speaker.
 SmartScript fixed{&revil};fixed.Los(&pet);assert(!fixed.consumed);owner.distance=17;fixed.Los(&owner);
 assert(!fixed.consumed);owner.distance=1;fixed.Los(&owner);
 assert(fixed.consumed&&fixed.mTalkerEntry==101282);fixed.Tick(5000);owner.Flush();assert(!instance.requests);
 fixed.invoker=nullptr;ActiveSpell active;owner.casting=true;owner.m_currentSpells[CURRENT_GENERIC_SPELL]=&active;fixed.Tick(1);assert(instance.requests==1&&!owner.casts&&!pet.casts);
 fixed.Los(&owner);fixed.Tick(6000);assert(instance.requests==1);
 // Reproduce the previously missed normal-cast failure using the actual
 // Spell::prepare casting gate. Triggered bootstrap does not interrupt it.
 SmartScript old{&revil};old.requestInstance=false;old.Los(&owner);old.Tick(5001);owner.Flush();assert(!owner.casts&&owner.casting);
 SmartScript lost{&revil};lost.requestInstance=false;lost.Los(&owner);lost.invoker=nullptr;lost.Tick(5001);owner.Flush();assert(!owner.casts&&instance.requests==1);
 assert(NativePrepare(&owner,true)&&owner.casting&&owner.m_currentSpells[CURRENT_GENERIC_SPELL]==&active);
 // Native prepare skips channels at this particular gate; do not confuse
 // them with the non-instant generic cast that reproduces the rejection.
 owner.m_currentSpells[CURRENT_GENERIC_SPELL]=nullptr;owner.m_currentSpells[CURRENT_CHANNELED_SPELL]=&active;assert(NativePrepare(&owner,false));
 owner.m_currentSpells[CURRENT_CHANNELED_SPELL]=nullptr;owner.casting=false;
 // Actual cross-cast action, two eligible Players, one base-actor target:
 // every player casts once, native CASTER target resolves to that player.
 for(Unit* killer:{static_cast<Unit*>(&owner),static_cast<Unit*>(&pet),static_cast<Unit*>(nullptr)})for(uint32 id:{199626u,199627u}){
  owner.casts=second.casts=pet.casts=revil.casts=0;owner.distance=40;second.distance=5;
  SmartScript cross{&revil};cross.players={&owner,&second};SmartScriptHolder row;row.type=86;row.action.cast.spell=id;row.action.cast.flags=2;
  cross.ProcessAction(row,killer);assert(owner.casts==1&&second.casts==1&&!pet.casts&&!revil.casts);
  assert(owner.actualTarget==&owner&&second.actualTarget==&second&&owner.spell==id);
 }
 std::cout<<"PASS: native pet-first LOS/TALK/TextOver and cast-in-progress failures; stair-local full speech, null-invoker instance request and triggered admission preserve the player's cast; native companion cross-casts retained\n";
}
