#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <vector>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;using int32=int32_t;
#include "SceneEnums.inc"
constexpr int UNIT_FIELD_FLAGS=0;
constexpr uint32 UNIT_FLAG_NON_ATTACKABLE=2,UNIT_FLAG_NOT_SELECTABLE=0x2000000;
enum DamageEffectType{DIRECT_DAMAGE};enum ReactStates{REACT_PASSIVE,REACT_AGGRESSIVE};
enum TempSummonType{TEMPSUMMON_DEAD_DESPAWN};
#define TC_LOG_DEBUG(...) ((void)0)
struct ObjectGuid{uint64 id=0;void Clear(){id=0;}explicit operator bool()const{return id!=0;}bool operator==(ObjectGuid o)const{return id==o.id;}};
struct Position{float m_positionX=0,m_positionY=0,m_positionZ=0,o=0;Position()=default;Position(float x,float y,float z,float a):m_positionX(x),m_positionY(y),m_positionZ(z),o(a){}float GetPositionX()const{return m_positionX;}float GetPositionY()const{return m_positionY;}float GetPositionZ()const{return m_positionZ;}float GetOrientation()const{return o;}};
struct Creature;struct TempSummon;struct CreatureAI;
struct WorldObject{virtual ~WorldObject()=default;};
struct Unit:WorldObject{virtual Creature* ToCreature(){return nullptr;}virtual uint32 GetEntry()const{return 0;}virtual void CastSpell(Unit*,uint32,bool){}};
struct Move{bool smooth=false,fly=false,uncompressed=false,generate=true;float speed=0;int animation=-1;Position destination;};
struct EffectMovementGenerator{explicit EffectMovementGenerator(uint32 id):_pointId(id){}uint32 _pointId,_arrivalSpellId=0;ObjectGuid _arrivalSpellTargetGuid;void MovementInform(Unit&);void Finalize(Unit&);};
struct MotionMaster{Unit* _owner;std::unique_ptr<EffectMovementGenerator> generator;explicit MotionMaster(Unit* u):_owner(u){}void Mutate(EffectMovementGenerator* p,MovementSlot){generator.reset(p);}void MoveTakeoff(uint32,float,float,float);void MoveSmoothFlyPath(uint32,Position const,float);};
struct CreatureData{uint32 id=115846,mapid=1220,areaId=8362;float posX=1816.98f,posY=734.469f,posZ=85.5647f,orientation=1.18135f;};
std::map<uint64,Creature*> registry;std::vector<std::unique_ptr<TempSummon>> summons;
std::function<CreatureAI*(Creature*)> factory;
bool failSummon=false;std::set<uint32> failCast;uint64 nextGuid=100;
struct Creature:Unit{
 uint32 entry=115846,map=1220,flags=32768;uint64 db=146706022;CreatureData data;bool hasData=true,alive=true,combat=false,casting=false,charmed=false,despawn=false,activeGrid=false,fly=false,noGravity=false;
 ObjectGuid guid,LastCharmerGUID;CreatureAI* ai=nullptr;ReactStates react=REACT_AGGRESSIVE;Position position,home;MotionMaster motion{this};std::vector<Move> moves;Move pending;
 std::set<std::pair<uint32,uint64>> auras;std::map<uint32,int> casts;unsigned visibility=0,stops=0,threat=0,despawns=0;
 explicit Creature(uint64 g=1):guid{g}{registry[g]=this;}~Creature();
 Creature* ToCreature()override{return this;}virtual TempSummon* ToTempSummon(){return nullptr;}
 uint32 GetEntry()const override{return entry;}uint32 GetMapId()const{return map;}uint64 GetDBTableGUIDLow()const{return db;}CreatureData const* GetCreatureData()const{return hasData?&data:nullptr;}
 ObjectGuid GetGUID()const{return guid;}CreatureAI* AI()const{return ai;}uint32 GetUInt32Value(int){return flags;}ReactStates GetReactState(){return react;}void SetReactState(ReactStates r){react=r;}
 void SetFlag(int,uint32 f){flags|=f;}void RemoveFlag(int,uint32 f){flags&=~f;}bool IsAlive(){return alive;}bool isInCombat(){return combat;}bool isCharmed(){return charmed;}
 void RemoveCharmAuras(){charmed=false;}void RemoveCharmedBy(Unit*){charmed=false;}void CastStop(){casting=false;}void CombatStop(bool){combat=false;++stops;}void DeleteThreatList(){++threat;}void UpdateObjectVisibility(){++visibility;}
 void setActive(bool v){activeGrid=v;}void SetCanFly(bool v){fly=v;}void SetDisableGravity(bool v){noGravity=v;}void DespawnOrUnsummon(){if(!despawn)++despawns;despawn=true;}
 bool HasAura(uint32 id,ObjectGuid caster={}){for(auto a:auras)if(a.first==id&&(!caster||a.second==caster.id))return true;return false;}
 void RemoveAurasDueToSpell(uint32 id,ObjectGuid caster={}){for(auto it=auras.begin();it!=auras.end();)if(it->first==id&&(!caster||it->second==caster.id))it=auras.erase(it);else ++it;}
 void CastSpell(Unit* target,uint32 id,bool)override{++casts[id];if(!failCast.count(id))target->ToCreature()->auras.insert({id,guid.id});}
 Position const& GetHomePosition()const{return home;}float GetPositionX(){return position.GetPositionX();}float GetPositionY(){return position.GetPositionY();}float GetPositionZ(){return position.GetPositionZ();}MotionMaster* GetMotionMaster(){return &motion;}
 TempSummon* SummonCreature(uint32,Position const&,TempSummonType);
};
struct TempSummon:Creature{Creature* owner;explicit TempSummon(Creature* o):Creature(nextGuid++),owner(o){hasData=false;db=0;entry=115885;}TempSummon* ToTempSummon()override{return this;}Unit* GetSummoner(){return owner;}};
namespace ObjectAccessor{Creature* GetCreature(Creature const&,ObjectGuid id){auto it=registry.find(id.id);return it!=registry.end()&&!it->second->despawn?it->second:nullptr;}Unit* GetUnit(Unit&,ObjectGuid id){auto it=registry.find(id.id);return it==registry.end()?nullptr:it->second;}}
struct ObjectMgr{CreatureData anchor;bool available=true;ObjectMgr(){anchor.id=107830;}CreatureData const* GetCreatureData(uint64 id){return available&&id==293175?&anchor:nullptr;}}mgr;auto sObjectMgr=&mgr;
struct CreatureAI{Creature* me;explicit CreatureAI(Creature* c):me(c){}virtual ~CreatureAI()=default;virtual void InitializeAI(){}virtual void JustRespawned(){}virtual void UpdateLifecycle(uint32){}virtual bool CanBeVisibleFor(WorldObject const*)const{return true;}virtual void DamageTakenAfterScaling(Unit*,uint32&,DamageEffectType){}virtual void UpdateAI(uint32){}virtual void MoveInLineOfSight(Unit*){}virtual void AttackStart(Unit*){}virtual void EnterCombat(Unit*){}virtual void JustDied(Unit*){}virtual void DoAction(int32 const){}virtual void MovementInform(uint32,uint32){}virtual void EnterEvadeMode(){me->combat=false;}};
struct AggressorAI:CreatureAI{using CreatureAI::CreatureAI;int updates=0,deaths=0;void UpdateAI(uint32)override{++updates;}void JustDied(Unit*)override{++deaths;}};
struct SmartAI:CreatureAI{using CreatureAI::CreatureAI;int updates=0,los=0,attacks=0;void UpdateAI(uint32)override{++updates;}void MoveInLineOfSight(Unit*)override{++los;}void AttackStart(Unit*)override{++attacks;}};
struct PassiveAI:CreatureAI{using CreatureAI::CreatureAI;};
struct CreatureScript{explicit CreatureScript(char const*){}virtual ~CreatureScript()=default;virtual CreatureAI* GetAI(Creature*)const=0;};
namespace GameTime{uint64 now=100;uint64 GetGameTime(){return now;}}
struct Quest{};struct Store{Quest q;struct Rotation{uint64 ResetTime=200;}rotation;bool have=true;Quest* GetQuestTemplate(int id){assert(id==44923);return &q;}Rotation* GetWorldQuest(Quest*){return have?&rotation:nullptr;}}store;auto sQuestDataStore=&store;
#include "vrekt.h"
#include "vrekt_scene.h"
Creature::~Creature(){delete ai;registry.erase(guid.id);}
TempSummon* Creature::SummonCreature(uint32 id,Position const& pos,TempSummonType){assert(id==115885);if(failSummon)return nullptr;auto p=std::make_unique<TempSummon>(this);p->position=p->home=pos;p->ai=factory(p.get());if(p->ai)p->ai->InitializeAI();auto result=p.get();summons.push_back(std::move(p));return result;}
namespace Movement{struct MoveSplineInit{Creature& c;explicit MoveSplineInit(Unit& u):c(*u.ToCreature()){c.pending={};}void SetSmooth(){c.pending.smooth=true;}void SetFly(){c.pending.fly=true;}void SetUncompressed(){c.pending.uncompressed=true;}void MoveTo(float x,float y,float z,bool gen=true,bool=true){c.pending.destination={x,y,z,0};c.pending.generate=gen;}void SetAnimation(AnimTier a){c.pending.animation=int(a);}void SetVelocity(float f){c.pending.speed=f;}void Launch(){c.moves.push_back(c.pending);}};}
#include "SceneNativeMovement.inc"
#include "SceneNativeInform.inc"
void Finish(Creature& c){assert(c.motion.generator);auto generator=std::move(c.motion.generator);generator->Finalize(c);}
int main(){
 npc_vrekt_scene scripts;factory=[&](Creature* c){return scripts.GetAI(c);};
 for(int failure=0;failure<4;++failure){
  GameTime::now=100;store.have=true;store.rotation.ResetTime=200;failSummon=failure==1;failCast={};if(failure==2)failCast={211409};if(failure==3)failCast={230506};
  Creature boss;boss.ai=scripts.GetAI(&boss);assert(boss.ai);boss.ai->InitializeAI();boss.ai->UpdateLifecycle(1);auto& ai=*static_cast<npc_vrekt_vaettfang*>(boss.ai);
  if(failSummon){assert(!ai.drake);failSummon=false;ai.UpdateLifecycle(999);assert(!ai.drake);ai.UpdateLifecycle(1);}
  auto drake=ObjectAccessor::GetCreature(boss,ai.drake);assert(drake&&drake->entry==115885&&drake->activeGrid);auto& flight=*static_cast<npc_vrekt_captured_drake*>(drake->ai);
  assert(drake->home.GetPositionX()==mgr.anchor.posX&&drake->home.GetPositionZ()==mgr.anchor.posZ);
  assert(drake->casts[230506]==1&&drake->casts[211409]==1); // Returned summon is initialized before the first later AI tick.
  flight.UpdateLifecycle(1);assert(drake->HasAura(230506)==(failure!=3));assert(boss.HasAura(211409,drake->guid)==(failure!=2));failCast.clear();flight.UpdateLifecycle(1000);
  assert(drake->HasAura(230506)&&boss.HasAura(211409,drake->guid));int cast=drake->casts[211409];flight.UpdateLifecycle(1000);assert(drake->casts[211409]==cast);
  boss.combat=true;flight.DoAction(1);flight.UpdateLifecycle(1000);assert(!flight.released&&drake->moves.empty());uint32 damage=999;flight.DamageTakenAfterScaling(&boss,damage,DIRECT_DAMAGE);assert(!damage);
  auto total=summons.size();ai.UpdateLifecycle(1000);ai.UpdateAI(1);assert(summons.size()==total&&ai.updates==1);
  boss.auras.insert({211409,999});drake->auras.insert({777,drake->guid.id});boss.alive=false;
  if(failure%2)flight.UpdateLifecycle(1);else ai.JustDied(nullptr); // player/pet/null killer identity is irrelevant; suspended owner AI may omit the original JustDied
  assert(flight.released&&!boss.HasAura(211409,drake->guid)&&boss.HasAura(211409,{999})&&!drake->HasAura(230506)&&drake->HasAura(777));
  flight.DoAction(1);flight.MovementInform(EFFECT_MOTION_TYPE,1);assert(drake->moves.empty());flight.UpdateLifecycle(999);assert(drake->moves.empty());flight.UpdateLifecycle(1);
  assert(drake->moves.size()==1&&drake->fly&&drake->noGravity&&drake->moves[0].animation==int(AnimTier::Hover)&&drake->moves[0].destination.GetPositionZ()==mgr.anchor.posZ+8);
  flight.MovementInform(POINT_MOTION_TYPE,1);assert(drake->moves.size()==1);Finish(*drake);assert(drake->moves.size()==2);
  auto move=drake->moves.back();assert(move.fly&&move.smooth&&move.uncompressed&&!move.generate&&move.speed==9&&move.destination.GetPositionX()==mgr.anchor.posX-45&&move.destination.GetPositionY()==mgr.anchor.posY+60&&move.destination.GetPositionZ()==mgr.anchor.posZ+60);
  flight.MovementInform(EFFECT_MOTION_TYPE,1);assert(drake->moves.size()==2);Finish(*drake);assert(drake->despawn&&drake->despawns==1);flight.MovementInform(EFFECT_MOTION_TYPE,2);assert(drake->despawns==1);
  boss.alive=true;ai.JustRespawned();ai.UpdateLifecycle(1000);auto next=ObjectAccessor::GetCreature(boss,ai.drake);assert(next&&next!=drake);GameTime::now=200;ai.UpdateLifecycle(1);assert(next->despawn&&!ai.drake);summons.clear();
 }
 // Missing anchor / externally bound summon must retry without leaking a copy.
 GameTime::now=100;Creature boss;boss.ai=scripts.GetAI(&boss);auto& ai=*static_cast<npc_vrekt_vaettfang*>(boss.ai);mgr.available=false;ai.UpdateLifecycle(1);assert(!ai.drake);mgr.available=true;
 factory=[](Creature*){return nullptr;};ai.UpdateLifecycle(1000);assert(!ai.drake&&summons.back()->despawn);factory=[&](Creature* c){return scripts.GetAI(c);};ai.UpdateLifecycle(1000);auto drake=ObjectAccessor::GetCreature(boss,ai.drake);assert(drake);
 // A removed summon cannot leave an old caster's chain on the boss when replaced.
 auto oldGuid=drake->guid;assert(boss.HasAura(211409,oldGuid));drake->DespawnOrUnsummon();ai.UpdateLifecycle(1000);assert(!boss.HasAura(211409,oldGuid));drake=ObjectAccessor::GetCreature(boss,ai.drake);assert(drake&&drake->guid.id!=oldGuid.id);
 auto& flight=*static_cast<npc_vrekt_captured_drake*>(drake->ai);boss.alive=false;flight.UpdateLifecycle(1);flight.UpdateLifecycle(20000);assert(drake->despawn);summons.clear();
 // Disappearing owner and rotation expiry during flight clean up, without a fake completion callback.
 for(int reason=0;reason<2;++reason){
  GameTime::now=100;Creature owner(10);owner.ai=scripts.GetAI(&owner);owner.ai->UpdateLifecycle(1);auto& controller=*static_cast<npc_vrekt_vaettfang*>(owner.ai);auto actor=ObjectAccessor::GetCreature(owner,controller.drake);assert(actor);auto& flying=*static_cast<npc_vrekt_captured_drake*>(actor->ai);
  if(!reason){actor->ToTempSummon()->owner=nullptr;flying.UpdateLifecycle(1);assert(actor->despawn&&!flying.released);}
  else{owner.alive=false;flying.UpdateLifecycle(1);flying.UpdateLifecycle(1000);GameTime::now=200;Finish(*actor);assert(actor->despawn&&actor->moves.size()==1);controller.UpdateLifecycle(1);assert(!controller.drake);}
  summons.clear();
 }
 GameTime::now=100;
 // Native village SmartAI returns after the shared rotation, without losing other flags or charm cleanup.
 Creature village(2);village.entry=107830;village.db=293175;village.flags=32768|0x10000000;village.ai=scripts.GetAI(&village);auto& ambient=*static_cast<npc_vrekt_village_drake*>(village.ai);
 village.combat=village.casting=village.charmed=true;village.LastCharmerGUID={3};ambient.UpdateLifecycle(1);assert(!ambient.CanBeVisibleFor(nullptr)&&!village.combat&&!village.casting&&!village.charmed&&!village.LastCharmerGUID);
 ambient.UpdateAI(1);ambient.AttackStart(&boss);ambient.MoveInLineOfSight(&boss);assert(!ambient.updates&&!ambient.attacks&&!ambient.los);uint32 damage=100;ambient.DamageTakenAfterScaling(&boss,damage,DIRECT_DAMAGE);assert(!damage);
 village.flags=32768|0x10000000;village.react=REACT_AGGRESSIVE;ambient.JustRespawned();assert(village.react==REACT_PASSIVE&&(village.flags&ambient.GateFlags)==ambient.GateFlags);
 GameTime::now=200;ambient.UpdateLifecycle(1);ambient.UpdateAI(1);ambient.AttackStart(&boss);ambient.MoveInLineOfSight(&boss);assert(ambient.updates==1&&ambient.attacks==1&&ambient.los==1&&ambient.CanBeVisibleFor(nullptr)&&village.react==REACT_AGGRESSIVE&&(village.flags&0x10000000)&&!(village.flags&ambient.GateFlags));
 Creature outside(3);outside.db=5;assert(!scripts.GetAI(&outside));outside.entry=107830;assert(!scripts.GetAI(&outside));TempSummon unrelated(&outside);assert(!scripts.GetAI(&unrelated));
 std::cout<<"PASS: production Vrekt scene lifecycle, exact summon ownership, failure retries, death-only release, native takeoff/fly spline setup and EffectMovementGenerator callbacks, timeout/expiry/respawn, village SmartAI restoration and external scope. World, spells, packets and spline travel remain fixtures.\n";
}
