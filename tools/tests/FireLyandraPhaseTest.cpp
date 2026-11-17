// Production controller plus native aura93/combat-stop/attack admission,
// threat-victim/SmartAI dispatch, regeneration and transient-action teardown.
// World lookup, movement, threat-container ranking, spell effects and respawn
// endpoints are controlled. This does not simulate a complete client/map.
#define _USE_MATH_DEFINES
#include <cmath>
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>
using uint32=uint32_t;using uint64=uint64_t;using int64=int64_t;using uint8=uint8_t;
#include "Constants.inc"
struct ObjectGuid {uint64 value=0;explicit operator bool()const{return value!=0;}bool IsEmpty()const{return !value;}bool IsPlayer()const{return false;}bool operator==(ObjectGuid const& b)const{return value==b.value;}bool operator!=(ObjectGuid const& b)const{return !(*this==b);}bool operator<(ObjectGuid const& b)const{return value<b.value;}static ObjectGuid const Empty;};ObjectGuid const ObjectGuid::Empty{};
struct Unit;using Creature=Unit;using Player=Unit;using WorldObject=Unit;using TempSummon=Unit;
struct CreatureAI;struct SmartAI;struct SmartScript;
using ObjectList=std::list<Unit*>;
using ControlList=std::set<ObjectGuid>;using UnitSet=std::set<Unit*>;using MovementGeneratorType=uint32;
uint32 warnings=0,errors=0,progress=0,baseDeaths=0;
bool expectedDiagnostic=false;
template<class... Args>void Warn(char const*,char const*format,Args...args){assert(expectedDiagnostic);++warnings;std::printf("COUNTERFACTUAL expected: ");std::printf(format,args...);std::puts("");}
template<class... Args>void Error(char const*,char const*format,Args...args){assert(expectedDiagnostic);++errors;std::printf("COUNTERFACTUAL expected: ");std::printf(format,args...);std::puts("");}
#define TC_LOG_WARN(...) Warn(__VA_ARGS__)
#define TC_LOG_ERROR(...) Error(__VA_ARGS__)
#define TC_LOG_DEBUG(...) ((void)0)
struct CreatureTemplate {uint32 ScriptID=0,unit_flags=0;std::string AIName="SmartAI";};
struct Scenario {uint32 id=957,step=3;uint32 GetScenarioId()const{return id;}uint32 GetCurrentStep()const{return step;}} currentScenario;
struct Scenarios {Scenario* GetScenario(uint32 id){return id==1?&currentScenario:nullptr;}} scenarios;auto sScenarioMgr=&scenarios;
struct Map {uint32 id=1480;bool IsDungeon()const{return true;}} map,otherMap{1481};
struct Motion {uint32 type=0;uint32 GetMotionSlotType(uint32){return type;}void MovementExpired(bool){}void MoveChase(Unit*){}void MoveTargetedHome(){}void MoveFollow(Unit*,float,float){}void MoveIdle(){}};
struct AuraBase {bool IsPermanent()const{return false;}};
struct AuraEffect {uint32 id=194360;Unit* caster=nullptr;AuraBase base;Unit* GetCaster()const{return caster;}AuraBase const* GetBase()const{return &base;}uint32 GetId()const{return id;}void HandleModUnattackable(struct AuraApplication const*,uint8,bool)const;};
using AuraEffectList=std::list<AuraEffect const*>;
struct AuraApplication {Unit* target;Unit* GetTarget()const{return target;}};
struct Threat {Unit* me=nullptr;std::vector<Unit*> actors;bool isThreatListEmpty()const{return actors.empty();}Unit* getHostilTarget();};
struct Formation {void MemberAttackStart(Unit*,Unit*){}};
struct Duel {ObjectGuid opponent;};
struct Vehicle {void Reset(bool=true){}};
struct World {float getRate(uint32)const{return 1;}bool getBoolConfig(uint32)const{return false;}} world;auto sWorld=&world;
struct Unit {
 using AuraEffectList=::AuraEffectList;
 ObjectGuid guid;uint32 entry=99615,dbGuid=370120,phase=1,scope=1,state=0,flags=0,react=2;Map* map=&::map;
 bool player=false,alive=true,world=true,expired=false,locked=false,scoped=true,regen=true,home=false,gm=false;float x=0;
 uint64 health=50,maximum=100;uint32 m_CombatTimer=0,attackCalls=0,regenCalls=0,enters=0,respawns=0,initializations=0,despawns=0;
 uint32 casts=0;Unit* currentVictim=nullptr;Unit* ownerUnit=nullptr;CreatureAI* ai=nullptr;std::unique_ptr<CreatureAI> ownedAI;
 CreatureTemplate tpl;Threat m_ThreatManager;Motion motion;AuraEffectList taunts,invisibility;std::set<uint32> auras;std::set<Unit*> m_attackers;
 Duel* duel=nullptr;ObjectGuid summoner;std::vector<uint32> dataRequests;std::function<void(uint32,uint32)> onData;
 Unit();~Unit();Unit(Unit const&)=delete;Unit& operator=(Unit const&)=delete;
 bool IsAlive()const{return alive;}bool isDead()const{return !alive;}bool IsInWorld()const{return world;}bool IsQuestObjectExpired()const{return expired;}
 bool IsDespawn()const{return !world;}
 uint32 GetEntry()const{return entry;}uint32 GetDBTableGUIDLow()const{return dbGuid;}ObjectGuid GetGUID()const{return guid;}uint32 GetGUIDLow()const{return uint32(guid.value);}
 Unit* ToCreature(){return player?nullptr:this;}Unit const* ToCreature()const{return player?nullptr:this;}Unit* ToPlayer(){return player?this:nullptr;}Unit* ToTempSummon(){return summoner.IsEmpty()?nullptr:this;}
 CreatureAI* AI(){return ai;}CreatureAI const* AI()const{return ai;}CreatureTemplate const* GetCreatureTemplate()const{return &tpl;}
 bool CanTargetQuestObject(Unit* actor)const{return actor&&(!scoped||!actor->scoped||scope==actor->scope);}
 bool InSamePhase(Unit const*actor)const{return actor&&phase==actor->phase;}
 Map* GetMap(){return map;}uint32 GetMapId()const{return map->id;}uint32 GetInstanceId()const{return 1;}
 bool HasAura(uint32 id)const{return auras.count(id)!=0;}bool HasAuraType(uint32 type)const{return type==SPELL_AURA_MOD_UNATTACKABLE&&HasAura(194360);}
 void RemoveAurasByType(uint32 type){if(type==SPELL_AURA_MOD_UNATTACKABLE){auras.erase(194360);flags&=~UNIT_FLAG_NON_ATTACKABLE;}}
 uint32 GetReactState()const{return react;}void SetReactState(uint32 r){react=r;}bool HasReactState(uint32 r)const{return react==r;}
 bool IsPlayer()const{return player;}bool IsMounted()const{return false;}bool isGameMaster()const{return gm;}bool IsPvP()const{return false;}bool IsFFAPvP()const{return false;}
 bool IsInEvadeMode()const{return (state&UNIT_STATE_EVADE)!=0;}bool HasUnitState(uint32 s)const{return (state&s)!=0;}void AddUnitState(uint32 s){state|=s;}void ClearUnitState(uint32 s){state&=~s;}
 bool HasFlag(uint32,uint32 f)const{return (flags&f)!=0;}void SetFlag(uint32,uint32 f){flags|=f;}void RemoveFlag(uint32,uint32 f){flags&=~f;}void ApplyModFlag(uint32,uint32 f,bool apply){if(apply)flags|=f;else flags&=~f;}
 bool isInCombat()const{return HasFlag(0,UNIT_FLAG_IN_COMBAT);}Unit* getVictim(){return currentVictim;}Threat& getThreatManager(){return m_ThreatManager;}
 AuraEffectList const& GetAuraEffectsByType(uint32 type)const{return type==SPELL_AURA_MOD_TAUNT?taunts:invisibility;}
 bool CanHaveThreatList()const{return !player;}bool IsFriendlyTo(Unit const* u)const{return !u->player&&u->entry==99763;}
 bool canCreatureAttack(Unit const* u)const{return u&&u->alive&&u->world&&!u->gm&&u->map==map&&u->phase==phase&&u->scope==scope&&!IsFriendlyTo(u);}
 bool canSeeOrDetect(Unit const*,bool)const{return true;}bool IsValidAttackTarget(Unit const* u)const{return canCreatureAttack(u);}bool isInAccessiblePlaceFor(Unit const*)const{return true;}
 void SetInFront(Unit*){}Unit* getAttackerForHelper(){return nullptr;}bool isSummon()const{return false;}Unit* GetOwner(){return nullptr;}Unit* ToTempSummon()const{return nullptr;}
 Unit* GetCharmerOrOwner(){return ownerUnit;}Unit* GetCharmerOrOwnerOrSelf(){return ownerUnit?ownerUnit:this;}bool HasUnitTypeMask(uint32)const{return false;}std::set<ObjectGuid> m_Controlled;
 Unit* GetCharmerOrOwnerPlayerOrPlayerItself(){return player?this:ownerUnit&&ownerUnit->player?ownerUnit:nullptr;}
 Unit* SelectVictim();Unit* SelectNearestTargetInAttackDistance(float){return nullptr;}bool _IsTargetAcceptable(Unit* u){return canCreatureAttack(u);}float m_CombatDistance=0;bool IsAIEnabled=true;
 Vehicle* GetVehicle(){return nullptr;}Vehicle* GetVehicleKit(){return nullptr;}bool IsVehicle()const{return false;}
 Motion* GetMotionMaster(){return &motion;}void StopMoving(){}void SetHomePosition(float,float,float){}float GetPositionX()const{return x;}float GetPositionY()const{return 0;}float GetPositionZ()const{return 0;}float GetOrientation()const{return 0;}
 float GetPosition()const{return x;}
 bool Attack(Unit*,bool);bool AttackStop(){currentVictim=nullptr;return true;}void RemoveAllAttackers(){}void CombatStop(bool=false);void ClearInCombat(){m_CombatTimer=0;flags&=~UNIT_FLAG_IN_COMBAT;}
 bool IsNonMeleeSpellCast(bool)const{return false;}void InterruptNonMeleeSpells(bool){}void SendAttackSwingError(uint32){}
 uint32 RemoveAurasWithInterruptFlags(uint32,uint32){return 0;}void SetInCombatWith(Unit*);void SetInCombatState(Unit*,bool=false);
 void SetCombatTimer(uint32 t){m_CombatTimer=t;}void UpdatePowerState(bool=true){}void SetContestedPvP(Unit*){}void UpdatePvP(bool){}
 uint64 GetHealth()const{return health;}uint64 GetMaxHealth()const{return maximum;}bool isRegeneratingHealth()const{return regen;}ObjectGuid GetCharmerOrOwnerGUID()const{return {};}
 float GetTotalAuraMultiplier(uint32)const{return 1;}int GetTotalAuraModifier(uint32)const{return 0;}void ModifyHealth(uint64 h){++regenCalls;health=std::min(maximum,health+h);}void RegenerateHealth();bool IsPolymorphed()const{return false;}
 void DeleteThreatList(){m_ThreatManager.actors.clear();}void ClearSaveThreatTarget(){}void RemoveAllAurasExceptType(uint32){auras.clear();flags&=~UNIT_FLAG_NON_ATTACKABLE;}
 bool isAnySummons()const{return false;}bool isTrainingDummy()const{return false;}void LoadCreaturesAddon(bool=false){}void SetLootRecipient(Unit*){}void SetCannotReachTarget(bool){}void ResetPlayerDamageReq(){}
 uint32 GetWaypointPath()const{return 0;}Formation* GetFormation(){return nullptr;}void ResumeMovement(){}void SetWalk(bool){}void SetSpawnHealth(){health=maximum;}
 bool AIM_Initialize(CreatureAI* =nullptr);void Respawn(bool,uint32);void DespawnOrUnsummon(){++despawns;world=false;CombatStop();}ObjectGuid GetSummonerGUID()const{return summoner;}
 void GetCreatureListWithEntryInGrid(std::list<Unit*>&,uint32,float);Unit* FindNearestCreature(uint32,float,bool=true);bool IsWithinDistInMap(Unit const*,float)const;
 static Unit* GetPlayer(Unit const&,ObjectGuid);void GroupEventHappens(uint32,Unit*){}void RewardPlayerAndGroupAtEvent(uint32,Unit*){}
};
std::map<uint64,Unit*> objects;uint64 nextGuid=0;
namespace ObjectAccessor {Unit* GetCreature(Unit const&,ObjectGuid g){auto i=objects.find(g.value);return i==objects.end()?nullptr:i->second;}Unit* GetUnit(Unit const&u,ObjectGuid g){return GetCreature(u,g);}}
Unit* Unit::GetPlayer(Unit const&u,ObjectGuid g){return ObjectAccessor::GetUnit(u,g);}
struct SmartScriptHolder {
 int64 entryOrGuid=0;uint32 source_type=0,event_id=0,link=0,type=0,actionType=0,targetType=0;
 bool enableTimed=true;
 struct {uint32 event_phase_mask=0,event_chance=100,event_flags=1;struct {uint32 param1=0,param2=0,param3=0,param4=0,param5=0;}raw;}event;
 struct {struct {uint32 param1=0,param2=0,param3=0,param4=0,param5=0,param6=0;}raw;}action;
 struct {float x=0,y=0,z=0,o=0;struct {uint32 param1=0,param2=0,param3=0,param4=0;}raw;}target;
 uint32 GetScriptType()const{return source_type;}uint32 GetEventType()const{return type;}uint32 GetActionType()const{return actionType;}uint32 GetTargetType()const{return targetType;}
};
using SmartAIEventList=std::vector<SmartScriptHolder>;
struct ScriptMgr {SmartAIEventList rows;SmartAIEventList GetScript(int64 entry,uint32 source){SmartAIEventList out;for(auto const&r:rows)if(r.entryOrGuid==entry&&r.source_type==source)out.push_back(r);return out;}} scriptMgr;auto sSmartScriptMgr=&scriptMgr;
struct Conditions {bool custom=false;std::vector<int> GetConditionsForSmartEvent(uint32,uint32,uint32){return custom?std::vector<int>{1}:std::vector<int>{};}}conditions;auto sConditionMgr=&conditions;
struct SmartScript {
 Unit* me;explicit SmartScript(Unit*u):me(u){}bool isProcessingTimedActionList=false,mUseTextTimer=false,mCancelTimedActionList=false;uint32 mTextTimer=0,mLastTextID=0,mTalkerEntry=0,mScriptType=0;
 ObjectGuid mTextGUID;SmartAIEventList mTimedActionList,mEvents,mStoredEvents;std::vector<uint32>mRemIDs;std::set<std::tuple<ObjectGuid,int64,uint32>>mCompletedLinkedCasts;
 std::function<void(uint32)> onUpdate;uint32 updates=0,resets=0,textCallbacks=0,epoch=0;
 void CancelPendingActions();bool HasCompletedLinkedCast(int64,uint32)const;
 void ResetLinkedCastRetries(){++epoch;mCompletedLinkedCasts.clear();}void OnReset(){++resets;ResetLinkedCastRetries();}
 std::function<void(SmartScriptHolder&,uint32)> timerCallback;
 void OnUpdate(uint32);Unit*GetBaseObject(){return me;}void InstallEvents(){}void UpdateLinkedCastRetries(uint32 diff){++updates;if(onUpdate)onUpdate(diff);}
 void UpdateTimer(SmartScriptHolder&e,uint32 diff){if(timerCallback)timerCallback(e,diff);}void RemoveStoredEvent(uint32){}
 void ProcessEventsFor(uint32 event,...){if(event==SMART_EVENT_TEXT_OVER)++textCallbacks;}ObjectList* GetTargets(SmartScriptHolder const&e){auto*out=new ObjectList();if(Unit*w=me->FindNearestCreature(e.target.raw.param1,float(e.target.raw.param2)))out->push_back(w);return out;}
};
#include "CancelPending.inc"
#include "LinkedReceipt.inc"
#include "ScriptUpdate.inc"
struct CreatureAI {
 Unit* me;explicit CreatureAI(Unit*u):me(u){}virtual ~CreatureAI()=default;
 virtual void AttackStart(Unit*){}virtual void UpdateAI(uint32){}virtual void EnterEvadeMode(){}virtual void Reset(){}virtual void JustReachedHome(){}virtual void JustDied(Unit*){}
 virtual void SetData(uint32 id,uint32 value){me->dataRequests.push_back(id);if(me->onData)me->onData(id,value);}virtual void InitializeAI(){}
 bool UpdateVictim();bool _EnterEvadeMode();bool CanAIAttack(Unit const*)const{return true;}
};
#include "UpdateVictim.inc"
#include "EnterEvade.inc"
struct SmartAI:CreatureAI {
 SmartScript script;explicit SmartAI(Unit*u):CreatureAI(u),script(u){}SmartScript*GetScript(){return &script;}
 bool mCanAutoAttack=true,mCanCombatMove=true,mEvadeDisabled=false,mRun=true;
 float mLastOOCPos=0;
 ObjectGuid mFollowGuid;uint32 mFollowArrivedTimer=0,mFollowArrivedEntry=0,mFollowCreditType=0,mFollowCredit=0;float mFollowDist=0,mFollowAngle=0;
 void UpdateAI(uint32)override;void AttackStart(Unit*)override;void EnterEvadeMode()override;
 void Reset()override{script.OnReset();}void JustReachedHome()override{}void JustDied(Unit*)override{++baseDeaths;++progress;currentScenario.step=4;}
 void InitializeAI()override{++me->initializations;script.OnReset();}
 bool IsAIControlled()const{return true;}void SetRun(bool){}void UpdatePath(uint32){}void UpdateDespawn(uint32){}void SetDespawnTime(uint32){}void StartDespawn(){}
 void DoMeleeAttackIfReady(){assert(!me->HasAura(194360));}bool HasEscortState(uint32)const{return false;}void AddEscortState(uint32){}void ReturnToLastOOCPos(){}
};
#include "SmartUpdate.inc"
#include "SmartAttack.inc"
#include "SmartEvade.inc"
Unit::Unit(){guid={++nextGuid};objects[guid.value]=this;m_ThreatManager.me=this;}
Unit::~Unit(){objects.erase(guid.value);}
Unit* Threat::getHostilTarget(){for(Unit*u:actors)if(me->canCreatureAttack(u))return u;return nullptr;}
Unit* Unit::SelectVictim(){
#include "SelectVictimBody.inc"
}
bool Unit::Attack(Unit* target,bool meleeAttack){(void)meleeAttack;Unit* victim=target;
#include "AttackPrefix.inc"
 ++attackCalls;this->currentVictim=victim;return true;
}
#include "CombatStop.inc"
#include "SetCombat.inc"
void Unit::SetInCombatState(Unit*,bool){if(!alive||IsInEvadeMode())return;m_CombatTimer=5000;if(!isInCombat()){flags|=UNIT_FLAG_IN_COMBAT;++enters;}}
#include "Unattackable.inc"
#include "Regenerate.inc"
bool Unit::AIM_Initialize(CreatureAI*){if(locked)return false;ownedAI.reset(new SmartAI(this));ai=ownedAI.get();if(alive)ai->InitializeAI();return true;}
void Unit::Respawn(bool force,uint32 delay){assert(force&&delay==1);++respawns;alive=true;world=true;health=maximum;flags=tpl.unit_flags;auras={195699};currentVictim=nullptr;m_ThreatManager.actors.clear();}
bool Unit::IsWithinDistInMap(Unit const*u,float range)const{return u&&u->world&&map==u->map&&phase==u->phase&&std::abs(x-u->x)<=range;}
Unit* Unit::FindNearestCreature(uint32 id,float range,bool wantAlive){Unit*out=nullptr;for(auto const&i:objects)if(i.second!=this&&i.second->entry==id&&i.second->alive==wantAlive&&IsWithinDistInMap(i.second,range)&&CanTargetQuestObject(i.second)){out=i.second;range=std::abs(x-out->x);}return out;}
void Unit::GetCreatureListWithEntryInGrid(std::list<Unit*>&out,uint32 id,float range){for(auto const&i:objects)if(i.second->entry==id&&IsWithinDistInMap(i.second,range))out.push_back(i.second);}
#include "fire_lyandra_phase.h"
static void Wreath(Unit&u){u.auras.insert(194360);AuraApplication app{&u};AuraEffect effect;effect.HandleModUnattackable(&app,AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK|AURA_EFFECT_HANDLE_REAL,true);}
static void RegenTick(Unit&u){auto& actor=u;auto* me=&actor;(void)me;
#include "RegenTick.inc"
}
static void Read(char const*path){std::ifstream f(path);assert(f);std::string line;while(std::getline(f,line)){std::stringstream ss(line);std::vector<std::string>a;std::string cell;while(std::getline(ss,cell,'\t'))a.push_back(cell);assert(a.size()==31);auto n=[&](size_t i){return uint32(std::stoul(a[i]));};SmartScriptHolder r;r.entryOrGuid=std::stoll(a[0]);r.source_type=n(1);r.event_id=n(2);r.link=n(3);r.type=n(5);r.event.event_phase_mask=n(6);r.event.event_chance=n(7);r.event.event_flags=n(8);r.event.raw={n(9),n(10),n(11),n(12),n(13)};r.actionType=n(14);r.action.raw={n(15),n(16),n(17),n(18),n(19),n(20)};r.targetType=n(21);r.target.raw={n(22),n(23),n(24),n(25)};r.target.x=std::stof(a[26]);r.target.y=std::stof(a[27]);r.target.z=std::stof(a[28]);r.target.o=std::stof(a[29]);scriptMgr.rows.push_back(r);}}

static void Home(Unit& owner){bool _arrived=true;
#include "HomeFinalize.inc"
}
struct Encounter {
 Unit player,wing,body,aethas,foreignAethas; npc_fire_artifact_lyandra boss;
 Encounter():boss(&body){
  currentScenario={957,3};warnings=errors=progress=baseDeaths=0;expectedDiagnostic=false;conditions.custom=false;
  player.player=true;player.entry=1;wing.entry=98686;wing.dbGuid=370121;wing.tpl.unit_flags=33555200;wing.flags=33555200;wing.auras={195699};wing.x=20;wing.AIM_Initialize();
  body.m_ThreatManager.actors={&player};body.flags=UNIT_FLAG_IN_COMBAT;body.currentVictim=&player;body.ai=&boss;
  aethas.entry=foreignAethas.entry=99763;aethas.summoner=body.GetGUID();foreignAethas.summoner=player.GetGUID();aethas.world=foreignAethas.world=false;
 }
 void Enter(bool receipt=true,bool startWing=true){
  Wreath(body);if(receipt)boss.GetScript()->mCompletedLinkedCasts.insert({body.GetGUID(),99615,19});
  if(receipt&&startWing&&wing.world)wing.AI()->SetData(1,1);
 }
 void Awake(){wing.auras.erase(195699);wing.flags&=~33555200u;}
 void OwnedActors(){aethas.world=foreignAethas.world=true;}
 void Quiet()const{assert(!warnings&&!errors&&!progress&&!baseDeaths);}
};
int main(int argc,char**argv){
 _set_abort_behavior(0,_CALL_REPORTFAULT);std::setvbuf(stdout,nullptr,_IONBF,0);assert(argc==2);Read(argv[1]);assert(FireLyandra::NativeWingData());
 auto native=scriptMgr.rows;uint32 cases=0;
 auto row=[&](uint32 id)->SmartScriptHolder const&{for(auto const&r:scriptMgr.rows)if(r.entryOrGuid==99615&&r.source_type==0&&r.event_id==id)return r;std::abort();};
 assert(row(17).action.raw.param1==194360&&row(19).action.raw.param1==195739&&row(19).action.raw.param2==512&&!row(21).link&&!row(31).link);
 {Encounter e;e.Enter();assert(!e.body.isInCombat()&&!e.body.currentVictim&&e.body.m_ThreatManager.actors.size()==1);RegenTick(e.body);assert(e.body.health==100);++cases;}
 {Encounter e;e.Enter();e.boss.SmartAI::AttackStart(&e.player);assert(e.body.attackCalls==1&&!e.body.HasAura(194360));++cases;}
 for(unsigned invoker=0;invoker<3;++invoker)for(bool taunt:{false,true}){
  Encounter e;Unit pet;pet.entry=90001;pet.ownerUnit=&e.player;pet.x=2;Unit guardian;guardian.entry=90002;guardian.ownerUnit=&e.player;
  Unit*target=invoker==0?&e.player:invoker==1?&pet:&guardian;e.body.m_ThreatManager.actors={target,&e.player};e.body.currentVictim=target;
  AuraEffect tauntEffect;tauntEffect.caster=target;if(taunt)e.body.taunts.push_back(&tauntEffect);
  e.Enter();e.boss.UpdateAI(1);assert(e.body.isInCombat()&&e.body.HasAura(194360)&&!e.body.currentVictim);RegenTick(e.body);assert(e.body.health==50&&!e.body.regenCalls);
  for(int i=0;i<5;++i){e.boss.AttackStart(target);e.boss.UpdateAI(100);assert(!e.body.attackCalls&&e.body.HasAura(194360)&&e.wing.dataRequests.size()==1);}
  e.Awake();e.boss.UpdateAI(60000);assert(e.body.health==50&&e.body.HasAura(194360)&&e.wing.respawns==0&&e.wing.dataRequests.size()==1);
  // Actual native DATA4/RemoveAllAuras is separately exercised by the Fire
  // SmartAI event host. Here its aura endpoint is controlled explicitly.
  e.wing.alive=false;e.body.taunts.clear();e.body.RemoveAllAurasExceptType(0);e.boss.UpdateAI(1);assert(e.body.currentVictim==target&&e.body.attackCalls==1);
  e.Quiet();++cases;
 }
 for(unsigned lost=0;lost<5;++lost){
  Encounter e;e.Enter();e.boss.UpdateAI(1);e.OwnedActors();e.boss.GetScript()->mUseTextTimer=true;e.boss.GetScript()->mTextTimer=7000;e.boss.GetScript()->mTimedActionList.resize(2);
  if(lost==0)e.player.alive=false;if(lost==1)e.player.world=false;if(lost==2)e.player.map=&otherMap;if(lost==3)e.player.phase=2;if(lost==4)e.player.gm=true;
  e.boss.UpdateAI(1);assert(e.body.IsInEvadeMode()&&!e.body.HasAura(194360)&&!e.boss.GetScript()->mUseTextTimer&&e.boss.GetScript()->mTimedActionList.empty());
  assert(!e.boss.GetScript()->HasCompletedLinkedCast(99615,19)&&e.aethas.despawns==1&&!e.foreignAethas.despawns);
  e.boss.UpdateAI(1);assert(e.wing.respawns==1&&e.wing.HasAura(195699)&&e.wing.HasFlag(0,33555200));e.boss.UpdateAI(10000);assert(!e.boss.GetScript()->textCallbacks);
  Home(e.body);assert(!e.body.IsInEvadeMode()&&e.body.health==100);e.player.alive=e.player.world=true;e.player.map=&map;e.player.phase=1;e.player.gm=false;
  e.body.m_ThreatManager.actors={&e.player};e.body.flags|=UNIT_FLAG_IN_COMBAT;e.body.currentVictim=&e.player;e.body.health=50;e.Enter();e.boss.UpdateAI(1);assert(e.body.HasAura(194360)&&e.body.isInCombat());e.Quiet();++cases;
 }
 {Encounter e;e.Enter(false);e.boss.UpdateAI(14999);assert(e.wing.dataRequests.empty()&&e.body.HasAura(194360));expectedDiagnostic=true;e.boss.UpdateAI(1);assert(warnings==1&&!errors&&!progress&&e.body.IsInEvadeMode());e.boss.UpdateAI(1);assert(e.wing.respawns==1&&!baseDeaths);++cases;}
 {Encounter e;e.Enter();e.boss.UpdateAI(14999);assert(e.wing.dataRequests.size()==1);e.boss.UpdateAI(1);assert(e.wing.dataRequests.size()==2&&!e.wing.respawns);e.boss.UpdateAI(15000);assert(e.wing.dataRequests.size()==3);expectedDiagnostic=true;e.boss.UpdateAI(15000);assert(warnings==1&&e.body.IsInEvadeMode()&&!progress);e.boss.UpdateAI(1);assert(e.wing.respawns==1);++cases;}
 {Encounter e;e.wing.world=false;e.Enter();e.boss.UpdateAI(10000);assert(e.wing.dataRequests.empty());e.wing.world=true;e.boss.UpdateAI(5000);assert(e.wing.dataRequests.size()==1);e.Awake();e.boss.UpdateAI(15000);e.Quiet();++cases;}
 {Encounter e;e.wing.world=false;e.Enter();e.boss.UpdateAI(29999);assert(!warnings);expectedDiagnostic=true;e.boss.UpdateAI(1);assert(warnings==1&&!progress&&e.body.IsInEvadeMode()&&!e.wing.respawns);++cases;}
 {Encounter e;e.Enter();e.boss.UpdateAI(1);e.wing.alive=false;e.boss.UpdateAI(999);assert(!warnings);expectedDiagnostic=true;e.boss.UpdateAI(1);assert(warnings==1&&!progress&&!baseDeaths);e.boss.UpdateAI(1);assert(e.wing.respawns==1&&e.wing.initializations==2);++cases;}
 for(unsigned mode=0;mode<5;++mode){Encounter e;e.Enter();e.boss.UpdateAI(1);e.boss.EnterEvadeMode();e.wing.locked=true;for(unsigned i=0;i<4;++i)e.boss.UpdateAI(500);assert(!warnings&&!e.wing.respawns);if(mode==0){e.wing.locked=false;e.boss.UpdateAI(500);assert(e.wing.respawns==1);e.Quiet();}else if(mode==1){expectedDiagnostic=true;e.boss.UpdateAI(500);assert(warnings==1&&!e.wing.respawns);e.wing.locked=false;e.boss.UpdateAI(5000);assert(!e.wing.respawns);}else if(mode==2){currentScenario.step=4;e.boss.UpdateAI(5000);assert(!e.wing.respawns&&!warnings);}else if(mode==3){e.body.expired=true;e.boss.UpdateAI(5000);assert(!e.wing.respawns&&!warnings);}else{e.body.alive=false;e.boss.UpdateAI(5000);assert(!e.wing.respawns&&!warnings);}assert(!progress);++cases;}
 for(unsigned mode=0;mode<5;++mode){Encounter e;e.Enter();e.boss.UpdateAI(1);e.Awake();e.OwnedActors();if(mode==0)e.wing.alive=false;if(mode==1)e.wing.world=false;if(mode==2)e.wing.phase=2;if(mode==3)e.wing.scope=2;e.boss.JustDied(&e.player);assert(baseDeaths==1&&progress==1&&e.aethas.world&&e.foreignAethas.world);assert(e.wing.despawns==unsigned(mode==4));e.boss.UpdateAI(1);assert(!e.wing.respawns&&!warnings&&!errors);++cases;}
 {Encounter e;e.OwnedActors();e.boss.JustDied(&e.player);assert(baseDeaths==1&&progress==1&&!e.wing.despawns&&!e.aethas.despawns);++cases;}
 for(unsigned mode=0;mode<5;++mode){Encounter e;if(mode==0)e.body.map=&otherMap;if(mode==1)currentScenario.id=958;if(mode==2)currentScenario.step=2;if(mode==3)currentScenario.step=4;if(mode==4)e.body.entry=123;Wreath(e.body);e.boss.AttackStart(&e.player);assert(!e.body.HasAura(194360)&&e.body.attackCalls==1);assert(!e.wing.respawns&&!e.wing.despawns);++cases;}
 {Encounter e;e.Enter();e.boss.UpdateAI(1);e.boss.GetScript()->mUseTextTimer=true;e.boss.GetScript()->mTextTimer=1;e.boss.GetScript()->mTimedActionList.resize(3);uint32 ran=0;e.boss.GetScript()->timerCallback=[&](SmartScriptHolder&,uint32){++ran;e.boss.Reset();};e.boss.UpdateAI(1);assert(ran==1&&!e.boss.GetScript()->isProcessingTimedActionList&&!e.boss.GetScript()->mCancelTimedActionList&&e.boss.GetScript()->mTimedActionList.empty()&&!e.boss.GetScript()->mUseTextTimer&&!e.boss.GetScript()->textCallbacks);e.boss.UpdateAI(10000);assert(ran==1&&!e.boss.GetScript()->textCallbacks&&!progress);++cases;}
 {Encounter e;SmartScript s(&e.body);s.mTimedActionList.resize(3);uint32 ran=0;s.timerCallback=[&](SmartScriptHolder&,uint32){++ran;};s.OnUpdate(1);assert(ran==3&&!s.mCancelTimedActionList);s.mUseTextTimer=true;s.mTextTimer=1;s.OnUpdate(2);assert(s.textCallbacks==1);++cases;}
 for(unsigned mode=0;mode<18;++mode){scriptMgr.rows=native;conditions.custom=false;auto alter=[&](uint32 type,uint32 id)->SmartScriptHolder&{for(auto&r:scriptMgr.rows)if(r.entryOrGuid==98686&&r.source_type==type&&r.event_id==id)return r;std::abort();};if(mode==0)alter(0,0).actionType=45;if(mode==1)alter(0,0).action.raw.param1=1;if(mode==2)alter(0,0).link=1;if(mode==3)alter(0,0).event.event_flags=0;if(mode==4)alter(0,0).event.event_chance=99;if(mode==5)alter(0,0).target.raw.param1=42;if(mode==6)alter(9,0).event.raw.param1=1;if(mode==7)alter(9,1).action.raw.param1=0;if(mode==8)alter(9,2).target.raw.param1=49;if(mode==9){auto r=alter(0,0);r.event_id=99;scriptMgr.rows.push_back(r);}if(mode==10){auto r=alter(0,0);r.entryOrGuid=-370121;scriptMgr.rows.push_back(r);}if(mode==11)conditions.custom=true;if(mode==12)alter(0,2).target.raw.param2=81;if(mode==13)alter(0,1).event.raw.param2=4999;if(mode==14){scriptMgr.rows.erase(std::remove_if(scriptMgr.rows.begin(),scriptMgr.rows.end(),[](auto const&r){return r.entryOrGuid==98686&&r.source_type==9&&r.event_id==3;}),scriptMgr.rows.end());}if(mode==15)alter(9,3).target.x=1;if(mode==16)alter(0,3).action.raw.param3=1;if(mode==17)alter(9,0).source_type=0;assert(!FireLyandra::NativeWingData());++cases;}
 scriptMgr.rows=native;conditions.custom=false;
 std::printf("PASS %u production-controller/native aura93/CombatStop/attack/victim/SmartAI/regen/transient-list cases: both raw-aura counterfactuals, player/pet/guardian/taunt, normal DATA4 endpoint/resume, wipe/real-owner cleanup/native home/retry, finite failed/missing/dead/locked-wing diagnostics, scope/lease/death/one-hit and reentrant cancellation. World ranking, spell effects, respawn and criterion endpoints controlled; full client separate.\n",cases);
}
