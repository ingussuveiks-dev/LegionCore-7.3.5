#include <limits>
#include <cassert>
#include <cstdint>
#include <atomic>
#include <mutex>
#include <memory>
#include <map>
#include <set>
#include <vector>
#include <list>
#include <functional>
#include <iostream>
#include <algorithm>
#include <chrono>
using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;using uint64=uint64_t;using int32=int32_t;
using Milliseconds=std::chrono::milliseconds;
uint32 urand(uint32,uint32);
using DamageEffectType=uint32;
enum {CLASS_PRIEST=5,QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_COMPLETE=1,CRITERIA_TYPE_SCRIPT_EVENT_2=92,
UNIT_FIELD_FLAGS=1,UNIT_FLAG_REMOVE_CLIENT_CONTROL=2,UNIT_STATE_STUNNED=4,UNIT_STATE_CASTING=8,TEMPSUMMON_TIMED_DESPAWN=1};
#include "EventMap.inc"
#include "Functions.inc"
struct Player;struct Creature;struct Unit;
struct ObjectGuid {uint64 high=0,low=0;bool IsPlayer()const{return high==1;}bool IsEmpty()const{return !high&&!low;}explicit operator bool()const{return !IsEmpty();}bool operator==(ObjectGuid v)const{return high==v.high&&low==v.low;}};
struct Reference {Player* player;Player* getSource()const{return player;}};
struct Map {std::vector<Reference> players;auto const& GetPlayers()const{return players;}};
struct Unit {
 virtual ~Unit()=default;Map* map=nullptr;ObjectGuid guid{2,1},owner,charmer;bool alive=true,inWorld=true,phase=true;uint32 entry=0,kills=0;
 virtual Player const* ToPlayer()const{return nullptr;}
 Player* ToPlayer(){return const_cast<Player*>(static_cast<Unit const*>(this)->ToPlayer());}
 Player* GetCharmerOrOwnerPlayerOrPlayerItself()const;
 ObjectGuid GetCharmerGUID()const{return charmer;}ObjectGuid GetOwnerGUID()const{return owner;}
#include "OwnerGuid.inc"
 ObjectGuid GetGUID()const{return guid;}Map* GetMap()const{return map;}bool IsAlive()const{return alive;}bool IsInWorld()const{return inWorld;}
 bool InSamePhase(Unit* other)const{return phase&&other->phase;}uint32 GetEntry()const{return entry;}
 float GetPositionX()const{return 1;}float GetPositionY()const{return 2;}float GetPositionZ()const{return 3;}
 virtual void RemoveAurasDueToSpell(uint32,ObjectGuid){};virtual void AddUnitState(uint32){}
 void Kill(Creature*);uint32 Deal(Creature*,uint32);
};
struct Player:Unit {
 uint32 classId=5,status=3,credits=0;bool contact=true,pending=false,rewarded=false,near=true,gm=false;
 std::function<void()> onCredit;
 Player(){guid={1,1};}Player const* ToPlayer()const override{return this;}
 bool CanContact()const{return contact;}bool isGameMaster()const{return gm;}bool HasPendingTeleport()const{return pending;}uint32 getClass()const{return classId;}
 bool IsWithinDistInMap(Unit*,float d)const{assert(d==100);return near;}bool GetQuestRewardStatus(uint32 q)const{assert(q==41993);return rewarded;}
 uint32 GetQuestStatus(uint32 q)const{assert(q==41993);return status;}
 void UpdateAchievementCriteria(uint32 type,uint32 asset){struct Cache{Player* player;};Cache value{this};Cache* cachePtr=&value;
#include "NativeCreditGate.inc"
 assert(type==92&&asset==50695);++credits;if(onCredit)onCredit();}
};
struct ObjectAccessor {
 inline static std::vector<Unit*> units;
 static Unit* GetUnit(Unit const& source,ObjectGuid guid){for(auto* u:units)if(u->guid==guid&&u->map==source.map&&u->inWorld)return u;return nullptr;}
 static Player* GetPlayer(Unit const& source,ObjectGuid guid){auto u=GetUnit(source,guid);return u?u->ToPlayer():nullptr;}
};
#include "OwnerPlayer.inc"
struct Scenario {uint32 id=1085,step=5;uint32 GetScenarioId()const{return id;}uint32 GetCurrentStep()const{return step;}} scenario;
struct ScenarioManager {Scenario* active=&scenario;Scenario* GetScenario(uint32 id){assert(id==7);return active;}} manager;
auto sScenarioMgr=&manager;
struct SpellInfo {uint32 Id=0;};
struct CreatureAI {
 virtual ~CreatureAI()=default;virtual void Reset(){};virtual void EnterCombat(Unit*){};virtual void JustSummoned(Creature*){};
 virtual void DamageTaken(Unit*,uint32&,DamageEffectType){};virtual void DamageTakenAfterScaling(Unit*,uint32&,DamageEffectType){};
 virtual void SpellHitTarget(Unit*,SpellInfo const*){};virtual void UpdateAI(uint32){};virtual void AttackStart(Unit*){};
};
struct InstanceScript {void OnCreatureDamageTaken(Creature*,Unit*,uint32&){};};
struct Creature:Unit {
 uint64 health=100000,maximum=100000,targetHealth=1000,targetMaximum=1000;float multiplier=0.01f;uint32 mapId=1489;
 bool casting=false,failCast=false,victim=true,failSpawn=false;uint32 interruptions=0,conversations=0,heals=0,stuns=0,flags=0,speeches=0,spawnRequests=0;
 std::vector<uint32> casts;std::vector<Creature*> allies;std::vector<ObjectGuid> strangles;
 CreatureAI* primary=nullptr;CreatureAI* i_disabledAI=nullptr;Player* nearest=nullptr;FunctionProcessor functions;
 Creature(){entry=106318;guid={2,10};}
 Creature* ToCreature(){return this;}CreatureAI* AI(){return primary;}InstanceScript* GetInstanceScript(){return nullptr;}
 uint64 GetHealth()const{return health;}uint64 GetHealth(Unit*)const{return targetHealth;}uint64 GetMaxHealth(Unit*)const{return targetMaximum;}
 uint64 CountPctFromMaxHealth(uint32 pct,Unit* u)const{return GetMaxHealth(u)*pct/100;}bool HealthBelowPct(uint32 p)const{return health<maximum*p/100;}
 float GetHealthMultiplierForTarget(Unit*)const{return multiplier;}uint32 GetInstanceId()const{return 7;}uint32 GetMapId()const{return mapId;}
 void SetFlag(uint32,uint32 f){flags|=f;}void StopAttack(){victim=false;}void InterruptNonMeleeSpells(bool){++interruptions;casting=false;}
 void AddDelayedEvent(uint64 delay,std::function<void()>&& fn){functions.AddDelayedEvent(delay,std::move(fn));}
 Player* FindNearestPlayer(float,bool=true){return nearest;}
 void RemoveAurasDueToSpell(uint32 id,ObjectGuid caster)override{assert(id==210703);strangles.erase(std::remove(strangles.begin(),strangles.end(),caster),strangles.end());stuns=uint32(strangles.size());}
 void AddUnitState(uint32 state)override{assert(state==UNIT_STATE_STUNNED);stuns=1;}void ClearUnitState(uint32){stuns=0;}
 bool HasUnitState(uint32 state)const{return state==UNIT_STATE_CASTING&&casting;}
 void CastSpell(Creature* t,uint32 id){assert(t==this&&id==210807);++heals;}
 void CreateConversation(uint32 id){assert(id==2015);++conversations;}
 Creature* SummonCreature(uint32,float,float,float,float,uint32,uint32){++spawnRequests;return failSpawn?nullptr:this;}
 float GetPositionX()const{return 1;}float GetPositionY()const{return 2;}float GetPositionZ()const{return 3;}
};
// The cast/grid/kill endpoint adapters do not substitute the actual aura engine,
// movement, packets or creature deletion; those remain live/client checks.
void Unit::Kill(Creature* c){++kills;c->alive=false;}
void GetCreatureListWithEntryInGrid(std::list<Creature*>& out,Creature* c,uint32 entry,float range){assert(range==50);for(auto a:c->allies)if(a->entry==entry)out.push_back(a);}
uint32 urand(uint32 a,uint32){return a;}int32 irand(int32 a,int32){return a;}
struct SummonList {explicit SummonList(Creature*){}void Summon(Creature*){}void DespawnAll(){}};
struct ScriptedAI:CreatureAI {
 Creature* me;Unit* attackTarget=nullptr;std::vector<uint32> talks;std::function<void()> onCast;
 explicit ScriptedAI(Creature* actor):me(actor){}void Talk(uint32 id){talks.push_back(id);}
 void DoCast(uint32 id){me->casts.push_back(id);if(me->failCast)return;if(onCast)onCast();}bool UpdateVictim(){return me->victim;}
 void AttackStart(Unit* target)override{attackTarget=target;}
};
struct TextMgr {void SendChat(Creature* actor,uint32 id){assert(id==2);++actor->speeches;}} textMgr;
auto sCreatureTextMgr=&textMgr;
#include "Defeat.inc"
uint32 Unit::Deal(Creature* victim,uint32 damage){
 DamageEffectType damagetype=0;if(victim->primary)victim->primary->DamageTaken(this,damage,damagetype);
 if(!damage)return 0;uint64 health=victim->GetHealth(this);
#include "ScaledHook.inc"
 if(health<=damage)victim->alive=false;else {victim->targetHealth-=damage;victim->health=victim->maximum*victim->targetHealth/victim->targetMaximum;}
 return damage;
}
struct Run {
 Map map,other;Player player,stranger;Unit pet,guardian,charmed;Creature boss,ally,ally2,ally3;std::unique_ptr<npc_lady_kalindriaAI> ai;
 Run(){scenario={};manager.active=&scenario;player.map=&map;stranger.map=&map;stranger.guid={1,2};stranger.classId=4;
  map.players={{&stranger},{&player}};boss.map=&map;boss.nearest=&stranger;
  pet.map=guardian.map=charmed.map=&map;pet.guid={2,2};guardian.guid={2,3};charmed.guid={2,4};pet.owner=guardian.owner=player.guid;charmed.charmer=player.guid;
  ally.map=ally2.map=ally3.map=&map;ally.entry=106134;ally2.entry=106137;ally3.entry=106253;ally.guid={2,5};ally2.guid={2,6};ally3.guid={2,7};boss.allies={&ally,&ally2,&ally3};
  ObjectAccessor::units={&player,&stranger,&pet,&guardian,&charmed,&boss,&ally,&ally2,&ally3};ai=std::make_unique<npc_lady_kalindriaAI>(&boss);boss.primary=ai.get();ai->Reset();}
 void Tick(uint32 ms){boss.functions.Update(ms);if(boss.primary)boss.primary->UpdateAI(ms);}
 void Strangle(){for(auto a:boss.allies){a->strangles.push_back(boss.guid);a->stuns=1;}}
};
int main(){
#ifdef BASELINE
 {Run r;assert(r.player.Deal(&r.boss,10)==1000&&!r.boss.alive&&!r.ai->outro&&!r.player.credits);}
 {Run r;r.Strangle();assert(!r.pet.Deal(&r.boss,100000)&&r.ai->outro);assert(r.stranger.credits==1&&!r.player.credits);r.pet.inWorld=false;r.Tick(3000);assert(r.boss.alive);}
 std::cout<<"PASS labelled ce34ea6c counterfactual: scaled one-hit bypasses50695; nearest unrelated player receives credit and removed pet leaves boss alive after3s. Expected failing baseline, not successful ordinary behavior.\n";
#elif defined(ADMISSION_BASELINE)
 {Run r;r.player.alive=false;assert(r.pet.Deal(&r.boss,10)==1000&&!r.boss.alive&&!r.ai->outro&&!r.player.credits);}
 {Run r;r.player.gm=true;assert(!r.pet.Deal(&r.boss,10)&&r.ai->outro&&!r.player.credits);r.Tick(3000);assert(!r.boss.alive&&!r.player.credits);}
 std::cout<<"PASS labelled8a8608e1 native-admission counterfactual: dead owner loses real pet victory; GM starts/finishes outro while actual native GM/contact gate rejects50695.\n";
#else
 uint32 variants=0;
 for(uint32 status:{3u,1u})for(int type=0;type<7;++type)for(bool suspended:{false,true})for(bool ownerAlive:{false,true}){
  Run r;r.player.status=status;r.player.alive=ownerAlive;Unit* attacker[]={&r.player,&r.pet,&r.guardian,&r.charmed,&r.ally,&r.ally2,&r.ally3};r.Strangle();
  if(suspended){r.boss.primary=nullptr;r.boss.i_disabledAI=r.ai.get();}
  r.ai->onCast=[&]{uint32 d=2000;r.ai->DamageTakenAfterScaling(attacker[type],d,0);assert(!d);};
  assert(!attacker[type]->Deal(&r.boss,10)&&r.boss.alive&&r.ai->outro&&r.player.credits==1&&!r.stranger.credits);
  assert(r.ally.strangles.empty()&&r.ally.heals==1&&r.boss.conversations==1);attacker[type]->inWorld=false;
  r.boss.casting=true;r.Tick(2999);assert(r.boss.alive);r.Tick(1);assert(!r.boss.alive&&r.boss.kills==1&&r.player.credits==1);r.Tick(5000);assert(r.boss.kills==1);++variants;
 }
 {Run r;r.player.Deal(&r.boss,1);assert(r.boss.alive&&!r.ai->outro&&!r.player.credits&&r.boss.targetHealth==900);}
 for(int invalid=0;invalid<17;++invalid){Run r;switch(invalid){case 0:r.player.classId=4;break;case 1:r.player.status=0;break;case 2:r.player.rewarded=true;break;case 3:r.player.gm=true;break;case 4:r.player.inWorld=false;break;case 5:r.player.contact=false;break;case 6:r.player.pending=true;break;case 7:r.player.map=&r.other;break;case 8:r.player.phase=false;break;case 9:r.player.near=false;break;case 10:r.boss.mapId=1220;break;case 11:scenario.id=972;break;case 12:scenario.step=6;break;case 13:manager.active=nullptr;break;case 14:r.boss.entry=106319;break;case 15:r.boss.inWorld=false;break;case 16:r.boss.alive=false;break;}
  uint32 d=1000;r.ai->DamageTakenAfterScaling(&r.player,d,0);assert(d==1000&&!r.ai->outro&&!r.player.credits);}
 {Run r;uint32 d=1000;r.ai->DamageTakenAfterScaling(nullptr,d,0);assert(d==1000);r.pet.owner={1,99};r.ai->DamageTakenAfterScaling(&r.pet,d,0);assert(d==1000);}
 {Run r;r.stranger.classId=5;uint32 d=1000;r.ai->DamageTakenAfterScaling(&r.ally,d,0);assert(d==1000);r.ally.owner=r.stranger.guid;r.stranger.status=0;r.ai->DamageTakenAfterScaling(&r.ally,d,0);assert(d==1000);}
 {Run r;r.pet.Deal(&r.boss,10);r.pet.map=&r.other;r.Tick(3000);assert(!r.boss.alive&&r.boss.kills==1&&!r.pet.kills);}
 {Run r;r.pet.Deal(&r.boss,10);r.pet.alive=false;r.Tick(3000);assert(!r.boss.alive&&r.boss.kills==1&&!r.pet.kills);}
 {Run r;r.pet.Deal(&r.boss,10);Unit replacement;replacement.guid={3,2};replacement.map=&r.map;ObjectAccessor::units={&replacement};r.Tick(3000);assert(!r.boss.alive&&!replacement.kills);}
 {Run r;r.pet.Deal(&r.boss,10);r.Tick(3000);assert(!r.boss.alive&&r.pet.kills==1&&!r.boss.kills);}
 {Run r;r.pet.Deal(&r.boss,10);r.ai->Reset();r.Tick(3000);assert(r.boss.alive&&!r.ai->outro);scenario.step=5;r.pet.Deal(&r.boss,10);r.Tick(3000);assert(!r.boss.alive&&r.player.credits==2);}
 {Run r;r.pet.Deal(&r.boss,10);r.ai.reset();r.boss.primary=nullptr;r.Tick(3000);assert(r.boss.alive);} // ASAN old AI really freed
 {Run r;r.boss.targetHealth=80;r.boss.health=8000;r.pet.Deal(&r.boss,10);assert(r.ai->outro&&!r.ai->outro_event);r.Tick(20);assert(std::count(r.boss.casts.begin(),r.boss.casts.end(),210703u)==0);
  SpellInfo late{210703};r.ally.strangles.push_back(r.boss.guid);r.ai->SpellHitTarget(&r.ally,&late);assert(r.ally.strangles.empty());}
 {Run r;r.boss.targetHealth=120;r.boss.health=12000;r.pet.Deal(&r.boss,1);assert(r.ai->outro_event);r.ai->UpdateAI(10);assert(r.boss.casts.back()==210703);r.boss.victim=false;r.boss.casting=true;r.Tick(3000);assert(r.boss.speeches==1);}
 {Run r;r.boss.targetHealth=120;r.boss.health=12000;r.pet.Deal(&r.boss,1);r.ai->UpdateAI(10);r.player.Deal(&r.boss,10);r.Tick(3000);assert(!r.boss.speeches&&!r.boss.alive);}
 {Run r;r.boss.targetHealth=120;r.boss.health=12000;r.pet.Deal(&r.boss,1);r.ai->UpdateAI(10);r.ai->Reset();SpellInfo late{210703};r.ally.strangles.push_back(r.boss.guid);r.ai->SpellHitTarget(&r.ally,&late);r.Tick(3000);assert(!r.boss.speeches&&r.ally.strangles.empty());}
 {Run r;r.Strangle();ObjectGuid other{2,99};r.ally.strangles.push_back(other);r.player.Deal(&r.boss,10);assert(r.ally.strangles.size()==1&&r.ally.strangles[0]==other&&r.ally.stuns==1);}
 {Run r;r.Strangle();r.ai->Reset();assert(r.ally.strangles.empty()&&!r.player.credits&&!r.ally.heals);}
 {Run r;r.boss.failCast=true;r.pet.Deal(&r.boss,10);r.Tick(3000);assert(!r.boss.alive&&r.player.credits==1);}
 {Run r;r.boss.failSpawn=true;r.ai->EnterCombat(&r.player);r.ai->UpdateAI(13000);r.ai->UpdateAI(0);assert(r.boss.spawnRequests==15&&!r.player.credits&&!r.ai->outro);}
 {Run r;assert(TuureDefeat::Participant(&r.boss)==&r.player);r.stranger.classId=5;assert(!TuureDefeat::Participant(&r.boss));r.stranger.status=0;assert(TuureDefeat::Participant(&r.boss)==&r.player);}
 {Run r;r.stranger.classId=5;r.stranger.gm=true;r.player.alive=false;assert(TuureDefeat::Participant(&r.boss)==&r.player);r.ally.Deal(&r.boss,10);assert(r.ai->outro&&r.player.credits==1&&!r.stranger.credits);}
 {Run r;r.boss.targetHealth=120;r.boss.health=12000;r.pet.Deal(&r.boss,1);r.ai->UpdateAI(10);assert(r.ai->attackTarget==&r.player);r.ai.reset();r.boss.primary=nullptr;r.Tick(3000);assert(!r.boss.speeches);}
 std::cout<<"PASS extracted full Lady AI/recipient and actual native owner/scaled dispatch/EventMap/FunctionProcessor: "<<variants<<" incomplete/complete player/pet/guardian/charmed/three-allied-killer and suspended-AI variants; invalid/ambiguous/missing owner, vanished/dead/other-map/full-GUID killer,3s exactly-once finite death, reset/retry/AI destruction, reentrant credit, strangle cleanup/late hit/shared caster/low-health talk. Grid/cast/aura/kill endpoints are adapters; client fight remains pending.\n";
#endif
}
