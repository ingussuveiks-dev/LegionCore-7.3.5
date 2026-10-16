#include <cassert>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <list>
#include <map>
#include <set>
using uint32=uint32_t;
constexpr int SMART_ACTION_SUMMON_CREATURE=12,SMART_ACTION_REMOVEAURASFROMSPELL=28,SMART_TARGET_POSITION=8,SMART_EVENT_LINK=61,CONDITION_NEAR_CREATURE=29;
enum TempSummonType {TEMPSUMMON_MANUAL_DESPAWN=8};
#define TC_LOG_DEBUG(...) ((void)0)
#define TC_LOG_ERROR(...) assert(false)
struct Aura{void ModStackAmount(int){}};
struct Unit;struct SummonAI{void AttackStart(Unit*){}};
struct Unit {uint32 entry=106698;float x=0,y=0,z=0;int map=1,phase=1;bool alive=true;std::set<uint32> auras;Aura aura;
 SummonAI ai;SummonAI* AI(){return &ai;}
 Unit* ToUnit(){return this;}void RemoveAllAuras(){auras.clear();}Aura* GetAura(uint32 id){return auras.count(id)?&aura:nullptr;}
 void RemoveAurasDueToSpell(uint32 id){auras.erase(id);}void GetPosition(float& px,float& py){px=x;py=y;}float GetPositionZ(){return z;}
};
using Creature=Unit;using ObjectList=std::list<Unit*>;
std::list<Unit*> actors;
Creature* GetClosestCreatureWithEntry(Unit* actor,uint32 entry,float range){for(auto p:actors)if(p->alive&&p->entry==entry&&p->map==actor->map&&p->phase==actor->phase&&std::abs(p->x-actor->x)<=range)return p;return nullptr;}
bool HasLivingDoomweaver(Unit* object){uint32 ConditionValue1=106698,ConditionValue2=50;bool condMeets=false;switch(CONDITION_NEAR_CREATURE){
#include "NearCondition.inc"
 }return condMeets;}
struct SmartScriptHolder {
 uint32 event_id=3,link=5,type=12,event=6,entryOrGuid=106698;
 struct {struct {uint32 creature=106692,type=8,duration=0,attackInvoker=0;}summonCreature;struct{uint32 spell=211683;int stack=0;}removeAura;}action;
 struct {float x=1021.39f,y=5137.38f,z=58.2669f,o=6.0872f;}target;
 int GetTargetType(){return type==12?SMART_TARGET_POSITION:19;}uint32 GetActionType(){return type;}uint32 GetEventType(){return event;}uint32 GetScriptType(){return 0;}
};
struct Base {
 bool spawnWorks=true;int attempts=0,spawns=0;Creature summon;
 Creature* SummonCreature(uint32 entry,float x,float y,float z,float o,TempSummonType type,uint32){assert(entry==106692&&x==1021.39f&&y==5137.38f&&z==58.2669f&&o==6.0872f&&type==8);++attempts;if(!spawnWorks)return nullptr;++spawns;return &summon;}
};
struct SmartScript {
 Unit* me;Base base;SmartScriptHolder removal;
 uint32 var0=0,var1=0;bool bvar=false;void* spell=nullptr;void* gob=nullptr;
 SmartScript(Unit* actor):me(actor){removal.event_id=5;removal.link=0;removal.type=28;removal.event=61;}
 Base* GetBaseObject(){return &base;}bool IsUnit(Unit* p){return p!=nullptr;}
 ObjectList* GetTargets(SmartScriptHolder&,Unit*){auto l=new ObjectList;if(auto skull=GetClosestCreatureWithEntry(me,106693,50))l->push_back(skull);return l;}
 SmartScriptHolder FindLinkedEvent(uint32 id){assert(id==5);return removal;}
 void ProcessEvent(SmartScriptHolder& e,Unit* unit,uint32,uint32,bool,void*,void*){ProcessAction(e,unit);}
 void ProcessAction(SmartScriptHolder& e,Unit* unit){switch(e.type){case SMART_ACTION_SUMMON_CREATURE:{
#include "PositionSummon.inc"
 break;}
#include "RemoveAura.inc"
 }
#include "LinkTail.inc"
};
int main(){
 for(auto order: {std::array<int,3>{0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0}})for(bool spawnWorks:{true,false}){
  Unit skull;skull.entry=106693;skull.auras={211683,12345};Unit finaleSkull;finaleSkull.entry=106693;finaleSkull.x=200;finaleSkull.auras={211683};
  Unit doom[3];doom[1].x=10;doom[2].x=20;actors={&doom[0],&doom[1],&doom[2],&skull,&finaleSkull};
  for(int death=0;death<3;++death){auto& actor=doom[order[death]];actor.alive=false;bool last=!HasLivingDoomweaver(&actor);
   assert(last==(death==2));SmartScript script{&actor};script.base.spawnWorks=spawnWorks;
   if(last){SmartScriptHolder e;script.ProcessAction(e,nullptr);assert(script.base.attempts==1&&script.base.spawns==int(spawnWorks));}
   assert(skull.auras.count(211683)==(death<2));assert(skull.alive&&skull.auras.count(12345));assert(finaleSkull.auras.count(211683));
  }
 }
 Unit skull;skull.entry=106693;skull.auras={211683,12345};Unit corpse;corpse.alive=false;actors={&skull,&corpse};SmartScript script{&corpse};
 auto removal=script.removal;for(int i=0;i<10000;++i)script.ProcessAction(removal,nullptr);assert(skull.alive&&skull.auras==std::set<uint32>{12345});
 // A distant pet must not define the last-caster condition's origin.
 Unit living,pet;pet.entry=416;pet.x=500;actors={&living,&pet};assert(!HasLivingDoomweaver(&pet)&&HasLivingDoomweaver(&corpse));
 std::cout<<"PASS: native last-caster condition, position summon, LINK and aura removal; six death orders, summon failure, null killer, 10000 cleanup repeats and skull/other aura preservation\n";
}
