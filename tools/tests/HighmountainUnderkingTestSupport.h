#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <vector>
using uint32=uint32_t;
using ObjectGuid=uint64_t;
using SpellEffIndex=int;
enum {QUEST_STATUS_INCOMPLETE=1,UNIT_STATE_CASTING=1,EFFECT_1=1,EFFECT_2=2,EFFECT_0=0,TEMPSUMMON_TIMED_DESPAWN=1,REACT_PASSIVE=1,UNIT_FIELD_FLAGS=1,UNIT_FLAG_NON_ATTACKABLE=2,UNIT_FLAG_IMMUNE_TO_PC=4,UNIT_FLAG_IMMUNE_TO_NPC=8,POINT_MOTION_TYPE=1};
struct Position {float x,y,z,o;};
struct Aura {uint32 stacks=1;uint32 GetStackAmount(){return stacks;}};
struct Unit:Position {
 uint32 entry=0,map=1220;ObjectGuid guid=1;bool alive=true,phase=true;
 std::map<std::pair<uint32,ObjectGuid>,Aura> auras;
 std::vector<uint32> casts;unsigned kills=0;
 uint32 GetEntry(){return entry;}uint32 GetMapId(){return map;}ObjectGuid GetGUID(){return guid;}
 std::set<uint32> GetPhases(){return phase?std::set<uint32>{9180}:std::set<uint32>{};}bool IsAlive(){return alive;}bool HasPhaseId(uint32 id){assert(id==9180);return phase;}
 Aura* GetAura(uint32 id,ObjectGuid who){auto i=auras.find({id,who});return i==auras.end()?nullptr:&i->second;}
 bool HasAura(uint32 id,ObjectGuid who){return GetAura(id,who)!=nullptr;}
 void RemoveAurasDueToSpell(uint32 id,ObjectGuid who){auras.erase({id,who});}
 void CastSpell(Unit* target,uint32 id,bool,void*,void*,ObjectGuid who){casts.push_back(id);target->auras[{id,who}]={};target->RemoveAurasDueToSpell(192270,who);}
 void Kill(Unit* target){++kills;target->alive=false;}
 float GetExactDist(Position const* p){return std::sqrt((x-p->x)*(x-p->x)+(y-p->y)*(y-p->y)+(z-p->z)*(z-p->z));}
};
struct GameObject {bool collision=true;void EnableCollision(bool value){collision=value;}};
struct Motion {unsigned moves=0;Position dest{};void MovePoint(uint32 id,Position const& p){assert(id==1);++moves;dest=p;}};
struct Creature:Unit {
 bool casting=false,despawn=false,walk=true;Unit* victim=nullptr;unsigned faces=0,summons=0,props=0;uint32 flags=0;bool failProp=false;GameObject rubble;Motion motion;
 bool HasUnitState(uint32){return casting;}Unit* getVictim(){return victim;}void SetFacingToObject(Unit* u){assert(u==victim);++faces;}
 void SummonCreature(uint32 entry,Position const&,uint32,uint32 duration){assert(entry==97442&&duration==30000);++summons;}
 void DespawnOrUnsummon(){despawn=true;}void SetPhaseId(std::set<uint32> const& p,bool){phase=p.count(9180)!=0;}
 void SetReactState(uint32 state){assert(state==REACT_PASSIVE);}void SetFlag(uint32,uint32 f){flags|=f;}void SetWalk(bool value){walk=value;}
 Motion* GetMotionMaster(){return &motion;}
 GameObject* SummonGameObject(uint32 entry,Position const&,float,float,float,float,float,uint32 duration){assert(entry==190386&&duration==30);++props;return failProp?nullptr:&rubble;}
};
struct Player:Unit {
 bool world=true,teleport=false,vehicle=false,transport=false,flight=false,flying=false,visible=true,los=true;
 uint32 status=QUEST_STATUS_INCOMPLETE,credits=0;Creature* mayla=nullptr;
 bool IsInWorld(){return world;}uint32 GetQuestStatus(uint32 id){assert(id==39780);return status;}
 bool IsBeingTeleported(){return teleport;}bool GetVehicle(){return vehicle;}bool GetTransport(){return transport;}
 bool isInFlight(){return flight;}bool IsFlying(){return flying;}
 uint32 GetQuestObjectiveData(uint32 q,uint32 id){assert(q==39780&&id==97352);return credits;}
 Creature* FindNearestCreature(uint32 id,float r){assert(id==93846);return mayla&&GetExactDist(mayla)<=r?mayla:nullptr;}
 bool InSamePhase(Creature* c){return c->phase==phase;}bool canSeeOrDetect(Creature*){return visible;}bool IsWithinLOSInMap(Creature*){return los;}
 void KilledMonsterCredit(uint32 id){assert(id==97352);++credits;}
};
struct PlayerScript {explicit PlayerScript(char const*){}virtual void OnUpdate(Player*,uint32){}};
struct SpellInfo {uint32 Id;};
struct EventMap {
 std::map<uint32,uint32> scheduled;uint32 now=0;
 void Reset(){scheduled.clear();now=0;}void Update(uint32 diff){now+=diff;}
 void ScheduleEvent(uint32 id,uint32 delay){scheduled[id]=now+delay;}
 uint32 ExecuteEvent(){for(auto it=scheduled.begin();it!=scheduled.end();++it)if(it->second<=now){auto id=it->first;scheduled.erase(it);return id;}return 0;}
};
std::map<ObjectGuid,Unit*> units;
namespace ObjectAccessor {Unit* GetUnit(Unit&,ObjectGuid id){auto i=units.find(id);return i==units.end()?nullptr:i->second;}}
struct ScriptedAI {
 Creature* me;unsigned melee=0;std::vector<uint32> talks;void Talk(uint32 id){talks.push_back(id);}
 explicit ScriptedAI(Creature* c):me(c){}virtual void IsSummonedBy(Unit*){}virtual void SpellFinishCast(SpellInfo const*){}virtual void MovementInform(uint32,uint32){}virtual void Reset(){}virtual void EnterCombat(Unit*){}virtual void SpellHitTarget(Unit*,SpellInfo const*){}virtual void JustDied(Unit*){}virtual void UpdateAI(uint32){}
 bool UpdateVictim(){return me->victim!=nullptr;}void DoCast(Creature* c,uint32 id){assert(c==me);me->casts.push_back(id);}void DoMeleeAttackIfReady(){++melee;}
};
struct Hook {void operator+=(int){}};
struct SpellScript {Unit* caster=nullptr;Unit* hit=nullptr;std::set<int> suppressed;Hook OnEffectLaunchTarget,OnEffectHitTarget,AfterHit;virtual void Register(){}Unit* GetCaster(){return caster;}Unit* GetHitUnit(){return hit;}void PreventHitDefaultEffect(int i){suppressed.insert(i);}};
#define PrepareSpellScript(name) public:
#define SpellEffectFn(...) 0
#define SpellHitFn(...) 0
