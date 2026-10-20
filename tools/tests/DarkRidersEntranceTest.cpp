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
#define TC_LOG_DEBUG(...) ((void)0)
struct Unit;struct Creature;struct Player;
using WorldObject=Unit;
struct ObjectGuid{Unit* value=nullptr;inline static constexpr std::nullptr_t Empty=nullptr;ObjectGuid(Unit* unit=nullptr):value(unit){}uint32 GetCounter(){return 0;}};
struct Unit{
 bool player=false;uint32 entry=1860,casts=0,spell=0;float distance=0;Unit* actualTarget=nullptr;std::vector<std::function<void()>> pending;
 virtual ~Unit()=default;virtual Creature* ToCreature(){return nullptr;}virtual Player* ToPlayer(){return nullptr;}Unit* ToUnit(){return this;}
 ObjectGuid GetGUID(){return this;}uint32 GetGUIDLow(){return 0;}uint32 GetEntry(){return entry;}char const* GetName(){return "fixture";}
 bool isType(uint32 mask){return (player?16u:8u)&mask;}bool HasAura(uint32){return false;}void InterruptNonMeleeSpells(bool){}
 bool IsUnit(){return true;}Unit* ToGameObject(){return nullptr;}Unit* ToCorpse(){return nullptr;}
 void AddDelayedEvent(uint32 delay,std::function<void()> task){assert(delay==200);pending.push_back(task);}
 void Flush(){auto tasks=std::move(pending);for(auto& f:tasks)f();}
 void CastSpell(Unit* explicitTarget,uint32 id,bool){
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
struct Player:Unit{Player(){player=true;entry=0;}Player* ToPlayer()override{return this;}};
struct Creature:Unit{bool combat=false,los=true;Creature(){entry=101282;}Creature* ToCreature()override{return this;}
 bool isInCombat(){return combat;}bool IsWithinDistInMap(Unit* unit,float range){return unit&&unit->distance<=range;}
 bool IsWithinLOSInMap(Unit*){return los;}bool IsHostileTo(Unit*){return false;}
};
struct ObjectAccessor{static Unit* GetUnit(Unit&,ObjectGuid guid){return guid.value;}};
struct SpellInfo{};struct Spells{SpellInfo info;SpellInfo* GetSpellInfo(uint32){return &info;}}spells;auto sSpellMgr=&spells;
Unit* GetScriptCreditCaster(Unit* invoker,SpellInfo const*){return invoker;} // Ordinary companion spells retain their caster.
using ObjectList=std::list<Unit*>;
struct SmartScriptHolder{
 uint32 type=1,targetType=1;
 struct{struct{uint32 textGroupID=0,duration=5000;}talk;
 struct{uint32 spell=199626,flags=0,targetType=18,targetParam1=100,targetParam2=0,targetParam3=0;}cast;}action;
 struct{struct{uint32 noHostile=1,maxDist=20,cooldownMin=0,cooldownMax=0;}los;struct{uint32 textGroupID=0,creatureEntry=101282;}textOver;}event;
};
struct Texts{uint32 speaker=0;void SendChat(Creature* actor,uint8,ObjectGuid){speaker=actor->entry;}}texts;auto sCreatureTextMgr=&texts;
struct SmartScript{
 Creature* me;Unit* invoker=nullptr;std::vector<Unit*> players;uint32 mTalkerEntry=0,mLastTextID=0,mTextTimer=0;bool mUseTextTimer=false,consumed=false,requirePlayer=true;
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
 }}
 bool PlayerCondition(Unit* object){bool condMeets=false;uint32 ConditionValue1=16;switch(CONDITION_TYPE_MASK){
#include "EntrancePlayerCondition.inc"
 }return condMeets;}
 void Los(Unit* unit){if(consumed||(requirePlayer&&!PlayerCondition(unit)))return;switch(SMART_EVENT_OOC_LOS){
#include "EntranceLos.inc"
 }if(mUseTextTimer)consumed=true;}
 void ProcessEventsFor(uint32,Unit*,uint32 var0,uint32 var1){auto e=SmartScriptHolder{};e.type=85;e.targetType=7;Unit* unit=nullptr;switch(SMART_EVENT_TEXT_OVER){
#include "EntranceTextOver.inc"
 }}
 void Tick(uint32 diff){
#include "EntranceTimer.inc"
 }
};
int main(){
 Player owner,second;Creature revil,pet;pet.entry=1860;SmartScript script{&revil};
 // Original event: a pet consumes LOS, TALK chooses the pet, and TEXT_OVER
 // cannot match101282. The player can arrive later without recovering it.
 script.requirePlayer=false;script.e.targetType=7;script.e.event.los.maxDist=2;pet.distance=1;
 script.Los(&pet);assert(script.consumed&&script.mTalkerEntry==1860);script.Tick(5001);pet.Flush();assert(!pet.casts&&!owner.casts);
 script.Los(&owner);assert(!owner.casts);
 // Scoped repaired spawn: pet-first ignored, normal entrance distance17
 // works, full native five-second line +200ms invoker-cast delay retained.
 SmartScript fixed{&revil};fixed.Los(&pet);assert(!fixed.consumed);owner.distance=17;fixed.Los(&owner);
 assert(fixed.consumed&&fixed.mTalkerEntry==101282);fixed.Tick(5000);owner.Flush();assert(!owner.casts);
 fixed.Tick(1);assert(!owner.casts);owner.Flush();assert(owner.casts==1&&owner.spell==199626&&owner.actualTarget==&owner&&!pet.casts);
 fixed.Los(&owner);fixed.Tick(6000);owner.Flush();assert(owner.casts==1);
 // Actual cross-cast action, two eligible Players, one base-actor target:
 // every player casts once, native CASTER target resolves to that player.
 for(Unit* killer:{static_cast<Unit*>(&owner),static_cast<Unit*>(&pet),static_cast<Unit*>(nullptr)})for(uint32 id:{199626u,199627u}){
  owner.casts=second.casts=pet.casts=revil.casts=0;owner.distance=40;second.distance=5;
  SmartScript cross{&revil};cross.players={&owner,&second};SmartScriptHolder row;row.type=86;row.action.cast.spell=id;row.action.cast.flags=2;
  cross.ProcessAction(row,killer);assert(owner.casts==1&&second.casts==1&&!pet.casts&&!revil.casts);
  assert(owner.actualTarget==&owner&&second.actualTarget==&second&&owner.spell==id);
 }
 std::cout<<"PASS: native pet-first LOS/TALK/TextOver failure reproduced; Player type condition, entrance17-yard LOS, speech/timer/invoker cast and native self-targeted companion cross-casts (player/pet/null, two players)\n";
}
