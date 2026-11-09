// Extracted complete production scripts and native aura getters/update dispatch.
// Deliberately modeled Unit/Player/map/network endpoints; not whole-client proof.
#include <cassert>
#include <cstdint>
#include <ctime>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <limits>
#include <vector>
#define M_PI 3.14159265358979323846
#include <array>
#include <atomic>
#include <mutex>
using uint64=uint64_t;using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;using int32=int32_t;
#include "Functions.inc"
enum{SPELL_FAILED_BAD_TARGETS=12,TYPEID_PLAYER=4,SKILL_RIDING=762,STAT_STRENGTH=0,SPELL_AURA_PERIODIC_DAMAGE=3,SPELL_AURA_MOD_TOTAL_STAT_PERCENTAGE=137,SPELL_AURA_MOD_THREAT=10,SPELL_AURA_MOD_FACTION=243,IN_PROGRESS=1,NOT_STARTED=0,FAIL=2,DONE=3,SPECIAL=4,HORDE=2,MOVE_RUN=1,POWER_INSANITY=1,SPEC_DRUID_RESTORATION=105,SPEC_MONK_MISTWEAVER=270,SPEC_PRIEST_HOLY=257,SPEC_PRIEST_DISCIPLINE=256,SPEC_PALADIN_HOLY=65,SPEC_SHAMAN_RESTORATION=264,GENDER_MALE=0,SPELL_AURA_TRANSFORM=56,SPELL_AURA_CONTROL_VEHICLE=236,SPELL_AURA_MOD_DAMAGE_PERCENT_DONE=79,SPELL_CAST_TYPE_NORMAL=0,SPELL_CAST_TYPE_MISSILE=1,TRIGGERED_FULL_MASK=1,SPELL_EFFECT_SCHOOL_DAMAGE=2,AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK=3};
using AuraEffectHandleModes=int;using AuraRemoveMode=int;
constexpr int EFFECT_3=3;
enum {EFFECT_0=0,SPELL_AURA_DUMMY=4,SPELL_AURA_MOUNTED=78,AURA_EFFECT_HANDLE_REAL=1,MINUTE=60,IN_MILLISECONDS=1000,PLAYER_FIELD_LOCAL_FLAGS=1,PLAYER_LOCAL_FLAG_CAN_USE_OBJECTS_MOUNTED=2,GAMEOBJECT_TYPE_FISHINGHOLE=25,GAMEOBJECT_TYPE_GATHERING_NODE=50,SPELL_AURA_ALLOW_USING_GAMEOBJECTS_WHILE_MOUNTED=475,ALLIANCE=1,alliance_spell=179245,horde_spell=179244,alliance_npc=89715,horde_npc=89713,ride_veh=46598,recId=4050,SPELL_TOUCH_OF_ELUNE_DAY=154796,SPELL_TOUCH_OF_ELUNE_NIGHT=154797};
using GuidList=std::list<int>;
struct Position{float x=0,y=0,z=0,o=0;void Relocate(float a,float b,float c,float d){x=a;y=b;z=c;o=d;}float GetPositionX()const{return x;}float GetPositionY()const{return y;}float GetPositionZ()const{return z;}Position()=default;Position(float a,float b,float c,float d):x(a),y(b),z(c),o(d){}};
struct Map{bool forbidden=false;bool IsBattlegroundOrArena(){return forbidden;}bool IsRaid(){return false;}bool IsDungeon(){return false;}};
struct Vehicle{uint32 aura=0;void Reset(){}uint32 GetRecAura(){return aura;}};
struct Player;struct Creature;struct AuraEffect;struct Aura;struct SpellScript;struct Item{bool equipped=true;uint32 powers=0;bool IsEquipped(){return equipped;}uint32 GetTotalPurchasedArtifactPowers(){return powers;}};struct SpellCastTargets;struct SpellInfo;struct CustomSpellValues;struct TriggerCastData;
using SpellEffIndex=int;enum{EFFECT_2=2,SPELL_AURA_ADD_FLAT_MODIFIER=107,SPELL_WARRIOR_SHIELD_BLOCK_AURA=132404,CRITERIA_TYPE_BE_SPELL_TARGET=28,SPELL_EFFECT_DUMMY=3,SPELL_EFFECT_SCRIPT_EFFECT=77,SPELL_DIRECT_DAMAGE=0,TEMPSUMMON_TIMED_DESPAWN=1,SPELL_AURA_MOD_RATING=189,SPELL_AURA_SCHOOL_ABSORB=69,AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK=2,AURA_REMOVE_BY_EXPIRE=2,AURA_REMOVE_BY_DEATH=3,CLASS_WARRIOR=1,CLASS_PALADIN=2,CLASS_HUNTER=3,CLASS_ROGUE=4,CLASS_PRIEST=5,CLASS_DEATH_KNIGHT=6,CLASS_SHAMAN=7,CLASS_MAGE=8,CLASS_WARLOCK=9,CLASS_MONK=10,CLASS_DRUID=11,CLASS_DEMON_HUNTER=12,SPELL_FLASK_OF_THE_NORTH_SP=67016,SPELL_FLASK_OF_THE_NORTH_AP=67017,SPELL_FLASK_OF_THE_NORTH_STR=67018,SPELL_MAGE_COMET_STORM_VISUAL=228601,SPELL_MAGE_FLURRY_VISUAL=228596,SPELL_MAGE_FLURRY_CHILL_PROC=228358,SPELL_MAGE_BONE_CHILLING=205027,SPELL_MAGE_BONE_CHILLING_BUFF=205766,SPELL_MAGE_BRAIN_FREEZE_AURA=190446,SPELL_MAGE_BRAIN_FREEZE_IMPROVED=231584};
using ObjectGuid=int;using SpellCastResult=int;enum {SPELL_CAST_OK=0,SPELL_FAILED_AFFECTING_COMBAT=1,SPELL_FAILED_NO_VALID_TARGETS=2,EFFECT_1=1,SPELL_AURA_WATER_BREATHING=82,SPELL_AURA_MOD_INCREASE_VEHICLE_FLIGHT_SPEED=206,SPELL_AURA_CAMERA_SELFIE=482,SPELL_AURA_PERIODIC_DUMMY=226,SPELL_AURA_PROC_TRIGGER_SPELL=42,SPELL_AURA_MOD_STUN=12,AURA_REMOVE_BY_CANCEL=1,LANG_UNIVERSAL=0,GOBELING_GUMBO_BURP=42755};
struct InstanceScript{std::map<uint32,int>states;int GetBossState(uint32 id){return states[id];}};
struct Unit{
 InstanceScript*instanceScript=nullptr;InstanceScript*GetInstanceScript(){return instanceScript;}bool pvp=false;void SetPvP(bool v){pvp=v;}float distance=0;float GetDistance(Unit*){return distance;}uint64 health=500,maxHealth64=0;uint64 GetMaxHealth(){return maxHealth64?maxHealth64:uint64(maxHealth);}uint64 GetHealth(){return health;}float GetHealthPct(){return 100.f*float(health)/float(GetMaxHealth());}uint64 CountPctFromMaxHealth(int32 v){return uint64(float(GetMaxHealth())*v/100.f); }bool IsPlayer(){return ToPlayer()!=nullptr;}uint32 GetTypeId(){return IsPlayer()?4:3;}std::function<void()>onCast;

 uint32 display=49,nativeDisplay=49,otherDisplay=0,gender=0;int32 power=0;float speed=1,maxHealth=1000;void SetDisplayId(uint32 v){display=v;}uint32 GetNativeDisplayId(){return nativeDisplay;}void RestoreDisplayId(){display=otherDisplay?otherDisplay:nativeDisplay;}uint32 getGender(){return gender;}int32 GetPower(int){return power;}float GetSpeedRate(int){return speed;}void RemoveAurasByType(int){nativeProvider=false;speed=1;}
 bool world=true,alive=true,preDelete=false,failCast=false,node=false,interaction=false,nativeProvider=false,combat=false,water=false;int guid=1;uint32 cls=1;float effectAmount=20;std::map<uint32,float>effectAmounts;std::vector<Position>destinations;uint32 getClass(){return cls;}Aura* GetAura(uint32);bool HasAuraEffect(uint32 id,int i){return GetAuraEffect(id,i)!=nullptr;}
 uint32 mapId=1220,instance=1;Map map;Position position;
 std::vector<uint32> cancelled;std::vector<float> heals;std::vector<float> amounts;std::set<uint32> auras,skills,spells;std::vector<uint32> casts;std::map<uint32,Creature*> rares;std::vector<std::shared_ptr<Creature>> summons;std::unique_ptr<Vehicle> vehicle;
 FunctionProcessor queue,combatQueue;struct Event{uint32 due;std::function<void()> call;};std::vector<Event> events;uint32 clock=0;unsigned moves=0;
 virtual ~Unit()=default;virtual Player* ToPlayer(){return nullptr;}
 Unit* FindNearestGameObjectOfType(int type,float radius){assert(type==25&&radius==100);return node?this:nullptr;}
 int GetGUID(){return guid;}bool IsInWater(){return water;}bool isInCombat(){return combat;}AuraEffect* GetAuraEffect(uint32,int);void CancelSpellVisualKit(uint32 id){cancelled.push_back(id);}
 void CastCustomSpell(Unit* t,uint32 id,float const* amount,float const* heal,void*,bool){assert(id==118594);amounts.push_back(*amount);heals.push_back(heal?*heal:0);CastSpell(t,id,true);}
 bool HasAura(uint32 id,int=0){return auras.count(id)!=0;}bool HasAuraType(int){return nativeProvider;}
 int CastSpell(Unit* target,uint32 id,bool=true,void* =nullptr){casts.push_back(id);if(onCast)onCast();if(!failCast)target->auras.insert(id);return failCast?99:0;}
 void RemoveAura(uint32 id){auras.erase(id);}void RemoveAurasDueToSpell(uint32 id){RemoveAura(id);}
 uint32 GetMapId(){return mapId;}uint32 GetInstanceId(){return instance;}bool IsInWorld(){return world;}bool IsPreDelete(){return preDelete;}bool IsAlive(){return alive;}Map* GetMap(){return &map;}
 Position GetPosition(){return position;}float GetPositionX(){return position.x;}float GetPositionY(){return position.y;}float GetPositionZ(){return position.z;}
 void NearTeleportTo(Position p){position=p;++moves;}void NearTeleportTo(float x,float y,float z,float o){NearTeleportTo(Position{x,y,z,o});}
 void AddDelayedEvent(uint32 delay,std::function<void()> f){queue.AddDelayedEvent(delay,std::move(f));}
 void Advance(uint32 diff){queue.Update(diff);combatQueue.Update(diff);}
 void CastSpell(float x,float y,float z,uint32 id,bool){destinations.push_back({x,y,z,0});casts.push_back(id);}
 float SpellDamageBonusDone(Unit*,void const*,uint32,int,std::vector<uint32>&,int){return 10;}
 void MovePosition(Position& p,float d,float){p.x+=d;}
 Creature* SummonCreature(uint32 id,Position p,int,uint32){return SummonCreature(id,p);}
 void AddDelayedCombat(uint32 diff,std::function<void()> f){combatQueue.AddDelayedEvent(diff,std::move(f));}
 void CastSpellDelay(Unit*t,uint32 id,bool,uint32 delay){AddDelayedCombat(delay,[this,t,id]{CastSpell(t,id,true);});}
 void CastSpell(SpellCastTargets const&,SpellInfo const*,CustomSpellValues const*,TriggerCastData const&);
 void ClearSpellTargets(uint32){}float GetAngle(Position const*){return 0.f;}void UpdateGroundPositionZ(float,float,float&){}
 void CastSpellDuration(Unit* target,uint32 id,bool,int32){CastSpell(target,id,true);}

 Creature* FindNearestCreature(uint32 entry,float range,bool onlyAlive){assert(range==500&&onlyAlive);auto it=rares.find(entry);return it==rares.end()?nullptr:it->second;}
 bool CreateVehicleKit(uint32 id,uint32,uint32 aura){assert(id==4050);if(vehicle)return false;vehicle=std::make_unique<Vehicle>();vehicle->aura=aura;return true;}
 Vehicle* GetVehicleKit(){return vehicle.get();}void RemoveVehicleKit(){vehicle.reset();}void SendSetVehicleRecId(uint32){}
 Creature* SummonCreature(uint32 entry,Position p);static Creature* GetCreature(Unit& owner,int guid);
};
struct Player:Unit{bool allowed=false,pvpScaling=false;uint32 skillValue=0;float strength=0;uint32 GetSkillValue(uint32){return skillValue;}bool HasPvpStatsScalingEnabled(){return pvpScaling;}float GetStat(int){return strength;}bool tank=false,inventory=false;uint32 spec=105;Item item;bool isInTankSpec(){return tank;}float CountPctFromMaxHealth(float p){return maxHealth*p/100;}uint32 GetSpecializationId(){return spec;}std::vector<uint32>credits;void UpdateAchievementCriteria(int,uint32 id){credits.push_back(id);}std::vector<int32>cooldowns;void ModifySpellCooldown(uint32 id,int32 change){assert(id==242609);cooldowns.push_back(change);}Item*GetItemByGuid(int guid){return inventory&&guid==1?&item:nullptr;}void SetItemCooldown(Item*,int){}unsigned whispers=0,emotes=0;bool pending=false;bool HasSpell(uint32 id){return spells.count(id)!=0;}void learnSpell(uint32 id,bool){spells.insert(id);}void removeSpell(uint32 id){spells.erase(id);}void BossWhisper(std::string const&,int,int){++whispers;}void TextEmote(std::string const&){++emotes;}struct Session{int GetSessionDbLocaleIndex(){return 0;}}session;Session* GetSession(){return &session;}uint32 team=ALLIANCE;Player* ToPlayer()override{return this;}bool HasSkill(uint32 id){return skills.count(id)!=0;}uint32 GetTeam(){return team;}bool HasPendingTeleport(){return pending;}bool HasFlag(int,int){return interaction;}void SetFlag(int,int){interaction=true;}void RemoveFlag(int,int){interaction=false;}Player* FindNearestGameObjectOfType(int type,float radius){assert((type==50&&radius==3)||(type==25&&radius==100));return node?this:nullptr;}};
struct Creature:Unit{uint32 entry=0;int guid=0;bool despawned=false;uint32 GetEntry(){return entry;}int GetGUID(){return guid;}void DespawnOrUnsummon(uint32 delay){assert(delay==100);despawned=true;}};
Creature* Unit::SummonCreature(uint32 entry,Position p){auto s=std::make_shared<Creature>();s->entry=entry;s->position=p;s->guid=int(summons.size()+1);summons.push_back(s);return s.get();}
Creature* Unit::GetCreature(Unit&owner,int guid){for(auto&s:owner.summons)if(s->guid==guid)return s.get();return nullptr;}
struct TaxiNodesEntry{struct{float X=10,Y=20,Z=30;}Pos;};struct TaxiStore{TaxiNodesEntry node;bool missing=false;TaxiNodesEntry const* LookupEntry(uint32){return missing?nullptr:&node;}}sTaxiNodesStore;
struct ObjectMgr{uint32 node=1;uint32 GetNearestTaxiNode(float,float,float,uint32,Player*){return node;}}objectMgr;auto sObjectMgr=&objectMgr;
namespace GameTime{tm value{};bool missing=false;tm const* GetDateAndTime(){return missing?nullptr:&value;}}
int errors=0;
struct SpellEffect{float BasePoints=100;float CalcValue(Unit*)const{return BasePoints;}};
struct SpellInfo{uint32 Id=30108;SpellEffect effect;SpellEffect* Effects[3]={&effect,&effect,&effect};};
struct AuraEffect{float amount=20;int index=0;int GetEffIndex()const{return index;}Aura*base=nullptr;Aura*GetBase()const{return base;}SpellInfo*GetSpellInfo()const;float GetAmount()const{return amount;}void SetAmount(float v){amount=v;}};
AuraEffect* Unit::GetAuraEffect(uint32 id,int){static AuraEffect e;e.amount=effectAmounts.count(id)?effectAmounts[id]:effectAmount;return auras.count(id)?&e:nullptr;}
template<class T,class P>T CalculatePct(T base,P pct){return T(base*pct/100);}
bool roll_chance_f(float){return false;}bool chance=true;bool roll_chance_i(int value){assert(value==30);return chance;}
bool pickHigh=false;uint32 urand(uint32 low,uint32 high){assert(low<=high);return pickHigh?high:low;}
struct BroadcastStore{bool missing=false;int entry=1;int const* LookupEntry(uint32 id){assert(id==90915||(id>=121859&&id<=121885));return missing?nullptr:&entry;}}sBroadcastTextStore;
struct DB2Manager{static std::string GetBroadcastTextValue(int const*,int){return "native text endpoint";}};
#include "Hooks.inc"
#define TC_LOG_ERROR(...) (++errors)
struct AuraScript;struct Application{Unit* target;Unit* GetTarget(){return target;}int GetRemoveMode()const{return AURA_REMOVE_BY_CANCEL;}};
struct Handler{int index=0;std::function<void(AuraScript*,AuraEffect const*,float&,bool&)>calc;std::function<void(AuraScript*,uint32,AuraEffect*)> effect;std::function<void(AuraScript*,uint32)> update;std::function<void(AuraScript*)> simple;
 bool IsEffectAffected(SpellInfo*,int i){return i==index;}void Call(AuraScript*s,AuraEffect const*e,float&a,bool&b){calc(s,e,a,b);}void Call(AuraScript*s,uint32 d,AuraEffect*e){effect(s,d,e);}void Call(AuraScript*s,uint32 d){update(s,d);}void Call(AuraScript*s){if(simple)simple(s);}};
struct Hook:std::vector<Handler>{void operator+=(Handler h){push_back(std::move(h));}};
struct Aura{uint32 stack=1;int32 GetDuration(){return 1000;}int itemGuid=0;int GetCastItemGUID(){return itemGuid;}AuraEffect effect;Unit*owner=nullptr,*caster=nullptr;bool removed=false;SpellInfo info;SpellInfo*m_spellInfo=&info;std::vector<AuraScript*>m_loadedScripts;Unit* GetUnitOwner(){return owner;}bool IsRemoved(){return removed;}void Remove(){removed=true;}void CallScriptEffectCalcAmountHandlers(AuraEffect const*,float&,bool&);void CallScriptEffectUpdateHandlers(uint32,AuraEffect*);void CallScriptAuraUpdateHandlers(uint32);};
struct AuraScript{Aura*m_aura=nullptr;Application*m_auraApplication=nullptr;int m_currentScriptState=SPELL_SCRIPT_STATE_NONE;Hook OnEffectUpdate,OnAuraUpdate,AfterEffectRemove,AfterEffectApply,OnEffectApply,OnEffectRemove,DoCalcMaxDuration,OnEffectPeriodic,DoEffectCalcAmount,OnEffectAbsorb,AfterEffectAbsorb,OnDispel;
 virtual~AuraScript()=default;virtual bool Load(){return true;}virtual bool Validate(SpellInfo const*){return true;}virtual void Register(){}bool ValidateSpellInfo(std::initializer_list<int>){return true;}
 void Remove(){m_aura->Remove();}Aura* GetAura(){return m_aura;}SpellInfo* GetSpellInfo(){return &m_aura->info;}Application const* GetTargetApplication(){return m_auraApplication;}uint32 GetStackAmount(){return m_aura->stack;}virtual uint32 CallSpecialFunction(uint32){return 0;}AuraEffect* GetEffect(int){return &m_aura->effect;}uint32 GetId(){return m_aura->info.Id;}Unit*GetCaster(){return m_aura->caster;}Unit*GetTarget()const;Unit*GetUnitOwner()const;
 void _PrepareScriptCall(int state){m_currentScriptState=state;m_auraApplication=nullptr;}void _FinishScriptCall(){m_currentScriptState=SPELL_SCRIPT_STATE_NONE;}};
#include "Target.inc"
#include "Owner.inc"
#include "Dispatch.inc"
#include "CalcDispatch.inc"
SpellInfo* AuraEffect::GetSpellInfo()const{return base->m_spellInfo;}
Aura* Unit::GetAura(uint32 id){static Aura a;return auras.count(id)?&a:nullptr;}
template<class T>Handler HandlerFor(void(T::*f)(uint32,AuraEffect*),int i){Handler h;h.index=i;h.effect=[f](AuraScript*s,uint32 d,AuraEffect*e){(static_cast<T*>(s)->*f)(d,e);};return h;}
template<class T>Handler HandlerFor(void(T::*f)(uint32)){Handler h;h.update=[f](AuraScript*s,uint32 d){(static_cast<T*>(s)->*f)(d);};return h;}
template<class T>Handler HandlerFor(void(T::*f)(AuraEffect const*,AuraEffectHandleModes),int i){Handler h;h.index=i;h.simple=[f](AuraScript*s){(static_cast<T*>(s)->*f)(nullptr,1);};return h;}
template<class T>Handler DurationHandler(void(T::*)(int32&)){return {};}
struct DamageInfo{uint32 damage=100;uint32 GetDamage(){return damage;}};struct DispelInfo{Unit*target;Unit*GetDispeller(){return target;}};
struct SpellScriptLoader{explicit SpellScriptLoader(char const*){}virtual~SpellScriptLoader()=default;virtual SpellScript*GetSpellScript()const{return nullptr;}virtual AuraScript*GetAuraScript()const{return nullptr;}};
template<class T>Handler HandlerFor(void(T::*f)(AuraEffect const*),int i){Handler h;h.index=i;h.simple=[f](AuraScript*s){(static_cast<T*>(s)->*f)(nullptr);};return h;}
struct CustomSpellValues{};struct TriggerCastData{int triggerFlags=0,spellGuid=0,originalCaster=0,casttime=0,SubType=0;};
struct SpellCastTargets{Position dest;void SetCaster(Unit*){}void SetDst(Position const&p){dest=p;}};
void Unit::CastSpell(SpellCastTargets const&t,SpellInfo const*info,CustomSpellValues const*,TriggerCastData const&data){assert(data.casttime==1750&&data.spellGuid==13&&data.SubType==1&&data.originalCaster==guid);casts.push_back(info->Id);destinations.push_back(t.dest);}
struct CastGuid{int GetSubType(){return SPELL_CAST_TYPE_NORMAL;}operator int(){return 13;}};struct FakeSpell{CastGuid m_castGuid[2];uint32 GetCastTime(){return 1750;}uint32 GetTargetCount(){return 2;}};
namespace Trinity{void NormalizeMapCoord(float&){} }
using WorldLocation=Position;struct SpellScript{FakeSpell spell;FakeSpell*GetSpell(){return &spell;}WorldLocation const*GetExplTargetDest(){return missingDest?nullptr:&dest;}bool missingDest=false;SpellInfo info;Unit*fixtureCaster=nullptr,*fixtureTarget=nullptr;WorldLocation dest;void PreventHitDefaultEffect(int){}int32 hitDamage=1000;int32 GetHitDamage(){return hitDamage;}void SetHitDamage(int32 v){hitDamage=v;}Unit*GetExplTargetUnit(){return fixtureTarget;}Unit*GetHitUnit(){return fixtureTarget;}WorldLocation const*GetHitDest(){return &dest;}SpellInfo*GetSpellInfo(){return &info;}struct CastHook{template<class T>void operator+=(T){}}OnEffectLaunch,OnCast,AfterCast,OnCheckCast,BeforeCast,OnEffectHit,OnEffectHitTarget,OnEffectLaunchTarget;virtual~SpellScript()=default;virtual bool Validate(SpellInfo const*){return true;}virtual void Register(){}Unit*GetCaster(){return fixtureCaster;}};
#define PrepareAuraScript(name) public:
#define PrepareSpellScript(name) public:
#define AuraEffectUpdateFn(fn,i,a) HandlerFor(&fn,i)
#define AuraUpdateFn(fn) HandlerFor(&fn)
#define AuraEffectApplyFn(fn,i,a,m) HandlerFor(&fn,i)
#define AuraEffectRemoveFn(fn,i,a,m) HandlerFor(&fn,i)
#define AuraCalcMaxDurationFn(fn) DurationHandler(&fn)
#define SpellCastFn(fn) &fn

#define AuraEffectPeriodicFn(fn,i,a) HandlerFor(&fn,i)
#define SpellCheckCastFn(fn) &fn

template<class T>Handler HandlerFor(void(T::*f)(AuraEffect const*,float&,bool&),int i){Handler h;h.index=i;h.calc=[f](AuraScript*s,AuraEffect const*e,float&a,bool&b){(static_cast<T*>(s)->*f)(e,a,b);};return h;}
struct AnyHook{template<class T>operator T()const{return T{};}};
#define AuraEffectCalcAmountFn(fn,i,a) HandlerFor(&fn,i)
#define AuraEffectAbsorbFn(...) Handler{}
#define AuraDispelFn(...) Handler{}
#define SpellEffectFn(fn,...) &fn
#define TC_LOG_WARN(...) (++warnings)
int warnings=0;
bool chooseHigh=false;int32 irand(int32 lo,int32 hi){assert(hi>=lo);return chooseHigh?hi:lo;}
float frand(float lo,float hi){return (lo+hi)/2;}
float rand_norm(){return 0.5f;}
template<class T>void AddPct(T& base,float pct){base+=base*pct/100;}
struct SpellManager{SpellInfo info;SpellInfo const*GetSpellInfo(uint32 id){info.Id=id;info.effect.BasePoints=400;return &info;}}spellManager;auto sSpellMgr=&spellManager;
namespace ObjectAccessor{std::map<int,Unit*>units;Unit*GetUnit(Unit& u,int guid){auto i=units.find(guid);return i!=units.end()&&i->second->mapId==u.mapId&&i->second->instance==u.instance?i->second:nullptr;}}
namespace CommandersValor{bool Allowed(Player*p){return p&&p->allowed;}}
constexpr uint32 SPELL_LASER_BEAM_BETWEEN_DMG=202862;
#include "VantusRuneLifecycle.h"
#include "Selected.inc"
struct Binding{Aura aura;Application app;Binding(AuraScript&s,Unit&owner):app{&owner}{aura.owner=aura.caster=&owner;aura.effect.base=&aura;s.m_aura=&aura;s.Load();s.Register();aura.m_loadedScripts={&s};s.m_auraApplication=&app;s.m_currentScriptState=AURA_SCRIPT_HOOK_EFFECT_AFTER_APPLY;}float Calc(int index,float amount){AuraEffect e;e.index=index;bool can=false;aura.CallScriptEffectCalcAmountHandlers(&e,amount,can);return amount;}};
void Check(bool b,char const*m){if(!b){std::cerr<<"FAIL "<<m<<'\n';std::exit(1);}}

int main(){
#if defined(CASE_VALOR)
 Player p;aura_commanders_valor_location s;Binding b(s,p);p.allowed=true;b.aura.CallScriptAuraUpdateHandlers(1);Check(!b.aura.removed&&!errors,"eligible owner retained without application");p.allowed=false;b.aura.CallScriptAuraUpdateHandlers(1);Check(b.aura.removed&&!errors,"ineligible owner removed");
#elif defined(CASE_TIRATHON)
 Player p;Unit npc;spell_tirathon_beam_dmg::spell_tirathon_beam_dmg_AuraScript s;Binding b(s,p);b.aura.caster=&npc;b.aura.CallScriptEffectUpdateHandlers(499,&b.aura.effect);Check(npc.casts.empty(),"beam deadline");b.aura.CallScriptEffectUpdateHandlers(1,&b.aura.effect);
#ifdef COUNTERFACTUAL
 Check(npc.casts.empty()&&errors==1,"old invalid target getter drops beam cast");
#else
 Check(npc.casts==std::vector<uint32>{202862}&&!errors&&p.casts.empty(),"original caster hits actual owner");b.aura.caster=nullptr;b.aura.CallScriptEffectUpdateHandlers(500,&b.aura.effect);Check(npc.casts.size()==1,"missing caster skips safely");
#endif
#elif defined(CASE_FROG)
 Player p;spell_gulp_frog_toxin::spell_gulp_frog_toxinAuraScript s;Binding b(s,p);b.aura.stack=9;s.OnStackChange(nullptr,1);Check(p.casts.empty(),"no early death");b.aura.stack=10;s.OnStackChange(nullptr,1);
#ifdef COUNTERFACTUAL
 Check(p.casts.empty(),"old tenth stack is missed");s.OnStackChange(nullptr,1);Check(p.casts==std::vector<uint32>{147656},"old eleventh application only");
#else
 Check(p.casts==std::vector<uint32>{147656},"native tenth stack");s.OnStackChange(nullptr,1);Check(p.casts.size()==1,"no duplicate accepted death cast");s.Load();p.casts.clear();p.failCast=true;s.OnStackChange(nullptr,1);p.failCast=false;p.onCast=[&]{s.OnStackChange(nullptr,1);};s.OnStackChange(nullptr,1);Check(p.casts.size()==2,"failed cast retries,accepted reentrant cast only once");
#endif
#elif defined(CASE_VOID)
 Player p,t;p.maxHealth64=1024;p.health=341;t.maxHealth64=2048;t.health=1500;spell_pri_void_shift::spell_pri_void_shift_SpellScript s;s.fixtureCaster=&p;s.fixtureTarget=&t;s.info.effect.BasePoints=25;s.HandleDummy(0);
#ifdef COUNTERFACTUAL
 Check(p.heals[0]==406&&p.amounts[1]==825,"original truncates both native percentages to whole percent");
#else
 Check(p.heals[0]==409&&p.amounts[1]==818,"fractional health percentage exchange");p.casts.clear();p.heals.clear();p.amounts.clear();p.maxHealth64=uint64(1)<<35;p.health=uint64(1)<<34;t.maxHealth64=uint64(1)<<36;t.health=uint64(1)<<34;s.HandleDummy(0);Check(p.amounts[0]==float(uint64(1)<<33)&&p.heals[1]==float(uint64(1)<<34),"large native64bit health without32bit narrowing");
#endif
#elif defined(CASE_SWIFT)
 Player p;spell_dru_swift_flight_passive::spell_dru_swift_flight_passive_AuraScript s;Binding b(s,p);b.aura.caster=nullptr;Check(s.Load(),"surviving eligible player owner Load");p.skillValue=375;Check(b.Calc(1,280)==310,"native375riding tier");p.skillValue=374;Check(b.Calc(1,280)==280,"ordinary lower riding preserved");
#elif defined(CASE_ZONE)
 Player p;spell_dk_anti_magic_zone::spell_dk_anti_magic_zone_AuraScript s;Binding b(s,p);b.aura.caster=nullptr;Check(b.Calc(0,1)==136800,"missing caster uses existing nonplayer fallback");p.strength=123;b.aura.caster=&p;Check(b.Calc(0,1)==137292,"original caster Strength formula preserved");Unit npc;b.aura.caster=&npc;Check(b.Calc(0,1)==136800,"ordinary NPC fallback");
#elif defined(CASE_STAMINA) || defined(CASE_THREAT)
 Player p;
#ifdef CASE_STAMINA
 spell_gen_artificial_stamina::spell_gen_artificial_stamina_AuraScript s;int index=0;
#else
 spell_gen_artificial_threat s;int index=2;
#endif
 Binding b(s,p);b.aura.caster=nullptr;Check(b.Calc(index,1)==1,"missing original artifact caster safely preserves incoming fallback");b.aura.caster=&p;b.aura.itemGuid=1;p.inventory=true;p.item.powers=8;
#ifdef CASE_STAMINA
 Check(b.Calc(index,1)==6,"native stamina trait factor");
#else
 Check(b.Calc(index,1)==8,"native threat trait factor");
#endif
 p.item.powers=200;Check(b.Calc(index,1)==101,"existing cap101");p.pvpScaling=true;Check(b.Calc(index,1)==0,"PvP scaling exclusion preserved");p.pvpScaling=false;p.item.equipped=false;Check(b.Calc(index,1)==1,"no equipped cast item");
#elif defined(CASE_CENSER)
 Player p;Unit npc;spell_censer_of_eternal_agony s;Binding b(s,p);b.aura.caster=nullptr;s.HandleEffectApply(nullptr,1);
#ifdef COUNTERFACTUAL
 Check(!p.pvp,"old missing caster fails recipient PvP flag");b.aura.caster=&npc;s.HandlePeriodicTick(nullptr);Check(npc.pvp&&!p.pvp,"old flags distinct caster instead");
#else
 Check(p.pvp,"actual recipient flagged");p.pvp=false;b.aura.caster=&npc;s.HandlePeriodicTick(nullptr);Check(p.pvp&&!npc.pvp,"periodic recipient enforcement without touching caster");
#endif
#elif defined(CASE_MAW)
 Unit p,t;spell_item_rancid_maw s;s.fixtureCaster=&p;s.fixtureTarget=&t;p.distance=10;s.hitDamage=1000;s.HandleDamage(0);
#ifdef COUNTERFACTUAL
 Check(s.hitDamage==900,"old reversed90percent at10yards");p.distance=19;s.hitDamage=1000;s.HandleDamage(0);Check(s.hitDamage==810,"old damage drops approaching20");
#else
 Check(s.hitDamage==500,"half damage at10yards");float previous=-1;for(float d:{0.f,0.25f,1.f,5.f,10.f,19.f,19.9f,20.f,30.f}){p.distance=d;s.hitDamage=1000;s.HandleDamage(0);Check(s.hitDamage>=previous&&s.hitDamage<=1000,"monotonic bounded native distance curve");previous=s.hitDamage;}p.distance=1.25f;s.hitDamage=1000;s.HandleDamage(0);Check(s.hitDamage==62,"fractional distance retained until native int32 final hit rounding");
#endif
#else
 using Rune=SELECTED_RUNE;
 struct Row{uint32 rune,helper,boss;};Row rows[]={SELECTED_ROWS};constexpr uint32 mapId=SELECTED_MAP;
#ifdef COUNTERFACTUAL
 Unit p;InstanceScript instance;p.instanceScript=&instance;p.mapId=mapId;p.auras.insert(rows[0].rune);instance.states[rows[0].boss]=IN_PROGRESS;p.failCast=true;Rune s;Binding b(s,p);b.aura.info.Id=rows[0].rune;s.OnUpdate(999,&b.aura.effect);s.OnUpdate(1,&b.aura.effect);auto first=p.casts.size();s.OnUpdate(1,&b.aura.effect);Check(first==1&&p.casts.size()==2,"old expired timer retries failed casts every frame");
#else
 for(auto row:rows){Unit p;InstanceScript instance;p.instanceScript=&instance;p.mapId=mapId;p.auras={row.rune,123};instance.states[row.boss]=IN_PROGRESS;Rune s;Binding b(s,p);b.aura.info.Id=row.rune;b.aura.caster=nullptr;s.OnUpdate(999,&b.aura.effect);Check(p.casts.empty(),"no early rune attempt");s.OnUpdate(1,&b.aura.effect);Check(p.casts==std::vector<uint32>{row.helper},"matching native rune/helper/encounter");s.OnUpdate(1000,&b.aura.effect);Check(p.casts.size()==1,"no recast existing effect");instance.states[row.boss]=FAIL;s.OnUpdate(1000,&b.aura.effect);Check(!p.HasAura(row.helper)&&p.HasAura(123)&&p.HasAura(row.rune),"failed encounter cleanup keeps parent/unrelated");instance.states[row.boss]=IN_PROGRESS;p.failCast=true;s.OnUpdate(1000,&b.aura.effect);auto n=p.casts.size();s.OnUpdate(1,&b.aura.effect);Check(p.casts.size()==n,"failed casts bounded to1s");p.failCast=false;s.OnUpdate(999,&b.aura.effect);Check(p.HasAura(row.helper),"retry at next authored period");p.mapId=mapId+1;s.OnUpdate(1000,&b.aura.effect);Check(!p.HasAura(row.helper),"wrong-map cleanup");p.auras.insert(row.helper);p.mapId=mapId;p.instanceScript=nullptr;s.OnUpdate(1000,&b.aura.effect);Check(!p.HasAura(row.helper),"no-instance cleanup");p.auras.insert(row.helper);s.OnRemove(nullptr,1);Check(!p.HasAura(row.helper)&&p.HasAura(123),"parent removal cleanup");}
#endif
#endif
#ifdef COUNTERFACTUAL
 std::cout<<"COUNTERFACTUAL pinned complete class demonstrates original bug;not successful ordinary gameplay\n";
#else
 std::cout<<"PASS selected complete production class/native aura getters+dispatch or helper;world/cast endpoints modeled;ASAN,not client traversal\n";
#endif
}
