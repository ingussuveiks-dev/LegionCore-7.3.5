#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <utility>
#include <vector>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;using int32=int32_t;
#include "NativeModes.inc"
enum{CLASS_PRIEST=5,QUEST_STATUS_NONE=0,QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_COMPLETE=1,
 EFFECT_0=0,EFFECT_1=1,EFFECT_ALL=255,SPELL_AURA_MOD_ROOT=26,SPELL_AURA_PERIODIC_TRIGGER_SPELL=23,
 SPELL_EFFECT_APPLY_AURA=6,SPELL_EFFECT_SEND_EVENT=61,SPELL_EFFECT_ANY=0,
 UNIT_FIELD_NPC_FLAGS=0,UNIT_NPC_FLAG_SPELLCLICK=16777216,
 SPELL_EFFECT_HANDLE_HIT_TARGET=1,SPELL_EFFECT_HANDLE_HIT=2,TARGET_FLAG_UNIT_MASK=1,TARGET_FLAG_GAMEOBJECT_MASK=2,
 CRITERIA_TYPE_SCRIPT_EVENT=46,CRITERIA_TYPE_SCRIPT_EVENT_2=92,SMART_ACTION_UPDATE_ACHIEVEMENT_CRITERIA=205};
using SpellEffIndex=uint32;using CriteriaTypes=uint32;
enum SpellCastResult{SPELL_CAST_OK,SPELL_FAILED_BAD_TARGETS,SPELL_FAILED_SPELL_IN_PROGRESS};
struct ObjectGuid{
 uint64 high=0,low=0;uint32 entry=0,kind=0;static ObjectGuid Empty;
 bool IsEmpty()const{return !high&&!low;}bool IsCreatureOrVehicle()const{return kind==2;}uint32 GetEntry()const{return entry;}
 bool operator==(ObjectGuid const& o)const{return high==o.high&&low==o.low&&entry==o.entry&&kind==o.kind;}
 bool operator!=(ObjectGuid const& o)const{return !(*this==o);}bool operator<(ObjectGuid const&o)const{return std::tie(high,low,entry,kind)<std::tie(o.high,o.low,o.entry,o.kind);}
};ObjectGuid ObjectGuid::Empty;
uint32 warnings=0;
#define TC_LOG_WARN(...) (++warnings)
#define TC_LOG_DEBUG(...) ((void)0)
#define PrepareSpellScript(x) public:
#define PrepareAuraScript(x) public:
#define SpellCheckCastFn(...) 0
#define SpellEffectFn(...) 0
#define SpellHitFn(...) 0
#define AuraEffectApplyFn(...) 0
#define AuraEffectRemoveFn(...) 0
#define AuraEffectUpdateFn(...) 0
struct Hooks{template<class T>void operator+=(T){}};
struct Player;struct Creature;struct Unit;struct GameObject;struct AuraRecord;struct CreatureAI;
struct QuestStatusData{uint32 Status=3;std::shared_ptr<uint8> ScriptLifetime;};
struct Reference{Player* player;Player* getSource()const{return player;}};
struct Map{
 uint32 id=42,mapId=1583;std::vector<Reference> players;std::vector<Creature*> actors;std::vector<std::shared_ptr<GameObject>> objects;
 auto const& GetPlayers()const{return players;}Creature* GetCreature(ObjectGuid const&);uint32 GetInstanceId()const{return id;}
 template<class... Args>void ScriptsStart(Args...){}
};
struct ZoneScript{void ProcessEvent(Unit*,uint32){assert(false);}};using InstanceScript=ZoneScript;inline int sEventScripts=0;
struct Unit{
 Map* map=nullptr;ObjectGuid guid;bool world=true,alive=true,preDelete=false;uint32 phase=6140;
 float x=1117.69f,y=1048.08f,z=311.943f,size=0;
 std::map<std::pair<uint32,ObjectGuid>,std::shared_ptr<AuraRecord>> auras;
 virtual ~Unit()=default;virtual Player* ToPlayer(){return nullptr;}virtual Creature* ToCreature(){return nullptr;}
 bool IsInWorld()const{return world;}bool IsAlive()const{return alive;}bool IsPreDelete()const{return preDelete;}Map* GetMap()const{return map;}
 uint32 GetMapId()const{return map?map->mapId:0;}uint32 GetInstanceId()const{return map?map->id:0;}ObjectGuid GetGUID()const{return guid;}
 bool InSamePhase(Unit* u)const{return u&&phase==u->phase;}bool IsWithinDistInMap(Unit* u,float r)const{return u&&map==u->map&&InSamePhase(u)&&IsWithinDist3d(u->x,u->y,u->z,r+u->size);}
 bool IsWithinDist3d(float a,float b,float c,float r,bool objectSize=true)const{float dx=x-a,dy=y-b,dz=z-c;if(objectSize)r+=size;return dx*dx+dy*dy+dz*dz<=r*r;}
 void GetCreatureListWithEntryInGrid(std::list<Creature*>&,uint32,float);
 bool HasAura(uint32,ObjectGuid=ObjectGuid::Empty)const;void RemoveAurasDueToSpell(uint32,ObjectGuid=ObjectGuid::Empty);
 virtual SpellCastResult CastSpell(Unit*,uint32,bool);
 ZoneScript* GetZoneScript(){return nullptr;}InstanceScript* GetInstanceScript(){return nullptr;}
};
struct Player:Unit{
 uint32 classId=5,questId=41631,status=3,grants=0,eventCalls=0,forcedCalls=0,resultCasts=0,screenHits=0;
 bool rewarded=false,contact=true,gm=false,teleport=false,visible=true,los=true,acceptCredit=true,failResult=false,deferResult=false;
 QuestStatusData quest;std::vector<std::function<void()>> delayed;std::function<void()> onCredit,onScreen;
 Player(){guid={1,7,0,1};}Player* ToPlayer()override{return this;}
 bool CanContact()const{return contact;}bool isGameMaster()const{return gm;}bool HasPendingTeleport()const{return teleport;}uint32 getClass()const{return classId;}
 bool GetQuestRewardStatus(uint32 q)const{return q==questId&&rewarded;}uint32 GetQuestStatus(uint32 q)const{return q==questId?(rewarded?6:status):0;}
 QuestStatusData* getQuestStatus(uint32 q){return q==questId&&status?&quest:nullptr;}
 bool canSeeOrDetect(Unit*)const{return visible;}bool IsWithinLOSInMap(Unit*)const{return los;}
 void AddDelayedEvent(uint64 delay,std::function<void()>&& f){assert(!delay);delayed.push_back(std::move(f));}
 void AddQuestDelayedEvent(uint32,uint64,std::function<void()>&&);
 void UpdateAchievementCriteria(CriteriaTypes,uint32,uint32=0,uint32=0,Unit* =nullptr,bool=false);
 SpellCastResult CastSpell(Unit*,uint32,bool)override;
};
struct CreatureAI{virtual ~CreatureAI()=default;virtual void Reset(){}virtual void UpdateLifecycle(uint32){}};
struct SmartAI:CreatureAI{Creature* me;explicit SmartAI(Creature* a):me(a){}void Reset()override{}};
struct Creature:Unit{
 uint32 entry=104985,npcFlags=UNIT_NPC_FLAG_SPELLCLICK,furyCasts=0,spawnCalls=0,despawns=0;
 uint64 dbGuid=370524;ObjectGuid charmer;CreatureAI* ai=nullptr;CreatureAI* disabled=nullptr;
 bool failFury=false,failSpawn=false,IsAIEnabled=true,m_AI_locked=false;std::function<void(uint32)> onRemove;std::function<void()> onSpawn;
 Creature(){guid={2,8,104985,2};}Creature* ToCreature()override{return this;}
 uint32 GetEntry()const{return entry;}uint64 GetDBTableGUIDLow()const{return dbGuid;}ObjectGuid GetCharmerGUID()const{return charmer;}CreatureAI* AI(){return ai;}
 void NativeTick(uint32 diff){auto i_AI=ai;auto i_disabledAI=disabled;
#include "NativeLifecycle.inc"
 }
 void SetFlag(uint32 field,uint32 mask){assert(!field);npcFlags|=mask;}void RemoveFlag(uint32 field,uint32 mask){assert(!field);npcFlags&=~mask;}
 void GetGameObjectListWithEntryInGrid(std::list<GameObject*>&,uint32,float);
 GameObject* SummonGameObject(uint32,float,float,float,float,float,float,float,float,uint32,ObjectGuid,void*,bool);
 void DespawnOrUnsummon(){++despawns;world=false;alive=false;}
 SpellCastResult CastSpell(Unit*,uint32,bool)override;
};
struct GameObject:Unit{
 ObjectGuid owner,summoner;bool spawned=true;uint32 entry=248572;
 GameObject(){guid={3,9,248572,3};x=1117.78f;y=1048.03f;z=310.663f;}
 bool isSpawned()const{return spawned;}ObjectGuid GetOwnerGUID()const{return owner;}ObjectGuid GetSummonerGUID()const{return summoner;}
};
Creature* Map::GetCreature(ObjectGuid const& id){for(auto a:actors)if(a&&a->guid==id)return a;return nullptr;}
void Unit::GetCreatureListWithEntryInGrid(std::list<Creature*>& result,uint32 entry,float r){if(map)for(auto a:map->actors)if(a&&a->entry==entry&&IsWithinDistInMap(a,r))result.push_back(a);}
void Creature::GetGameObjectListWithEntryInGrid(std::list<GameObject*>& result,uint32 id,float r){for(auto o:map->objects)if(o->entry==id&&IsWithinDistInMap(o.get(),r))result.push_back(o.get());}
GameObject* Creature::SummonGameObject(uint32 id,float a,float b,float c,float facing,float r0,float r1,float r2,float r3,uint32 lifetime,ObjectGuid viewer,void* list,bool creator){
 assert(id==248572&&a==1117.78f&&b==1048.03f&&c==310.663f&&facing==2.09636f&&!r0&&!r1&&!r2&&!r3&&!lifetime&&viewer.IsEmpty()&&!list&&!creator);
 ++spawnCalls;if(failSpawn)return nullptr;auto go=std::make_shared<GameObject>();go->map=map;go->summoner=guid;map->objects.push_back(go);if(onSpawn){auto f=onSpawn;f();}return go.get();
}
struct ProgressCounter{uint64 Counter=0;};
struct AchievementMgr{std::map<uint32,ProgressCounter> counters;auto const* GetCriteriaProgressMap()const{return &counters;}};
struct Scenario{uint32 id=1065,step=5;AchievementMgr achievement;uint32 GetScenarioId()const{return id;}uint32 GetCurrentStep()const{return step;}auto& GetAchievementMgr(){return achievement;}};
struct ScenarioMgr{Scenario scenario;bool present=true;Scenario* GetScenario(uint32 id){return present&&id==42?&scenario:nullptr;}} scenarioMgr;auto sScenarioMgr=&scenarioMgr;
void Player::UpdateAchievementCriteria(CriteriaTypes type,uint32 asset,uint32,uint32,Unit*,bool forced){assert(asset==49811);if(type==46)return;assert(type==92);++eventCalls;if(forced)++forcedCalls;if(acceptCredit&&scenarioMgr.scenario.step==5){++grants;scenarioMgr.scenario.achievement.counters[47119].Counter=1;scenarioMgr.scenario.step=6;}if(onCredit){auto f=onCredit;f();}}
#include "QuestDelay.inc"
struct EffectInfo{uint32 Effect=6,ApplyAuraName=26,TriggerSpell=0,MiscValue=0;uint32 GetProvidedTargetMask()const{return 1;}};
struct SpellInfo{
 uint32 Id;EffectInfo data[2];EffectInfo* Effects[2]={&data[0],&data[1]};
 SpellInfo(uint32 id=207996):Id(id){if(id==207949){data[1].ApplyAuraName=23;data[1].TriggerSpell=207960;}else{data[0].ApplyAuraName=260;data[1].Effect=61;data[1].MiscValue=49811;}}
 int32 GetDuration()const{return 30000;}EffectInfo const* GetEffect(uint32 i,uint32)const{return &data[i];}
};
struct SpellScript{
 Unit* caster=nullptr;Unit* explicitTarget=nullptr;Unit* hit=nullptr;bool blocked[2]={false,false};
 Hooks OnCheckCast,OnEffectHitTarget,AfterHit;virtual ~SpellScript()=default;virtual bool Validate(SpellInfo const*){return true;}virtual void Register(){}
 Unit* GetCaster(){return caster;}Unit* GetExplTargetUnit(){return explicitTarget;}Unit* GetHitUnit(){return hit;}void PreventHitDefaultEffect(uint32 i){assert(i<2);blocked[i]=true;}
};
struct AuraEffect{uint32 index=0;uint32 GetEffIndex()const{return index;}};
struct AuraApplication{uint32 mode=0;uint32 GetRemoveMode()const{return mode;}};
struct AuraScript{
 std::weak_ptr<AuraRecord> record;Hooks AfterEffectApply,OnEffectUpdate,AfterEffectRemove;
 virtual ~AuraScript()=default;virtual bool Load(){return true;}virtual void Register(){}
 Unit* GetUnitOwner();ObjectGuid GetCasterGUID();AuraApplication* GetTargetApplication();void Remove();int32 GetDuration();int32 GetMaxDuration();AuraRecord* GetAura();
};
struct AuraRecord{
 Unit* owner=nullptr;ObjectGuid caster;uint32 spell=207949;int32 m_duration=30000,m_allDuration=0,maxDuration=30000;bool removed=false;
 std::shared_ptr<AuraScript> script;AuraApplication application;
 bool IsExpired()const{return !m_duration;}
 int32 GetAllDuration()const{return m_allDuration;}
 void Advance(uint32 diff){
#include "NativeDuration.inc"
 }
};
Unit* AuraScript::GetUnitOwner(){return record.lock()->owner;}ObjectGuid AuraScript::GetCasterGUID(){return record.lock()->caster;}AuraApplication* AuraScript::GetTargetApplication(){return &record.lock()->application;}
int32 AuraScript::GetDuration(){return record.lock()->m_duration;}int32 AuraScript::GetMaxDuration(){return record.lock()->maxDuration;}AuraRecord* AuraScript::GetAura(){return record.lock().get();}
#include "Takeover.inc"
using AuraKey=std::pair<uint32,ObjectGuid>;using AuraMap=std::map<AuraKey,std::shared_ptr<AuraRecord>>;
void RemoveAura(Unit*,std::shared_ptr<AuraRecord>,uint32);
void AuraScript::Remove(){auto a=record.lock();if(a)RemoveAura(a->owner,a,AURA_REMOVE_BY_DEFAULT);}
void RemoveAura(Unit* owner,std::shared_ptr<AuraRecord> aura,uint32 mode){if(aura->removed)return;owner->auras.erase({aura->spell,aura->caster});aura->removed=true;aura->application.mode=mode;if(auto script=std::dynamic_pointer_cast<spell_lights_wrath_control_channel>(aura->script)){AuraEffect root{0};script->Removed(&root,AURA_EFFECT_HANDLE_REAL);}}
bool Unit::HasAura(uint32 id,ObjectGuid caster)const{for(auto const& a:auras)if(a.first.first==id&&(caster.IsEmpty()||a.first.second==caster))return true;return false;}
void Unit::RemoveAurasDueToSpell(uint32 id,ObjectGuid caster){std::vector<std::shared_ptr<AuraRecord>> found;for(auto const& a:auras)if(a.first.first==id&&(caster.IsEmpty()||a.first.second==caster))found.push_back(a.second);for(auto a:found)RemoveAura(this,a,AURA_REMOVE_BY_DEFAULT);if(auto c=ToCreature())if(c->onRemove){auto f=c->onRemove;f(id);}}
std::shared_ptr<AuraRecord> AddAura(Unit* owner,uint32 spell,ObjectGuid caster){auto aura=std::make_shared<AuraRecord>();aura->owner=owner;aura->spell=spell;aura->caster=caster;owner->auras[{spell,caster}]=aura;return aura;}
SpellCastResult Unit::CastSpell(Unit*,uint32,bool){assert(false);return SPELL_FAILED_BAD_TARGETS;}
SpellCastResult Creature::CastSpell(Unit* target,uint32 spell,bool triggered){assert(target==this&&spell==207956&&!triggered);++furyCasts;if(failFury)return SPELL_FAILED_BAD_TARGETS;AddAura(this,spell,guid);return SPELL_CAST_OK;}
using WorldObject=Unit;
struct Spell{
 Unit* m_caster;Unit* unitTarget;GameObject* gameObjTarget=nullptr;Unit* focusObject=nullptr;
 uint32 effectHandleMode=1,m_diffMode=0;SpellInfo info;SpellInfo* m_spellInfo=&info;
 void EffectSendEvent(uint32);
};
#include "NativeSendEvent.inc"
std::vector<std::shared_ptr<spell_lights_wrath_control_result>> pendingResults;
void Deliver(std::shared_ptr<spell_lights_wrath_control_result> script){script->hit=script->caster;for(uint32 i:{0u,1u}){script->Hit(i);if(!script->blocked[i]){auto p=script->caster->ToPlayer();if(i==0){++p->screenHits;if(p->onScreen){auto f=p->onScreen;f();}}else{Spell s{p,p};s.EffectSendEvent(1);}}}script->After();}
SpellCastResult Player::CastSpell(Unit* target,uint32 spell,bool triggered){assert(target==this&&spell==207996&&!triggered);++resultCasts;auto script=std::make_shared<spell_lights_wrath_control_result>();script->caster=this;script->explicitTarget=this;if(script->Check()!=SPELL_CAST_OK||failResult)return SPELL_FAILED_BAD_TARGETS;if(deferResult)pendingResults.push_back(script);else Deliver(script);return SPELL_CAST_OK;}
void Expire(Unit& owner){using AuraMap=::AuraMap;auto& m_ownedAuras=owner.auras;auto RemoveOwnedAura=[&](AuraMap::iterator& i,uint32 mode){auto aura=(i++)->second;RemoveAura(&owner,aura,mode);};
#include "NativeExpiry.inc"
}
void Advance(Unit& u,uint32 diff){std::vector<std::shared_ptr<AuraRecord>> snapshot;for(auto a:u.auras)snapshot.push_back(a.second);for(auto a:snapshot)if(!a->removed)a->Advance(diff);}
struct Run{
 static inline uint32 nextSerial=0;uint32 serial=++nextSerial;
 Map map;Player player,observer;std::unique_ptr<Creature> actor;std::unique_ptr<npc_lights_wrath_takeover> ai;
 std::shared_ptr<spell_lights_wrath_control_request> request;std::shared_ptr<AuraRecord> aura;
 Run(){warnings=0;pendingResults.clear();scenarioMgr={};scenarioMgr.scenario.achievement.counters[47117].Counter=1;player.map=observer.map=&map;observer.guid.low=77;observer.classId=1;map.players={{&player},{&observer}};MakeActor();}
 void MakeActor(){actor=std::make_unique<Creature>();actor->map=&map;map.actors={actor.get()};ai=std::make_unique<npc_lights_wrath_takeover>(actor.get());actor->ai=ai.get();}
 bool Start(){request=std::make_shared<spell_lights_wrath_control_request>();request->caster=&player;request->explicitTarget=actor.get();if(request->Check()!=SPELL_CAST_OK)return false;
  request->hit=&player;request->Hit(0);if(request->blocked[0])return false;
  aura=AddAura(&player,207949,player.guid);auto script=std::make_shared<spell_lights_wrath_control_channel>();aura->script=script;script->record=aura;assert(script->Load());AuraEffect root{0},periodic{1};script->Apply(&root,AURA_EFFECT_HANDLE_REAL);script->Apply(&periodic,AURA_EFFECT_HANDLE_REAL);request->Hit(1);assert(!request->blocked[1]);AddAura(actor.get(),207949,player.guid);Tick(0);Tick(0);return ai->state&&ai->state->phase==LightsWrathTakeover::Phase::Channel;}
 void Tick(uint32 diff){actor->NativeTick(diff);}
 void Natural(bool ready=true){assert(aura->m_duration>0);auto remaining=uint32(aura->m_duration);Advance(player,remaining-1);Advance(*actor,remaining-1);Expire(player);Expire(*actor);Tick(remaining-1);assert(!player.grants);Advance(player,1);Advance(*actor,1);Expire(player);Expire(*actor);assert(ai->state->phase==(ready?LightsWrathTakeover::Phase::Ready:LightsWrathTakeover::Phase::Cancelled));}
 void Complete(){auto oldCasts=player.resultCasts;if(!Start()){std::cerr<<"Start failure fixture "<<serial<<" stage "<<scenarioMgr.scenario.step<<" reset "<<ai->resetting<<" phase "<<(ai->state?int(ai->state->phase):-1)<<" pc "<<player.status<<" actor world "<<actor->world<<" ai "<<(actor->ai==ai.get())<<'\n';assert(false);}Natural();Tick(199);assert(player.resultCasts==oldCasts);Tick(1);assert(player.resultCasts==oldCasts+1);assert(player.grants==1&&ai->state->phase==LightsWrathTakeover::Phase::Controlled);Tick(0);assert(actor->despawns==1&&map.objects.size()==1);}
};
void ClearCancelled(Run& r){r.Tick(0);r.Tick(0);assert(!r.ai->state&&!r.ai->resetting&&(r.actor->npcFlags&UNIT_NPC_FLAG_SPELLCLICK));}
using ObjectList=std::list<Unit*>;
Player* GetScriptCreditPlayer(Unit* target){return target->ToPlayer();}
struct LegacyScript{
 Creature* source;Unit* GetBaseObject(){return source;}
 struct Holder{struct{struct{uint32 type=92,misc1=49811,misc2=0,misc3=0;}achievementCriteria;}action;};
 ObjectList* GetTargets(Holder const&,Unit* invoker){return new ObjectList{invoker};}
 void Credit(Unit* unit){Holder e;switch(SMART_ACTION_UPDATE_ACHIEVEMENT_CRITERIA){
#include "NativeLegacyCredit.inc"
 default:assert(false);
 }}
};
int main(){
 {Run r;r.player.failResult=true;assert(r.player.CastSpell(&r.player,207996,false)!=SPELL_CAST_OK&&!r.player.eventCalls);LegacyScript old{r.actor.get()};old.Credit(&r.player);assert(r.player.grants==1&&r.player.forcedCalls==1);r.actor->failSpawn=true;assert(!r.actor->SummonGameObject(248572,1117.78f,1048.03f,310.663f,2.09636f,0,0,0,0,0,{},nullptr,false));r.actor->DespawnOrUnsummon();assert(r.map.objects.empty()&&r.actor->despawns==1);}
 {Run r;Spell native{&r.player,&r.player};native.EffectSendEvent(1);LegacyScript old{r.actor.get()};old.Credit(&r.player);assert(r.player.grants==1&&r.player.eventCalls==2&&r.player.forcedCalls==1);}
 std::cout<<"COUNTERFACTUAL: actual old native205 still credits after failed result cast and then consumes a failed chest source; successful native207996 plus205 duplicates event delivery. No actual client/production crash claimed.\n";
 uint32 valid=0;
 for(uint32 status:{1u,3u})for(uint32 mode=0;mode<3;++mode){Run r;r.player.status=status;assert(r.Start());if(mode==1)r.actor->IsAIEnabled=false;if(mode==2){r.actor->disabled=r.ai.get();r.actor->ai=nullptr;}r.Natural();r.Tick(199);assert(!r.player.resultCasts);r.Tick(1);assert(r.player.grants==1&&!r.player.forcedCalls&&!r.observer.grants);r.Tick(0);assert(r.actor->despawns==1&&r.map.objects.size()==1);assert(r.map.objects.front()->owner.IsEmpty()&&r.map.objects.front()->summoner==r.actor->guid);assert(r.player.eventCalls==1);++valid;}
 for(uint32 invalid=0;invalid<23;++invalid){Run r;
  if(invalid==0)r.player.classId=8;if(invalid==1)r.player.questId=42011;if(invalid==2)r.player.status=0;if(invalid==3)r.player.rewarded=true;
  if(invalid==4)r.player.alive=false;if(invalid==5)r.player.world=false;if(invalid==6)r.player.contact=false;if(invalid==7)r.player.gm=true;
  if(invalid==8)r.player.teleport=true;if(invalid==9)r.player.phase=1;if(invalid==10)r.player.x+=6;if(invalid==11)r.player.visible=false;if(invalid==12)r.player.los=false;
  if(invalid==13)r.actor->entry=1;if(invalid==14)r.actor->dbGuid=1;if(invalid==15)r.actor->world=false;if(invalid==16)r.actor->preDelete=true;
  if(invalid==17)r.actor->alive=false;if(invalid==18)r.map.mapId=576;if(invalid==19)scenarioMgr.scenario.id=1101;if(invalid==20)scenarioMgr.present=false;
  if(invalid==21)scenarioMgr.scenario.achievement.counters.clear();if(invalid==22)r.actor->charmer={1,99,0,1};
  assert(!r.Start()&&!r.player.grants&&r.map.objects.empty()&&!r.ai->state);
 }
 for(uint32 step=0;step<9;++step)if(step!=5){Run r;scenarioMgr.scenario.step=step;assert(!r.Start()&&!r.player.grants&&!r.actor->spawnCalls);}
 for(auto mode:{AURA_REMOVE_BY_DEFAULT,AURA_REMOVE_BY_CANCEL,AURA_REMOVE_BY_ENEMY_SPELL,AURA_REMOVE_BY_DEATH}){Run r;assert(r.Start());RemoveAura(&r.player,r.aura,mode);assert(r.ai->state->phase==LightsWrathTakeover::Phase::Cancelled);ClearCancelled(r);assert(!r.player.grants&&!r.player.HasAura(207949,r.player.guid)&&!r.actor->HasAura(207956,r.actor->guid));r.Complete();}
 {Run r;assert(r.Start());r.Tick(60000);assert(!r.player.grants&&!r.player.resultCasts&&r.aura->m_duration==30000);r.Natural();r.Tick(200);r.Tick(0);assert(r.player.grants==1);}
 {Run r;r.actor->failFury=true;AddAura(r.actor.get(),207956,r.actor->guid);assert(r.Start());assert(!r.ai->state->furyObserved);r.Natural(false);ClearCancelled(r);assert(!r.player.grants&&warnings==1);}
 {Run r;r.actor->failFury=true;assert(r.Start());Advance(r.player,4999);r.Tick(4999);assert(r.actor->furyCasts==1);r.actor->failFury=false;Advance(r.player,1);r.Tick(1);r.Tick(0);assert(r.actor->furyCasts==2&&r.ai->state->furyObserved);r.Natural();r.Tick(200);r.Tick(0);assert(r.player.grants==1);}
 {Run r;assert(r.Start());r.Natural();r.player.failResult=true;r.Tick(200);assert(!r.player.grants&&r.player.resultCasts==1&&!r.actor->spawnCalls);r.player.failResult=false;r.Tick(4999);assert(r.player.resultCasts==1);r.Tick(1);r.Tick(0);assert(r.player.grants==1&&r.player.resultCasts==2);}
 {Run r;assert(r.Start());r.Natural();r.player.acceptCredit=false;r.Tick(200);assert(!r.player.grants&&!r.actor->spawnCalls);r.Tick(4999);assert(r.player.resultCasts==1);r.player.acceptCredit=true;r.Tick(1);r.Tick(0);assert(r.player.grants==1&&r.player.eventCalls==2&&!r.player.forcedCalls);}
 {Run r;r.actor->failSpawn=true;assert(r.Start());r.Natural();r.Tick(200);r.Tick(0);assert(r.player.grants==1&&!r.actor->despawns&&r.actor->spawnCalls==1);r.actor->failSpawn=false;r.Tick(1999);assert(r.actor->spawnCalls==1);r.Tick(1);assert(r.actor->spawnCalls==2&&r.actor->despawns==1&&r.player.eventCalls==1);}
 {Run r;assert(r.Start());auto held=r.player.quest.ScriptLifetime;r.player.quest={};r.player.quest.ScriptLifetime=std::make_shared<uint8>();auto script=std::dynamic_pointer_cast<spell_lights_wrath_control_channel>(r.aura->script);script->Tick(0,nullptr);assert(r.aura->removed&&!r.player.grants);ClearCancelled(r);assert(held!=r.player.quest.ScriptLifetime);r.Complete();}
 {Run r;assert(r.Start());auto old=r.request;auto state=r.ai->state;r.ai->Reset();assert(!r.ai->state&&state->phase==LightsWrathTakeover::Phase::Cancelled&&r.aura->removed);assert(old->Check()==SPELL_FAILED_BAD_TARGETS);r.Complete();}
 {Run r;assert(r.Start());r.ai.reset();r.actor->ai=nullptr;auto script=std::dynamic_pointer_cast<spell_lights_wrath_control_channel>(r.aura->script);script->Tick(0,nullptr);assert(r.aura->removed&&!r.player.grants);r.ai=std::make_unique<npc_lights_wrath_takeover>(r.actor.get());r.actor->ai=r.ai.get();r.Complete();}
 {Run r;assert(r.Start());r.Natural();r.player.deferResult=true;r.Tick(200);assert(pendingResults.size()==1&&!r.player.grants);auto old=pendingResults.front();auto held=r.player.quest.ScriptLifetime;r.player.quest={};r.player.quest.ScriptLifetime=std::make_shared<uint8>();Deliver(old);assert(!r.player.grants&&old->blocked[0]&&old->blocked[1]);r.Tick(0);ClearCancelled(r);r.player.deferResult=false;r.Complete();assert(old->Check()==SPELL_FAILED_BAD_TARGETS&&held!=r.player.quest.ScriptLifetime);}
 {Run r;assert(r.Start());r.Natural();r.player.deferResult=true;r.Tick(200);auto old=pendingResults.front();r.ai.reset();r.actor->ai=nullptr;r.ai=std::make_unique<npc_lights_wrath_takeover>(r.actor.get());r.actor->ai=r.ai.get();Deliver(old);assert(!r.player.grants);r.player.deferResult=false;r.Complete();assert(old->Check()==SPELL_FAILED_BAD_TARGETS);}
 {Run r;r.observer.classId=5;assert(r.Start());spell_lights_wrath_control_request other;other.caster=&r.observer;other.explicitTarget=r.actor.get();assert(other.Check()==SPELL_FAILED_SPELL_IN_PROGRESS);r.Natural();r.Tick(200);r.Tick(0);assert(r.player.grants==1&&!r.observer.grants);}
 {Run r;assert(r.Start());r.Natural();r.player.onScreen=[&]{r.player.quest={};r.player.quest.ScriptLifetime=std::make_shared<uint8>();};r.Tick(200);assert(!r.player.grants&&!r.actor->spawnCalls&&r.player.screenHits==1);}
 {Run r;assert(r.Start());r.Natural();r.player.onCredit=[&]{r.ai.reset();r.actor->ai=nullptr;};r.Tick(200);assert(r.player.grants==1&&!r.actor->spawnCalls);r.ai=std::make_unique<npc_lights_wrath_takeover>(r.actor.get());r.actor->ai=r.ai.get();r.Tick(0);assert(r.actor->spawnCalls==1&&r.player.eventCalls==1);}
 {Run r;scenarioMgr.scenario.step=6;scenarioMgr.scenario.achievement.counters[47119].Counter=1;r.Tick(0);assert(r.actor->spawnCalls==1&&r.actor->despawns==1&&!r.player.eventCalls);}
 {Run r;scenarioMgr.scenario.step=6;scenarioMgr.scenario.achievement.counters[47119].Counter=1;auto go=std::make_shared<GameObject>();go->map=&r.map;go->summoner={2,888,104985,2};r.map.objects.push_back(go);r.Tick(0);assert(!r.actor->spawnCalls&&r.actor->despawns==1&&r.map.objects.size()==1&&!r.player.eventCalls);}
 {Run r;scenarioMgr.scenario.step=7;scenarioMgr.scenario.achievement.counters[47119].Counter=1;r.Tick(0);assert(!r.actor->spawnCalls&&r.actor->despawns==1);}
 {Run r;scenarioMgr.scenario.step=6;scenarioMgr.scenario.achievement.counters[47119].Counter=1;r.actor->charmer={1,88,0,1};r.Tick(0);assert(!r.actor->spawnCalls&&!r.actor->despawns);r.actor->charmer={};r.Tick(0);assert(r.actor->spawnCalls==1);}
 {Run r;assert(r.Start());r.Natural();r.Tick(200);r.map.players.clear();r.player.world=false;r.Tick(0);assert(r.actor->spawnCalls==1&&r.actor->despawns==1);}
 {Run r;assert(r.Start());r.Natural();r.Tick(200);r.actor->onSpawn=[&]{r.actor->preDelete=true;r.map.actors.clear();};r.Tick(0);assert(r.map.objects.size()==1&&!r.actor->despawns);}
 {Run r;spell_lights_wrath_control_result unrequested;unrequested.caster=&r.player;assert(unrequested.Check()==SPELL_FAILED_BAD_TARGETS&&!r.player.grants);}
 {Run r;auto request=std::make_shared<spell_lights_wrath_control_request>();request->caster=r.actor.get();request->explicitTarget=r.actor.get();assert(request->Check()==SPELL_FAILED_BAD_TARGETS&&!r.ai->state);}
 {Run r;auto request=std::make_shared<spell_lights_wrath_control_request>();request->caster=&r.player;request->explicitTarget=r.actor.get();assert(request->Check()==SPELL_CAST_OK);r.Tick(4999);assert(r.ai->state);r.Tick(1);ClearCancelled(r);assert(!r.actor->furyCasts&&!r.player.eventCalls);r.Complete();}
 for(uint32 invalid=0;invalid<7;++invalid){Run r;assert(r.Start());if(invalid==0)r.player.alive=false;if(invalid==1)r.player.gm=true;if(invalid==2)r.player.teleport=true;if(invalid==3)r.player.phase=1;if(invalid==4)r.actor->guid.high+=1;if(invalid==5)r.actor->preDelete=true;if(invalid==6)r.player.guid.high+=1;auto script=std::dynamic_pointer_cast<spell_lights_wrath_control_channel>(r.aura->script);script->Tick(0,nullptr);assert(r.aura->removed&&!r.player.grants&&!r.actor->spawnCalls);}
 {Run r;assert(r.Start());r.Natural();r.player.deferResult=true;r.Tick(200);auto old=pendingResults.front();Map other;Creature sameGuid;sameGuid.map=&other;other.actors={&sameGuid};other.players={{&r.player}};r.player.map=&other;assert(sameGuid.guid==r.actor->guid);Deliver(old);assert(old->blocked[0]&&old->blocked[1]&&!r.player.grants&&old->Check()==SPELL_FAILED_BAD_TARGETS);}
 {Run r;assert(r.Start());r.Natural();r.player.deferResult=true;r.Tick(200);auto old=pendingResults.front();r.player.deferResult=false;r.Tick(5000);assert(r.player.grants==1);Deliver(old);assert(old->blocked[0]&&old->blocked[1]&&r.player.eventCalls==1);r.Tick(0);assert(r.actor->spawnCalls==1);}
 {Run r;assert(r.Start());auto script=std::dynamic_pointer_cast<spell_lights_wrath_control_channel>(r.aura->script);r.ai.reset();r.actor->ai=nullptr;r.ai=std::make_unique<npc_lights_wrath_takeover>(r.actor.get());r.actor->ai=r.ai.get();spell_lights_wrath_control_request retry;retry.caster=&r.player;retry.explicitTarget=r.actor.get();assert(retry.Check()==SPELL_CAST_OK);r.aura->m_duration=30000;r.aura->removed=false;AuraEffect root{0},periodic{1};script->Apply(&root,AURA_EFFECT_HANDLE_REAPPLY);script->Apply(&periodic,AURA_EFFECT_HANDLE_REAPPLY);r.Tick(0);r.Tick(0);r.Natural();r.Tick(200);r.Tick(0);assert(r.player.grants==1);}
 {Run r;assert(r.Start());r.actor->onRemove=[&](uint32 spell){if(spell==207956){r.actor->onRemove={};r.ai->Reset();}};r.ai->Reset();assert(r.aura->removed&&!r.ai->state&&!r.player.grants);r.Complete();}
 for(bool zeroDuration:{false,true}){Run r;assert(r.Start());if(zeroDuration)r.aura->m_duration=0;RemoveAura(&r.player,r.aura,AURA_REMOVE_BY_EXPIRE);assert(r.ai->state->phase==LightsWrathTakeover::Phase::Cancelled&&!r.player.grants);}
 {Run r;assert(r.Start());r.aura->maxDuration=15000;r.aura->m_duration=0;r.aura->m_allDuration=30000;RemoveAura(&r.player,r.aura,AURA_REMOVE_BY_EXPIRE);assert(r.ai->state->phase==LightsWrathTakeover::Phase::Cancelled&&!r.player.grants);}
 {Run r;{spell_lights_wrath_control_request request;request.caster=&r.player;request.explicitTarget=r.actor.get();assert(request.Check()==SPELL_CAST_OK);}assert(r.ai->state->phase==LightsWrathTakeover::Phase::Cancelled);ClearCancelled(r);r.Complete();}
 {Run r;assert(r.Start());r.Natural();r.player.deferResult=true;r.Tick(200);auto old=pendingResults.front();r.player.map=nullptr;Deliver(old);assert(old->blocked[0]&&old->blocked[1]&&!r.player.grants);}
 {Run r;assert(r.Start());r.ai->state->phase=LightsWrathTakeover::Phase::Cancelled;r.actor->onRemove=[&](uint32 spell){if(spell==207956){r.actor->onRemove={};r.ai.reset();r.actor->ai=nullptr;r.ai=std::make_unique<npc_lights_wrath_takeover>(r.actor.get());r.actor->ai=r.ai.get();assert(r.Start());}};r.Tick(0);assert(r.ai->state&&r.ai->state->phase==LightsWrathTakeover::Phase::Channel&&!r.aura->removed);r.Natural();r.Tick(200);r.Tick(0);assert(r.player.grants==1);}
 assert(LightsWrathTakeover::controlRequest.expired());
 std::cout<<"PASS full actual takeover request/aura/result/controller with native quest lease/duration/expiry/locked AI handoff/SendEvent: "<<valid<<" accepted status/primary-disabled/no-primary variants,23 initial denials and stage bounds; true30s+200ms, no timer/cancel/death progress, retries/native acceptance/map-owned creation/recovery, AI retirement/reset/held-lease retake/deferred stale effect and reentrant state changes. Native effect/world/criteria endpoints remain adapters; actual full client/combat/periodic damage pending.\n";
}
