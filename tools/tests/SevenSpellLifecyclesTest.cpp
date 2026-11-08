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
#include <vector>
using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;using int32=int32_t;
using AuraEffectHandleModes=int;
enum {EFFECT_0=0,SPELL_AURA_DUMMY=4,SPELL_AURA_MOUNTED=78,AURA_EFFECT_HANDLE_REAL=1,MINUTE=60,IN_MILLISECONDS=1000,PLAYER_FIELD_LOCAL_FLAGS=1,PLAYER_LOCAL_FLAG_CAN_USE_OBJECTS_MOUNTED=2,GAMEOBJECT_TYPE_GATHERING_NODE=50,SPELL_AURA_ALLOW_USING_GAMEOBJECTS_WHILE_MOUNTED=475,ALLIANCE=1,alliance_spell=179245,horde_spell=179244,alliance_npc=89715,horde_npc=89713,ride_veh=46598,recId=4050,SPELL_TOUCH_OF_ELUNE_DAY=154796,SPELL_TOUCH_OF_ELUNE_NIGHT=154797};
using GuidList=std::list<int>;
struct Position{float x=0,y=0,z=0,o=0;Position()=default;Position(float a,float b,float c,float d):x(a),y(b),z(c),o(d){}};
struct Map{bool forbidden=false;bool IsBattlegroundOrArena(){return forbidden;}bool IsRaid(){return false;}bool IsDungeon(){return false;}};
struct Vehicle{uint32 aura=0;void Reset(){}uint32 GetRecAura(){return aura;}};
struct Player;struct Creature;
struct Unit{
 bool world=true,alive=true,preDelete=false,failCast=false,node=false,interaction=false,nativeProvider=false;uint32 mapId=1220,instance=1;Map map;Position position;
 std::set<uint32> auras,skills;std::vector<uint32> casts;std::map<uint32,Creature*> rares;std::vector<std::shared_ptr<Creature>> summons;std::unique_ptr<Vehicle> vehicle;
 struct Event{uint32 due;std::function<void()> call;};std::vector<Event> events;uint32 clock=0;unsigned moves=0;
 virtual ~Unit()=default;virtual Player* ToPlayer(){return nullptr;}
 bool HasAura(uint32 id){return auras.count(id)!=0;}bool HasAuraType(int){return nativeProvider;}
 void CastSpell(Unit* target,uint32 id,bool){casts.push_back(id);if(!failCast)target->auras.insert(id);}
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
struct Player:Unit{bool pending=false;uint32 team=ALLIANCE;Player* ToPlayer()override{return this;}bool HasSkill(uint32 id){return skills.count(id)!=0;}uint32 GetTeam(){return team;}bool HasPendingTeleport(){return pending;}bool HasFlag(int,int){return interaction;}void SetFlag(int,int){interaction=true;}void RemoveFlag(int,int){interaction=false;}void* FindNearestGameObjectOfType(int type,float radius){assert(type==50&&radius==3);return node?this:nullptr;}};
struct Creature:Unit{uint32 entry=0;int guid=0;bool despawned=false;uint32 GetEntry(){return entry;}int GetGUID(){return guid;}void DespawnOrUnsummon(uint32 delay){assert(delay==100);despawned=true;}};
Creature* Unit::SummonCreature(uint32 entry,Position p){auto s=std::make_shared<Creature>();s->entry=entry;s->position=p;s->guid=int(summons.size()+1);summons.push_back(s);return s.get();}
Creature* Unit::GetCreature(Unit&owner,int guid){for(auto&s:owner.summons)if(s->guid==guid)return s.get();return nullptr;}
struct TaxiNodesEntry{struct{float X=10,Y=20,Z=30;}Pos;};struct TaxiStore{TaxiNodesEntry node;bool missing=false;TaxiNodesEntry const* LookupEntry(uint32){return missing?nullptr:&node;}}sTaxiNodesStore;
struct ObjectMgr{uint32 node=1;uint32 GetNearestTaxiNode(float,float,float,uint32,Player*){return node;}}objectMgr;auto sObjectMgr=&objectMgr;
namespace GameTime{tm value{};bool missing=false;tm const* GetDateAndTime(){return missing?nullptr:&value;}}
int errors=0;
struct SpellInfo{};struct AuraEffect{int GetEffIndex(){return 0;}};
#include "Hooks.inc"
#define TC_LOG_ERROR(...) (++errors)
struct AuraScript;struct Application{Unit* target;Unit* GetTarget(){return target;}};
struct Handler{int index=0;std::function<void(AuraScript*,uint32,AuraEffect*)> effect;std::function<void(AuraScript*,uint32)> update;std::function<void(AuraScript*)> simple;
 bool IsEffectAffected(SpellInfo*,int i){return i==index;}void Call(AuraScript*s,uint32 d,AuraEffect*e){effect(s,d,e);}void Call(AuraScript*s,uint32 d){update(s,d);}void Call(AuraScript*s){if(simple)simple(s);}};
struct Hook:std::vector<Handler>{void operator+=(Handler h){push_back(std::move(h));}};
struct Aura{Unit*owner=nullptr,*caster=nullptr;bool removed=false;SpellInfo info;SpellInfo*m_spellInfo=&info;std::vector<AuraScript*>m_loadedScripts;Unit* GetUnitOwner(){return owner;}bool IsRemoved(){return removed;}void CallScriptEffectUpdateHandlers(uint32,AuraEffect*);void CallScriptAuraUpdateHandlers(uint32);};
struct AuraScript{Aura*m_aura=nullptr;Application*m_auraApplication=nullptr;int m_currentScriptState=SPELL_SCRIPT_STATE_NONE;Hook OnEffectUpdate,OnAuraUpdate,AfterEffectRemove,AfterEffectApply,OnEffectApply,OnEffectRemove,DoCalcMaxDuration;
 virtual~AuraScript()=default;virtual bool Load(){return true;}virtual bool Validate(SpellInfo const*){return true;}virtual void Register(){}bool ValidateSpellInfo(std::initializer_list<int>){return true;}
 Unit*GetCaster(){return m_aura->caster;}Unit*GetTarget()const;Unit*GetUnitOwner()const;
 void _PrepareScriptCall(int state){m_currentScriptState=state;m_auraApplication=nullptr;}void _FinishScriptCall(){m_currentScriptState=SPELL_SCRIPT_STATE_NONE;}};
#include "Target.inc"
#include "Owner.inc"
#include "Dispatch.inc"
template<class T>Handler HandlerFor(void(T::*f)(uint32,AuraEffect*),int i){Handler h;h.index=i;h.effect=[f](AuraScript*s,uint32 d,AuraEffect*e){(static_cast<T*>(s)->*f)(d,e);};return h;}
template<class T>Handler HandlerFor(void(T::*f)(uint32)){Handler h;h.update=[f](AuraScript*s,uint32 d){(static_cast<T*>(s)->*f)(d);};return h;}
template<class T>Handler HandlerFor(void(T::*f)(AuraEffect const*,AuraEffectHandleModes),int i){Handler h;h.index=i;h.simple=[f](AuraScript*s){(static_cast<T*>(s)->*f)(nullptr,1);};return h;}
template<class T>Handler DurationHandler(void(T::*)(int32&)){return {};}
struct SpellScript{Unit*fixtureCaster=nullptr;struct CastHook{template<class T>void operator+=(T){}}AfterCast;virtual~SpellScript()=default;virtual void Register(){}Unit*GetCaster(){return fixtureCaster;}};
#define PrepareAuraScript(name) public:
#define PrepareSpellScript(name) public:
#define AuraEffectUpdateFn(fn,i,a) HandlerFor(&fn,i)
#define AuraUpdateFn(fn) HandlerFor(&fn)
#define AuraEffectApplyFn(fn,i,a,m) HandlerFor(&fn,i)
#define AuraEffectRemoveFn(fn,i,a,m) HandlerFor(&fn,i)
#define AuraCalcMaxDurationFn(fn) DurationHandler(&fn)
#define SpellCastFn(fn) &fn
#include "Selected.inc"
struct Binding{Aura aura;Application app;Binding(AuraScript&s,Unit&owner,Unit*caster):app{&owner}{aura.owner=&owner;aura.caster=caster;s.m_aura=&aura;s.Load();s.Register();aura.m_loadedScripts={&s};s.m_auraApplication=&app;s.m_currentScriptState=AURA_SCRIPT_HOOK_EFFECT_AFTER_APPLY;}void Tick(uint32 d){AuraEffect e;aura.CallScriptEffectUpdateHandlers(d,&e);aura.CallScriptAuraUpdateHandlers(d);}void Remove(AuraScript&s){s.m_currentScriptState=AURA_SCRIPT_HOOK_EFFECT_AFTER_REMOVE;s.m_auraApplication=&app;for(auto&h:s.AfterEffectRemove)h.Call(&s);for(auto&h:s.OnEffectRemove)h.Call(&s);}};
void Require(bool yes,char const*message){if(!yes){std::cerr<<"FAIL "<<message<<'\n';std::exit(1);}}
int main(){
#if defined(CASE_ELUNE)
 Player player;spell_gen_touch_of_elune script;Binding b(script,player,&player);GameTime::value.tm_hour=5;script.HandleApply(nullptr,1);Require(player.HasAura(154797),"night application");GameTime::value.tm_hour=6;b.Tick(59999);Require(player.HasAura(154797),"minute boundary");b.Tick(1);
#ifdef COUNTERFACTUAL
 Require(errors==1&&player.HasAura(154797)&&!player.HasAura(154796),"pinned no-application getter failure");std::cout<<"COUNTERFACTUAL Elune minute update calls application-only GetTarget, logs error, leaves night aura at daybreak\n";
#else
 Require(!errors&&player.HasAura(154796)&&!player.HasAura(154797),"real no-application dispatch changes day aura");auto casts=player.casts.size();b.Tick(60000);Require(player.casts.size()==casts,"no duplicate stat cast");GameTime::value.tm_hour=17;b.Tick(60000);Require(player.HasAura(154796),"17h day");GameTime::value.tm_hour=18;b.Tick(90000);Require(player.HasAura(154797)&&!player.HasAura(154796),"18h night overshoot");b.Remove(script);Require(player.auras.empty(),"remove both owned stats");b.aura.owner=nullptr;b.Tick(60000);Require(!errors,"missing owner");
#endif
#elif defined(CASE_BLACKSMITH)
 Player p;p.auras={182993,209563};spell_item_leystone_hoofplates hoof;spell_item_demonsteel_stirrups stir;Binding h(hoof,p,&p),s(stir,p,&p);for(int i=0;i<40;i++){h.Tick(50);s.Tick(50);}
#ifdef COUNTERFACTUAL
 Require(p.casts.empty(),"pinned normal ticks fail both consumables");std::cout<<"COUNTERFACTUAL both mount consumable timers never decrease at 50ms update, no buffs after 2s\n";
#else
 Require(p.HasAura(233043)&&p.HasAura(233044)&&p.casts.size()==2,"normal ticks cast both native buffs");h.Tick(100000);s.Tick(100000);Require(p.casts.size()==2,"existing buffs/overshoot");h.Remove(hoof);s.Remove(stir);Require(!p.HasAura(233043)&&!p.HasAura(233044),"remove owned buffs");p.mapId=1;h.Tick(2000);s.Tick(2000);Require(p.casts.size()==2,"wrong map");p.mapId=1220;p.failCast=true;h.Tick(2000);s.Tick(2000);Require(p.casts.size()==4&&!p.HasAura(233044),"failed cast no grants");h.Tick(1999);s.Tick(1999);Require(p.casts.size()==4,"bounded retry");p.failCast=false;h.Tick(1);s.Tick(1);Require(p.casts.size()==6,"retry succeeds");int32 duration=0;hoof.CalculateMaxDuration(duration);Require(duration==7200000,"ordinary duration");p.skills.insert(164);stir.CalculateMaxDuration(duration);Require(duration==28800000,"blacksmith duration");Unit npc;h.aura.caster=&npc;hoof.CalculateMaxDuration(duration);Require(duration==7200000,"nonplayer duration safe");h.aura.caster=nullptr;h.Tick(2000);
#endif
#elif defined(CASE_GOLEM)
 Player p;p.skills.insert(182);p.node=true;spell_item_golem golem;Binding b(golem,p,&p);b.Tick(499);Require(!p.interaction,"500ms initial delay");b.Tick(1);Require(p.interaction,"nearby herb enables interaction");b.Remove(golem);
#ifdef COUNTERFACTUAL
 Require(p.interaction,"pinned mount leaves leaked interaction flag");std::cout<<"COUNTERFACTUAL Sky Golem grants mounted interaction and never releases it on dismount\n";
#else
 Require(!p.interaction,"dismount clears own flag");b.Tick(2000);p.skills.clear();b.Tick(2000);Require(!p.interaction,"profession loss releases");p.skills.insert(182);b.Tick(2000);p.map.forbidden=true;b.Tick(2000);Require(!p.interaction,"instance entry releases");p.map.forbidden=false;b.Tick(2000);p.nativeProvider=true;b.Remove(golem);Require(p.interaction,"native other aura retained");p.nativeProvider=false;p.interaction=true;spell_item_golem preexisting;Binding other(preexisting,p,&p);other.Tick(500);p.node=false;other.Tick(2000);other.Remove(preexisting);Require(p.interaction,"preexisting flag retained");other.aura.owner=nullptr;other.Tick(2000);other.Remove(preexisting);
#endif
#elif defined(CASE_CHAUFFEUR)
 Player p;p.auras.insert(179245);auto old=std::make_unique<spell_mount_summon_chauffeur>();Binding b(*old,p,&p);old->OnApply(nullptr,1);Require(p.vehicle&&p.events.size()==1,"pending real mount attempt");b.Remove(*old);p.RemoveVehicleKit();old.reset();spell_mount_summon_chauffeur next;Binding n(next,p,&p);next.OnApply(nullptr,1);
#ifdef COUNTERFACTUAL
 // Execute the pinned dangling-this callback under ASAN only in a separately captured
 // expected-failing run; ordinary counterfactual proves stale callback is still queued.
 Require(p.events.size()==2,"pinned stale callback persists after aura destruction");std::cout<<"COUNTERFACTUAL destroyed chauffeur AuraScript remains captured by the old queued callback; use -Baseline -AddressSanitizer for UAF evidence\n";
#ifdef RUN_DANGLING_BASELINE
 p.Advance(500);
#endif

#else
 p.Advance(499);Require(p.summons.empty(),"not before delay");p.Advance(1);Require(p.summons.size()==1&&p.summons[0]->entry==89715,"cancelled destroyed attempt cannot touch replacement; one alliance driver");n.Remove(next);Require(p.summons[0]->despawned,"own driver cleanup");p.RemoveVehicleKit();p.team=2;p.auras={179244};spell_mount_summon_chauffeur horde;Binding h(horde,p,&p);horde.OnApply(nullptr,1);p.Advance(500);Require(p.summons.size()==2&&p.summons.back()->entry==89713,"horde driver");h.Remove(horde);p.RemoveVehicleKit();spell_mount_summon_chauffeur dead;Binding d(dead,p,&p);dead.OnApply(nullptr,1);p.alive=false;p.Advance(500);Require(p.summons.size()==2,"dead owner no driver");d.Remove(dead);p.RemoveVehicleKit();p.alive=true;spell_mount_summon_chauffeur missing;Binding m(missing,p,&p);m.aura.owner=nullptr;missing.OnApply(nullptr,1);m.Remove(missing);
#endif
#elif defined(CASE_CRYSTAL)
 Player p;Creature first,last;first.entry=91871;first.position={1,2,3,0};last.entry=98285;last.position={9,8,7,0};p.rares={{91871,&first},{98285,&last}};spell_masters_hunter_seeking_crystal_teleport crystal;crystal.fixtureCaster=&p;crystal.HandleAfterCast();
#ifdef COUNTERFACTUAL
 Require(p.events.size()==2,"pinned multiple moves");p.Advance(500);Require(p.moves==2&&p.position.x==9,"pinned last entry wins");std::cout<<"COUNTERFACTUAL one crystal use teleports twice when two authored rares are in range; last entry wins\n";
#else
 Require(p.events.size()==1,"one target one move");p.Advance(499);Require(!p.moves,"real delay");p.Advance(1);Require(p.moves==1&&p.position.x==1,"first eligible priority retained");for(int mode=0;mode<6;mode++){p.world=true;p.alive=true;p.preDelete=false;p.pending=false;p.mapId=1220;p.instance=1;crystal.HandleAfterCast();if(mode==0)p.mapId=1;if(mode==1)p.instance=2;if(mode==2)p.pending=true;if(mode==3)p.alive=false;if(mode==4)p.preDelete=true;if(mode==5)p.world=false;p.Advance(500);Require(p.moves==1,"stale state cancels");}p.world=true;p.alive=true;p.preDelete=false;p.pending=false;p.rares.clear();crystal.HandleAfterCast();Require(p.events.empty(),"no rare no move");Creature special;special.entry=96424;p.rares[96424]=&special;crystal.HandleAfterCast();Require(p.casts.size()==1&&p.casts[0]==190820,"special rare buff preserved");p.Advance(500);Require(p.moves==2,"special move");crystal.fixtureCaster=nullptr;crystal.HandleAfterCast();
#endif
#elif defined(CASE_ARGUS)
 Player p;spell_flight_masters_whistle_argus whistle;whistle.fixtureCaster=&p;whistle.HandleAfterCast();p.mapId=1;p.Advance(500);
#ifdef COUNTERFACTUAL
 Require(p.moves==1&&p.position.x==10,"pinned stale taxi coordinates applied on another map");std::cout<<"COUNTERFACTUAL Argus whistle applies old taxi XYZ after caster changes map in its 500ms delay\n";
#else
 Require(!p.moves,"map change cancels");for(int mode=0;mode<5;mode++){p.world=true;p.alive=true;p.preDelete=false;p.pending=false;p.instance=1;whistle.HandleAfterCast();if(mode==0)p.instance=2;if(mode==1)p.pending=true;if(mode==2)p.alive=false;if(mode==3)p.preDelete=true;if(mode==4)p.world=false;p.Advance(500);Require(!p.moves,"stale lifecycle cancels");}p.world=true;p.alive=true;p.preDelete=false;p.pending=false;whistle.HandleAfterCast();p.Advance(499);Require(!p.moves,"native delay");p.Advance(1);Require(p.moves==1&&p.position.x==10&&p.position.y==20,"selected taxi delivery");objectMgr.node=0;whistle.HandleAfterCast();Require(p.events.empty(),"no eligible taxi");objectMgr.node=1;sTaxiNodesStore.missing=true;whistle.HandleAfterCast();Require(p.events.empty(),"missing taxi record");Unit npc;whistle.fixtureCaster=&npc;whistle.HandleAfterCast();whistle.fixtureCaster=nullptr;whistle.HandleAfterCast();
#endif
#endif
#ifndef COUNTERFACTUAL
 std::cout<<"PASS extracted full production script with native aura getters/dispatch where applicable; lifecycle alternatives checked; modeled endpoints are not a full native/client traversal\n";
#endif
 return 0;
}
