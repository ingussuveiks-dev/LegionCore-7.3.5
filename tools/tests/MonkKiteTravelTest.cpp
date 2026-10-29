#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <vector>
using uint8=uint8_t;using int8=int8_t;using uint32=uint32_t;using int32=int32_t;using ObjectGuid=uint64_t;
enum { CLASS_MONK=10,QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_COMPLETE=1,SPELL_CAST_OK=0,SPELL_FAILED_BAD_TARGETS=1,
 EFFECT_0=0,EFFECT_2=2,EFFECT_ALL=255,SPELL_EFFECT_ANY=0,SPELL_EFFECT_FORCE_CAST=140,SPELL_EFFECT_KILL_CREDIT=90,
 NPC_CLICK_CAST_CASTER_CLICKER=1,NPC_CLICK_CAST_TARGET_CLICKER=2,NPC_CLICK_CAST_ORIG_CASTER_OWNER=4,
 MAX_SPELL_EFFECTS=3,SPELL_AURA_CONTROL_VEHICLE=236,SPELLVALUE_BASE_POINT0=0,MAX_EFFECT_MASK=7,SPELL_EFFECT_HANDLE_HIT_TARGET=1,SMART_EVENT_ON_SPELLCLICK=73 };
using SpellCastResult=uint32;using SpellEffIndex=uint32;
uint32 warnings=0,errors=0;
#define TC_LOG_ERROR(...) (++errors)
#define TC_LOG_DEBUG(...) ((void)0)
#define TC_LOG_WARN(...) (++warnings)
struct Unit;struct Player;struct Creature;struct SmartAI;class spell_monk_kite_travel;
struct NativeEffect { uint32 ApplyAuraName=0,MiscValue=0,Effect=0,TriggerSpell=0; };
struct SpellInfo { uint32 Id=202475,EffectMask=7; NativeEffect storage[3]={{0,0,140,202474},{4,0,6,0},{0,102531,90,0}};NativeEffect* Effects[3]={&storage[0],&storage[1],&storage[2]};NativeEffect const*GetEffect(uint32 i,uint32)const{return Effects[i];} } spellInfo;
struct SpellMgr { SpellInfo const*GetSpellInfo(uint32){return &spellInfo;} } spellMgr;auto*sSpellMgr=&spellMgr;
struct Map {} originMap,otherMap;
struct QuestStatusData { std::shared_ptr<uint8> ScriptLifetime=std::make_shared<uint8>(); };
struct Vehicle { std::map<int,int> Seats;uint32 GetCreatureEntry(){return 102531;} };
struct OutdoorPvP { void HandleSpellClick(Player*,Unit*){} };
struct Unit {
 bool inMap=true,condition=true,requirements=true,reject=false,defer=false,IsAIEnabled=true;uint32 casts=0;std::function<void()> queued;
 virtual~Unit()=default;virtual Player*ToPlayer(){return nullptr;}virtual Creature*ToCreature(){return nullptr;}
 bool IsPlayer(){return ToPlayer()!=nullptr;}Vehicle*GetVehicleKit(){return nullptr;}uint32 GetEntry(){return 102531;}ObjectGuid GetGUID(){return 1;}ObjectGuid GetOwnerGUID(){return 0;}
 uint32 GetSpawnMode(){return 0;}bool IsInMap(Unit*){return inMap;}bool HandleSpellClick(Unit*,int8=-1);
 void CastSpell(Unit*,SpellInfo const*,bool,void*,void*,ObjectGuid);
 void CastCustomSpell(uint32,int,int8,Unit*,bool,void*,void*,ObjectGuid){assert(false);}
};
struct Player:Unit {
 bool world=true,contact=true,alive=true,pending=false,rewarded=false,hasQuest=true;uint32 klass=10,map=1514,instance=0,status=3;
 uint32 nativeCredits=0,groupCredits=0,transfers=0,transferRequests=0,transferResult=0;bool supplyCredit=true,negativeObjective=false;Player*peer=nullptr;Map*mapObject=&originMap;QuestStatusData quest;
 Player*ToPlayer()override{return this;}OutdoorPvP*GetOutdoorPvP(){return nullptr;}
 bool IsInWorld(){return world;}bool CanContact(){return contact;}bool IsAlive(){return alive;}bool HasPendingTeleport(){return pending;}uint32 getClass(){return klass;}uint32 GetMapId(){return map;}Map*GetMap(){return mapObject;}uint32 GetInstanceId(){return instance;}
 bool GetQuestRewardStatus(uint32 id){assert(id==40633);return rewarded;}uint32 GetQuestStatus(uint32 id){assert(id==40633);return hasQuest?status:0;}
 QuestStatusData*getQuestStatus(uint32 id){assert(id==40633);return hasQuest?&quest:nullptr;}
 int32 GetQuestObjectiveData(uint32 id,uint32 objective){assert(id==40633&&objective==102531);return negativeObjective?-1:int32(nativeCredits);}
 void KilledMonsterCredit(uint32 id){assert(id==102531);if(supplyCredit&&hasQuest&&!rewarded&&(status==3||status==1))++nativeCredits;}
 void RewardPlayerAndGroupAtEvent(uint32 id,Unit*){assert(id==102531);++groupCredits;if(peer)++peer->groupCredits;}
 SpellCastResult CastSpell(Unit*target,uint32 id,bool triggered){assert(target==this&&id==202481&&triggered);++transferRequests;if(!transferResult)++transfers;return transferResult;}
};
struct Hook { void operator+=(int){} };
struct SpellScript {
 Unit*caster=nullptr,*hit=nullptr;Hook OnCheckCast,OnEffectHitTarget,AfterHit;bool prevented=false;
 virtual~SpellScript()=default;virtual bool Validate(SpellInfo const*){return true;}virtual void Register(){}
 bool ValidateSpellInfo(std::initializer_list<uint32> spells){return spells.size()==2;}
 Unit*GetCaster(){return caster;}Unit*GetHitUnit(){return hit;}
 void PreventHitDefaultEffect(uint32){prevented=true;}
};
#define PrepareSpellScript(name) public:
#define SpellCheckCastFn(...) 0
#define SpellHitFn(...) 0
#define SpellEffectFn(...) 0
#include "MonkTravel.inc"
struct Spell { uint32 effectHandleMode=1,m_diffMode=0;Unit*unitTarget;SpellInfo const*m_spellInfo=&spellInfo;void EffectKillCreditPersonal(uint32); };
struct Aura { static Aura*TryRefreshStackOrCreate(SpellInfo const*,uint32,Unit*,Unit*,float*,void*,ObjectGuid){assert(false);return nullptr;}void ApplyForTargets(){assert(false);} };
inline int SpellValueMod(int i){return i;}
struct ClickInfo { uint32 castFlags=1,spellId=202475;bool IsFitToRequirements(Unit*a,Unit*)const{return a->requirements;} };
using SpellClickInfoContainer=std::map<int,ClickInfo>;using SpellClickInfoMapBounds=std::pair<SpellClickInfoContainer::const_iterator,SpellClickInfoContainer::const_iterator>;
struct ObjectMgr { SpellClickInfoContainer clicks{{0,{}}};auto GetSpellClickInfoMapBounds(uint32 id){assert(id==102531);return SpellClickInfoMapBounds{clicks.begin(),clicks.end()};} } objectMgr;auto*sObjectMgr=&objectMgr;
using ConditionList=int;struct ConditionSourceInfo { Unit*actor;ConditionSourceInfo(Unit*a,Unit*):actor(a){} };
struct ConditionMgr { int GetConditionsForSpellClickEvent(uint32,uint32){return 0;}bool IsObjectMeetToConditions(ConditionSourceInfo&i,int){return i.actor->condition;} } conditionMgr;auto*sConditionMgr=&conditionMgr;
struct Script { bool legacy=false;void ProcessEventsFor(uint32 id,Unit*u){assert(id==73);if(!legacy)return;if(Player*p=u->ToPlayer()){
 struct {struct {struct {uint32 creature=102531;} killedMonster;} action;} e;
 Player*targetPlayer=p;
#include "NativeGroupCredit.inc"
 p->CastSpell(p,202481,true);
 }} };
struct SmartAI { Script script;Script*GetScript(){return &script;}void OnSpellClick(Unit*); };
struct Creature:Unit { SmartAI ai;Creature*ToCreature()override{return this;}SmartAI*AI(){return &ai;} };
#include "NativeClick.inc"
#include "NativeOnClick.inc"
#include "NativePersonal.inc"
void Unit::CastSpell(Unit*,SpellInfo const*,bool triggered,void*,void*,ObjectGuid)
{
 assert(!triggered);++casts;if(reject)return;
 auto script=std::make_shared<spell_monk_kite_travel>();script->caster=this;
 if(script->Check()!=SPELL_CAST_OK)return;
 auto actual=[this,script]{script->hit=this;script->Guard(2);if(!script->prevented){Spell spell{1,0,this};spell.EffectKillCreditPersonal(2);}script->After();script->After();};
 if(defer)queued=actual;else actual();
}
int main()
{
 _set_abort_behavior(0,_CALL_REPORTFAULT);
 {spell_monk_kite_travel script;assert(script.Validate(&spellInfo));spellInfo.storage[2].MiscValue=1;assert(!script.Validate(&spellInfo));spellInfo.storage[2].MiscValue=102531;}
 for(uint32 failure:{0u,1u}){Creature actor;Player p,peer;p.peer=&peer;actor.ai.script.legacy=true;if(failure==0)p.condition=false;else p.reject=true;actor.HandleSpellClick(&p);assert(!p.nativeCredits&&p.groupCredits==1&&peer.groupCredits==1&&p.transfers==1);}
 for(bool ai:{false,true})for(uint32 failure=0;failure<15;++failure){
  Creature actor;actor.IsAIEnabled=ai;Player p,peer;p.peer=&peer;
  if(failure==1)p.condition=false;if(failure==2)p.requirements=false;if(failure==3)p.reject=true;
  if(failure==4)p.klass=1;if(failure==5)p.map=1;if(failure==6)p.alive=false;if(failure==7)p.world=false;
  if(failure==8)p.contact=false;if(failure==9)p.pending=true;if(failure==10)p.rewarded=true;
  if(failure==11)p.hasQuest=false;if(failure==12)p.status=0;if(failure==13)p.status=6;if(failure==14)p.status=1;
  actor.HandleSpellClick(&p);bool accepted=failure==0||failure==14;
  assert(p.transfers==uint32(accepted)&&p.nativeCredits==uint32(accepted)&&!p.groupCredits&&!peer.groupCredits&&!peer.transfers);
 }
 for(uint32 change=0;change<6;++change){
  Creature actor;Player p;p.defer=true;actor.HandleSpellClick(&p);assert(p.queued&&!p.nativeCredits&&!p.transfers);
  if(change==1)p.quest.ScriptLifetime=std::make_shared<uint8>();if(change==2)p.mapObject=&otherMap;
  if(change==3)p.instance=1;if(change==4)p.pending=true;if(change==5)p.hasQuest=false;
  p.queued();assert(p.nativeCredits==uint32(change==0)&&p.transfers==uint32(change==0));
 }
 {Creature actor;Player p;p.supplyCredit=false;actor.HandleSpellClick(&p);assert(!p.nativeCredits&&!p.transfers);}
 {Creature actor;Player p;p.negativeObjective=true;actor.HandleSpellClick(&p);assert(p.nativeCredits==1&&!p.transfers);}
 {Creature actor;Player p;p.transferResult=7;actor.HandleSpellClick(&p);assert(p.nativeCredits==1&&!p.transfers&&p.transferRequests==1&&warnings==1);p.transferResult=0;actor.HandleSpellClick(&p);assert(p.transfers==1);}
 {Creature actor;Unit pet;actor.HandleSpellClick(&pet);assert(!errors);}
 std::cout<<"PASS: native HandleSpellClick/SmartAI/personal90 with production Monk check/hit: rejected click/cast old counterfactuals, real/deferred hits, class/quest/map/death/lease/missing credit/pet/group isolation, native teleport rejection and new-click retry; no synthetic credit or transfer on failed hit. Native cast completion/force-summon/teleport destination are endpoint adapters, not a client flight.\n";
}
