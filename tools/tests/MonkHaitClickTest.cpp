#include <cassert>
#include <cstdint>
#include <functional>
#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <iostream>
using uint8=uint8_t;using int8=int8_t;using uint32=uint32_t;using int32=int32_t;using uint64=uint64_t;
#include "Functions.inc"
struct ObjectGuid {bool IsEmpty()const{return !low&&!high;} uint64 low=0,high=0;static ObjectGuid Empty;bool operator==(ObjectGuid const&b)const{return low==b.low&&high==b.high;}bool operator!=(ObjectGuid const&b)const{return !(*this==b);}bool operator<(ObjectGuid const&b)const{return high==b.high?low<b.low:high<b.high;} };
ObjectGuid ObjectGuid::Empty{};
enum { CLASS_MONK=10,QUEST_STATUS_NONE=0,QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_COMPLETE=1,MAX_QUEST_LOG_SIZE=25,
 QUEST_SPECIAL_FLAGS_KILL=1,QUEST_SPECIAL_FLAGS_CAST=2,UNIT_FIELD_NPC_FLAGS=0,UNIT_FIELD_FLAGS=1,
 UNIT_NPC_FLAG_SPELLCLICK=1,UNIT_FLAG_PLAYER_CONTROLLED=8,INTERACTION_DISTANCE=5,
 SPELL_CAST_OK=0,SPELL_FAILED_BAD_TARGETS=1,SPELL_FAILED_NOT_READY=2,EFFECT_0=0,EFFECT_1=1,
 SPELL_EFFECT_SEND_EVENT=61,SPELL_EFFECT_KILL_CREDIT2=134,SPELL_EFFECT_KILL_CREDIT=90,TARGET_UNIT_CASTER=1,
 NPC_CLICK_CAST_CASTER_CLICKER=1,NPC_CLICK_CAST_TARGET_CLICKER=2,NPC_CLICK_CAST_ORIG_CASTER_OWNER=4,
 MAX_SPELL_EFFECTS=3,SPELL_AURA_CONTROL_VEHICLE=236,SPELLVALUE_BASE_POINT0=0,MAX_EFFECT_MASK=7,
 SPELL_EFFECT_HANDLE_HIT_TARGET=1,SPELL_EFFECT_HANDLE_HIT=2,TARGET_FLAG_UNIT_MASK=1,TARGET_FLAG_GAMEOBJECT_MASK=2,
 CRITERIA_TYPE_SCRIPT_EVENT=73,CRITERIA_TYPE_SCRIPT_EVENT_2=92,CONFIG_GROUP_XP_DISTANCE=1 };
using SpellCastResult=uint32;using SpellEffIndex=uint32;
uint32 warnings=0,errors=0,clockTime=0;uint32 getMSTime(){return clockTime;}
#include "Timer.inc"
#define TC_LOG_WARN(...) (++warnings)
#define TC_LOG_ERROR(...) (++errors)
#define TC_LOG_DEBUG(...) ((void)0)
struct Player;struct Creature;struct CreatureAI;struct Unit;
struct ZoneScript;
struct Map { ZoneScript*script=nullptr; bool dungeon=true;uint32 events=0;std::map<ObjectGuid,Creature*> actors;Creature*GetCreature(ObjectGuid id){auto i=actors.find(id);return i==actors.end()?nullptr:i->second;}uint32 GetDifficultyID(){return 12;}bool IsDungeon()const{return dungeon;}void ScriptsStart(int,uint32 id,Unit*,void*){assert(id==46736);++events;} } originMap,otherMap;
int sEventScripts=0;
struct WorldObject {
 Map*mapObject=&originMap;uint32 map=1014,instance=8;ObjectGuid guid{1,0xeeee111122223333};bool world=true;float distance=0;
 virtual~WorldObject()=default;virtual bool IsCreature()const{return false;}bool IsInWorld()const{return world;}Map*GetMap()const{return mapObject;}
 uint32 GetMapId()const{return map;}uint32 GetInstanceId()const{return instance;}ObjectGuid GetGUID()const{return guid;}float GetDistance(WorldObject const*p)const{return p->distance;}
};
struct ZoneScript { ObjectGuid accepted;Creature*openingActor=nullptr;ObjectGuid GetGuidData(uint32)const{return accepted;}void SetGuidData(uint32,ObjectGuid);uint32 nativeEvents=0;void ProcessEvent(WorldObject*,uint32 id){assert(id==46736);++nativeEvents;} };using InstanceScript=ZoneScript;
struct Vehicle { uint32 GetCreatureEntry(){assert(false);return 0;} };
struct OutdoorPvP { void HandleSpellClick(Player*,Unit*){} };
struct Target { uint32 value=0;uint32 GetTarget()const{return value;} };
struct NativeEffect { uint32 Effect=0;int32 MiscValue=0;uint32 ApplyAuraName=0;Target TargetA;uint32 GetProvidedTargetMask()const{return 0;} };
struct SpellInfo { uint32 Id=197120,EffectMask=3;NativeEffect storage[3]={{61,46736,0,{0}},{134,97778,0,{1}},{}};NativeEffect*Effects[3]={&storage[0],&storage[1],&storage[2]};NativeEffect const*GetEffect(uint32 i,uint32)const{return Effects[i];} } clickInfo,personalInfo;
struct SpellMgr { SpellInfo const*GetSpellInfo(uint32 id){assert(id==197120||id==194020);return id==197120?&clickInfo:&personalInfo;} } spellMgr;auto*sSpellMgr=&spellMgr;
struct QuestStatusData { uint32 Status=3;std::shared_ptr<uint8> ScriptLifetime; };
struct Quest { bool nativeFlags=true,raidAllowed=false;bool HasSpecialFlag(uint32 flags){assert(flags==3);return nativeFlags;}bool IsAllowedInRaid(uint32 d){assert(d==12);return raidAllowed;} } questTemplate;
struct QuestStore { bool present=true;Quest*GetQuestTemplate(uint32 id){assert(id==12103);return present?&questTemplate:nullptr;} } questStore;auto*sQuestDataStore=&questStore;
struct Scenario { uint32 id=943,step=0;uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;} } scenario;
struct ScenarioMgr { bool present=true;Scenario*GetScenario(uint32){return present?&scenario:nullptr;} } scenarioMgr;auto*sScenarioMgr=&scenarioMgr;
struct GroupReference { Player*player=nullptr;GroupReference*following=nullptr;Player*getSource(){return player;}GroupReference*next(){return following;} };
struct Group { bool raid=false;GroupReference*first=nullptr;bool isRaidGroup(){return raid;}GroupReference*GetFirstMember(){return first;} };
struct World { float getFloatConfig(uint32 id){assert(id==1);return 100;} } worldEndpoint;auto*sWorld=&worldEndpoint;
struct Unit:WorldObject {
 bool alive=true,condition=true,requirements=true,reject=false,inMap=true,IsAIEnabled=true,defer=false;uint32 casts=0;std::function<void()> queued;
 virtual Player*ToPlayer(){return nullptr;}virtual Creature*ToCreature(){return nullptr;}bool IsPlayer(){return ToPlayer()!=nullptr;}bool IsAlive()const{return alive;}
 virtual uint32 GetEntry(){return 97778;}Vehicle*GetVehicleKit(){return nullptr;}ObjectGuid GetOwnerGUID(){return {};}uint32 GetSpawnMode(){return 0;}bool IsInMap(Unit*){return inMap;}
 ZoneScript*GetZoneScript(){return nullptr;}InstanceScript*GetInstanceScript(){return mapObject->script;}
 bool HandleSpellClick(Unit*,int8=-1);void CastSpell(Unit*,SpellInfo const*,bool,void*,void*,ObjectGuid);
 void CastCustomSpell(uint32,int,int8,Unit*,bool,void*,void*,ObjectGuid){assert(false);}
};
struct Player:Unit {
 bool contact=true,pending=false,rewarded=false,hasQuest=true,phase=true,visible=true,los=true,supplyGroup=true,supplyPersonal=true,negativeGroup=false,negativePersonal=false,talkConsumes=false;
 uint32 klass=10,slot=0,personalResult=0,personalDelay=0,groupCalls=0,personalCalls=0,personalRequests=0,criteria=0;int32 groupReceipt=0,personalReceipt=0;
 Group*group=nullptr;WorldObject*corpse=nullptr;QuestStatusData quest;FunctionProcessor functions;std::function<void()> onChild;
 Player*ToPlayer()override{return this;}OutdoorPvP*GetOutdoorPvP(){return nullptr;}bool CanContact(){return contact;}bool HasPendingTeleport(){return pending;}uint32 getClass(){return klass;}
 bool GetQuestRewardStatus(uint32 id){assert(id==12103);return rewarded;}uint32 GetQuestStatus(uint32 id){assert(id==12103);return hasQuest?quest.Status:0;}uint32 FindQuestSlot(uint32){return slot;}
 QuestStatusData*getQuestStatus(uint32 id){assert(id==12103);return hasQuest?&quest:nullptr;}Group*GetGroup(){return group;}bool InInstance()const{return true;}WorldObject*GetCorpse()const{return corpse;}
 bool InSamePhase(Creature*){return phase;}bool canSeeOrDetect(Creature*){return visible;}bool IsWithinDistInMap(Creature*,float){return distance<=5;}bool IsWithinLOSInMap(Creature*){return los;}
 int32 GetQuestObjectiveData(uint32 id,uint32 objective){assert(id==12103);if(objective==97778)return negativeGroup?-1:groupReceipt;assert(objective==98514);return negativePersonal?-1:personalReceipt;}
 void KilledMonsterCredit(uint32 id,ObjectGuid={}) { assert(id==97778||id==98514);if(id==97778)++groupCalls;else ++personalCalls;
  if(hasQuest&&!rewarded&&quest.Status==3&&(!group||!group->raid||questTemplate.raidAllowed)){if(id==97778&&supplyGroup)groupReceipt=1;if(id==98514&&supplyPersonal)personalReceipt=1;}}
 bool TalkedToCreature(uint32 id,ObjectGuid){assert(id==97778);return talkConsumes;}
 void RewardPlayerAndGroupAtEvent(uint32,WorldObject*);bool IsAtGroupRewardDistance(WorldObject const*)const;
 void UpdateAchievementCriteria(uint32 type,uint32 id,uint32){assert((type==73||type==92)&&id==46736);++criteria;}
 void AddDelayedEvent(uint64 delay,std::function<void()>&&f){functions.AddDelayedEvent(delay,std::move(f));}void AddQuestDelayedEvent(uint32,uint64,std::function<void()>&&);
 SpellCastResult CastSpell(Unit*,uint32,bool);
 void Tick(uint32 delta){clockTime+=delta;functions.Update(delta);}
};
struct CreatureAI { Creature*me;explicit CreatureAI(Creature*p):me(p){}virtual~CreatureAI()=default;virtual void OnSpellClick(Unit*){}virtual void DoAction(int32); };
using ScriptedAI=CreatureAI;
struct Creature:Unit {
 uint32 entry=97778,db=362105,npcflags=0,flags=0,starts=0;bool hidden=false;CreatureAI*current=nullptr,*disabled=nullptr;Creature*chuan=nullptr,*demon=nullptr;
 Creature(){static uint64 n=100;guid={n++,0xf130123456789abc};originMap.actors[guid]=this;}
 ~Creature(){originMap.actors.erase(guid);if(disabled!=current)delete disabled;delete current;}
 Creature*ToCreature()override{return this;}bool IsCreature()const override{return true;}uint32 GetEntry()override{return entry;}uint32 GetDBTableGUIDLow(){return db;}
 CreatureAI*GetAI(){return current;}CreatureAI*GetDisabledAI(){return disabled;}CreatureAI*AI(){return current;}
 bool HasFlag(uint32 field,uint32 mask){return ((field==0?npcflags:flags)&mask)!=0;}void SetFlag(uint32 field,uint32 mask){(field==0?npcflags:flags)|=mask;}
 void RemoveFlag(uint32 field,uint32 mask){(field==0?npcflags:flags)&=~mask;}bool ShouldHideFor(ObjectGuid){return hidden;}
 Creature*FindNearestCreature(uint32 id,float,bool){auto p=id==98001?chuan:demon;return p&&p->alive&&p->world?p:nullptr;}
};
void CreatureAI::DoAction(int32 id){assert(id==1);++me->starts;}
#include "HaitOpeningProtocol.inc"
// The old native click host controls only the new accepted-owner endpoint;
// HaitOpeningTest separately compiles the real State/full instance/native map.
void ZoneScript::SetGuidData(uint32 id,ObjectGuid owner){assert(id==MonkHaitOpening::AcceptData&&openingActor);if(accepted.IsEmpty()){accepted=owner;++openingActor->starts;}}
struct npc_master_hait { struct npc_master_haitAI:ScriptedAI {
#include "ActorAdmission.inc"
};};
struct OldAI:CreatureAI {using CreatureAI::CreatureAI;
#include "OldHook.inc"
};
struct Spell { ObjectGuid m_originalTargetGUID;uint32 effectHandleMode=1,m_diffMode=0;Unit*unitTarget=nullptr,*m_caster=nullptr;WorldObject*gameObjTarget=nullptr,*focusObject=nullptr;SpellInfo const*m_spellInfo=&clickInfo;
 void EffectKillCreditPersonal(uint32);void EffectKillCredit(uint32);void EffectSendEvent(uint32);
};
struct Hook {void operator+=(int){} };
struct SpellScript {
 Spell*m_spell=nullptr;Unit*caster=nullptr,*hit=nullptr;bool prevented=false;Hook OnCheckCast,OnEffectHit,OnEffectHitTarget,AfterHit;
 virtual~SpellScript()=default;virtual bool Validate(SpellInfo const*){return true;}virtual bool Load(){return true;}virtual void Register(){}
 Unit*GetCaster(){return caster;}Unit*GetHitUnit(){return hit;}ObjectGuid GetOriginalTargetGUID()const;void PreventHitDefaultEffect(uint32){prevented=true;}
};
#define PrepareSpellScript(name) public:
#define SpellCheckCastFn(...) 0
#define SpellEffectFn(...) 0
#define SpellHitFn(...) 0
#include "Click.inc"
struct Aura {static Aura*TryRefreshStackOrCreate(SpellInfo const*,uint32,Unit*,Unit*,float*,void*,ObjectGuid){assert(false);return nullptr;}void ApplyForTargets(){assert(false);} };
inline int SpellValueMod(int i){return i;}
struct ClickInfo {uint32 castFlags=1,spellId=197120;bool IsFitToRequirements(Unit*p,Unit*)const{return p->requirements;} };
using SpellClickInfoContainer=std::map<int,ClickInfo>;using SpellClickInfoMapBounds=std::pair<SpellClickInfoContainer::const_iterator,SpellClickInfoContainer::const_iterator>;
struct ObjectMgr {SpellClickInfoContainer rows{{0,{}}};auto GetSpellClickInfoMapBounds(uint32 id){assert(id==97778);return SpellClickInfoMapBounds{rows.begin(),rows.end()};} } objectMgr;auto*sObjectMgr=&objectMgr;
using ConditionList=int;struct ConditionSourceInfo {Unit*clicker;ConditionSourceInfo(Unit*p,Unit*):clicker(p){} };
struct ConditionMgr {int GetConditionsForSpellClickEvent(uint32,uint32){return 0;}bool IsObjectMeetToConditions(ConditionSourceInfo&i,int){return i.clicker->condition;} } conditionMgr;auto*sConditionMgr=&conditionMgr;
#include "NativeClick.inc"
#include "NativePersonal.inc"
#include "NativeCredit.inc"
#include "NativeEvent.inc"
#include "NativeGroup.inc"
#include "NativeDistance.inc"
#include "NativeDelayed.inc"
#include "NativeOriginalGuid.inc"
SpellCastResult Player::CastSpell(Unit*target,uint32 id,bool triggered){assert(target==this&&id==194020&&triggered);++personalRequests;if(onChild)onChild();if(personalResult)return personalResult;
 auto effect=[this]{Spell spell;spell.m_spellInfo=&personalInfo;spell.unitTarget=this;spell.EffectKillCreditPersonal(0);};if(personalDelay)AddDelayedEvent(personalDelay,std::move(effect));else effect();return 0;}
struct Invocation {
 Spell native;spell_monk_hait_click script;
 Invocation(Unit*caster,ObjectGuid original){native.m_originalTargetGUID=original;native.m_caster=caster;native.unitTarget=caster;script.m_spell=&native;script.caster=caster;script.hit=caster;}
 void Hit(){script.prevented=false;script.Event(0);if(!script.prevented){native.effectHandleMode=2;native.EffectSendEvent(0);}script.prevented=false;script.Credit(1);if(!script.prevented){native.effectHandleMode=1;native.EffectKillCredit(1);}script.After();script.After();}
};
void Unit::CastSpell(Unit*target,SpellInfo const*info,bool triggered,void*,void*,ObjectGuid){assert(info==&clickInfo&&!triggered);++casts;if(reject)return;
 auto invocation=std::make_shared<Invocation>(this,target->GetGUID());if(invocation->script.Load()&&invocation->script.Check()!=0)return;
 auto finish=[invocation]{invocation->Hit();};if(defer)queued=finish;else finish();}
using Hait=npc_master_hait::npc_master_haitAI;
struct Run {
 Creature actor,chuan,demon;Player player;ZoneScript instance;
 Run(){originMap.script=&instance;instance.openingActor=&actor;clockTime=0;questTemplate={};questStore.present=true;scenario={};scenarioMgr.present=true;chuan.entry=98001;chuan.db=362111;demon.entry=98011;demon.db=362135;actor.chuan=&chuan;chuan.demon=&demon;actor.current=new Hait(&actor);}
 Hait*Content(){return static_cast<Hait*>(actor.disabled?actor.disabled:actor.current);}
 void Click(){actor.HandleSpellClick(&player);}void Hit(){auto f=std::move(player.queued);assert(f);f();}
 void Success(){if(!(player.groupReceipt==1&&player.personalReceipt==1&&actor.starts==1&&!actor.npcflags))std::cerr<<"Unexpected receipt/start: group="<<player.groupReceipt<<" personal="<<player.personalReceipt<<" starts="<<actor.starts<<" flags="<<actor.npcflags<<" time="<<clockTime<<" warnings="<<warnings<<"\n";assert(player.groupReceipt==1&&player.personalReceipt==1&&actor.starts==1&&!actor.npcflags);}
};
int main(){
 _set_abort_behavior(0,_CALL_REPORTFAULT);std::cout<<std::unitbuf;personalInfo.Id=194020;personalInfo.EffectMask=1;personalInfo.storage[0]={90,98514,0,{1}};
 uint32 cases=0;
 {spell_monk_hait_click s;assert(s.Validate(&clickInfo));for(uint32 field=0;field<5;++field){auto saved=clickInfo.storage[0];auto saved1=clickInfo.storage[1];auto saved2=personalInfo.storage[0];if(field==0)++clickInfo.storage[0].MiscValue;if(field==1)++clickInfo.storage[1].Effect;if(field==2)++clickInfo.storage[1].MiscValue;if(field==3)++clickInfo.storage[1].TargetA.value;if(field==4)++personalInfo.storage[0].MiscValue;assert(!s.Validate(&clickInfo));clickInfo.storage[0]=saved;clickInfo.storage[1]=saved1;personalInfo.storage[0]=saved2;++cases;}}
 // Real old hook bypasses both rejected native conditions and failed casts.
 for(uint32 failure=0;failure<3;++failure){Run r;delete r.actor.current;r.actor.current=new OldAI(&r.actor);if(failure==0)r.player.condition=false;if(failure==1)r.player.requirements=false;if(failure==2)r.player.reject=true;r.Click();assert(r.player.personalReceipt==1&&r.player.groupReceipt==1&&r.actor.starts==1&&!r.actor.npcflags&&r.player.personalRequests==0);++cases;}
 std::cout<<"EXPECTED BROKEN bb877ffc3: native rejected conditions/requirements/cast still invokes old hook, granting both receipts and starting scene. No invalid pointer executed.\n";
 for(uint32 failure=0;failure<3;++failure){Run r;if(failure==0)r.player.condition=false;if(failure==1)r.player.requirements=false;if(failure==2)r.player.reject=true;r.Click();assert(!r.player.groupCalls&&!r.player.personalCalls&&!r.actor.starts&&r.actor.npcflags==1&&!r.player.personalRequests);++cases;}
 {Run r;r.Click();r.Success();assert(r.player.groupCalls==1&&r.player.personalCalls==1&&r.player.personalRequests==1&&r.player.criteria==2&&r.instance.nativeEvents==1);r.Click();r.Success();assert(r.player.groupCalls==1&&r.player.personalCalls==1);++cases;}
 {Run r;r.Click();r.Success();delete r.actor.current;r.actor.current=new Hait(&r.actor);assert(r.actor.npcflags);r.Click();assert(r.actor.starts==1&&r.player.groupCalls==1&&r.player.personalCalls==1);++cases;}
 for(uint32 fail=0;fail<35;++fail){Run r;
  if(fail==0)r.player.alive=false;if(fail==1)r.player.world=false;if(fail==2)r.player.contact=false;if(fail==3)r.player.pending=true;if(fail==4)r.player.klass=1;
  // Outside intro Load delegates native use; test the explicit Check directly.
  if(fail==5)r.player.map=1;if(fail==6)r.player.rewarded=true;if(fail==7)r.player.hasQuest=false;if(fail==8)r.player.quest.Status=1;if(fail==9)r.player.quest.Status=0;
  if(fail==10)r.player.slot=25;if(fail==11)scenarioMgr.present=false;if(fail==12)scenario.id=1;if(fail==13)scenario.step=1;if(fail==14)questStore.present=false;if(fail==15)questTemplate.nativeFlags=false;
  if(fail==16)r.actor.db=362271;if(fail==17)r.actor.map=1;if(fail==18)r.actor.entry=1;if(fail==19)r.actor.alive=false;if(fail==20)r.actor.world=false;
  if(fail==21)r.actor.mapObject=&otherMap;if(fail==22)r.actor.npcflags=0;if(fail==23)r.actor.flags=8;if(fail==24)r.player.phase=false;if(fail==25)r.actor.hidden=true;
  if(fail==26)r.player.visible=false;if(fail==27)r.player.distance=6;if(fail==28)r.player.los=false;if(fail==29)r.actor.chuan=nullptr;if(fail==30)r.chuan.db=1;
  if(fail==31)r.chuan.demon=nullptr;if(fail==32)r.demon.db=1;if(fail==33)r.chuan.alive=false;if(fail==34)r.demon.alive=false;
  Invocation cast(&r.player,r.actor.guid);assert(cast.script.Check()!=0);cast.Hit();assert(!r.player.groupCalls&&!r.player.personalRequests&&!r.actor.starts);++cases;
 }
 for(uint32 guidCase=0;guidCase<4;++guidCase){Run r;ObjectGuid id=guidCase==0?ObjectGuid{}:guidCase==1?r.player.guid:guidCase==2?r.chuan.guid:r.actor.guid;Invocation cast(&r.player,id);assert((cast.script.Check()==0)==(guidCase==3));cast.Hit();if(guidCase==3)r.Success();else assert(!r.player.groupCalls&&!r.actor.starts);++cases;}
 {Run r;Invocation a(&r.player,r.actor.guid);assert(a.script.Check()==0&&a.script.Check()==0);Invocation b(&r.player,r.actor.guid);assert(b.script.Check()==SPELL_FAILED_NOT_READY);a.Hit();r.Success();++cases;}
 {Run r;{Invocation failed(&r.player,r.actor.guid);assert(failed.script.Check()==0);}assert(r.Content()->_pendingClick.expired());r.Click();r.Success();++cases;}
 for(uint32 change=0;change<14;++change){Run r;r.player.defer=true;r.Click();assert(r.player.queued&&!r.player.groupReceipt&&!r.actor.starts);
  if(change==1)r.player.quest.ScriptLifetime=std::make_shared<uint8>();if(change==2)r.player.mapObject=&otherMap;if(change==3)++r.player.instance;if(change==4)r.player.pending=true;
  if(change==5)r.player.hasQuest=false;if(change==6)r.actor.alive=false;if(change==7){delete r.actor.current;r.actor.current=new Hait(&r.actor);}if(change==8)r.player.distance=6;
  if(change==9)r.player.los=false;if(change==10)scenario.step=1;if(change==11)r.actor.chuan=nullptr;if(change==12)r.player.quest.Status=1;
  if(change==13){r.actor.disabled=r.actor.current;r.actor.current=new CreatureAI(&r.actor);}
  r.Hit();if(change==0||change==13)r.Success();else assert(!r.player.groupCalls&&!r.player.personalRequests&&!r.actor.starts);++cases;
 }
 for(uint32 mode=0;mode<5;++mode){Run r;if(mode==0)r.player.supplyGroup=false;if(mode==1)r.player.negativeGroup=true;if(mode==2)r.player.talkConsumes=true;if(mode==3)r.player.personalReceipt=1;if(mode==4)r.player.groupReceipt=1;r.Click();if(mode<3)assert(!r.actor.starts&&!r.player.personalRequests);else {r.Success();assert(r.player.personalRequests==(mode==3?0u:1u));}++cases;}
 {Run r;r.player.personalResult=7;auto before=warnings;r.Click();assert(warnings==before+1&&r.player.groupReceipt==1&&!r.player.personalReceipt&&!r.actor.starts&&r.actor.npcflags==1);r.player.personalResult=0;r.Click();r.Success();++cases;}
 for(uint32 delay:{1u,1000u,2000u,5000u}){Run r;r.player.personalDelay=delay;auto before=warnings;r.Click();assert(!r.actor.starts&&r.player.groupReceipt==1);r.player.Tick(delay<10?10:delay);if(delay<2000){r.player.Tick(10);r.Success();}else {assert(!r.actor.starts&&warnings==before+1&&r.actor.npcflags==1);r.player.personalDelay=0;r.Click();r.Success();}++cases;}
 {Run r;clockTime=0xfffffff0;r.player.personalDelay=100;r.Click();r.player.Tick(100);r.player.Tick(10);r.Success();++cases;}
 for(uint32 change=0;change<5;++change){Run r;r.player.supplyPersonal=false;r.Click();assert(!r.actor.starts);if(change==0)r.player.quest.ScriptLifetime=std::make_shared<uint8>();if(change==1)r.player.hasQuest=false;if(change==2){delete r.actor.current;r.actor.current=new Hait(&r.actor);}if(change==3)r.player.pending=true;if(change==4)r.player.mapObject=&otherMap;auto before=warnings;r.player.Tick(2500);assert(!r.actor.starts&&warnings==before&&r.actor.npcflags==1);++cases;}
 {Run r;r.player.supplyPersonal=false;auto before=warnings;r.Click();r.player.Tick(1999);assert(!r.actor.starts&&warnings==before);r.player.Tick(10);assert(warnings==before+1&&!r.actor.starts&&r.actor.npcflags==1);r.player.supplyPersonal=true;r.Click();r.Success();++cases;}
 {Run r;r.player.onChild=[&]{Invocation nested(&r.player,r.actor.guid);assert(nested.script.Check()==SPELL_FAILED_NOT_READY);};r.Click();r.Success();++cases;}
 {Run r;Unit pet;r.actor.HandleSpellClick(&pet);r.Content()->OnSpellClick(nullptr);r.Content()->OnSpellClick(&pet);assert(!r.actor.starts&&!r.player.groupCalls&&!errors);++cases;}
 {Run r;auto player=std::make_unique<Player>();player->supplyPersonal=false;r.actor.HandleSpellClick(player.get());assert(!r.Content()->_pendingClick.expired());player.reset();assert(r.Content()->_pendingClick.expired()&&!r.actor.starts);++cases;}
 {Run r;Player peer,far,offline,ghost,dead,withoutQuest,otherInstance;WorldObject corpse;peer.guid.low=2;Group group;GroupReference refs[8];Player*members[]={&r.player,&peer,&far,&offline,&ghost,&dead,&withoutQuest,&otherInstance};for(int i=0;i<8;++i){refs[i].player=members[i];refs[i].following=i<7?&refs[i+1]:nullptr;members[i]->group=&group;}group.first=refs;
  far.map=1;offline.world=false;ghost.alive=false;ghost.corpse=&corpse;dead.alive=false;withoutQuest.hasQuest=false;++otherInstance.instance;r.Click();r.Success();assert(peer.groupReceipt==1&&dead.groupReceipt==1&&!peer.personalReceipt&&!dead.personalReceipt&&!far.groupCalls&&!offline.groupCalls&&!ghost.groupCalls&&!withoutQuest.groupReceipt&&!otherInstance.groupCalls);++cases;}
 for(bool allowed:{false,true}){Run r;Group group;GroupReference member{&r.player};group.first=&member;group.raid=true;r.player.group=&group;questTemplate.raidAllowed=allowed;r.Click();if(allowed)r.Success();else assert(!r.player.groupCalls&&!r.actor.starts);++cases;}
 for(uint32 db:{362105u,362271u,0u}){Creature actor;actor.db=db;actor.current=new Hait(&actor);assert((actor.npcflags==1)==(db==362105));++cases;}
 {Creature actor;actor.map=1;actor.current=new Hait(&actor);assert(!actor.npcflags);++cases;}
 assert(!errors);std::cout<<"PASS "<<cases<<" ASAN cases: actual production Hight check/effect/receipt/lease; native click,61,134,90,group/distance,quest-owned delay,original GUID and wrap-safe elapsed time. Includes old rejected-click counterfactual, repeat/concurrent/reentrant/canceled attempts, deadline/retry and lifetime cleanup. World/cast completion/quest objective mutation/accepted-instance endpoint controlled; full scenario and client not certified.\n";
}
