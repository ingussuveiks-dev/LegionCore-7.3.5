#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <deque>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <vector>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;using SpellEffIndex=uint32;
enum {CLASS_WARRIOR=1,QUEST_STATUS_INCOMPLETE=3,CURRENT_CHANNELED_SPELL=1,UNIT_FIELD_CREATED_BY_SPELL=2,EFFECT_0=0,EFFECT_1=1,EFFECT_ALL=99,SPELL_EFFECT_ANY=99,SPELL_AURA_TRANSFORM=56,SPELL_AURA_PERIODIC_TRIGGER_SPELL=23,SPELL_EFFECT_TELEPORT_L=252,SPELL_EFFECT_TRIGGER_MISSILE=32,SPELL_EFFECT_SUMMON=28,SMART_ACTION_UPDATE_ACHIEVEMENT_CRITERIA=205,SMART_EVENT_DEATH=6,DONE=1,ENCOUNTER_FRAME_DISENGAGE=2,ENCOUNTER_FRAME_INSTANCE_END=3,SMART_ESCORT_ESCORTING=1};
enum SpellCastResult {SPELL_CAST_OK,SPELL_FAILED_BAD_TARGETS};enum ReactStates{REACT_PASSIVE,REACT_AGGRESSIVE};using CriteriaTypes=uint32;
enum {SMART_ACTION_INVOKER_CAST=85,SMART_EVENT_LINK=61,SMARTCAST_INVOKER_PLAYER=256,SMART_TARGET_ACTION_INVOKER_PLAYER=29,SMARTCAST_INTERRUPT_PREVIOUS=1,SMARTCAST_TRIGGERED=2,SMARTCAST_AURA_NOT_PRESENT=32,SPELL_EFFECT_KILL_CREDIT=90,SPELL_EFFECT_KILL_CREDIT2=134};
uint32 warnings=0;
#define TC_LOG_WARN(...) (++warnings)
#define TC_LOG_DEBUG(...) ((void)0)
#define TC_LOG_ERROR(...) assert(false)
struct ObjectGuid {uint64 id=0;void Clear(){id=0;}bool IsEmpty()const{return !id;}bool operator==(ObjectGuid r)const{return id==r.id;}bool operator!=(ObjectGuid r)const{return id!=r.id;}};
struct Position {float x=0,y=0,z=0,o=0;float GetPositionZ()const{return z;}float GetExactDist2d(Position const*p)const{return std::hypot(x-p->x,y-p->y);}float GetExactDist(Position const*p)const{return std::hypot(GetExactDist2d(p),z-p->z);}};
struct Map{} world,elsewhere;
struct Player;struct Unit;struct Creature;struct TempSummon;struct CreatureAI;
struct WorldObject:Position {virtual ~WorldObject()=default;virtual Unit* ToUnit(){return nullptr;}virtual ObjectGuid GetGUID()const{return {};}virtual bool IsInWorld()const{return true;}virtual Map*GetMap()const{return &world;}bool HasQuestObjectScope(){return false;}bool CanTargetQuestObject(Unit*){return true;}};
struct EffectData {uint32 Effect=0,ApplyAuraName=0,TriggerSpell=0,MiscValue=0,MiscValueB=0;bool IsEffect()const{return Effect!=0;}};
using SpellEffectInfo=EffectData;
struct SpellInfo {uint32 Id=0;EffectData first,second;EffectData* Effects[2]{&first,&second};explicit SpellInfo(uint32 id=0):Id(id){if(id==207244){first.Effect=6;first.ApplyAuraName=56;}if(id==207249)first.Effect=252;if(id==207297){first.Effect=32;first.TriggerSpell=207299;}if(id==207299){first.Effect=28;first.MiscValue=104737;first.MiscValueB=64;}if(id==207258||id==207275){second.ApplyAuraName=23;second.TriggerSpell=207274;}}bool HasEffect(uint32 type)const{return first.Effect==type||second.Effect==type;}};
struct SpellManager {SpellInfo scene{204743};SpellInfo const*GetSpellInfo(uint32 id){assert(id==204743);return &scene;}}spellManager;auto*sSpellMgr=&spellManager;
std::vector<std::function<void()>> delayed;uint32 sceneAttempts=0;bool sceneRejected=false;
struct Spell {SpellInfo info;SpellInfo const* GetSpellInfo(){return &info;}};
struct QuestStatusData {std::shared_ptr<uint8> ScriptLifetime=std::make_shared<uint8>();};
struct Unit:WorldObject {
    ObjectGuid guid;Map*map=&world;bool inWorld=true,alive=true,combat=true,contact=true,teleport=false,friendly=false;uint32 phase=1;Player* owner=nullptr;std::set<uint32> auras;Spell channel;
    Unit*ToUnit()override{return this;}virtual Player*ToPlayer(){return nullptr;}virtual Creature*ToCreature(){return nullptr;}
    Player*GetCharmerOrOwnerPlayerOrPlayerItself();ObjectGuid GetGUID()const{return guid;}Map*GetMap()const{return map;}
    bool IsInWorld()const{return inWorld;}bool IsAlive()const{return alive;}bool isInCombat()const{return combat;}bool CanContact()const{return contact;}bool HasPendingTeleport()const{return teleport;}
    bool InSamePhase(Unit*p)const{return phase==p->phase;}bool InSamePhase(WorldObject*p)const{return p&&p->ToUnit()&&phase==p->ToUnit()->phase;}bool IsWithinDistInMap(Unit*p,float d)const{return map==p->map&&GetExactDist(p)<=d;}bool IsWithinDistInMap(WorldObject*p,float d)const{return p&&p->ToUnit()&&IsWithinDistInMap(p->ToUnit(),d);}
    bool IsFriendlyTo(Unit*){return friendly;}bool HasAura(uint32 id,ObjectGuid={})const{return auras.count(id)!=0;}
    void InterruptNonMeleeSpells(bool){channel.info.Id=0;}void AddDelayedEvent(uint32 delay,std::function<void()> callback){assert(delay==200);delayed.push_back(callback);}
    void RemoveAurasDueToSpell(uint32 id,ObjectGuid){auras.erase(id);}Spell*GetCurrentSpell(uint32){return channel.info.Id?&channel:nullptr;}
    void InterruptSpell(uint32){channel.info.Id=0;}SpellCastResult CastSpell(Position const&,uint32,bool);SpellCastResult CastSpell(Unit*,uint32,bool);
};
struct Player:Unit {
    uint32 cls=1,status=3,group=1,credit=0,creditType=0,creditId=0;bool rewarded=false,hasQuest=true,overrideGroup=false;QuestStatusData quest;
    Player*ToPlayer()override{return this;}uint32 getClass(){return cls;}uint32 GetQuestStatus(uint32){return status;}bool GetQuestRewardStatus(uint32){return rewarded;}
    QuestStatusData*getQuestStatus(uint32){return hasQuest?&quest:nullptr;}bool IsInSameGroupWith(Player*p)const{return p==this||(group&&group==p->group);}
    void UpdateAchievementCriteria(CriteriaTypes t,uint32 id,uint32,uint32,Unit*,bool force){++credit;creditType=t;creditId=id;overrideGroup=force;}
};
Player*Unit::GetCharmerOrOwnerPlayerOrPlayerItself(){return ToPlayer()?ToPlayer():owner;}
struct Creature:Unit {
    uint32 entry=103339,spawn=370996,mapId=1539,created=0;CreatureAI*ai=nullptr;bool IsAIEnabled=true,blocked=false;ReactStates react=REACT_AGGRESSIVE;
    Creature*ToCreature()override{return this;}virtual TempSummon*ToTempSummon(){return nullptr;}
    uint32 GetEntry()const{return entry;}uint32 GetDBTableGUIDLow()const{return spawn;}uint32 GetMapId()const{return mapId;}uint32 GetInstanceId()const{return 42;}
    uint32 GetUInt32Value(uint32)const{return created;}Position GetPosition()const{return *this;}CreatureAI*AI(){return ai;}
    void MovePositionToFirstCollision(Position&p,float d,float angle){if(blocked)return;p.x+=d*std::cos(o+angle);p.y+=d*std::sin(o+angle);}
    void SetReactState(ReactStates r){react=r;}void CombatStop(bool){combat=false;}void DespawnOrUnsummon(){inWorld=false;}
};
struct TempSummon:Creature {Creature*summoner=nullptr;TempSummon*ToTempSummon()override{return this;}ObjectGuid GetSummonerGUID()const{return summoner?summoner->guid:ObjectGuid{};}Unit*GetSummoner()const{return summoner;}};
std::map<uint64,Unit*> registry;
namespace ObjectAccessor {Player*GetPlayer(Creature&,ObjectGuid id){auto i=registry.find(id.id);return i==registry.end()?nullptr:i->second->ToPlayer();}Creature*GetCreature(Creature&,ObjectGuid id){auto i=registry.find(id.id);return i==registry.end()?nullptr:i->second->ToCreature();}}
namespace ObjectAccessor {Unit*GetUnit(Unit&,ObjectGuid id){auto i=registry.find(id.id);return i==registry.end()?nullptr:i->second;}WorldObject*GetWorldObject(Unit&u,ObjectGuid id){return GetUnit(u,id);}}
bool IsUnit(WorldObject*p){return p&&p->ToUnit();}
struct Scenario {uint32 id=1037,step=6;uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;}}scenarioState;
struct Manager {bool present=true;Scenario*GetScenario(uint32){return present?&scenarioState:nullptr;}} manager;auto*sScenarioMgr=&manager;
using ObjectList=std::vector<WorldObject*>;
#include "CreditPlayer.inc"
#include "CreditCaster.inc"
uint32 nativeTarget=7,nativeCriteriaType=92,nativeCriteriaId=49058;
struct Event {uint32 type=205,eventType=61,link=0,event_id=4,entryOrGuid=103339;struct {struct {uint32 type=92,misc1=49058,misc2=0,misc3=0;}achievementCriteria;struct {uint32 spell=204743,flags=0;}cast;}action;struct {struct {uint32 dist=0;}playerDistance;}target;uint32 GetTargetType(){return 7;}uint32 GetActionType(){return type;}uint32 GetEventType(){return eventType;}uint32 GetScriptType(){return 0;}};
using SmartScriptHolder=Event;struct GameObject{};
struct NativeCredit {
    Unit*invoker=nullptr;WorldObject*base=nullptr;bool duplicate=false;WorldObject*GetBaseObject(){return base;}
    ObjectList*GetTargets(Event const&,Unit*){assert(nativeTarget==7);if(!invoker)return nullptr;auto r=new ObjectList{invoker};if(duplicate)r->push_back(invoker);return r;}
    Unit*GetLastInvoker(){return invoker;}
    Event FindLinkedEvent(uint32 link){assert(link==4);Event e;e.action.achievementCriteria.type=nativeCriteriaType;e.action.achievementCriteria.misc1=nativeCriteriaId;return e;}
    void ProcessEvent(Event&e,Unit*u,uint32 a,uint32 b,bool c,SpellInfo const*d,GameObject*g){RunAction(e,u,a,b,c,d,g);}
    void RunAction(Event&e,Unit*unit,uint32 var0=0,uint32 var1=0,bool bvar=false,SpellInfo const*spell=nullptr,GameObject*gob=nullptr){switch(e.type){
#include "InvokerCast.inc"
#include "CreditAction.inc"
    }
#include "LinkTail.inc"
    void Run(Unit*unit){Event e;e.type=85;e.eventType=6;e.link=4;e.event_id=3;RunAction(e,unit);}
};
struct NativeScript {NativeCredit credit;Unit*last=nullptr;uint32 deaths=0;void ProcessEventsFor(uint32,Unit*u){last=u;++deaths;credit.invoker=u;credit.Run(u);}};
struct CreatureAI {virtual ~CreatureAI()=default;};
struct Instance {void SetBossState(uint32,uint32){}void SaveToDB(){}void SendEncounterUnit(uint32,Creature*){}};
struct SmartAI:CreatureAI {
    Creature*me;NativeScript script;Instance*instance=nullptr;uint32 _bossId=0,aggros=0,hits=0,updates=0,summons=0;
    explicit SmartAI(Creature*p):me(p){p->ai=this;script.credit.base=p;}
    virtual void Reset(){}virtual void EnterCombat(Unit*){++aggros;}virtual void JustDied(Unit*);virtual void SpellHit(Unit*,SpellInfo const*){++hits;}
    virtual void JustSummoned(Creature*){++summons;}virtual void UpdateAI(uint32){++updates;}NativeScript*GetScript(){return &script;}
    bool HasEscortState(uint32){return false;}void EndPath(bool){}
};
#include "SmartDeath.inc"
struct Hooks {template<class T>void operator+=(T){}};
struct SpellScript {
    Unit*caster=nullptr;SpellInfo packetInfo;Position dest;bool hasDest=false,prevented=false;
    virtual ~SpellScript()=default;virtual bool Load(){return true;}virtual bool Validate(SpellInfo const*){return true;}virtual void Register(){}
    Unit*GetCaster(){return caster;}SpellInfo const*GetSpellInfo(){return &packetInfo;}Position*GetExplTargetDest(){return hasDest?&dest:nullptr;}
    bool ValidateSpellInfo(std::initializer_list<uint32>){return true;}void PreventHitDefaultEffect(uint32){prevented=true;}
    Hooks OnCheckCast,OnEffectHit,OnEffectHitTarget;
};
#define PrepareSpellScript(...) public:
#define SpellCheckCastFn(...) 0
#define SpellEffectFn(...) 0
#include "../../src/server/scripts/Legion/ClassHalls/stromkar_split.h"
struct Pending {std::unique_ptr<spell_stromkar_split_transport> script;Unit*caster;uint32 id;Position dest;};
std::deque<Pending> pending;std::vector<std::unique_ptr<TempSummon>> copies;uint32 rejectId=0,missingId=0;uint64 nextGuid=100;
SpellCastResult Request(Unit*unit,Position const*dest,uint32 id){
    if(id==204743){++sceneAttempts;return sceneRejected?SPELL_FAILED_BAD_TARGETS:SPELL_CAST_OK;}
    if(rejectId==id)return SPELL_FAILED_BAD_TARGETS;
    auto s=std::make_unique<spell_stromkar_split_transport>();s->caster=unit;s->packetInfo=SpellInfo(id);s->packetInfo.Effects[0]=&s->packetInfo.first;s->packetInfo.Effects[1]=&s->packetInfo.second;
    if(dest){s->dest=*dest;s->hasDest=true;}assert(s->Validate(s->GetSpellInfo()));
    if(!s->Load()||s->Check()!=SPELL_CAST_OK)return SPELL_FAILED_BAD_TARGETS;
    if(missingId!=id)pending.push_back({std::move(s),unit,id,dest?*dest:Position{}});
    return SPELL_CAST_OK;
}
SpellCastResult Unit::CastSpell(Position const&p,uint32 id,bool){return Request(this,&p,id);}SpellCastResult Unit::CastSpell(Unit*,uint32 id,bool){return Request(this,nullptr,id);}
uint32 NativeCount(int32_t damage){struct {uint32 ID=64;} propertiesValue;auto*properties=&propertiesValue;uint32 numSummons=1;switch(properties->ID){
#include "Count.inc"
    default:break;}return numSummons;}
TempSummon*Copy(Creature*owner,Position const&p){auto v=std::make_unique<TempSummon>();v->entry=104737;v->created=207299;v->summoner=owner;v->guid.id=nextGuid++;static_cast<Position&>(*v)=p;auto raw=v.get();registry[raw->guid.id]=raw;copies.push_back(std::move(v));dynamic_cast<npc_stromkar_illusionist*>(owner->ai)->JustSummoned(raw);return raw;}
void DeliverOne(){assert(!pending.empty());auto p=std::move(pending.front());pending.pop_front();p.script->Guard(0);if(p.script->prevented)return;
    auto actor=p.caster->ToCreature();assert(actor);
    if(p.id==207244){actor->auras.insert(207244);actor->ai=dynamic_cast<npc_stromkar_illusionist*>(actor->ai);dynamic_cast<npc_stromkar_illusionist*>(actor->ai)->SpellHit(actor,p.script->GetSpellInfo());}
    if(p.id==207297)Request(actor,&p.dest,207299);
    if(p.id==207299)for(uint32 i=0;i<NativeCount(0);++i)Copy(actor,p.dest);
    if(p.id==207249)static_cast<Position&>(*actor)=p.dest;
    if(p.id==207258||p.id==207275){actor->auras.insert(p.id);actor->channel.info.Id=p.id;}
}
void Deliver(){uint32 budget=100;while(!pending.empty()&&budget--)DeliverOne();assert(budget);}
struct Fixture {
    Creature actor;Player player;Unit pet,guardian,ally;Player stranger,mage;npc_stromkar_illusionist ai{&actor};
    Fixture(){pending.clear();copies.clear();registry.clear();delayed.clear();sceneAttempts=0;sceneRejected=false;scenarioState={};manager.present=true;warnings=0;rejectId=missingId=0;actor.guid.id=1;player.guid.id=2;stranger.guid.id=3;mage.guid.id=4;mage.cls=8;stranger.group=2;stranger.status=0;pet.owner=guardian.owner=&player;ally.friendly=true;registry[1]=&actor;registry[2]=&player;registry[3]=&stranger;registry[4]=&mage;ai.Reset();ai.EnterCombat(&player);}
    void Begin(){assert(actor.CastSpell(&actor,207244,false)==SPELL_CAST_OK);Deliver();assert(ai.active);}
    void Tick(uint32 diff=100){ai.UpdateAI(diff);Deliver();}
    void Settle(){for(int i=0;i<20;++i){if(i==10)actor.auras.erase(207244);Tick();}assert(ai.teleported&&actor.HasAura(207258,actor.guid));assert(copies.size()==2);for(auto const&c:copies)assert(c->HasAura(207275,c->guid));}
};
int main(int argc,char**argv){
    if(argc==2){std::ifstream file(argv[1]);assert(file);std::array<uint32,8> row{};uint32 count=0;while(file>>row[0]>>row[1]>>row[2]>>row[3]>>row[4]>>row[5]>>row[6]>>row[7]){assert(row[0]==count++);if(row[0]==0)assert(row[2]==4&&row[4]==204595);if(row[0]==1)assert(row[4]==207244);if(row[0]==2)assert(row[4]==215690);if(row[0]==3)assert(row[1]==4&&row[2]==6&&row[4]==204743);if(row[0]==4){assert(row[3]==205&&row[6]==7&&row[7]==0);nativeTarget=row[6];nativeCriteriaType=row[4];nativeCriteriaId=row[5];}}assert(count==5);}
    assert(NativeCount(0)==1&&NativeCount(-1)==1&&NativeCount(2)==2);
    {Fixture f;f.Begin();f.Settle();auto epoch=f.ai.generation;SpellInfo split(207244);f.ai.SpellHit(&f.actor,&split);assert(f.ai.generation==epoch);for(auto&c:copies)c->auras.erase(207275);f.actor.auras.erase(207258);for(int i=0;i<5;++i)f.Tick();assert(pending.empty());for(auto&c:copies)assert(c->auras.empty());assert(!f.actor.HasAura(207258,f.actor.guid));f.ai.Reset();for(auto&c:copies)assert(!c->inWorld);assert(!f.player.credit);}
    for(int invalid=0;invalid<15;++invalid){Fixture f;if(invalid==0)f.actor.mapId=1;if(invalid==1)f.actor.spawn=1;if(invalid==2)scenarioState.id=991;if(invalid==3)scenarioState.step=5;if(invalid==4)scenarioState.step=7;if(invalid==5)manager.present=false;if(invalid==6)f.player.status=0;if(invalid==7)f.player.rewarded=true;if(invalid==8)f.player.alive=false;if(invalid==9)f.player.contact=false;if(invalid==10)f.player.map=&elsewhere;if(invalid==11)f.player.phase=2;if(invalid==12)f.player.teleport=true;if(invalid==13)f.player.x=51;if(invalid==14)f.actor.combat=false;assert(f.actor.CastSpell(&f.actor,207244,false)!=SPELL_CAST_OK);assert(!f.ai.active&&copies.empty()&&!f.player.credit);}
    for(int cancel=0;cancel<7;++cancel){Fixture f;f.Begin();f.ai.UpdateAI(100);assert(pending.size()==2);if(cancel==0)f.ai.Reset();if(cancel==1)f.actor.alive=false;if(cancel==2)f.actor.combat=false;if(cancel==3)scenarioState.step=7;if(cancel==4)f.player.quest.ScriptLifetime=std::make_shared<uint8>();if(cancel==5)f.player.status=0;if(cancel==6)f.player.map=&elsewhere;Deliver();assert(copies.empty());f.ai.UpdateAI(100);assert(!f.ai.active&&!f.player.credit);}
    {Fixture f;f.Begin();rejectId=207297;f.Tick();assert(warnings==1&&copies.empty());rejectId=0;for(int i=0;i<18;++i)f.Tick();f.actor.auras.erase(207244);f.Settle();assert(copies.size()==2);}
    {Fixture f;f.Begin();missingId=207297;for(int i=0;i<6;++i)f.Tick(1500);assert(!f.ai.active&&copies.empty());}
    {Fixture f;f.actor.blocked=true;f.actor.CastSpell(&f.actor,207244,false);Deliver();assert(!f.ai.active&&copies.empty()&&warnings==1);}
    {Fixture f;f.Begin();f.Tick();assert(copies.size()==2);auto duplicate=Copy(&f.actor,f.ai.images[0].position);assert(!duplicate->inWorld&&copies.size()==3);auto foreign=Copy(&f.actor,Position{99,99});assert(!foreign->inWorld);f.ai.Reset();auto late=Copy(&f.actor,Position{});assert(!late->inWorld);}
    for(int killer=0;killer<7;++killer){Fixture f;f.actor.alive=false;Unit*k=&f.player;if(killer==1)k=&f.pet;if(killer==2)k=&f.guardian;if(killer==3)k=&f.ally;if(killer==4)k=&f.mage;if(killer==5)k=&f.stranger;if(killer==6)k=nullptr;f.ai.JustDied(k);f.ai.JustDied(k);assert(f.ai.script.deaths==1);assert(f.player.credit==(killer<=4?1u:0u));if(killer<=4)assert(f.player.creditType==92&&f.player.creditId==49058&&f.player.overrideGroup);assert(!f.stranger.credit&&!f.mage.credit);}
    {Fixture f;f.player.alive=false;f.actor.alive=false;f.ai.JustDied(&f.pet);assert(f.player.credit==1);}
    {Fixture f;f.actor.spawn=1;f.ai.JustDied(&f.stranger);assert(f.ai.script.last==&f.stranger);}
    {Fixture f;f.ai.script.credit.duplicate=true;f.actor.alive=false;f.ai.JustDied(&f.pet);assert(f.player.credit==1);}
    {Fixture f;f.actor.alive=false;sceneRejected=true;f.ai.JustDied(&f.pet);assert(f.player.credit==1&&sceneAttempts==0&&delayed.size()==1);for(auto&cb:delayed)cb();assert(sceneAttempts==1&&f.player.credit==1);}
    {Fixture f;f.actor.alive=false;f.ai.JustDied(&f.pet);assert(f.player.credit==1&&delayed.size()==1);registry.erase(f.player.guid.id);for(auto&cb:delayed)cb();assert(sceneAttempts==0&&f.player.credit==1);}
    for(int invalid=0;invalid<6;++invalid){Fixture f;f.actor.alive=false;if(invalid==0)f.player.quest.ScriptLifetime=std::make_shared<uint8>();if(invalid==1)f.player.status=0;if(invalid==2)f.player.rewarded=true;if(invalid==3)scenarioState.step=5;if(invalid==4)scenarioState.id=991;if(invalid==5)f.player.x=51;f.ai.JustDied(&f.pet);assert(!f.player.credit);}
    {Fixture f;f.ai.witnessGuid.Clear();f.ai.lifetime.reset();f.actor.alive=false;f.ai.JustDied(&f.pet);assert(f.player.credit==1);}
    {Fixture f;f.actor.CastSpell(&f.actor,207244,false);assert(!pending.empty());f.ai.Reset();Deliver();assert(!f.ai.active&&!f.actor.HasAura(207244,f.actor.guid)&&copies.empty());}
    {Fixture f;f.Begin();f.ai.UpdateAI(100);DeliverOne();assert(pending.size()==2);f.ai.Reset();Deliver();assert(copies.empty());}
    {Fixture f;f.Begin();f.Tick();f.actor.auras.erase(207244);f.ai.UpdateAI(100);assert(!pending.empty());auto origin=f.actor.GetPosition();f.ai.Reset();Deliver();assert(f.actor.GetExactDist(&origin)==0&&!f.actor.HasAura(207258,f.actor.guid));for(auto&c:copies)assert(!c->inWorld&&!c->HasAura(207275,c->guid));}
    for(uint32 missing:{207249u,207258u,207275u}){Fixture f;f.Begin();missingId=missing;for(int i=0;i<15&&f.ai.active;++i){f.actor.auras.erase(207244);f.Tick(1500);}assert(!f.ai.active&&!f.player.credit);for(auto&c:copies)assert(!c->inWorld);}
    {Fixture f;f.Begin();f.Settle();for(auto&c:copies)c->inWorld=false;for(int i=0;i<19;++i)f.Tick(1000);assert(!f.ai.active&&copies.size()==2&&!f.player.credit);}
    std::cout<<"PASS production split controller/native death+credit/count branches: two real receipt slots, observed teleport/channel ack, expiry/no replay, reset/late/duplicate/source/quest lifetime/15 admission refusals, retries and player/pet/guardian/ally/group/one-hit exactly-once owned credit. World collision, casts/projectile delivery and scenario/group award are explicit endpoints, not client traversal.\n";
}
