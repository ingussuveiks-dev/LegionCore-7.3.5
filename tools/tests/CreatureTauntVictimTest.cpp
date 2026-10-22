#include <cassert>
#include <iostream>
#include <list>
#include <set>
#include <stdexcept>
#include <vector>
#include <algorithm>
// Keep std::list storage, but fail deterministically if the extracted old code
// dereferences its end sentinel, instead of depending on allocator contents.
template<class T> struct CheckedList : std::list<T> {
 using Base=std::list<T>;using Base::Base;
 struct const_iterator {
  typename Base::const_iterator at,end;
  T operator*() const {if(at==end)throw std::out_of_range("taunt list end dereferenced");return *at;}
  const_iterator& operator++(){assert(at!=end);++at;return *this;}
  bool operator==(const_iterator const& b)const{return at==b.at;}
  bool operator!=(const_iterator const& b)const{return at!=b.at;}
 };
 const_iterator begin()const{return {Base::begin(),Base::end()};}
 const_iterator end()const{return {Base::end(),Base::end()};}
};
enum {SPELL_AURA_MOD_TAUNT,SPELL_AURA_MOD_INVISIBILITY,REACT_PASSIVE,REACT_AGGRESSIVE,UNIT_STATE_CASTING,UNIT_MASK_CONTROLABLE_GUARDIAN};
constexpr float ATTACK_DISTANCE=5;
struct Creature;struct AuraEffect;
struct Unit {
 using AuraEffectList=CheckedList<AuraEffect*>;
 bool alive=true,visible=true,valid=true,accessible=true,attackable=true,friendly=false,player=false,guardian=false,combat=false;
 bool IsAlive(){return alive;}bool isInAccessiblePlaceFor(Creature*){return accessible;}bool IsPlayer(){return player;}
 bool isInCombat(){return combat;}Unit* helper=nullptr;Unit* getAttackerForHelper(){return helper;}
 Creature* ToCreature();std::vector<int> m_Controlled;
};
struct Aura {bool permanent=false;bool IsPermanent(){return permanent;}};
struct AuraEffect {Unit* caster=nullptr;Aura base;int reads=0;Unit* GetCaster(){++reads;return caster;}Aura* GetBase(){return &base;}};
struct Threat {Unit* target=nullptr;bool empty=false;bool isThreatListEmpty(){return empty;}Unit* getHostilTarget(){return target;}};
struct AIStub {int evades=0;void EnterEvadeMode(){++evades;}};
struct Temp {Unit* owner=nullptr;Unit* GetOwner(){return owner;}};
using UnitSet=std::set<Unit*>;using ControlList=std::vector<int>;
struct Creature:Unit {
 using AuraEffectList=CheckedList<AuraEffect*>;
 AuraEffectList taunts,invisibility;Threat m_ThreatManager;UnitSet m_attackers;
 Unit* victim=nullptr;Unit* nearest=nullptr;Unit* front=nullptr;bool threat=true,passive=false,aggressive=false,summon=false,vehicle=false,casting=false;
 Temp temp;AIStub ai;float m_CombatDistance=0;
 AuraEffectList const& GetAuraEffectsByType(int t){return t==SPELL_AURA_MOD_TAUNT?taunts:invisibility;}
 Unit* getVictim(){return victim;}bool CanHaveThreatList(){return threat;}
 bool HasReactState(int s){return s==REACT_PASSIVE?passive:aggressive;}
 bool canSeeOrDetect(Unit* u,bool){return u->visible;}bool IsValidAttackTarget(Unit* u){return u->valid;}
 bool IsFriendlyTo(Unit* u){return u->friendly;}bool canCreatureAttack(Unit* u){return u->attackable&&u->alive;}
 bool HasUnitState(int){return casting;}void SetInFront(Unit* u){front=u;}
 bool isSummon(){return summon;}Temp* ToTempSummon(){return &temp;}
 bool _IsTargetAcceptable(Unit* u){return u->valid;}bool GetVehicle(){return vehicle;}
 Unit* SelectNearestTargetInAttackDistance(float){return nearest;}AIStub* AI(){return &ai;}
 bool HasUnitTypeMask(int){return guardian;}Unit* SelectVictim();
};
Creature* Unit::ToCreature(){return static_cast<Creature*>(this);}
namespace ObjectAccessor {Unit* controlled=nullptr;Unit* GetUnit(Creature&,int){return controlled;}}
#include "SelectVictim.inc"
int main(int argc,char**){
 Unit target,live;Unit dead;dead.alive=false;AuraEffect missing{nullptr},deadEffect{&dead},liveEffect{&live};
 Creature c;c.taunts.push_back(&missing);c.m_ThreatManager.target=&target;
 if(argc>1){try{c.SelectVictim();}catch(std::out_of_range const&){std::cout<<"PASS: baseline reproduces end-sentinel dereference with one missing pet taunter\n";return 0;}return 1;}
 assert(c.SelectVictim()==&target&&c.front==&target);
 c.taunts={&deadEffect};assert(c.SelectVictim()==&target);
 c.taunts={&missing,&deadEffect,&missing};assert(c.SelectVictim()==&target);
 // Every older position, missing/dead/invisible/friendly/unreachable candidates,
 // and both a player and a pet as the surviving older taunter.
 for(bool player:{false,true})for(int position=1;position<=6;++position){
  live.player=player;c.taunts.clear();c.taunts.push_back(&missing);
  for(int i=1;i<position;++i)c.taunts.push_back(i%2?&deadEffect:&missing);
  c.taunts.push_back(&liveEffect);assert(c.SelectVictim()==&live&&c.front==&live);
 }
 for(int invalid=0;invalid<4;++invalid){
  Unit rejected;rejected.visible=invalid!=0;rejected.valid=invalid!=1;rejected.accessible=invalid!=2;rejected.alive=invalid!=3;
  AuraEffect bad{&rejected};c.taunts={&missing,&bad,&liveEffect};assert(c.SelectVictim()==&live);
 }
 c.taunts={&liveEffect};c.victim=&target;assert(c.SelectVictim()==&target); // preserve active latest taunt behavior
 c.taunts.clear();c.front=nullptr;assert(c.SelectVictim()==&target&&c.front==&target);
 c.casting=true;c.front=nullptr;assert(c.SelectVictim()==&target&&!c.front);c.casting=false;
 c.m_ThreatManager.empty=true;c.taunts={&missing};assert(!c.SelectVictim()&&c.ai.evades==1);
 c.vehicle=true;assert(!c.SelectVictim()&&c.ai.evades==1);c.vehicle=false;
 c.aggressive=true;c.nearest=&live;assert(c.SelectVictim()==&live);c.aggressive=false;c.nearest=nullptr;
 c.threat=false;c.passive=true;assert(!c.SelectVictim()&&c.ai.evades==1);
 c.passive=false;c.helper=&live;assert(c.SelectVictim()==&live);c.helper=nullptr;
 c.summon=true;Unit owner;owner.combat=true;owner.helper=&live;c.temp.owner=&owner;assert(c.SelectVictim()==&live);
 owner.helper=nullptr;owner.m_Controlled={1};Unit guardian;guardian.combat=true;guardian.helper=&live;ObjectAccessor::controlled=&guardian;assert(c.SelectVictim()==&live);
 std::cout<<"PASS: extracted full SelectVictim: stale/dead pet/player taunts, older candidates, exhaustion, normal threat, passive/pet/owner/guardian, casting, vehicle, nearest-target and evade paths\n";
}
