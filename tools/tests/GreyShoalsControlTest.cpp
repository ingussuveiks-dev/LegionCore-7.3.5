#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <vector>
#include <memory>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;using int32=int32_t;
#include "ControlEnums.inc"
enum DamageEffectType{DIRECT_DAMAGE};
constexpr int UNIT_FIELD_FLAGS=0,UNIT_FIELD_CHARM=1,UNIT_FIELD_CHARMED_BY=2,UNIT_FIELD_BYTES_2=3,UNIT_FIELD_PET_NAME_TIMESTAMP=4;
constexpr uint32 UNIT_FLAG_NON_ATTACKABLE=2,UNIT_FLAG_NOT_SELECTABLE=0x2000000,UNIT_FLAG_PLAYER_CONTROLLED=0x1000000,UNIT_FLAG_REMOVE_CLIENT_CONTROL=4,UNIT_FLAG_PVP_ATTACKABLE=8;
constexpr int UNIT_STATE_POSSESSED=1,UNIT_MASK_MINION=2,MOTION_SLOT_ACTIVE=1,MOTION_SLOT_IDLE=0,UNIT_BYTES_2_OFFSET_PVP_FLAG=1,UNIT_BYTE2_FLAG_UNK1=1;
constexpr int CLASS_WARLOCK=9,CLASS_MAGE=8,CREATURE_TYPE_DEMON=3,SMART_EVENT_CHARMED=0,SPELL_AURA_MOUNTED=78;
constexpr int SPELL_AURA_MOD_POSSESS=2,SPELL_AURA_MOD_CHARM=6,SPELL_AURA_AOE_CHARM=177,AURA_EFFECT_HANDLE_REAL=1;
#define TC_LOG_DEBUG(...) ((void)0)
#define TC_LOG_FATAL(...) assert(false)
#define TC_LOG_ERROR(...) assert(false)
#define ASSERT(x) assert(x)
struct ObjectGuid{int id=0;void Clear(){id=0;}explicit operator bool()const{return id!=0;}int GetCounter()const{return id;}int GetGUIDLow()const{return id;}
 bool operator<(ObjectGuid o)const{return id<o.id;}bool operator==(ObjectGuid o)const{return id==o.id;}bool operator!=(ObjectGuid o)const{return id!=o.id;}};
struct WorldObject{virtual ~WorldObject()=default;};
struct Unit;struct Player;struct Creature;struct AuraApplication;struct CreatureAI;
std::map<int,Unit*> registry;
struct Map{bool IsBattleground(){return false;}};
struct Motion{void Clear(bool=true){}void InitDefault(){}};
struct Hostile{void deleteReferences(){}};
struct Zone{void OnUnitCharmed(Unit*,Unit*){}void OnUnitRemoveCharmed(Unit*,Unit*){}};
struct CharmInfo{int petNumber=42;void InitCharmCreateSpells(){}void InitPossessCreateSpells(){}void SetPetNumber(int n,bool){petNumber=n;}};
struct CreatureTemplate{int Type=CREATURE_TYPE_DEMON,unit_class=1;};
struct ObjectMgr{int GeneratePetNumber(){return 42;}}objectMgr;auto sObjectMgr=&objectMgr;
struct Unit:WorldObject{
 ObjectGuid guid,charmGuid,charmerGuid,minionGuid;std::set<ObjectGuid> m_Controlled;Unit* owner=nullptr;
 bool m_ControlledByPlayer=false,_isWalkingBeforeCharm=false,walking=false,possessed=false,vehicle=false,alive=true,combat=false,casting=false,inWorld=true;
 bool IsAIEnabled=true,NeedChangeAI=false;uint32 flags=0;int faction=2817,originalFaction=2817,klass=1;
 int castStops=0,combatStops=0,threatClears=0,attackStops=0,deletedCharm=0,aurasRemoved=0,controlApplied=0;
 ReactStates react=REACT_AGGRESSIVE;ObjectGuid LastCharmerGUID;Zone* m_zoneScript=nullptr;CharmInfo* charmInfo=nullptr;
 virtual bool IsPlayer()const{return false;}virtual Player* ToPlayer(){return nullptr;}virtual Creature* ToCreature(){return nullptr;}
 bool IsCreature(){return ToCreature()!=nullptr;}bool IsInWorld(){return inWorld;}bool IsAlive(){return alive;}bool isInCombat(){return combat;}
 ObjectGuid GetGUID()const{return guid;}ObjectGuid GetOwnerGUID()const{return owner?owner->guid:ObjectGuid{};}ObjectGuid GetCharmerGUID()const{return charmerGuid;}
 Unit* GetCharmer(){return registry[charmerGuid.id];}Unit* GetCharm()const{return registry[charmGuid.id];}ObjectGuid GetMinionGUID()const{return minionGuid;}
 bool isCharmed(){return bool(charmerGuid);}bool IsVehicle(){return vehicle;}bool IsOnVehicle(Unit*){return false;}
 void CastStop(){casting=false;++castStops;}void CombatStop(bool=false){combat=false;++combatStops;}void AttackStop(){++attackStops;}
 void DeleteThreatList(){++threatClears;}void ClearSaveThreatTarget(){}Hostile& getHostileRefManager(){static Hostile h;return h;}
 Map* GetMap(){static Map m;return &m;}Motion* GetMotionMaster(){static Motion m;return &m;}
 void PauseMovement(int,int){}void StopMoving(){}void RestoreFaction(){faction=originalFaction;}void setFaction(int f){faction=f;}int getFaction(){return faction;}
 void SetFlag(int,uint32 f){flags|=f;}void RemoveFlag(int,uint32 f){flags&=~f;}uint32 GetUInt32Value(int){return flags;}
 bool HasUnitState(int){return possessed;}void ClearUnitState(int){possessed=false;}void AddUnitState(int){possessed=true;}
 void SetClass(int k){klass=k;}int getClass(){return klass;}int GetEntry(){return 115026;}int GetTypeId(){return IsPlayer()?4:3;}
 void SetUInt32Value(int,uint32){}int GetByteValue(int,int){return 0;}void SetByteValue(int,int,int){}void SetByteFlag(int,int,int){}void RemoveByteFlag(int,int,int){}
 bool IsWalking(){return walking;}void SetWalk(bool b){walking=b;}void SendMovementFlagUpdate(bool=false){}
 bool AddGuidValue(int f,ObjectGuid g){auto& v=f==UNIT_FIELD_CHARM?charmGuid:charmerGuid;if(v)return false;v=g;return true;}
 bool RemoveGuidValue(int f,ObjectGuid g){auto& v=f==UNIT_FIELD_CHARM?charmGuid:charmerGuid;if(v!=g)return false;v.Clear();return true;}
 Player* GetCharmerOrOwnerPlayerOrPlayerItself();void SetCharm(Unit*,bool);Unit* GetFirstControlled()const;
 bool SetCharmedBy(Unit*,CharmType,AuraApplication const* =nullptr);void RemoveCharmedBy(Unit*);void RemoveCharmAuras();void RemoveAurasByType(int);
 CharmInfo* GetCharmInfo(){return charmInfo;}CharmInfo* InitCharmInfo(){charmInfo=new CharmInfo;return charmInfo;}
 void DeleteCharmInfo(){delete charmInfo;charmInfo=nullptr;react=REACT_AGGRESSIVE;++deletedCharm;}
 void ApplyControlStatesIfNeeded(){++controlApplied;}ReactStates GetReactState(){return react;}void SetReactState(ReactStates r){react=r;}
};
struct Player:Unit{
 Unit* mover=this;int removedBars=0,charmBars=0,possessBars=0,vehicleBars=0;
 bool IsPlayer()const override{return true;}Player* ToPlayer()override{return this;}void* GetTransport(){return nullptr;}
 void StopCastingCharm(){}void StopCastingBindSight(){}bool isAFK(){return false;}void ToggleAFK(){}
 void SetClientControl(Unit* u,bool apply){if(apply)mover=u;else if(mover==u)mover=nullptr;}
 void VehicleSpellInitialize(){++vehicleBars;}void PossessSpellInitialize(){++possessBars;}void CharmSpellInitialize(){++charmBars;}
 void SendRemoveControlBar(){++removedBars;}void UpdatePvPState(){}
};
Player* Unit::GetCharmerOrOwnerPlayerOrPlayerItself(){if(IsPlayer())return ToPlayer();if(owner)return owner->ToPlayer();return GetCharmer()?GetCharmer()->ToPlayer():nullptr;}
namespace ObjectAccessor{Unit* GetUnit(Unit const&,ObjectGuid g){return registry[g.id];}}
struct CreatureData{int areaId=8297;};struct GameObjectData{int areaId=8297;};
struct CreatureAI{Creature* me;explicit CreatureAI(Creature* c):me(c){}virtual ~CreatureAI()=default;
 virtual bool CanBeVisibleFor(WorldObject const*)const{return true;}virtual void UpdateLifecycle(uint32){}virtual void InitializeAI(){}virtual void JustRespawned(){}
 virtual void UpdateAI(uint32){}virtual void MoveInLineOfSight(Unit*){}virtual void DamageTakenAfterScaling(Unit*,uint32&,DamageEffectType){}
 virtual void OnCharmed(bool);};
struct Creature:Unit{CreatureData data;bool hasData=true,guardian=false;int map=1220,entry=115026,visibility=0;CreatureAI* ai=nullptr;CreatureTemplate info;
 Creature* ToCreature()override{return this;}CreatureAI* AI(){return ai;}CreatureTemplate const* GetCreatureTemplate(){return &info;}
 bool isGuardian(){return guardian;}bool HasUnitTypeMask(int){return guardian;}
 CreatureData const* GetCreatureData()const{return hasData?&data:nullptr;}int GetMapId()const{return map;}int GetEntry()const{return entry;}
 void UpdateObjectVisibility(){++visibility;}};
struct SmartScript{int charmEvents=0;void ProcessEventsFor(int,Unit*,int,int,bool){++charmEvents;}};
struct SmartAI:CreatureAI{SmartScript script;using CreatureAI::CreatureAI;SmartScript* GetScript(){return &script;}
 void OnCharmed(bool)override;void EnterEvadeMode(){me->combat=false;}};
struct CreatureScript{explicit CreatureScript(char const*){}virtual CreatureAI* GetAI(Creature*)const{return nullptr;}};
struct GameObject:WorldObject{GameObjectData data;GameObjectData const* GetGOData()const{return &data;}int GetMapId()const{return 1220;}int GetEntry()const{return 266170;}void UpdateObjectVisibility(){}};
struct GameObjectAI{GameObject* go;explicit GameObjectAI(GameObject* g):go(g){}virtual bool CanBeVisibleFor(WorldObject const*)const{return true;}virtual void InitializeAI(){}virtual void UpdateAI(uint32){}};
struct Quest{};struct Rotation{int64_t ResetTime=200;};struct Store{Quest quest;Rotation rotation;bool have=true;Quest* GetQuestTemplate(int){return &quest;}Rotation* GetWorldQuest(Quest*){return have?&rotation:nullptr;}}store;
auto sQuestDataStore=&store;namespace GameTime{int64_t now=100;int64_t GetGameTime(){return now;}}
struct AuraApplication{Unit* target;Unit* GetTarget()const{return target;}bool GetRemoveMode()const{return false;}};
struct AuraEffect{Unit* caster=nullptr;Unit* GetCaster()const{return caster;}
 void HandleModPossess(AuraApplication const*,uint8,bool)const;void HandleModCharm(AuraApplication const*,uint8,bool)const;void HandleCharmConvert(AuraApplication const*,uint8,bool)const;};
#include "ControlBaseHook.inc"
#include "ControlSmartHook.inc"
#include "ControlRelations.inc"
#include "ControlFirst.inc"
#include "ControlCharm.inc"
#include "ControlAuraHooks.inc"
#include "ControlAuras.inc"
struct AppliedAura{Unit* target;int type;AuraEffect effect;};std::vector<AppliedAura> auras;
void Unit::RemoveAurasByType(int type){for(size_t i=0;i<auras.size();){auto aura=auras[i];if(aura.target!=this||aura.type!=type){++i;continue;}
 auras.erase(auras.begin()+i);++aurasRemoved;AuraApplication app{this};
 if(type==SPELL_AURA_MOD_POSSESS)aura.effect.HandleModPossess(&app,AURA_EFFECT_HANDLE_REAL,false);
 else if(type==SPELL_AURA_AOE_CHARM)aura.effect.HandleCharmConvert(&app,AURA_EFFECT_HANDLE_REAL,false);
 else if(type==SPELL_AURA_MOD_CHARM)aura.effect.HandleModCharm(&app,AURA_EFFECT_HANDLE_REAL,false);
}}
#ifdef CONTROL_BASELINE
#include "ControlBaseline.inc"
#else
#include "ControlCurrent.inc"
#endif
int main(){
 Creature ordinary;CreatureAI base(&ordinary);base.OnCharmed(true);assert(ordinary.NeedChangeAI&&!ordinary.IsAIEnabled);
 for(int type:{SPELL_AURA_MOD_CHARM,SPELL_AURA_MOD_POSSESS,SPELL_AURA_AOE_CHARM,0}){
  for(bool missingCaster:{false,true}){
   GameTime::now=100;store.have=true;Player player;Creature npc,pet;player.guid={1};npc.guid={2};pet.guid={3};
   registry={{1,&player},{2,&npc},{3,&pet}};pet.owner=&player;pet.guardian=true;player.minionGuid=pet.guid;player.m_Controlled.insert(pet.guid);
   npc_grey_shoals_rotation ai(&npc);npc.ai=&ai;ai.InitializeAI();player.klass=CLASS_WARLOCK;
   AuraApplication app{&npc};CharmType charmType=type==SPELL_AURA_MOD_POSSESS?CHARM_TYPE_POSSESS:(type==SPELL_AURA_AOE_CHARM?CHARM_TYPE_CONVERT:CHARM_TYPE_CHARM);
   assert(npc.SetCharmedBy(&player,charmType,&app));assert(npc.IsAIEnabled&&!npc.NeedChangeAI&&npc.ai==&ai);
   assert(npc.isCharmed()&&player.charmGuid==npc.guid&&player.m_Controlled.count(npc.guid));
   if(type)auras.push_back({&npc,type,{missingCaster?nullptr:&player}});
   auras.push_back({&npc,999,{&player}}); // Unrelated aura must survive.
   ai.UpdateLifecycle(1);assert(npc.isCharmed());GameTime::now=200;ai.UpdateLifecycle(1);
#ifdef CONTROL_BASELINE
   assert(npc.isCharmed()&&player.charmGuid==npc.guid);std::cout<<"baseline: hidden actor retains control, auraType="<<type<<'\n';
   npc.RemoveCharmAuras();if(npc.isCharmed())npc.RemoveCharmedBy(nullptr);
#else
   assert(!npc.isCharmed()&&!player.charmGuid&&!player.m_Controlled.count(npc.guid));
   assert(player.m_Controlled.count(pet.guid)&&player.minionGuid==pet.guid&&pet.owner==&player);
   assert(player.GetFirstControlled()==&pet&&player.removedBars==1);
   assert(npc.faction==npc.originalFaction&&npc.react==REACT_PASSIVE&&!npc.LastCharmerGUID);
   assert((npc.flags&ai.GateFlags)==ai.GateFlags&&!npc.possessed);
   // Native player-uncontrol sends RemoveControlBar and retains CharmInfo in
   // this branch. Do not invent a deletion that production does not perform.
   assert(npc.charmInfo);if(type!=SPELL_AURA_MOD_POSSESS)assert(npc.charmInfo->petNumber==0);
   assert(npc.IsAIEnabled&&!npc.NeedChangeAI&&npc.ai==&ai);
   if(type==SPELL_AURA_MOD_POSSESS)assert(player.mover==&player&&!(player.flags&UNIT_FLAG_REMOVE_CLIENT_CONTROL)&&!(npc.flags&UNIT_FLAG_PLAYER_CONTROLLED));
   int removed=npc.aurasRemoved;ai.UpdateLifecycle(1);assert(npc.aurasRemoved==removed&&player.removedBars==1);
   assert(auras.size()==1&&auras.front().type==999);
   // A forced/scripted control after the gate is already inactive must be
   // removed before Sync's unchanged-state return, then restore passive flags.
   assert(npc.SetCharmedBy(&player,charmType,&app));npc.react=REACT_AGGRESSIVE;
   if(type)auras.push_back({&npc,type,{&player}});
   ai.UpdateLifecycle(1);assert(!npc.isCharmed()&&npc.react==REACT_PASSIVE&&player.removedBars==2);
   assert(!npc.LastCharmerGUID&&player.GetFirstControlled()==&pet&&auras.size()==1);
   GameTime::now=100;store.rotation.ResetTime=300;ai.UpdateLifecycle(1);assert(npc.react==REACT_AGGRESSIVE&&!(npc.flags&ai.GateFlags));store.rotation.ResetTime=200;
#endif
   auras.clear();registry.clear();delete npc.charmInfo;npc.charmInfo=nullptr;
  }
 }
 // Native non-player conversion follows its own CharmInfo deletion branch.
 GameTime::now=100;Creature npc,charmer;charmer.guid={8};npc.guid={9};registry={{8,&charmer},{9,&npc}};
 npc_grey_shoals_rotation ai(&npc);npc.ai=&ai;ai.InitializeAI();
 assert(npc.SetCharmedBy(&charmer,CHARM_TYPE_CONVERT));auras.push_back({&npc,SPELL_AURA_AOE_CHARM,{&charmer}});
 store.have=false;ai.UpdateLifecycle(1);
#ifdef CONTROL_BASELINE
 assert(npc.isCharmed());npc.RemoveCharmAuras();
#else
 assert(!npc.isCharmed()&&npc.charmInfo==nullptr&&npc.react==REACT_PASSIVE&&charmer.m_Controlled.empty());assert(!npc.LastCharmerGUID);
#endif
 store.have=true;auras.clear();registry.clear();
#ifdef CONTROL_BASELINE
 std::cout<<"PASS: pinned baseline reproduces hidden actors retaining native charm/control; manual test cleanup follows each reproduction\n";
#else
 std::cout<<"PASS: native SmartAI retains AI; native charm/possession/convert/direct cleanup, missing aura caster, primary pet, repeated expiry and new rotation\n";
#endif
}
