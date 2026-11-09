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
using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;using int32=int32_t;
using AuraEffectHandleModes=int;using AuraRemoveMode=int;
enum {EFFECT_0=0,SPELL_AURA_DUMMY=4,SPELL_AURA_MOUNTED=78,AURA_EFFECT_HANDLE_REAL=1,MINUTE=60,IN_MILLISECONDS=1000,PLAYER_FIELD_LOCAL_FLAGS=1,PLAYER_LOCAL_FLAG_CAN_USE_OBJECTS_MOUNTED=2,GAMEOBJECT_TYPE_FISHINGHOLE=25,GAMEOBJECT_TYPE_GATHERING_NODE=50,SPELL_AURA_ALLOW_USING_GAMEOBJECTS_WHILE_MOUNTED=475,ALLIANCE=1,alliance_spell=179245,horde_spell=179244,alliance_npc=89715,horde_npc=89713,ride_veh=46598,recId=4050,SPELL_TOUCH_OF_ELUNE_DAY=154796,SPELL_TOUCH_OF_ELUNE_NIGHT=154797};
using GuidList=std::list<int>;
struct Position{float x=0,y=0,z=0,o=0;Position()=default;Position(float a,float b,float c,float d):x(a),y(b),z(c),o(d){}};
struct Map{bool forbidden=false;bool IsBattlegroundOrArena(){return forbidden;}bool IsRaid(){return false;}bool IsDungeon(){return false;}};
struct Vehicle{uint32 aura=0;void Reset(){}uint32 GetRecAura(){return aura;}};
struct Player;struct Creature;struct AuraEffect;
using ObjectGuid=int;using SpellCastResult=int;enum {SPELL_CAST_OK=0,SPELL_FAILED_AFFECTING_COMBAT=1,SPELL_FAILED_NO_VALID_TARGETS=2,EFFECT_1=1,SPELL_AURA_WATER_BREATHING=82,SPELL_AURA_MOD_INCREASE_VEHICLE_FLIGHT_SPEED=206,SPELL_AURA_CAMERA_SELFIE=482,SPELL_AURA_PERIODIC_DUMMY=226,SPELL_AURA_PROC_TRIGGER_SPELL=42,SPELL_AURA_MOD_STUN=12,AURA_REMOVE_BY_CANCEL=1,LANG_UNIVERSAL=0,GOBELING_GUMBO_BURP=42755};
struct Unit{
 bool world=true,alive=true,preDelete=false,failCast=false,node=false,interaction=false,nativeProvider=false,combat=false,water=false;uint32 mapId=1220,instance=1;Map map;Position position;
 std::vector<uint32> cancelled;std::vector<float> amounts;std::set<uint32> auras,skills,spells;std::vector<uint32> casts;std::map<uint32,Creature*> rares;std::vector<std::shared_ptr<Creature>> summons;std::unique_ptr<Vehicle> vehicle;
 struct Event{uint32 due;std::function<void()> call;};std::vector<Event> events;uint32 clock=0;unsigned moves=0;
 virtual ~Unit()=default;virtual Player* ToPlayer(){return nullptr;}
 Unit* FindNearestGameObjectOfType(int type,float radius){assert(type==25&&radius==100);return node?this:nullptr;}
 int GetGUID(){return 1;}bool IsInWater(){return water;}bool isInCombat(){return combat;}AuraEffect* GetAuraEffect(uint32,int);void CancelSpellVisualKit(uint32 id){cancelled.push_back(id);}
 void CastCustomSpell(Unit* t,uint32 id,float* amount,void*,void*,bool){amounts.push_back(*amount);CastSpell(t,id,true);}
 bool HasAura(uint32 id){return auras.count(id)!=0;}bool HasAuraType(int){return nativeProvider;}
 void CastSpell(Unit* target,uint32 id,bool=true){casts.push_back(id);if(!failCast)target->auras.insert(id);}
 void RemoveAura(uint32 id){auras.erase(id);}void RemoveAurasDueToSpell(uint32 id){RemoveAura(id);}
 uint32 GetMapId(){return mapId;}uint32 GetInstanceId(){return instance;}bool IsInWorld(){return world;}bool IsPreDelete(){return preDelete;}bool IsAlive(){return alive;}Map* GetMap(){return &map;}
 Position GetPosition(){return position;}float GetPositionX(){return position.x;}float GetPositionY(){return position.y;}float GetPositionZ(){return position.z;}
 void NearTeleportTo(Position p){position=p;++moves;}void NearTeleportTo(float x,float y,float z,float o){NearTeleportTo(Position{x,y,z,o});}
 void AddDelayedEvent(uint32 delay,std::function<void()> f){events.push_back({clock+delay,std::move(f)});}
 void Advance(uint32 diff){clock+=diff;auto copy=std::move(events);events.clear();for(auto&e:copy)if(e.due<=clock)e.call();else events.push_back(std::move(e));}
 Creature* FindNearestCreature(uint32 entry,float range,bool onlyAlive){assert(range==500&&onlyAlive);auto it=rares.find(entry);return it==rares.end()?nullptr:it->second;}
 bool CreateVehicleKit(uint32 id,uint32,uint32 aura){assert(id==4050);if(vehicle)return false;vehicle=std::make_unique<Vehicle>();vehicle->aura=aura;return true;}
 Vehicle* GetVehicleKit(){return vehicle.get();}void RemoveVehicleKit(){vehicle.reset();}void SendSetVehicleRecId(uint32){}
 Creature* SummonCreature(uint32 entry,Position p);static Creature* GetCreature(Unit& owner,int guid);
};
struct Player:Unit{unsigned whispers=0,emotes=0;bool pending=false;bool HasSpell(uint32 id){return spells.count(id)!=0;}void learnSpell(uint32 id,bool){spells.insert(id);}void removeSpell(uint32 id){spells.erase(id);}void BossWhisper(std::string const&,int,int){++whispers;}void TextEmote(std::string const&){++emotes;}struct Session{int GetSessionDbLocaleIndex(){return 0;}}session;Session* GetSession(){return &session;}uint32 team=ALLIANCE;Player* ToPlayer()override{return this;}bool HasSkill(uint32 id){return skills.count(id)!=0;}uint32 GetTeam(){return team;}bool HasPendingTeleport(){return pending;}bool HasFlag(int,int){return interaction;}void SetFlag(int,int){interaction=true;}void RemoveFlag(int,int){interaction=false;}Player* FindNearestGameObjectOfType(int type,float radius){assert((type==50&&radius==3)||(type==25&&radius==100));return node?this:nullptr;}};
struct Creature:Unit{uint32 entry=0;int guid=0;bool despawned=false;uint32 GetEntry(){return entry;}int GetGUID(){return guid;}void DespawnOrUnsummon(uint32 delay){assert(delay==100);despawned=true;}};
Creature* Unit::SummonCreature(uint32 entry,Position p){auto s=std::make_shared<Creature>();s->entry=entry;s->position=p;s->guid=int(summons.size()+1);summons.push_back(s);return s.get();}
Creature* Unit::GetCreature(Unit&owner,int guid){for(auto&s:owner.summons)if(s->guid==guid)return s.get();return nullptr;}
struct TaxiNodesEntry{struct{float X=10,Y=20,Z=30;}Pos;};struct TaxiStore{TaxiNodesEntry node;bool missing=false;TaxiNodesEntry const* LookupEntry(uint32){return missing?nullptr:&node;}}sTaxiNodesStore;
struct ObjectMgr{uint32 node=1;uint32 GetNearestTaxiNode(float,float,float,uint32,Player*){return node;}}objectMgr;auto sObjectMgr=&objectMgr;
namespace GameTime{tm value{};bool missing=false;tm const* GetDateAndTime(){return missing?nullptr:&value;}}
int errors=0;
struct SpellEffect{float BasePoints=100;float CalcValue(Unit*)const{return BasePoints;}};
struct SpellInfo{SpellEffect effect;SpellEffect* Effects[3]={&effect,&effect,&effect};};
struct AuraEffect{float amount=20;int GetEffIndex(){return 0;}float GetAmount()const{return amount;}};
AuraEffect* Unit::GetAuraEffect(uint32 id,int){static AuraEffect e;return auras.count(id)?&e:nullptr;}
float CalculatePct(float base,float pct){return base*pct/100;}
bool chance=true;bool roll_chance_i(int value){assert(value==30);return chance;}
uint32 urand(uint32 low,uint32 high){assert(low<=high);return low;}
struct BroadcastStore{bool missing=false;int entry=1;int const* LookupEntry(uint32 id){assert(id==90915||(id>=121859&&id<=121885));return missing?nullptr:&entry;}}sBroadcastTextStore;
struct DB2Manager{static std::string GetBroadcastTextValue(int const*,int){return "native text endpoint";}};
#include "Hooks.inc"
#define TC_LOG_ERROR(...) (++errors)
struct AuraScript;struct Application{Unit* target;Unit* GetTarget(){return target;}int GetRemoveMode()const{return AURA_REMOVE_BY_CANCEL;}};
struct Handler{int index=0;std::function<void(AuraScript*,uint32,AuraEffect*)> effect;std::function<void(AuraScript*,uint32)> update;std::function<void(AuraScript*)> simple;
 bool IsEffectAffected(SpellInfo*,int i){return i==index;}void Call(AuraScript*s,uint32 d,AuraEffect*e){effect(s,d,e);}void Call(AuraScript*s,uint32 d){update(s,d);}void Call(AuraScript*s){if(simple)simple(s);}};
struct Hook:std::vector<Handler>{void operator+=(Handler h){push_back(std::move(h));}};
struct Aura{Unit*owner=nullptr,*caster=nullptr;bool removed=false;SpellInfo info;SpellInfo*m_spellInfo=&info;std::vector<AuraScript*>m_loadedScripts;Unit* GetUnitOwner(){return owner;}bool IsRemoved(){return removed;}void Remove(){removed=true;}void CallScriptEffectUpdateHandlers(uint32,AuraEffect*);void CallScriptAuraUpdateHandlers(uint32);};
struct AuraScript{Aura*m_aura=nullptr;Application*m_auraApplication=nullptr;int m_currentScriptState=SPELL_SCRIPT_STATE_NONE;Hook OnEffectUpdate,OnAuraUpdate,AfterEffectRemove,AfterEffectApply,OnEffectApply,OnEffectRemove,DoCalcMaxDuration,OnEffectPeriodic;
 virtual~AuraScript()=default;virtual bool Load(){return true;}virtual bool Validate(SpellInfo const*){return true;}virtual void Register(){}bool ValidateSpellInfo(std::initializer_list<int>){return true;}
 Aura* GetAura(){return m_aura;}SpellInfo* GetSpellInfo(){return &m_aura->info;}Application const* GetTargetApplication(){return m_auraApplication;}Unit*GetCaster(){return m_aura->caster;}Unit*GetTarget()const;Unit*GetUnitOwner()const;
 void _PrepareScriptCall(int state){m_currentScriptState=state;m_auraApplication=nullptr;}void _FinishScriptCall(){m_currentScriptState=SPELL_SCRIPT_STATE_NONE;}};
#include "Target.inc"
#include "Owner.inc"
#include "Dispatch.inc"
template<class T>Handler HandlerFor(void(T::*f)(uint32,AuraEffect*),int i){Handler h;h.index=i;h.effect=[f](AuraScript*s,uint32 d,AuraEffect*e){(static_cast<T*>(s)->*f)(d,e);};return h;}
template<class T>Handler HandlerFor(void(T::*f)(uint32)){Handler h;h.update=[f](AuraScript*s,uint32 d){(static_cast<T*>(s)->*f)(d);};return h;}
template<class T>Handler HandlerFor(void(T::*f)(AuraEffect const*,AuraEffectHandleModes),int i){Handler h;h.index=i;h.simple=[f](AuraScript*s){(static_cast<T*>(s)->*f)(nullptr,1);};return h;}
template<class T>Handler DurationHandler(void(T::*)(int32&)){return {};}
struct SpellScriptLoader{explicit SpellScriptLoader(char const*){}virtual~SpellScriptLoader()=default;virtual AuraScript*GetAuraScript()const{return nullptr;}};
template<class T>Handler HandlerFor(void(T::*f)(AuraEffect const*),int i){Handler h;h.index=i;h.simple=[f](AuraScript*s){(static_cast<T*>(s)->*f)(nullptr);};return h;}
struct SpellScript{Unit*fixtureCaster=nullptr;struct CastHook{template<class T>void operator+=(T){}}AfterCast,OnCheckCast;virtual~SpellScript()=default;virtual void Register(){}Unit*GetCaster(){return fixtureCaster;}};
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
#include "Selected.inc"
struct Binding{Aura aura;Application app;Binding(AuraScript&s,Unit&owner,Unit*caster):app{&owner}{aura.owner=&owner;aura.caster=caster;s.m_aura=&aura;s.Load();s.Register();aura.m_loadedScripts={&s};Set(s);}void Set(AuraScript&s){s.m_auraApplication=&app;s.m_currentScriptState=AURA_SCRIPT_HOOK_EFFECT_AFTER_APPLY;}void Tick(uint32 d){AuraEffect e;aura.CallScriptEffectUpdateHandlers(d,&e);aura.CallScriptAuraUpdateHandlers(d);}};
void Require(bool v,char const*m){if(!v){std::cerr<<"FAIL "<<m<<'\n';std::exit(1);}}
int main(){
#if defined(CASE_BARDING)
 Player p;p.skills.insert(165);spell_stonehide_leather_barding s;Binding b(s,p,&p);int32 duration=1;s.CalculateMaxDuration(duration);Require(duration==28800000,"leatherworker28.8m-ms");p.skills.clear();s.CalculateMaxDuration(duration);Require(duration==7200000,"ordinary duration");Unit npc;b.aura.caster=&npc;s.CalculateMaxDuration(duration);Require(duration==7200000,"NPC receives ordinary duration safely");b.aura.caster=nullptr;s.CalculateMaxDuration(duration);Require(duration==7200000,"missing caster duration");
#elif defined(CASE_BLESSING)
 Player p;spell_underlight_blessing s;Binding b(s,p,&p);p.water=true;b.aura.caster=nullptr;b.Tick(1499);Require(p.casts.empty(),"original1.5s deadline");b.Tick(1);Require(p.HasAura(188042),"missing caster: owner swimming form");p.water=false;b.Tick(1500);Require(!p.HasAura(188042),"leaving water removes form");b.aura.owner=nullptr;b.Tick(5000);
#elif defined(CASE_ROD)
 Player p;spell_underlight_main_aura s;Binding b(s,p,&p);b.aura.caster=nullptr;s.HandleEffectApply(nullptr,1);Require(p.HasSpell(201891),"missing caster owner learns fishing ability");p.auras.insert(216441);s.HandleEffectRemove(nullptr,1);Require(!p.HasSpell(201891)&&!p.HasAura(216441),"owner removes ability and ownhelper");Unit npc;b.aura.owner=&npc;s.HandleEffectApply(nullptr,1);s.HandleEffectRemove(nullptr,1);b.aura.owner=nullptr;s.HandleEffectRemove(nullptr,1);
#elif defined(CASE_UNDERCURRENT)
 Player p;p.node=true;p.position={10,20,30,1};spell_undercurrent_fishing_tele s;s.fixtureCaster=&p;Require(s.CheckElevation()==SPELL_CAST_OK,"actual fishing-hole gate");s.HandleAfterCast();p.mapId=1;p.Advance(500);
#ifdef COUNTERFACTUAL
 Require(p.moves==1,"original stale travel");std::cout<<"COUNTERFACTUAL Undercurrent applies old fishing-pool coordinates on another map\n";return 0;
#else
 Require(!p.moves,"foreign-map travel refused");for(int mode=0;mode<6;++mode){p.world=p.alive=true;p.preDelete=p.pending=false;p.mapId=1220;p.instance=1;s.HandleAfterCast();if(mode==0)p.instance=2;if(mode==1)p.pending=true;if(mode==2)p.alive=false;if(mode==3)p.preDelete=true;if(mode==4)p.world=false;if(mode==5)p.mapId=1;p.Advance(500);Require(!p.moves,"late admission");}p.world=p.alive=true;p.pending=p.preDelete=false;s.HandleAfterCast();p.Advance(499);Require(!p.moves,"500ms deadline");p.Advance(1);Require(p.moves==1,"ordinary selected pool");p.node=false;s.HandleAfterCast();Require(p.events.empty(),"missing pool");p.combat=true;Require(s.CheckElevation()==SPELL_FAILED_AFFECTING_COMBAT,"combatgate");s.fixtureCaster=nullptr;s.HandleAfterCast();
#endif
#elif defined(CASE_ZEN)
 Player p;spell_monk_remove_zen_flight::spell_monk_remove_zen_flight_AuraScript s;Binding b(s,p,&p);p.mapId=1669;b.aura.caster=nullptr;b.Tick(999);Require(!b.aura.removed,"initial1s");b.Tick(1);Require(b.aura.removed,"owner cannot fly on Argus after caster loss");b.aura.removed=false;p.mapId=1220;p.spells.insert(233368);b.Tick(2000);Require(!b.aura.removed,"Legion flying entitlement retained");p.spells.clear();b.Tick(2000);Require(b.aura.removed,"missing entitlement");b.aura.owner=nullptr;b.Tick(2000);
#elif defined(CASE_T21)
 Player p;spell_pal_t21_prot s;Binding b(s,p,&p);b.Tick(333);b.Tick(333);b.Tick(333);Require(p.casts.empty(),"predeadline");b.Tick(333);Require(p.casts.size()==1,"first initial statcast");b.Tick(50);
#ifdef COUNTERFACTUAL
 Require(p.casts.size()==2,"original overshoot repeats");std::cout<<"COUNTERFACTUAL Paladin T21 overshot initial deadline repeats253331 every update;exact deadline stops\n";return 0;
#else
 Require(p.casts.size()==1,"overshot deadline rearmed");b.Tick(950);Require(p.casts.size()==2,"next exact deadline still refreshes");b.Tick(100000);Require(p.casts.size()==3,"lag emits only one bounded refresh");p.auras.insert(251870);b.Tick(1000);Require(p.amounts.back()==120,"native4P without Shield");p.auras.insert(132403);b.Tick(999);Require(p.amounts.back()==120,"no early4Prefresh");b.Tick(1);Require(p.amounts.back()==100,"Shield disables4Pbonus");p.auras.erase(132403);b.Tick(1000);Require(p.amounts.back()==120,"Shield expiry restores4Pbonus");s.OnRemove(nullptr,1);Require(!p.HasAura(253331),"native cleanup");spell_pal_t21_prot missing;Binding m(missing,p,nullptr);m.Tick(1001);Require(missing.update==1000,"missing caster rearms bounded retry");m.aura.caster=&p;m.Tick(1000);Require(p.HasAura(253331),"caster recovery at next period");
#endif
#elif defined(CASE_WHISPER)
 Player p;spell_request_the_master_call_on_you s;Binding b(s,p,&p);b.aura.caster=nullptr;s.OnApply(nullptr,1);s.OnTick(nullptr);Require(p.whispers==2,"both owner text endpoints after casterloss");sBroadcastTextStore.missing=true;s.OnTick(nullptr);Require(p.whispers==2,"missing broadcast refuses");b.aura.owner=nullptr;s.OnTick(nullptr);
#elif defined(CASE_WINNING)
 Player p;spell_gen_winning_hand s;Binding b(s,p,&p);b.aura.caster=nullptr;s.OnApply(nullptr,1);Require(p.emotes==1,"owner emote after casterloss");sBroadcastTextStore.missing=true;s.OnApply(nullptr,1);Require(p.emotes==1,"missing text");b.aura.owner=nullptr;s.OnApply(nullptr,1);
#elif defined(CASE_GUMBO)
 Player p;spell_gen_gobelin_gumbo::spell_gen_gobelin_gumbo_AuraScript s;Binding b(s,p,nullptr);b.Tick(9999);Require(p.casts.empty(),"burp10s deadline");b.Tick(1);
#ifdef COUNTERFACTUAL
 Require(p.casts.empty(),"original missing caster stalls owner burp");std::cout<<"COUNTERFACTUAL Goblin Gumbo timer stops when caster no longer resolves although consumer aura survives\n";return 0;
#else
 Require(p.casts==std::vector<uint32>{42755},"consumer burp after casterloss");chance=false;b.Tick(10000);Require(p.casts.size()==1,"30pct chance gate retained");chance=true;b.Tick(10000);Require(p.casts.size()==2,"period rearmed");b.aura.owner=nullptr;b.Tick(10000);
#endif
#elif defined(CASE_SELFIE)
 Player p;spell_item_camera_selfie::spell_item_camera_selfie_AuraScript s;Binding b(s,p,nullptr);p.auras={181767,98765};s.OnRemove(nullptr,1);
#ifdef COUNTERFACTUAL
 Require(p.HasAura(181767)&&p.cancelled.empty(),"original casterloss leaks filter/kit");std::cout<<"COUNTERFACTUAL selfie cleanup skips surviving owner filter and54169 visual kit after casterloss\n";return 0;
#else
 Require(!p.HasAura(181767)&&p.HasAura(98765)&&p.cancelled==std::vector<uint32>{54169},"owner filter+kitcleanup retains unrelated aura");for(uint32 id:{181779u,181773u}){p.auras.insert(id);s.OnRemove(nullptr,1);Require(!p.HasAura(id),"each native filter removed");}b.aura.owner=nullptr;s.OnRemove(nullptr,1);
#endif
#endif
 Require(!errors,"no invalid application getter diagnostics");std::cout<<"PASS selected complete production class/native getter+update dispatcher ASAN;services controlled,not client traversal\n";return 0;
}
