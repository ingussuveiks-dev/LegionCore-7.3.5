#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <vector>
using uint8=uint8_t;using uint32=uint32_t;using ObjectGuid=uint64_t;
enum {DIFFICULTY_NORMAL=1,DIFFICULTY_HEROIC=2,DIFFICULTY_MYTHIC_KEYSTONE=8,DIFFICULTY_MYTHIC_DUNGEON=23};
enum {SPELL_FRAGMENT=194325,SPELL_COSMIC_SCYTHE=205330,NPC_SOUL_FRAGMENT=98761,NPC_COSMIC_SCYTHE_2=100839,TEMPSUMMON_CORPSE_TIMED_DESPAWN=1};
struct Position{float x=0,y=0,z=0;float GetPositionX()const{return x;}float GetPositionY()const{return y;}float GetPositionZ()const{return z;}};
static float frand(float,float){return .25f;}
struct SpellInfo{uint32 Id;};
struct Aura{bool* present;void Remove(){*present=false;}};
struct Unit;
struct Creature
{
 ObjectGuid guid;bool world=true;Unit* owner=nullptr;unsigned despawns=0;
 static std::map<ObjectGuid,Creature*> registry;
 ObjectGuid GetGUID()const{return guid;}bool IsInWorld()const{return world;}
 void DespawnOrUnsummon(){world=false;++despawns;}Unit* GetAnyOwner(){return owner;}
 static Creature* GetCreature(Creature&,ObjectGuid id){auto it=registry.find(id);return it==registry.end()?nullptr:it->second;}
 float GetRelativeAngle(Unit*){return 0;}Position GetNearPosition(float r,float){return {r,0,0};}
};
std::map<ObjectGuid,Creature*> Creature::registry;
static ObjectGuid nextFragmentGuid=1000;
struct Unit
{
 std::vector<Creature> actors;std::vector<float> angles;int failed=-1;unsigned attempted=0;std::map<ObjectGuid,bool> draining;bool stun=true;Aura aura{};
 Unit(){actors.reserve(16);}Position GetFirstCollisionPosition(float radius,float angle){assert(radius==1.5f);angles.push_back(angle);return {std::cos(angle)*radius,std::sin(angle)*radius,0};}
 Creature* SummonCreature(unsigned entry,Position const&,unsigned type,unsigned lifetime){assert(entry==98761&&type==1&&lifetime==5000);unsigned index=attempted++;if(int(index)==failed)return nullptr;actors.push_back({nextFragmentGuid++,true,this});auto c=&actors.back();Creature::registry[c->guid]=c;draining[c->guid]=true;return c;}
 Aura* GetAura(unsigned spell,ObjectGuid caster){assert(spell==194344);auto it=draining.find(caster);if(it==draining.end()||!it->second)return nullptr;aura.present=&it->second;return &aura;}
 bool HasAura(unsigned spell){assert(spell==194344);return std::any_of(draining.begin(),draining.end(),[](auto const& row){return row.second;});}
 void RemoveAurasDueToSpell(unsigned spell){assert(spell==194327);stun=false;}
};
struct AI {virtual void SpellHitTarget(Unit*,SpellInfo const*){}virtual void JustDied(Unit*){}virtual ~AI()=default;};
#include "HarbaronAuraCleanupActual.inc"
struct Harbaron:AI
{
 Creature actor{1};Creature* me=&actor;unsigned difficulty=1;uint8 scytheRange=0;std::vector<ObjectGuid> trashGUID;
 unsigned GetDifficultyID()const{return difficulty;}void SummonCreatureDelay(unsigned,unsigned,float,float,float,float){}
#include "HarbaronFragmentActual.inc"
#include "HarbaronCleanupActual.inc"
};
struct Fragment:AI
{
 Creature* me;
 explicit Fragment(Creature* actor):me(actor){}
#include "HarbaronDeathActual.inc"
};
int main()
{
 unsigned cases=0;
 for(unsigned difficulty:{1u,2u,23u,8u})
 {
  unsigned expected=(difficulty==23||difficulty==8)?4:2;
  for(int failed=-1;failed<int(expected);++failed)
  {
   Creature::registry.clear();Harbaron boss;boss.difficulty=difficulty;Unit player;player.failed=failed;SpellInfo spell{194325};boss.SpellHitTarget(&player,&spell);
#ifdef EXPECT_BASELINE
   if(difficulty==8){assert(player.attempted==2);++cases;continue;}
#endif
   assert(player.attempted==expected&&boss.trashGUID.size()==expected-(failed>=0));
   for(unsigned i=1;i<player.angles.size();++i)assert(std::fabs(player.angles[i]-player.angles[i-1]-6.28f/expected)<.001f);
   ++cases;SpellInfo unrelated{1};boss.SpellHitTarget(&player,&unrelated);assert(player.attempted==expected);++cases;
   if(failed<0)
   {
    for(unsigned i=0;i<player.actors.size();++i){Fragment fragment{&player.actors[i]};fragment.JustDied(nullptr);assert(player.stun==(i+1<player.actors.size()));++cases;}
    player.stun=true;boss.SpellHitTarget(&player,&spell);assert(player.attempted==2*expected&&boss.trashGUID.size()==2*expected);++cases;
   }
   boss.DespawnSouls();for(auto const& actor:player.actors)assert(!actor.world&&actor.despawns==1);++cases;
#ifndef EXPECT_BASELINE
   assert(!player.HasAura(194344)&&!player.stun&&boss.trashGUID.empty());++cases;
#endif
   boss.DespawnSouls();for(auto const& actor:player.actors)assert(actor.despawns==1);++cases;
  }
 }
#ifndef EXPECT_BASELINE
 // Two actual target owners can overlap, while unrelated same-spell casters
 // must retain their own drain/parent. Cleanup is not a map-wide aura clear.
 {Creature::registry.clear();Harbaron boss;Unit first,second;SpellInfo spell{194325};boss.SpellHitTarget(&first,&spell);boss.SpellHitTarget(&second,&spell);second.draining[999]=true;boss.DespawnSouls();assert(!first.HasAura(194344)&&!first.stun&&second.HasAura(194344)&&second.stun);++cases;
  auto fragment=&first.actors.front();fragment->owner=nullptr;RemoveSoulFragmentAuras(fragment);++cases;}
#endif
#ifdef EXPECT_BASELINE
 std::cout<<"COUNTERFACTUAL "<<cases<<" extracted native cases; original Mythic+ Fragment takes2 instead of4; expected broken,not a passing gameplay run\n";
#else
 std::cout<<"PASS "<<cases<<" extracted actual Fragment/death/cleanup cases: Normal/Heroic2,Mythic/Mythic+4; spacing,failed spawn,unrelated spell,repeated casts,per-caster last-fragment stun cleanup and idempotent despawn. Controlled summon/owner/geometry endpoints,not full native/client combat\n";
#endif
}
