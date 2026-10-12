#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <vector>
using uint8=uint8_t; using uint32=uint32_t; using int32=int32_t;
enum {QUEST_STATUS_INCOMPLETE=1,QUEST_STATUS_COMPLETE,REACT_PASSIVE,UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE=2,UNIT_FLAG_IMMUNE_TO_PC=256,UNIT_FLAG_IMMUNE_TO_NPC=512,UNIT_STATE_CASTING=1,TEMPSUMMON_TIMED_DESPAWN,EFFECT_0=0,SPELL_EFFECT_SEND_EVENT=61};
enum SpellCastResult {SPELL_CAST_OK,SPELL_FAILED_DONT_REPORT}; using SpellEffIndex=int;
struct ObjectGuid {unsigned value=0;bool operator<(ObjectGuid r)const{return value<r.value;}bool operator==(ObjectGuid r)const{return value==r.value;}};
struct Position {float x=0;};
struct Player; struct Creature; struct ScriptedAI;
struct Unit:Position {
 ObjectGuid guid;std::set<unsigned> auras;std::map<unsigned,std::set<ObjectGuid>> auraOwners;std::vector<std::pair<Unit*,unsigned>> casts;virtual ~Unit()=default;
 virtual Player* ToPlayer(){return nullptr;}ObjectGuid GetGUID(){return guid;}
 void CastSpell(Unit* target,unsigned id,bool){casts.emplace_back(target,id);if(id==195145||id==195148||id==195150||id==195155){target->auras.insert(id);target->auraOwners[id].insert(guid);}}
 void RemoveAurasDueToSpell(unsigned id,ObjectGuid caster={}){if(caster.value){auraOwners[id].erase(caster);if(auraOwners[id].empty())auras.erase(id);}else{auras.erase(id);auraOwners.erase(id);}}bool HasAura(unsigned id){return auras.count(id)!=0;}
};
std::map<ObjectGuid,Creature*> world;unsigned serial=100;
struct QuestState {int Status=QUEST_STATUS_INCOMPLETE;std::shared_ptr<uint8> ScriptLifetime;};
struct Player:Unit {
 QuestState quest;bool alive=true,inWorld=true,teleport=false,flight=false,flying=false,phase=true,visible=true,los=true,hostile=true,vehicle=false,transport=false;
 unsigned map=1220;std::map<unsigned,int> objectives;std::map<unsigned,std::vector<ObjectGuid>> summons;
 explicit Player(unsigned id){guid={id};}Player* ToPlayer()override{return this;}
 bool IsAlive(){return alive;}bool IsInWorld(){return inWorld;}unsigned GetMapId(){return map;}int GetQuestStatus(unsigned id){assert(id==39776);return quest.Status;}
 // The core's overload takes ObjectID (worm entry), NOT QuestObjective.ID.
 int GetQuestObjectiveData(unsigned id,unsigned object){assert(id==39776);assert(object==97013||object==99216||object==99217);return objectives[object];}
 bool IsBeingTeleported(){return teleport;}bool isInFlight(){return flight;}bool IsFlying(){return flying;}bool GetVehicle(){return vehicle;}bool GetTransport(){return transport;}
 bool InSamePhase(Unit*){return phase;}bool canSeeOrDetect(Unit*){return visible;}bool IsWithinLOSInMap(Unit*){return los;}bool IsValidAttackTarget(Unit*){return hostile;}
 float GetDistance(Unit* u){return std::abs(x-u->x);}bool IsWithinDistInMap(Unit* u,float d){return GetDistance(u)<=d;}
 void GetCreatureListWithEntryInGrid(std::list<Creature*>&,unsigned,float);
 std::vector<ObjectGuid>* GetSummonList(unsigned entry){return &summons[entry];}
 void AddQuestDelayedEvent(unsigned,unsigned,std::function<void()> f){if(!quest.ScriptLifetime)quest.ScriptLifetime=std::make_shared<uint8>(uint8(0));f();}
 QuestState* getQuestStatus(unsigned){return &quest;}unsigned getFaction(){return 1;}
 Position GetNearPosition(float distance,float){return {x+distance};}
 Creature* SummonCreature(unsigned,Position,unsigned,unsigned,int,ObjectGuid);
};
struct Creature:Unit {
 Player* owner=nullptr;ScriptedAI* ai=nullptr;unsigned entry;bool alive=true,gone=false,IsAIEnabled=true,personal=false,casting=false;Unit* victim=nullptr;ObjectGuid damageOwner;
 explicit Creature(unsigned id,Player* p=nullptr):owner(p),entry(id){guid={++serial};world[guid]=this;}
 ~Creature(){world.erase(guid);}unsigned GetEntry(){return entry;}bool IsAlive(){return alive;}bool IsDespawn(){return gone;}
 Creature* ToTempSummon(){return owner?this:nullptr;}Unit* GetSummoner(){return owner;}ScriptedAI* AI(){return ai;}
 void DespawnOrUnsummon(){gone=true;}void SetOwnerGUID(ObjectGuid id){damageOwner=id;}void setFaction(unsigned){}void RemoveFlag(unsigned,unsigned){}void SetReactState(unsigned){}
 void AddPlayerInPersonnalVisibilityList(ObjectGuid id){assert(owner && owner->guid==id);personal=true;}Unit* getVictim(){return victim;}bool HasUnitState(unsigned){return casting;}
};
void Player::GetCreatureListWithEntryInGrid(std::list<Creature*>& out,unsigned entry,float range){for(auto const& p:world)if(p.second->entry==entry&&GetDistance(p.second)<=range)out.push_back(p.second);}
struct ObjectAccessor {static Creature* GetCreature(Unit&,ObjectGuid guid){auto i=world.find(guid);return i==world.end()?nullptr:i->second;}};
struct ScriptedAI {
 Creature* me;explicit ScriptedAI(Creature* c):me(c){c->ai=this;}virtual ~ScriptedAI()=default;
 virtual void IsSummonedBy(Unit*){}virtual void SetGUID(ObjectGuid const&,int32){}virtual void JustSummoned(Creature*){}virtual void DoAction(int32){}virtual void JustDied(Unit*){}virtual void UpdateAI(uint32){}
 void AttackStart(Unit* target){me->victim=target;}void DoMeleeAttackIfReady(){}
};
struct SummonList {std::vector<Creature*> values;explicit SummonList(Creature*){}void Summon(Creature* c){values.push_back(c);}void DespawnAll(){for(auto c:values)c->DespawnOrUnsummon();values.clear();}};
struct PlayerScript {explicit PlayerScript(char const*){}virtual void OnUpdate(Player*,uint32){}virtual void OnLogin(Player*){}virtual void OnLogout(Player*){}virtual void OnMapChanged(Player*){};};
struct SpellScript {
 Unit* caster=nullptr;struct Info {unsigned Id;} info;bool prevented=false;
 Unit* GetCaster(){return caster;}Info* GetSpellInfo(){return &info;}void PreventHitDefaultEffect(int){prevented=true;}
 struct Hook{template<class T>void operator+=(T){}} OnCheckCast,OnEffectHit;virtual void Register(){}
};
#define PrepareSpellScript(name) public:
#define SpellCheckCastFn(fn) &fn
#define SpellEffectFn(fn,index,type) &fn
#include "HighmountainWormSupport.inc"
std::vector<std::unique_ptr<Creature>> allocated;std::vector<std::unique_ptr<npc_highmountain_worm_ally>> ais;
Creature* Player::SummonCreature(unsigned entry,Position pos,unsigned,unsigned duration,int,ObjectGuid viewer){assert(duration==46000&&viewer==guid);allocated.emplace_back(new Creature(entry,this));auto c=allocated.back().get();c->x=pos.x;ais.emplace_back(new npc_highmountain_worm_ally(c));ais.back()->IsSummonedBy(this);return c;}
spell_highmountain_call_tribe Call(Player& p,unsigned id){spell_highmountain_call_tribe s;s.caster=&p;s.info.Id=id;return s;}
void ClearWorld(){ais.clear();allocated.clear();}
int main()
{
 player_highmountain_worm_support script;
 for(auto const& tribe:WormSupport::Tribes){
  Player p(1),other(2);Creature worm(tribe.worm);worm.x=10;
  script.OnUpdate(&p,500);assert(p.auras==std::set<unsigned>{tribe.bar});
  auto call=Call(p,tribe.call);assert(call.Check()==SPELL_CAST_OK);call.Handle(0);assert(call.prevented&&allocated.size()==1);
  auto ally=allocated.back().get();auto ai=ais.back().get();assert(ally->personal&&ally->damageOwner==p.guid);
  ai->UpdateAI(100);assert(ally->victim==&worm&&ally->casts.back()==std::make_pair(static_cast<Unit*>(&worm),tribe.attack));
  if(tribe.ally==99221)assert(ally->casts.front().second==192412);
  auto duplicate=Call(p,tribe.call);assert(duplicate.Check()!=SPELL_CAST_OK);
  script.OnUpdate(&other,500);auto second=Call(other,tribe.call);assert(second.Check()==SPELL_CAST_OK);second.Handle(0);assert(allocated.size()==2);ais.back()->UpdateAI(100);
  Creature totem(97508,&p);ai->JustSummoned(&totem);
  p.objectives[tribe.worm]=1;script.OnUpdate(&p,500);ai->UpdateAI(500);assert(p.auras.empty()&&ally->gone&&totem.gone&&!allocated.back()->gone);if(tribe.ally==99219)assert(worm.HasAura(195155));
  script.OnLogout(&p);script.OnLogout(&other);assert(allocated.back()->gone);ClearWorld();
 }
 // Overlapping areas: only nearest eligible worm's bar, then next unfinished objective.
 {Player p(3);Creature a(97013),b(99216);a.x=10;b.x=20;script.OnUpdate(&p,500);assert(p.HasAura(195145)&&!p.HasAura(195148));auto wrong=Call(p,195146);assert(wrong.Check()!=SPELL_CAST_OK);
  p.objectives[97013]=1;script.OnUpdate(&p,500);assert(!p.HasAura(195145)&&p.HasAura(195148));script.OnLogout(&p);}
 // Revalidate cast completion, do not silently retarget; no quest credit is issued by calling.
 {Player p(60);Creature first(99216),replacement(99216);first.x=10;replacement.x=20;script.OnUpdate(&p,500);
  auto c=Call(p,195146);assert(c.Check()==SPELL_CAST_OK);replacement.x=5;assert(c.Check()==SPELL_CAST_OK);
  assert(c.target==first.guid);first.alive=false;assert(c.Check()!=SPELL_CAST_OK);c.Handle(0);assert(allocated.empty());script.OnLogout(&p);}
 for(int reason=0;reason<9;++reason){Player p(10+reason);Creature worm(97013);script.OnUpdate(&p,500);auto c=Call(p,195144);assert(c.Check()==SPELL_CAST_OK);
  if(reason==0)worm.alive=false;if(reason==1)p.x=100;if(reason==2)p.quest.Status=QUEST_STATUS_COMPLETE;if(reason==3)p.phase=false;
  if(reason==4)p.los=false;if(reason==5)p.alive=false;if(reason==6)p.map=1;if(reason==7)p.objectives[97013]=1;
  if(reason==8)p.quest=QuestState{}; c.Handle(0);assert(allocated.empty());script.OnUpdate(&p,500);if(reason!=8)assert(p.auras.empty());script.OnLogout(&p);
 }
 for(int reason=0;reason<8;++reason){Player p(30+reason);Creature worm(99216);script.OnUpdate(&p,500);auto c=Call(p,195146);assert(c.Check()==SPELL_CAST_OK);c.Handle(0);auto ally=allocated.back().get();auto ai=ais.back().get();ai->UpdateAI(100);
  if(reason==0)p.quest=QuestState{};if(reason==1)p.alive=false;if(reason==2)p.teleport=true;if(reason==3)p.x=100;
  if(reason==4)p.vehicle=true;if(reason==5)p.transport=true;if(reason==6)script.OnLogout(&p);
  ai->UpdateAI(reason==7?45000:100);assert(ally->gone&&!worm.HasAura(195155));script.OnLogout(&p);ClearWorld();
 }
 {Player p(50);p.quest.Status=QUEST_STATUS_COMPLETE;p.auras={195145,195148,195150};script.OnLogin(&p);assert(p.auras.empty());}
 assert(script.timers.empty());
 std::cout<<"PASS: all three native calls, nearest unfinished worm, duplicate and two-owner isolation, cast-time invalidation, objective ObjectIDs, summon ownership, native attacks/healing summon, quest lifetime, timeout and bar/debuff/totem cleanup.\n";
}
