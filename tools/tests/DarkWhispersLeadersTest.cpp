#include <cassert>
#include <cstdint>
#include <cmath>
#include <iostream>
#include <list>
using uint32=uint32_t;using CriteriaTypes=uint32;
constexpr int SMART_ACTION_CAST=11,SMART_ACTION_UPDATE_ACHIEVEMENT_CRITERIA=205;
constexpr int SMART_TARGET_ACTION_INVOKER=7,SMART_TARGET_PLAYER_DISTANCE=18,SMART_TARGET_POSITION=8,CONDITION_NEAR_CREATURE=29;
constexpr int SPELL_CAST_OK=0,SPELL_FAILED_BAD_TARGETS=1,SPELL_FAILED_SPELL_IN_PROGRESS=2;
using SpellCastResult=int;
#define TC_LOG_DEBUG(...) ((void)0)
#define ENSURE_AI(type,ai) (ai)
#include "CastFlags.inc"
struct Player;struct Unit;
struct AI{void SetCombatMove(bool){}};
struct Unit {
 virtual ~Unit()=default;virtual Player* ToPlayer(){return nullptr;}Unit* ToUnit(){return this;}
 int entry=0,map=1498,phase=6434;float x=0;bool alive=true,castWorks=true;int casts=0;Unit* castTarget=nullptr;AI ai;
 uint32 GetEntry(){return entry;}bool HasAura(uint32){return false;}AI* AI(){return &ai;}
 bool IsUnit(){return true;}void GetPosition(float& px,float& py,float& pz){px=x;py=pz=0;}
 void InterruptNonMeleeSpells(bool){}int CastSpell(Unit* p,uint32 spell,bool trigger){assert(spell==212550&&trigger);++casts;castTarget=p;return castWorks?SPELL_CAST_OK:SPELL_FAILED_BAD_TARGETS;}
 int CastSpell(float,float,float,uint32,bool){return SPELL_FAILED_BAD_TARGETS;}
};
struct Player:Unit{int credits=0;Player* ToPlayer()override{return this;}
 void UpdateAchievementCriteria(CriteriaTypes type,uint32 asset,uint32,uint32,Unit*,bool){assert(type==92&&asset==50959);++credits;}};
using ObjectList=std::list<Unit*>;
ObjectList world;
Unit* GetClosestCreatureWithEntry(Unit* object,uint32 entry,float range){for(auto u:world)if(u->entry==int(entry)&&u->alive&&u->map==object->map&&std::abs(u->x-object->x)<=range)return u;return nullptr;}
bool NearOther(Unit* object,uint32 ConditionValue1,uint32 ConditionValue2){bool condMeets=false;switch(CONDITION_NEAR_CREATURE){
#include "NearCondition.inc"
 }return condMeets;}
struct SmartScriptHolder{
 int actionType=205,targetType=18;struct {struct {uint32 type=92,misc1=50959,misc2=0,misc3=0;}achievementCriteria;
 struct {uint32 spell=212550,flags=18;}cast;}action;
 struct {float x=0,y=0,z=0;struct{uint32 dist=100,spell=0;}playerDistance;}target;
 int GetTargetType(){return targetType;}
};
struct FakeGO{int CastSpell(Unit*,uint32){return SPELL_FAILED_BAD_TARGETS;}};
struct SmartScript{
 Unit* me;FakeGO* go=nullptr;bool IsPlayer(Unit* p){return p&&p->ToPlayer();}bool IsUnit(Unit* p){return p!=nullptr;}
 Unit* GetBaseObject(){return me;}
 ObjectList* GetWorldObjectsInDist(float dist){auto l=new ObjectList;for(auto p:world)if(p->map==me->map&&p->phase==me->phase&&std::abs(p->x-me->x)<=dist)l->push_back(p);return l;}
 ObjectList* GetTargets(SmartScriptHolder& e,Unit* trigger){auto l=new ObjectList;switch(e.targetType){
#include "PlayerDistance.inc"
#include "InvokerTarget.inc"
 }return l;}
 void RecalcTimer(SmartScriptHolder&,int,int){}
 void Run(SmartScriptHolder& e,Unit* unit){switch(e.actionType){
#include "CriteriaAction.inc"
#include "CastAction.inc"
 }}
};
int main(){
 Unit pet;pet.entry=416;Player owner;owner.x=20;
 Unit leaders[2];leaders[0].entry=106825;leaders[1].entry=106827;leaders[1].x=12;
 SmartScriptHolder credit,conversation;conversation.actionType=11;
 for(int order:{0,1})for(Unit* killer:{static_cast<Unit*>(&owner),&pet,static_cast<Unit*>(nullptr)}){
  owner.credits=owner.casts=0;leaders[0].alive=leaders[1].alive=true;world={&owner,&pet,&leaders[0],&leaders[1]};
  auto& first=leaders[order];auto& last=leaders[1-order];first.alive=false;
  assert(NearOther(&first,last.entry,50)); // negative condition must block first death
  last.alive=false;assert(!NearOther(&last,first.entry,50));
  SmartScript script{&last};script.Run(credit,killer);assert(owner.credits==1);
  script.Run(conversation,killer);assert(owner.casts==1&&owner.castTarget==&owner);
 }
 // Reproduce the old pet target dropping player credit through native action205.
 SmartScript old{&leaders[0]};world={&owner,&pet};owner.credits=0;credit.targetType=7;old.Run(credit,&pet);assert(!owner.credits);
 credit.targetType=18;old.Run(credit,&pet);assert(owner.credits==1);
 // Optional conversation failure cannot undo or block the earlier essential credit.
 owner.castWorks=false;old.Run(conversation,&pet);assert(owner.credits==1);
 // Corpse origin must gate the last leader, regardless of ranged killer location.
 leaders[1].alive=true;world.push_back(&leaders[1]);pet.x=500;
 assert(!NearOther(&pet,106827,50)&&NearOther(&leaders[0],106827,50));
 Player outsider;outsider.map=1;world.push_back(&outsider);old.Run(credit,&pet);assert(!outsider.credits);
 int previous=owner.credits;owner.x=500;old.Run(credit,&pet);assert(owner.credits==previous);
 std::cout<<"PASS: native player-distance selection, criterion205 and forced self cast; both leader orders, player/pet/null killers, corpse proximity, missing player, foreign map and failed optional conversation\n";
}
