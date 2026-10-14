#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <vector>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;using int32=int32_t;
using SpellEffIndex=uint8;using SpellCastResult=int;using AuraEffectHandleModes=int;
enum {ALLIANCE=469,HORDE=67,QUEST_STATUS_NONE=0,QUEST_STATUS_COMPLETE=1,QUEST_STATUS_INCOMPLETE=3,
 QUEST_OBJECTIVE_MONSTER=0,QUEST_OBJECTIVE_FLAG_OPTIONAL=4,REACT_PASSIVE=0,EFFECT_MOTION_TYPE=8,
 SPELL_CAST_OK=0,SPELL_FAILED_BAD_TARGETS=1,TRIGGERED_FULL_MASK=1,EFFECT_0=0,SPELL_EFFECT_TRIGGER_SPELL_4=160,
 SPELL_EFFECT_SUMMON=28,SPELL_AURA_DUMMY=4,AURA_EFFECT_HANDLE_REAL=1,AURA_REMOVE_BY_EXPIRE=1,
 SPELL_EFFECT_HANDLE_LAUNCH=1,TARGET_DEST_TARGET_BACK=65,TARGET_UNIT_TARGET_ANY=25,UNIT_FIELD_NPC_FLAGS=1,UNIT_NPC_FLAG_SPELLCLICK=16777216};
struct ObjectGuid {uint32 value=0;bool IsEmpty()const{return !value;}bool operator==(ObjectGuid g)const{return value==g.value;}bool operator!=(ObjectGuid g)const{return !(*this==g);}bool operator<(ObjectGuid g)const{return value<g.value;}static ObjectGuid const Empty;};ObjectGuid const ObjectGuid::Empty{};
struct Position {float x=0,y=0,z=0,o=0;void GetPosition(float& a,float& b,float& c){a=x;b=y;c=z;}void Relocate(float a,float b,float c){x=a;y=b;z=c;}void Relocate(Position const& p){*this=p;}float GetPositionX()const{return x;}float GetPositionY()const{return y;}float GetPositionZ()const{return z;}float GetOrientation()const{return o;}float GetRelativeAngle(Position*){return 0;}};
struct Spline {uint32 id=1;bool done=true;Position end;uint32 GetId(){return id;}bool Finalized(){return done;}Position FinalDestination(){return end;}};
struct Player;struct Creature;struct TempSummon;struct Motion;
struct WorldObject:Position {uint32 map=1220,zone=7541,instance=0;bool world=true,alive=true,expired=false;ObjectGuid guid;std::weak_ptr<uint8> bound;
 virtual ~WorldObject()=default;bool IsInWorld()const{return world;}bool IsAlive()const{return alive;}uint32 GetMapId()const{return map;}uint32 GetZoneId()const{return zone;}uint32 GetInstanceId()const{return instance;}ObjectGuid GetGUID()const{return guid;}bool IsQuestObjectExpired(){return expired || (!bound.expired()?false:bound.use_count()==0&&scoped);}bool scoped=false;
 float GetExactDist(float a,float b,float c)const{return std::sqrt((x-a)*(x-a)+(y-b)*(y-b)+(z-c)*(z-c));}
 float GetExactDist(WorldObject const* t)const{return GetExactDist(t->x,t->y,t->z);}
 bool IsWithinDistInMap(WorldObject const* t,float d)const{return t&&map==t->map&&instance==t->instance&&GetExactDist(t)<=d;}
};
struct Unit:WorldObject {Spline storage;Spline* movespline=&storage;virtual Player* ToPlayer(){return nullptr;}virtual Creature* ToCreature(){return nullptr;}virtual Motion* GetMotionMaster()=0;bool flight=false;bool isInFlight(){return flight;}bool IsPlayer(){return ToPlayer()!=nullptr;}void SetSplineTimer(uint32){}uint32 GetEntry(){return 0;}Unit* getVictim(){return nullptr;}};
struct AI {virtual ~AI()=default;virtual void SetGUID(ObjectGuid const&,int32){}virtual ObjectGuid GetGUID(int32){return {};}virtual void DoAction(int32){}};
struct Creature:Unit {uint32 entry=91983,spawn=294335,despawnDelay=0,npcFlags=16777216;::AI* ai=nullptr;bool HasFlag(uint32,uint32 flags){return (npcFlags&flags)!=0;}Creature* ToCreature()override{return this;}uint32 GetEntry(){return entry;}uint32 GetDBTableGUIDLow(){return spawn;}auto AI()->::AI*{return ai;}virtual TempSummon* ToTempSummon(){return nullptr;}void SetReactState(int){}void DespawnOrUnsummon(uint32 ms){despawnDelay=ms;}Motion* GetMotionMaster()override{return nullptr;}};
struct TempSummon:Creature {Unit* owner=nullptr;Unit* GetSummoner(){return owner;}TempSummon* ToTempSummon()override{return this;}};
struct QuestObjective {uint32 Type=0,ObjectID=91484,StorageIndex=0,Amount=1,Flags=0;};
struct Quest {std::vector<QuestObjective> objs{QuestObjective{}};auto const& GetObjectives()const{return objs;}};
struct Store {std::map<uint32,Quest> quests{{38318,{}},{38613,{}}};Quest const* GetQuestTemplate(uint32 q){auto i=quests.find(q);return i==quests.end()?nullptr:&i->second;}} store;auto sQuestDataStore=&store;
struct QuestStatusData {int Status=QUEST_STATUS_INCOMPLETE;std::shared_ptr<uint8> ScriptLifetime;};
struct JumpArrivalCastArgs {uint32 SpellId=0;ObjectGuid Target;};
namespace Movement {struct SpellEffectExtraData{};}
struct Motion {Player* owner;uint32 type=0;explicit Motion(Player* p):owner(p){}uint32 GetCurrentMovementGeneratorType(){return type;}void MoveJump(Position const&,float,float,uint32,bool,JumpArrivalCastArgs const*);};
struct SpellCastTargets {Unit* unit=nullptr;Position dest;bool dst=false;void SetUnitTarget(Unit* p){unit=p;}void SetDst(Position const& p){dest=p;dst=true;}bool HasDst(){return dst;}Unit* GetUnitTarget(){return unit;}ObjectGuid GetObjectTargetGUID(){return unit?unit->guid:ObjectGuid::Empty;}};
struct Effect {uint32 MiscValue=0,MiscValueB=0,TriggerSpell=0;struct Target {uint32 value=46;uint32 GetTarget(){return value;}Target& operator=(uint32 v){value=v;return *this;}} TargetA;};
struct SpellInfo {uint32 Id=182469;Effect effect;Effect* Effects[1]{&effect};Effect* GetEffect(uint8,uint32){return &effect;}};
struct SpellMgr {SpellInfo summon;SpellInfo const* GetSpellInfo(uint32 id){summon.Id=id;return &summon;}} mgr;auto sSpellMgr=&mgr;
std::map<ObjectGuid,Creature*> creatures;
struct Player:Unit {uint32 team=ALLIANCE;bool item=true,teleport=false,vehicle=false,transport=false,phase=true,visible=true,los=true,grantFails=false,auraFails=false,jumpFails=false,clickVisible=true;uint32 updates=0,completions=0,casts=0;ObjectGuid selection;
 std::map<uint32,QuestStatusData> quests{{38318,{}},{38613,{}}};std::map<uint32,uint32> progress;std::vector<std::function<void()>> events;std::vector<ObjectGuid> hooks;std::map<uint32,bool> auras;Motion motion{this};
 Player* ToPlayer()override{return this;}Motion* GetMotionMaster()override{return &motion;}uint32 GetTeam(){return team;}bool IsBeingTeleported(){return teleport;}bool GetVehicle(){return vehicle;}bool GetTransport(){return transport;}
 bool InSamePhase(WorldObject*){return phase;}bool canSeeOrDetect(WorldObject*){return visible;}bool canSeeSpellClickOn(Creature*){return clickVisible;}bool IsWithinLOSInMap(WorldObject*){return los;}bool GetItemByEntry(uint32){return item;}bool HasAura(uint32 id){return auras[id];}auto GetSummonList(uint32){return &hooks;}
 int GetQuestStatus(uint32 q){auto i=quests.find(q);return i==quests.end()?QUEST_STATUS_NONE:i->second.Status;}uint32 GetQuestObjectiveData(uint32 q,uint32){return progress[q];}void SetQuestObjectiveData(Quest const* q,QuestObjective const*,uint32 n){for(auto const& p:store.quests)if(&p.second==q)progress[p.first]=n;}
 void SendQuestUpdateAddCredit(Quest const*,ObjectGuid,QuestObjective const&,uint32){++updates;}bool CanCompleteQuest(uint32 q){return progress[q]==1;}void CompleteQuest(uint32 q){quests[q].Status=QUEST_STATUS_COMPLETE;++completions;}void SetQuestStatus(uint32 q,int state){quests[q].Status=state;}
 QuestStatusData* getQuestStatus(uint32 q){auto i=quests.find(q);return i==quests.end()?nullptr:&i->second;}void AddDelayedEvent(uint64,std::function<void()> f){events.push_back(std::move(f));}void AddQuestDelayedEvent(uint32,uint64,std::function<void()>&&);
 void BindQuestObject(uint32 q,WorldObject* p){if(!quests[q].ScriptLifetime)quests[q].ScriptLifetime=std::make_shared<uint8>();p->bound=quests[q].ScriptLifetime;p->scoped=true;}
 void Tick(){auto pending=std::move(events);events.clear();for(auto& f:pending)f();}void Accept(uint32 q){quests.erase(q);quests[q]={};progress[q]=0;}
 void SetFallInformation(uint32,float){}void SetKnockBackTime(uint32){}void SetSplineTimer(uint32){}ObjectGuid GetSelection(){return selection;}
 int CastSpell(Unit*,uint32,bool);int CastSpell(SpellCastTargets const&,SpellInfo const*,void*,int);
};
#include "QuestDelayedEvent.inc"
namespace ObjectAccessor {Creature* GetCreature(WorldObject& owner,ObjectGuid guid){auto i=creatures.find(guid);return i==creatures.end()||!i->second->world||i->second->map!=owner.map||i->second->instance!=owner.instance?nullptr:i->second;}Unit* GetUnit(Unit& o,ObjectGuid g){return GetCreature(o,g);}}
void Motion::MoveJump(Position const& p,float speedXY,float speedZ,uint32 id,bool,JumpArrivalCastArgs const*){assert(id==182469&&speedXY>0&&speedZ==10);owner->storage.end=p;++owner->storage.id;owner->storage.done=false;type=EFFECT_MOTION_TYPE;}
struct DelayCastEvent {uint32 Spell=0;ObjectGuid TargetGUID;};
struct Spell {Unit* m_caster;SpellInfo* m_spellInfo;uint32 m_diffMode=0;int effectHandleMode=SPELL_EFFECT_HANDLE_LAUNCH;SpellCastTargets m_targets;Position* destTarget=&m_targets.dest;
 DelayCastEvent* GetSpellTriggerDelay(uint8,uint32){return nullptr;}void EffectJumpDest(SpellEffIndex);void CalculateJumpSpeeds(uint8,float,float&,float&);};
#include "JumpSpeeds.inc"
#include "JumpDest.inc"
struct EffectHandler {std::function<void(void*,uint8)> action;template<class T>explicit EffectHandler(void(T::*f)(uint8)):action([f](void* p,uint8 i){(static_cast<T*>(p)->*f)(i);}){}};
struct HookList {std::vector<EffectHandler> effects;void operator+=(int){}void operator+=(EffectHandler h){effects.push_back(std::move(h));}void Run(void* s,uint8 i){for(auto& h:effects)h.action(s,i);}};
struct SpellScript {Unit* caster=nullptr;Unit* target=nullptr;Position hitDest;bool prevented=false;HookList BeforeCast,AfterCast,OnCheckCast,OnEffectLaunchTarget,OnEffectHitTarget,OnEffectHit;
 virtual ~SpellScript()=default;virtual void Register(){}Unit* GetCaster(){return caster;}Unit* GetExplTargetUnit(){return target;}Position* GetHitDest(){return &hitDest;}void PreventHitDefaultEffect(uint8){prevented=true;}};
struct AuraEffect{};struct AuraApplication {int mode=AURA_REMOVE_BY_EXPIRE;int GetRemoveMode(){return mode;}};
struct AuraScript {Unit* target=nullptr;AuraApplication application;HookList AfterEffectRemove;virtual ~AuraScript()=default;virtual void Register(){}Unit* GetTarget(){return target;}AuraApplication* GetTargetApplication(){return &application;}};
struct ScriptedAI:AI {Creature* me;explicit ScriptedAI(Creature* p):me(p){p->ai=this;}};
struct PlayerScript {explicit PlayerScript(char const*){}virtual ~PlayerScript()=default;virtual void OnLogin(Player*){}};
#define PrepareSpellScript(x) public:
#define PrepareAuraScript(x) public:
#define SpellEffectFn(F,I,T) EffectHandler(&F)
#define SpellCheckCastFn(...) 0
#define SpellCastFn(...) 0
#define AuraEffectRemoveFn(...) 0
#include "NoWings.inc"
int Player::CastSpell(SpellCastTargets const& targets,SpellInfo const* info,void*,int){assert(info->Id==182467&&targets.unit&&targets.dst);++casts;if(grantFails)return 1;
 spell_stormheim_grapple_summon fix;fix.caster=this;fix.target=targets.unit;fix.hitDest=*this;fix.PositionHook(0); // Native caster-summon18 initially overwrites destination.
 auto* hook=new TempSummon;static uint32 next=1000;hook->guid={++next};hook->entry=91971;hook->spawn=0;hook->owner=this;hook->Relocate(fix.hitDest);hook->map=map;hook->instance=instance;new npc_stormheim_grapple_hook(hook);creatures[hook->guid]=hook;hooks.push_back(hook->guid);if(!auraFails)auras[182468]=true;return 0;}
int Player::CastSpell(Unit* target,uint32 id,bool){assert(id==182469);++casts;if(jumpFails)return 1;spell_stormheim_grapple_jump script;script.caster=this;script.target=target;script.Before();SpellInfo info;Spell jump{this,&info};jump.m_targets.SetUnitTarget(target);jump.m_targets.SetDst(*target);jump.EffectJumpDest(0);auras[182470]=true;script.End();return 0;}
void Start(Player& p,Creature& point){spell_stormheim_grapple_start script;script.caster=&p;script.target=&point;script.Register();assert(script.Check()==SPELL_CAST_OK);script.OnEffectLaunchTarget.Run(&script,0);assert(script.prevented);}
void EndHook(Player& p,int mode=AURA_REMOVE_BY_EXPIRE){p.auras[182468]=false;aura_stormheim_grapple_hook script;script.target=&p;script.application.mode=mode;script.End(nullptr,1);}
Creature Anchor(uint32 team){Creature c;c.entry=team==ALLIANCE?91983:91975;c.spawn=team==ALLIANCE?294335:294296;c.guid={c.spawn};c.Relocate(40,0,8);return c;}
void Setup(Player& p,Creature& c,uint32 team=ALLIANCE){creatures.clear();p.team=team;p.quests.erase(team==ALLIANCE?38613:38318);creatures[c.guid]=&c;}
void Land(Player& p){p.Relocate(p.storage.end);p.storage.done=true;p.Tick();}
int main(){
 for(uint32 team:{ALLIANCE,HORDE}){Player p;auto point=Anchor(team);Setup(p,point,team);uint32 q=team==ALLIANCE?38318:38613;
  Start(p,point);assert(p.progress[q]==0&&p.casts==1&&p.hooks.size()==1);auto hook=creatures[p.hooks.front()];assert(hook->GetExactDist(&point)==0&&hook->despawnDelay==10000&&hook->scoped);
  EndHook(p);assert(p.casts==2&&!p.storage.done&&p.progress[q]==0);p.Tick();assert(p.progress[q]==0);Land(p);assert(p.progress[q]==1&&p.completions==1&&p.updates==1);p.Tick();hook->AI()->DoAction(1);assert(p.casts==2&&hook->despawnDelay==5000);
 }
 for(int failure=0;failure<13;++failure){Player p;auto point=Anchor(ALLIANCE);Setup(p,point);Start(p,point);
  if(failure==0)p.quests.erase(38318);if(failure==1)p.Accept(38318);if(failure==2)p.item=false;if(failure==3)p.alive=false;if(failure==4)p.map=1;if(failure==5)p.instance=1;if(failure==6)p.phase=false;if(failure==7)p.los=false;if(failure==8)p.transport=true;if(failure==9)point.world=false;if(failure==10)p.jumpFails=true;
  if(failure==12)p.world=false;EndHook(p,failure==11?2:AURA_REMOVE_BY_EXPIRE);assert(p.casts==1+(failure==10));p.Tick();assert(p.progress[38318]==0);
 }
 for(int failure=0;failure<8;++failure){Player p;auto point=Anchor(ALLIANCE);Setup(p,point);Start(p,point);EndHook(p);
  if(failure==0)++p.storage.id;if(failure==1)p.map=1;if(failure==2)p.instance=1;if(failure==3)p.Accept(38318);if(failure==4)p.item=false;if(failure==5)point.world=false;if(failure==6)p.phase=false;
  p.storage.done=true;if(failure!=7)p.Relocate(p.storage.end);else p.Relocate(point.x,point.y,point.z-10);p.Tick();assert(p.progress[38318]==0&&p.completions==0);
 }
 {Player p;auto point=Anchor(ALLIANCE);Setup(p,point);Start(p,point);EndHook(p);for(int i=0;i<105;++i)p.Tick();Land(p);assert(!p.progress[38318]);}
 {Player p;auto point=Anchor(ALLIANCE);Setup(p,point);p.grantFails=true;Start(p,point);assert(p.hooks.empty()&&!p.HasAura(182468)&&p.casts==1);p.grantFails=false;Start(p,point);EndHook(p);Land(p);assert(p.progress[38318]==1);}
 {Player p;auto point=Anchor(ALLIANCE);Setup(p,point);p.auraFails=true;Start(p,point);assert(!p.HasAura(182468));assert(p.progress[38318]==0&&creatures[p.hooks[0]]->despawnDelay==2000);p.auraFails=false;Start(p,point);EndHook(p);Land(p);assert(p.casts==3&&p.progress[38318]==1);}
 {Player p;auto point=Anchor(ALLIANCE);Setup(p,point);point.spawn=294273;Start(p,point);assert(!creatures[p.hooks[0]]->scoped);EndHook(p);Land(p);assert(!p.progress[38318]);}
 {Player p;auto point=Anchor(HORDE);Setup(p,point);Start(p,point);EndHook(p);Land(p);assert(!p.progress[38318]);}
 {Player first,second;auto point=Anchor(ALLIANCE);Setup(first,point);Start(first,point);Start(second,point);EndHook(first);assert(first.casts==2&&second.casts==1);Land(first);assert(first.progress[38318]==1&&!second.progress[38318]);EndHook(second);Land(second);assert(second.progress[38318]==1);}
 {Player p;auto point=Anchor(ALLIANCE);Setup(p,point);spell_stormheim_grapple_start script;script.caster=&p;script.target=&point;
  for(int failure=0;failure<11;++failure){p.item=true;p.vehicle=false;p.flight=false;p.visible=true;p.los=true;p.storage.done=true;p.auras[182468]=false;p.phase=true;p.clickVisible=true;point.npcFlags=16777216;point.entry=91983;point.map=1220;
   if(failure==0)p.item=false;if(failure==1)p.vehicle=true;if(failure==2)p.flight=true;if(failure==3)p.visible=false;if(failure==4)p.los=false;if(failure==5)p.storage.done=false;if(failure==6)p.auras[182468]=true;if(failure==7)point.entry=123;if(failure==8)point.map=1;
   if(failure==9)point.npcFlags=0;if(failure==10)p.clickVisible=false;assert(script.Check()==SPELL_FAILED_BAD_TARGETS);
  }
 }
 {Player p;p.quests[38318].Status=QUEST_STATUS_COMPLETE;p.quests[38613].Status=QUEST_STATUS_COMPLETE;StormheimNoWings::RepairLegacy(&p);assert(p.GetQuestStatus(38318)==QUEST_STATUS_INCOMPLETE&&p.GetQuestStatus(38613)==QUEST_STATUS_COMPLETE);p.progress[38318]=1;p.quests[38318].Status=QUEST_STATUS_COMPLETE;StormheimNoWings::RepairLegacy(&p);assert(p.GetQuestStatus(38318)==QUEST_STATUS_COMPLETE);}
 std::cout<<"PASS: production gun start, scoped hook, natural aura expiry, native jump effect/speeds and arrival/lifetime credit; both factions, failures, interruption, retry, legacy empty COMPLETE and unrelated points.\n";
}
