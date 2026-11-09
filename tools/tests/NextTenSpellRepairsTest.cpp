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
#include <atomic>
#include <mutex>
using uint64=uint64_t;using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;using int32=int32_t;
#include "Functions.inc"
using AuraEffectHandleModes=int;using AuraRemoveMode=int;
enum {EFFECT_0=0,SPELL_AURA_DUMMY=4,SPELL_AURA_MOUNTED=78,AURA_EFFECT_HANDLE_REAL=1,MINUTE=60,IN_MILLISECONDS=1000,PLAYER_FIELD_LOCAL_FLAGS=1,PLAYER_LOCAL_FLAG_CAN_USE_OBJECTS_MOUNTED=2,GAMEOBJECT_TYPE_FISHINGHOLE=25,GAMEOBJECT_TYPE_GATHERING_NODE=50,SPELL_AURA_ALLOW_USING_GAMEOBJECTS_WHILE_MOUNTED=475,ALLIANCE=1,alliance_spell=179245,horde_spell=179244,alliance_npc=89715,horde_npc=89713,ride_veh=46598,recId=4050,SPELL_TOUCH_OF_ELUNE_DAY=154796,SPELL_TOUCH_OF_ELUNE_NIGHT=154797};
using GuidList=std::list<int>;
struct Position{float x=0,y=0,z=0,o=0;float GetPositionX()const{return x;}float GetPositionY()const{return y;}float GetPositionZ()const{return z;}Position()=default;Position(float a,float b,float c,float d):x(a),y(b),z(c),o(d){}};
struct Map{bool forbidden=false;bool IsBattlegroundOrArena(){return forbidden;}bool IsRaid(){return false;}bool IsDungeon(){return false;}};
struct Vehicle{uint32 aura=0;void Reset(){}uint32 GetRecAura(){return aura;}};
struct Player;struct Creature;struct AuraEffect;struct Aura;struct SpellScript;struct Item{};
using SpellEffIndex=int;enum{EFFECT_2=2,SPELL_AURA_ADD_FLAT_MODIFIER=107,SPELL_WARRIOR_SHIELD_BLOCK_AURA=132404,CRITERIA_TYPE_BE_SPELL_TARGET=28,SPELL_EFFECT_DUMMY=3,SPELL_EFFECT_SCRIPT_EFFECT=77,SPELL_DIRECT_DAMAGE=0,TEMPSUMMON_TIMED_DESPAWN=1,SPELL_AURA_MOD_RATING=189,SPELL_AURA_SCHOOL_ABSORB=69,AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK=2,AURA_REMOVE_BY_EXPIRE=2,AURA_REMOVE_BY_DEATH=3,CLASS_WARRIOR=1,CLASS_PALADIN=2,CLASS_HUNTER=3,CLASS_ROGUE=4,CLASS_PRIEST=5,CLASS_DEATH_KNIGHT=6,CLASS_SHAMAN=7,CLASS_MAGE=8,CLASS_WARLOCK=9,CLASS_MONK=10,CLASS_DRUID=11,CLASS_DEMON_HUNTER=12,SPELL_FLASK_OF_THE_NORTH_SP=67016,SPELL_FLASK_OF_THE_NORTH_AP=67017,SPELL_FLASK_OF_THE_NORTH_STR=67018,SPELL_MAGE_COMET_STORM_VISUAL=228601,SPELL_MAGE_FLURRY_VISUAL=228596,SPELL_MAGE_FLURRY_CHILL_PROC=228358,SPELL_MAGE_BONE_CHILLING=205027,SPELL_MAGE_BONE_CHILLING_BUFF=205766,SPELL_MAGE_BRAIN_FREEZE_AURA=190446,SPELL_MAGE_BRAIN_FREEZE_IMPROVED=231584};
using ObjectGuid=int;using SpellCastResult=int;enum {SPELL_CAST_OK=0,SPELL_FAILED_AFFECTING_COMBAT=1,SPELL_FAILED_NO_VALID_TARGETS=2,EFFECT_1=1,SPELL_AURA_WATER_BREATHING=82,SPELL_AURA_MOD_INCREASE_VEHICLE_FLIGHT_SPEED=206,SPELL_AURA_CAMERA_SELFIE=482,SPELL_AURA_PERIODIC_DUMMY=226,SPELL_AURA_PROC_TRIGGER_SPELL=42,SPELL_AURA_MOD_STUN=12,AURA_REMOVE_BY_CANCEL=1,LANG_UNIVERSAL=0,GOBELING_GUMBO_BURP=42755};
struct Unit{
 bool world=true,alive=true,preDelete=false,failCast=false,node=false,interaction=false,nativeProvider=false,combat=false,water=false;int guid=1;uint32 cls=1;float effectAmount=20;std::map<uint32,float>effectAmounts;std::vector<Position>destinations;uint32 getClass(){return cls;}Aura* GetAura(uint32);bool HasAuraEffect(uint32 id,int i){return GetAuraEffect(id,i)!=nullptr;}
 uint32 mapId=1220,instance=1;Map map;Position position;
 std::vector<uint32> cancelled;std::vector<float> amounts;std::set<uint32> auras,skills,spells;std::vector<uint32> casts;std::map<uint32,Creature*> rares;std::vector<std::shared_ptr<Creature>> summons;std::unique_ptr<Vehicle> vehicle;
 FunctionProcessor queue;struct Event{uint32 due;std::function<void()> call;};std::vector<Event> events;uint32 clock=0;unsigned moves=0;
 virtual ~Unit()=default;virtual Player* ToPlayer(){return nullptr;}
 Unit* FindNearestGameObjectOfType(int type,float radius){assert(type==25&&radius==100);return node?this:nullptr;}
 int GetGUID(){return guid;}bool IsInWater(){return water;}bool isInCombat(){return combat;}AuraEffect* GetAuraEffect(uint32,int);void CancelSpellVisualKit(uint32 id){cancelled.push_back(id);}
 void CastCustomSpell(Unit* t,uint32 id,float const* amount,void*,void*,bool){amounts.push_back(*amount);CastSpell(t,id,true);}
 bool HasAura(uint32 id){return auras.count(id)!=0;}bool HasAuraType(int){return nativeProvider;}
 void CastSpell(Unit* target,uint32 id,bool=true,void* =nullptr){casts.push_back(id);if(!failCast)target->auras.insert(id);}
 void RemoveAura(uint32 id){auras.erase(id);}void RemoveAurasDueToSpell(uint32 id){RemoveAura(id);}
 uint32 GetMapId(){return mapId;}uint32 GetInstanceId(){return instance;}bool IsInWorld(){return world;}bool IsPreDelete(){return preDelete;}bool IsAlive(){return alive;}Map* GetMap(){return &map;}
 Position GetPosition(){return position;}float GetPositionX(){return position.x;}float GetPositionY(){return position.y;}float GetPositionZ(){return position.z;}
 void NearTeleportTo(Position p){position=p;++moves;}void NearTeleportTo(float x,float y,float z,float o){NearTeleportTo(Position{x,y,z,o});}
 void AddDelayedEvent(uint32 delay,std::function<void()> f){queue.AddDelayedEvent(delay,std::move(f));}
 void Advance(uint32 diff){queue.Update(diff);}
 void CastSpell(float x,float y,float z,uint32 id,bool){destinations.push_back({x,y,z,0});casts.push_back(id);}
 float SpellDamageBonusDone(Unit*,void const*,uint32,int,std::vector<uint32>&,int){return 10;}
 void MovePosition(Position& p,float d,float){p.x+=d;}
 Creature* SummonCreature(uint32 id,Position p,int,uint32){return SummonCreature(id,p);}
 void CastSpellDuration(Unit* target,uint32 id,bool,int32){CastSpell(target,id,true);}

 Creature* FindNearestCreature(uint32 entry,float range,bool onlyAlive){assert(range==500&&onlyAlive);auto it=rares.find(entry);return it==rares.end()?nullptr:it->second;}
 bool CreateVehicleKit(uint32 id,uint32,uint32 aura){assert(id==4050);if(vehicle)return false;vehicle=std::make_unique<Vehicle>();vehicle->aura=aura;return true;}
 Vehicle* GetVehicleKit(){return vehicle.get();}void RemoveVehicleKit(){vehicle.reset();}void SendSetVehicleRecId(uint32){}
 Creature* SummonCreature(uint32 entry,Position p);static Creature* GetCreature(Unit& owner,int guid);
};
struct Player:Unit{std::vector<uint32>credits;void UpdateAchievementCriteria(int,uint32 id){credits.push_back(id);}std::vector<int32>cooldowns;void ModifySpellCooldown(uint32 id,int32 change){assert(id==242609);cooldowns.push_back(change);}Item*GetItemByGuid(int){return nullptr;}void SetItemCooldown(Item*,int){}unsigned whispers=0,emotes=0;bool pending=false;bool HasSpell(uint32 id){return spells.count(id)!=0;}void learnSpell(uint32 id,bool){spells.insert(id);}void removeSpell(uint32 id){spells.erase(id);}void BossWhisper(std::string const&,int,int){++whispers;}void TextEmote(std::string const&){++emotes;}struct Session{int GetSessionDbLocaleIndex(){return 0;}}session;Session* GetSession(){return &session;}uint32 team=ALLIANCE;Player* ToPlayer()override{return this;}bool HasSkill(uint32 id){return skills.count(id)!=0;}uint32 GetTeam(){return team;}bool HasPendingTeleport(){return pending;}bool HasFlag(int,int){return interaction;}void SetFlag(int,int){interaction=true;}void RemoveFlag(int,int){interaction=false;}Player* FindNearestGameObjectOfType(int type,float radius){assert((type==50&&radius==3)||(type==25&&radius==100));return node?this:nullptr;}};
struct Creature:Unit{uint32 entry=0;int guid=0;bool despawned=false;uint32 GetEntry(){return entry;}int GetGUID(){return guid;}void DespawnOrUnsummon(uint32 delay){assert(delay==100);despawned=true;}};
Creature* Unit::SummonCreature(uint32 entry,Position p){auto s=std::make_shared<Creature>();s->entry=entry;s->position=p;s->guid=int(summons.size()+1);summons.push_back(s);return s.get();}
Creature* Unit::GetCreature(Unit&owner,int guid){for(auto&s:owner.summons)if(s->guid==guid)return s.get();return nullptr;}
struct TaxiNodesEntry{struct{float X=10,Y=20,Z=30;}Pos;};struct TaxiStore{TaxiNodesEntry node;bool missing=false;TaxiNodesEntry const* LookupEntry(uint32){return missing?nullptr:&node;}}sTaxiNodesStore;
struct ObjectMgr{uint32 node=1;uint32 GetNearestTaxiNode(float,float,float,uint32,Player*){return node;}}objectMgr;auto sObjectMgr=&objectMgr;
namespace GameTime{tm value{};bool missing=false;tm const* GetDateAndTime(){return missing?nullptr:&value;}}
int errors=0;
struct SpellEffect{float BasePoints=100;float CalcValue(Unit*)const{return BasePoints;}};
struct SpellInfo{uint32 Id=30108;SpellEffect effect;SpellEffect* Effects[3]={&effect,&effect,&effect};};
struct AuraEffect{float amount=20;int index=0;int GetEffIndex()const{return index;}float GetAmount()const{return amount;}void SetAmount(float v){amount=v;}};
AuraEffect* Unit::GetAuraEffect(uint32 id,int){static AuraEffect e;e.amount=effectAmounts.count(id)?effectAmounts[id]:effectAmount;return auras.count(id)?&e:nullptr;}
float CalculatePct(float base,float pct){return base*pct/100;}
bool roll_chance_f(float){return false;}bool chance=true;bool roll_chance_i(int value){assert(value==30);return chance;}
uint32 urand(uint32 low,uint32 high){assert(low<=high);return low;}
struct BroadcastStore{bool missing=false;int entry=1;int const* LookupEntry(uint32 id){assert(id==90915||(id>=121859&&id<=121885));return missing?nullptr:&entry;}}sBroadcastTextStore;
struct DB2Manager{static std::string GetBroadcastTextValue(int const*,int){return "native text endpoint";}};
#include "Hooks.inc"
#define TC_LOG_ERROR(...) (++errors)
struct AuraScript;struct Application{Unit* target;Unit* GetTarget(){return target;}int GetRemoveMode()const{return AURA_REMOVE_BY_CANCEL;}};
struct Handler{int index=0;std::function<void(AuraScript*,AuraEffect const*,float&,bool&)>calc;std::function<void(AuraScript*,uint32,AuraEffect*)> effect;std::function<void(AuraScript*,uint32)> update;std::function<void(AuraScript*)> simple;
 bool IsEffectAffected(SpellInfo*,int i){return i==index;}void Call(AuraScript*s,AuraEffect const*e,float&a,bool&b){calc(s,e,a,b);}void Call(AuraScript*s,uint32 d,AuraEffect*e){effect(s,d,e);}void Call(AuraScript*s,uint32 d){update(s,d);}void Call(AuraScript*s){if(simple)simple(s);}};
struct Hook:std::vector<Handler>{void operator+=(Handler h){push_back(std::move(h));}};
struct Aura{uint32 stack=1;int32 GetDuration(){return 1000;}int GetCastItemGUID(){return 0;}AuraEffect effect;Unit*owner=nullptr,*caster=nullptr;bool removed=false;SpellInfo info;SpellInfo*m_spellInfo=&info;std::vector<AuraScript*>m_loadedScripts;Unit* GetUnitOwner(){return owner;}bool IsRemoved(){return removed;}void Remove(){removed=true;}void CallScriptEffectCalcAmountHandlers(AuraEffect const*,float&,bool&);void CallScriptEffectUpdateHandlers(uint32,AuraEffect*);void CallScriptAuraUpdateHandlers(uint32);};
struct AuraScript{Aura*m_aura=nullptr;Application*m_auraApplication=nullptr;int m_currentScriptState=SPELL_SCRIPT_STATE_NONE;Hook OnEffectUpdate,OnAuraUpdate,AfterEffectRemove,AfterEffectApply,OnEffectApply,OnEffectRemove,DoCalcMaxDuration,OnEffectPeriodic,DoEffectCalcAmount,OnEffectAbsorb,AfterEffectAbsorb,OnDispel;
 virtual~AuraScript()=default;virtual bool Load(){return true;}virtual bool Validate(SpellInfo const*){return true;}virtual void Register(){}bool ValidateSpellInfo(std::initializer_list<int>){return true;}
 Aura* GetAura(){return m_aura;}SpellInfo* GetSpellInfo(){return &m_aura->info;}Application const* GetTargetApplication(){return m_auraApplication;}uint32 GetStackAmount(){return m_aura->stack;}virtual uint32 CallSpecialFunction(uint32){return 0;}AuraEffect* GetEffect(int){return &m_aura->effect;}uint32 GetId(){return m_aura->info.Id;}Unit*GetCaster(){return m_aura->caster;}Unit*GetTarget()const;Unit*GetUnitOwner()const;
 void _PrepareScriptCall(int state){m_currentScriptState=state;m_auraApplication=nullptr;}void _FinishScriptCall(){m_currentScriptState=SPELL_SCRIPT_STATE_NONE;}};
#include "Target.inc"
#include "Owner.inc"
#include "Dispatch.inc"
#include "CalcDispatch.inc"
Aura* Unit::GetAura(uint32 id){static Aura a;return auras.count(id)?&a:nullptr;}
template<class T>Handler HandlerFor(void(T::*f)(uint32,AuraEffect*),int i){Handler h;h.index=i;h.effect=[f](AuraScript*s,uint32 d,AuraEffect*e){(static_cast<T*>(s)->*f)(d,e);};return h;}
template<class T>Handler HandlerFor(void(T::*f)(uint32)){Handler h;h.update=[f](AuraScript*s,uint32 d){(static_cast<T*>(s)->*f)(d);};return h;}
template<class T>Handler HandlerFor(void(T::*f)(AuraEffect const*,AuraEffectHandleModes),int i){Handler h;h.index=i;h.simple=[f](AuraScript*s){(static_cast<T*>(s)->*f)(nullptr,1);};return h;}
template<class T>Handler DurationHandler(void(T::*)(int32&)){return {};}
struct DamageInfo{uint32 damage=100;uint32 GetDamage(){return damage;}};struct DispelInfo{Unit*target;Unit*GetDispeller(){return target;}};
struct SpellScriptLoader{explicit SpellScriptLoader(char const*){}virtual~SpellScriptLoader()=default;virtual SpellScript*GetSpellScript()const{return nullptr;}virtual AuraScript*GetAuraScript()const{return nullptr;}};
template<class T>Handler HandlerFor(void(T::*f)(AuraEffect const*),int i){Handler h;h.index=i;h.simple=[f](AuraScript*s){(static_cast<T*>(s)->*f)(nullptr);};return h;}
using WorldLocation=Position;struct SpellScript{SpellInfo info;Unit*fixtureCaster=nullptr,*fixtureTarget=nullptr;WorldLocation dest;void PreventHitDefaultEffect(int){}Unit*GetHitUnit(){return fixtureTarget;}WorldLocation const*GetHitDest(){return &dest;}SpellInfo*GetSpellInfo(){return &info;}struct CastHook{template<class T>void operator+=(T){}}AfterCast,OnCheckCast,BeforeCast,OnEffectHit,OnEffectHitTarget,OnEffectLaunchTarget;virtual~SpellScript()=default;virtual bool Validate(SpellInfo const*){return true;}virtual void Register(){}Unit*GetCaster(){return fixtureCaster;}};
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
#include "Selected.inc"
struct Binding{Aura aura;Application app;Binding(AuraScript&s,Unit&owner):app{&owner}{aura.owner=aura.caster=&owner;s.m_aura=&aura;s.Load();s.Register();aura.m_loadedScripts={&s};s.m_auraApplication=&app;s.m_currentScriptState=AURA_SCRIPT_HOOK_EFFECT_AFTER_APPLY;}float Calc(int index,float amount){AuraEffect e;e.index=index;bool can=false;aura.CallScriptEffectCalcAmountHandlers(&e,amount,can);assert(can);return amount;}};
void Check(bool b,char const*m){if(!b){std::cerr<<"FAIL "<<m<<'\n';std::exit(1);}}
int main(){
#if defined(CASE_UMBRAL)
 Player p;spell_gen_umbral_shift s;Binding b(s,p);auto amount=b.Calc(1,123);
#ifdef COUNTERFACTUAL
 Check(errors==1&&amount==123,"old no-app mastery stays active");std::cout<<"COUNTERFACTUAL Umbral calculation logs invalid GetTarget and retains mastery without third item\n";
#else
 Check(!errors&&amount==0,"absent enabler");p.auras.insert(253825);Check(b.Calc(1,123)==123&&!errors,"present enabler");p.auras.clear();Check(b.Calc(1,321)==0,"removed enabler");
#endif
#elif defined(CASE_SHATTER)
 Player p;spell_warr_shatter_the_bones s;Binding b(s,p);Check(b.Calc(0,17)==0&&!errors,"without Shield Block");p.auras.insert(132404);Check(b.Calc(0,17)==17,"with Shield Block");p.auras.clear();Check(b.Calc(0,17)==0,"Shield removal");
#elif defined(CASE_FLASK)
 Unit p;spell_item_flask_of_the_north::spell_item_flask_of_the_north_SpellScript s;s.fixtureCaster=&p;p.cls=CLASS_MONK;s.HandleDummy(0);Check(p.casts==std::vector<uint32>{67016},"Monk native hybrid first choice");p.cls=CLASS_DEMON_HUNTER;s.HandleDummy(0);Check(p.casts.back()==67017,"DH agility");for(unsigned cls:{1u,2u,3u,4u,5u,6u,7u,8u,9u,11u}){p.cls=cls;s.HandleDummy(0);}chooseHigh=true;p.cls=CLASS_MONK;s.HandleDummy(0);Check(p.casts.back()==67017,"Monk native agility alternate");chooseHigh=false;auto count=p.casts.size();p.cls=0;s.HandleDummy(0);Check(p.casts.size()==count,"unknown class no invalid range");s.fixtureCaster=nullptr;s.HandleDummy(0);
#elif defined(CASE_COOKIE)
 Player p;spell_item_chocolate_cookie::spell_item_chocolate_cookie_AuraScript s;Binding b(s,p);b.aura.caster=nullptr;b.aura.stack=90;s.OnStackChange(nullptr,1);Check(p.credits.empty(),"no early91milestone");b.aura.stack=91;s.OnStackChange(nullptr,1);Check(p.credits==std::vector<uint32>{99041},"actual recipient91criterion");b.aura.stack=92;s.OnStackChange(nullptr,1);Check(p.credits.size()==1,"exact91gate");Unit nonplayer;b.aura.owner=&nonplayer;s.OnStackChange(nullptr,1);b.aura.owner=nullptr;s.OnStackChange(nullptr,1);
#elif defined(CASE_WAILING)
 Player p;spell_item_walling_souls s;Binding b(s,p);s.GetSpellInfo()->effect.BasePoints=30;DamageInfo damage;float amount=0;s.Trigger(nullptr,damage,amount);
#ifdef COUNTERFACTUAL
 Check(p.cooldowns==std::vector<int32>{-30000},"zero absorb incorrectly reduces cap");std::cout<<"COUNTERFACTUAL Wailing Souls grants maximum cooldown reduction at zero absorption\n";
#else
 Check(p.cooldowns.empty(),"zero absorption");for(float f:{0.1f,0.99f})s.Trigger(nullptr,damage,f);Check(p.cooldowns.empty(),"fractional absorption has no whole damage");amount=1;s.Trigger(nullptr,damage,amount);Check(p.cooldowns==std::vector<int32>{-30000},"positive capped formula retained");amount=50;s.Trigger(nullptr,damage,amount);Check(p.cooldowns.back()==-2000,"positive ratio retained");
#endif
#else
 for(int mode=0;mode<7;++mode){Unit p,target;p.guid=1;target.guid=2;ObjectAccessor::units={{2,&target}};
#if defined(CASE_COMET)
 spell_mage_comet_storm s;s.fixtureCaster=&p;s.dest={10,20,30,0};s.HandleDummy(0);Check(p.queue.SizeQueue()==7,"seven comets queued");
#elif defined(CASE_FLURRY)
 spell_mage_flurry::spell_mage_flurry_SpellScript s;s.fixtureCaster=&p;s.fixtureTarget=&target;p.auras={190446,231584,205027,251859};p.effectAmounts[190446]=50;p.effectAmounts[251859]=10;s.HandleBeforeCast();s.HandleLaunchTarget(0);Check(p.amounts.size()==1&&p.amounts[0]==15,"original immediate damage");
#elif defined(CASE_STORM)
 spell_storm_comet s;s.fixtureTarget=&target;s.HandleDummy(0);Check(target.queue.SizeQueue()==23,"authored random lower bound23");
#elif defined(CASE_STOMP)
 spell_gen_reckless_stomp::spell_gen_reckless_stomp_SpellScript s;s.fixtureCaster=&p;s.HandleDummy(0);Check(p.casts.size()==10,"ten immediate bolts");
#elif defined(CASE_DISPEL)
 spell_warl_unstable_affliction::spell_warl_unstable_affliction_AuraScript s;Binding b(s,p);DispelInfo dispel{&target};s.HandleDispel(&dispel);Check(p.queue.SizeQueue()==1,"one native backlash queued");
#endif
 Unit& queueOwner=
#if defined(CASE_STORM)
 target;
#else
 p;
#endif
 unsigned before=p.casts.size();
#if defined(CASE_COMET)
 uint32 first=150;
#elif defined(CASE_FLURRY)
 uint32 first=300;
#elif defined(CASE_STORM)
 uint32 first=10;
#else
 uint32 first=50;
#endif
 queueOwner.Advance(first-1);Check(p.casts.size()==before&&target.summons.empty(),"no queued effect before native deadline");
 if(mode==6)ObjectAccessor::units.clear();if(mode==0){p.mapId=target.mapId=1;}if(mode==1){p.instance=target.instance=2;}if(mode==2)queueOwner.preDelete=true;if(mode==3)queueOwner.world=false;if(mode==4)queueOwner.alive=false;
 queueOwner.Advance(2000);
#if defined(CASE_STORM)
 unsigned emitted=target.summons.size();
#else
 unsigned emitted=unsigned(p.casts.size())-before;
#endif
#ifdef COUNTERFACTUAL
 #if defined(CASE_FLURRY) || defined(CASE_DISPEL)
 if(mode!=6)
#endif
 Check(emitted>0,"pinned queued effects still delivered after origin change");
#else
 bool expected=mode>=4;
#if defined(CASE_FLURRY) || defined(CASE_DISPEL)
 if(mode==6)expected=false;
#endif
 Check((emitted>0)==expected,"origin/world refusal;original death/native cast handling retained");
 if(mode==5){
#if defined(CASE_COMET)
 Check(emitted==7,"seven authored comet impacts");
#elif defined(CASE_FLURRY)
 Check(p.amounts==std::vector<float>({15.f,16.5f,18.f}),"three authored BrainFreeze/T21 snapshots");
#elif defined(CASE_STORM)
 Check(emitted==23,"authored lower-bound ice count");
#elif defined(CASE_STOMP)
 Check(emitted==10&&p.casts.size()==20,"two ten-bolt halves");
#elif defined(CASE_DISPEL)
 Check(emitted==1,"one backlash");
#endif
 }
 auto castsAfter=p.casts.size();auto summonsAfter=target.summons.size();queueOwner.Advance(2000);Check(p.casts.size()==castsAfter&&target.summons.size()==summonsAfter,"no duplicate queued deliveries");
#endif
 }
#ifdef COUNTERFACTUAL
 std::cout<<"COUNTERFACTUAL original delayed effect follows its queue owner across maps/instances\n";
#endif
#endif
#ifndef COUNTERFACTUAL
 std::cout<<"PASS selected full class/native getter,calc dispatcher or FunctionProcessor ASAN alternatives;world/combat endpoints modeled,not client traversal\n";
#endif
return 0;}
