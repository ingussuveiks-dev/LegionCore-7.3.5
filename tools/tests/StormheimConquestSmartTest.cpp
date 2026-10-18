#include <cassert>
#include <cstdint>
#include <iostream>
#include <list>
#include <vector>
using uint32=uint32_t;using SpellCastResult=int;
constexpr int SMART_ACTION_CAST=11,SMART_TARGET_POSITION=8,SPELL_CAST_OK=0,SPELL_FAILED_BAD_TARGETS=1,SPELL_FAILED_SPELL_IN_PROGRESS=2;
#define TC_LOG_DEBUG(...) ((void)0)
#define ENSURE_AI(type,ai) (ai)
#include "ConquestCastFlags.inc"
struct AI{void SetCombatMove(bool){}};
struct Unit{bool works=true;AI ai;Unit* ToUnit(){return this;}bool IsUnit(){return true;}bool HasAura(uint32){return false;}AI* AI(){return &ai;}
 void InterruptNonMeleeSpells(bool){}void GetPosition(float& x,float& y,float& z){x=y=z=0;}
 int CastSpell(Unit*,uint32 id,bool trigger){assert(id==229225&&trigger);return works?SPELL_CAST_OK:SPELL_FAILED_BAD_TARGETS;}
 int CastSpell(float,float,float,uint32,bool){return SPELL_FAILED_BAD_TARGETS;}
};
using ObjectList=std::list<Unit*>;
struct Holder{int type=11,entryOrGuid=11525400,event_id=1;bool enableTimed=true;
 struct{struct{uint32 spell=229225,flags=2;}cast;}action;struct{float x=0,y=0,z=0;}target;int GetTargetType(){return 1;}};
struct GO{int CastSpell(Unit*,uint32){return SPELL_FAILED_BAD_TARGETS;}};
struct SmartScript{Unit* me;GO* go=nullptr;int recalcs=0,tail=0;std::vector<Holder> mTimedActionList;
 bool IsUnit(Unit* p){return p!=nullptr;}ObjectList* GetTargets(Holder&,Unit*){return new ObjectList{me};}
 void RecalcTimer(Holder&,int a,int b){assert(a==500&&b==500);++recalcs;}
 void Action(Holder& e){Unit* unit=me;switch(e.type){
#include "ConquestCast.inc"
 }++tail;}
 void Timed(Holder& e){Action(e);
#include "ConquestAdvance.inc"
 }
};
int main(){Unit actor;actor.works=false;SmartScript script{&actor};Holder visual,summon;summon.event_id=2;summon.enableTimed=false;
 script.Action(visual);assert(script.recalcs==1&&!script.tail); // failed cast skipped old LINK tail
 script.mTimedActionList={visual,summon};script.Timed(script.mTimedActionList[0]);assert(script.mTimedActionList[1].enableTimed&&!script.mTimedActionList[0].enableTimed);
 // The separate next row now executes summon/cleanup even after cast failure.
 script.mTimedActionList[1].type=12;script.Action(script.mTimedActionList[1]);assert(script.tail==1);
 std::cout<<"PASS: native SMART_ACTION_CAST failure skips LINK continuation; extracted native timed-list advance still enables independent summon/cleanup. Actual delays and world summons remain integration boundaries.\n";
}
