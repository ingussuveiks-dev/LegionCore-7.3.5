#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <vector>
using uint32=uint32_t;
enum {UNIT_STATE_CASTING=1};
uint32 urand(uint32 low,uint32){return low;}
struct Unit {bool cc=false;bool HasCrowdControlAura(Unit*){return cc;}};
struct Creature:Unit {
 std::set<uint32> spells;Unit* victim=nullptr;bool world=true,alive=true,combat=true,casting=false;
 std::vector<std::pair<Unit*,uint32>> casts;
 bool GetCreatureSpell(uint32 id){return spells.count(id)!=0;}bool IsInWorld(){return world;}bool IsAlive(){return alive;}bool isInCombat(){return combat;}
 Unit* getVictim(){return victim;}bool HasUnitState(uint32){return casting;}void CastSpell(Unit* target,uint32 id,bool){casts.emplace_back(target,id);}
};
struct EventMap {
 std::map<uint32,uint32> scheduled;uint32 now=0;
 void Reset(){scheduled.clear();now=0;}void Update(uint32 diff){now+=diff;}void CancelEvent(uint32 id){scheduled.erase(id);}
 void ScheduleEvent(uint32 id,uint32 delay){scheduled[id]=now+delay;}
 uint32 ExecuteEvent(){for(auto it=scheduled.begin();it!=scheduled.end();++it)if(it->second<=now){auto id=it->first;scheduled.erase(it);return id;}return 0;}
};
struct SpellInfo {bool allowed=true;bool CanAutoCast(Unit*,Unit*)const{return allowed;}};
struct Manager {std::map<uint32,SpellInfo> spells;SpellInfo const* GetSpellInfo(uint32 id){auto i=spells.find(id);return i==spells.end()?nullptr:&i->second;}} manager;
auto sSpellMgr=&manager;
struct AIInfo {uint32 cooldown=100;};std::map<uint32,AIInfo> AISpellInfo;
struct AggressorAI {
 Creature* me;EventMap spellCasts;bool controlled=false;unsigned baseUpdates=0,baseResets=0,baseEnters=0;
 explicit AggressorAI(Creature* c):me(c){}virtual void Reset(){++baseResets;spellCasts.Reset();}
 virtual void EnterCombat(Unit* who){++baseEnters;me->victim=who;for(auto id:me->spells)spellCasts.ScheduleEvent(id,100);}
 virtual void UpdateAI(uint32){++baseUpdates;}bool IsInControl(){return controlled;}
};
#include "HighmountainSnowblindCombat.inc"
int main()
{
 for(auto spells:{std::set<uint32>{194019,197591},std::set<uint32>{196402,197483,213738}}){
  Creature mob;Unit victim;mob.spells=spells;for(auto id:spells)manager.spells[id]={};npc_highmountain_mightstone ai(&mob);ai.Reset();ai.EnterCombat(&victim);
  for(auto id:spells)assert(ai.spellCasts.scheduled.count(id)==(id==197591||id==213738));
  mob.casting=true;ai.UpdateAI(100);assert(mob.casts.empty());mob.casting=false;victim.cc=true;ai.UpdateAI(1);assert(mob.casts.empty());victim.cc=false;
  ai.controlled=true;ai.UpdateAI(1);assert(mob.casts.empty());ai.controlled=false;
  for(unsigned i=0;i<spells.size();++i)ai.UpdateAI(1);
  assert(!mob.casts.empty());for(auto cast:mob.casts){assert(cast.first==&victim);assert(cast.second==194019||cast.second==196402||cast.second==197483);}
  assert(ai.baseUpdates>0&&ai.baseResets==1&&ai.baseEnters==1);
  ai.Reset();mob.casts.clear();ai.UpdateAI(10000);assert(mob.casts.empty());
 }
 for(int reason=0;reason<5;++reason){Creature mob;Unit victim;mob.spells={194019};npc_highmountain_mightstone ai(&mob);ai.EnterCombat(&victim);
  if(reason==0)mob.world=false;if(reason==1)mob.alive=false;if(reason==2)mob.combat=false;if(reason==3)mob.victim=nullptr;if(reason==4)manager.spells[194019].allowed=false;
  ai.UpdateAI(100);assert(mob.casts.empty());manager.spells[194019].allowed=true;
 }
 std::cout<<"PASS: extracted Mightstone AI attacks the victim, cancels only overridden generic events, preserves base behavior, waits during casting/control/CC, stops out of combat/death/world and clears timers on reset.\n";
}
