#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <vector>
using uint8=uint8_t;using int8=int8_t;using uint32=uint32_t;using int32=int32_t;using ObjectGuid=uint64_t;
enum{NPC_CLICK_CAST_CASTER_CLICKER=1,NPC_CLICK_CAST_TARGET_CLICKER=2,NPC_CLICK_CAST_ORIG_CASTER_OWNER=4,
 MAX_SPELL_EFFECTS=3,SPELL_AURA_CONTROL_VEHICLE=236,SPELLVALUE_BASE_POINT0=0,MAX_EFFECT_MASK=7,
 SPELL_EFFECT_HANDLE_HIT_TARGET=1,SMART_EVENT_ON_SPELLCLICK=73,SMART_ACTION_CALL_KILLEDMONSTER=33,SMART_EVENT_LINK=61};
struct Unit;struct Player;struct Creature;struct SmartAI;struct Spell;
inline uint32 errors=0;
#define TC_LOG_ERROR(...) (++errors)
#define TC_LOG_DEBUG(...) ((void)0)
struct Effect{uint32 ApplyAuraName=0,MiscValue=0;};
struct SpellInfo{uint32 Id=209441,EffectMask=7;Effect effects[3]={{4,0},{0,105697},{0,5626}};Effect const* GetEffect(uint32 i,uint32)const{return &effects[i];}} spellInfo;
struct SpellMgr{auto GetSpellInfo(uint32){return &spellInfo;}} spellMgr;auto sSpellMgr=&spellMgr;
struct Vehicle{std::map<int,int> Seats;uint32 GetCreatureEntry(){return 105697;}};
struct OutdoorPvP{void HandleSpellClick(Player*,Unit*){}};
struct Unit{
 bool inMap=true,condition=true,requirements=true,reject=false,defer=false,IsAIEnabled=true;uint32 casts=0;std::function<void()> queued;
 virtual ~Unit()=default;virtual Player* ToPlayer(){return nullptr;}virtual Creature* ToCreature(){return nullptr;}
 bool IsPlayer(){return ToPlayer()!=nullptr;}Vehicle* GetVehicleKit(){return nullptr;}uint32 GetEntry(){return 105697;}ObjectGuid GetGUID(){return 1;}ObjectGuid GetOwnerGUID(){return 0;}
 uint32 GetSpawnMode(){return 0;}bool IsInMap(Unit*){return inMap;}bool HandleSpellClick(Unit*,int8=-1);
 void CastSpell(Unit*,SpellInfo const*,bool,void*,void*,ObjectGuid);
 void CastCustomSpell(uint32,int,int8,Unit*,bool,void*,void*,ObjectGuid){assert(false);}
};
struct Player:Unit{
 uint32 nativeCredits=0,groupCredits=0,taxis=0;bool taxiReject=false;Player* peer=nullptr;
 Player* ToPlayer()override{return this;}OutdoorPvP* GetOutdoorPvP(){return nullptr;}
 void KilledMonsterCredit(uint32 id){assert(id==105697);++nativeCredits;}
 void RewardPlayerAndGroupAtEvent(uint32 id,Unit*){assert(id==105697);++groupCredits;if(peer)++peer->groupCredits;}
 void ActivateTaxiPathTo(uint32 id,uint32 spell){assert(id==5626&&spell==209441);if(!taxiReject)++taxis;}
};
struct Spell{uint32 effectHandleMode=1,m_diffMode=0;Unit* unitTarget;SpellInfo* m_spellInfo=&spellInfo;void EffectKillCreditPersonal(uint32);void EffectSendTaxi(uint32);};
using SpellEffIndex=uint32;inline int SpellValueMod(int i){return i;}
struct Aura{static Aura* TryRefreshStackOrCreate(SpellInfo const*,uint32,Unit*,Unit*,float*,void*,ObjectGuid){assert(false);return nullptr;}void ApplyForTargets(){assert(false);}};
struct ClickInfo{uint32 castFlags=1,spellId=209441;bool IsFitToRequirements(Unit* a,Unit*)const{return a->requirements;}};
using SpellClickInfoContainer=std::map<int,ClickInfo>;using SpellClickInfoMapBounds=std::pair<SpellClickInfoContainer::const_iterator,SpellClickInfoContainer::const_iterator>;
struct ObjectMgr{SpellClickInfoContainer clicks{{0,{}}};auto GetSpellClickInfoMapBounds(uint32 id){assert(id==105697);return SpellClickInfoMapBounds{clicks.begin(),clicks.end()};}} objectMgr;auto sObjectMgr=&objectMgr;
using ConditionList=int;struct ConditionSourceInfo{Unit* actor;ConditionSourceInfo(Unit* a,Unit*):actor(a){}};
struct ConditionMgr{int GetConditionsForSpellClickEvent(uint32,uint32){return 0;}bool IsObjectMeetToConditions(ConditionSourceInfo& i,int){return i.actor->condition;}} conditionMgr;auto sConditionMgr=&conditionMgr;
struct SmartScriptHolder{uint32 link=1,event_id=0,entryOrGuid=105697;struct{struct{uint32 creature=105697;}killedMonster;}action;uint32 GetScriptType(){return 0;}uint32 GetActionType(){return 0;}uint32 GetEventType(){return 0;}};
struct Script{bool legacy=false;void ProcessEventsFor(uint32 id,Unit* u){assert(id==73);if(!legacy)return;SmartScriptHolder e;Unit* unit=u;uint32 var0=0,var1=0;bool bvar=false;void* spell=nullptr,*gob=nullptr;auto targetPlayer=u->ToPlayer();if(targetPlayer){
#include "NativeGroupCredit.inc"
 }
#include "NativeMissingLink.inc"
 }SmartScriptHolder FindLinkedEvent(uint32){return {};}void ProcessEvent(SmartScriptHolder&,Unit*,uint32,uint32,bool,void*,void*){assert(false);}};
struct SmartAI{Script script;Script* GetScript(){return &script;}void OnSpellClick(Unit*);};
struct Creature:Unit{SmartAI ai;Creature* ToCreature()override{return this;}SmartAI* AI(){return &ai;}};
#include "NativeClick.inc"
#include "NativeOnClick.inc"
#include "NativePersonal.inc"
#include "NativeTaxi.inc"
void Unit::CastSpell(Unit*,SpellInfo const*,bool triggered,void*,void*,ObjectGuid){assert(!triggered);++casts;if(reject)return;auto hit=[this]{Spell s{1,0,this};s.EffectKillCreditPersonal(1);s.EffectSendTaxi(2);};if(defer)queued=hit;else hit();}
int main(){
 {Creature actor;Player p,peer;p.peer=&peer;actor.ai.script.legacy=true;p.condition=false;assert(!actor.HandleSpellClick(&p));assert(!p.casts&&!p.nativeCredits&&p.groupCredits==1&&peer.groupCredits==1&&errors==1);}
 {Creature actor;Player p;p.reject=true;actor.ai.script.legacy=true;errors=0;assert(actor.HandleSpellClick(&p));assert(p.casts==1&&!p.nativeCredits&&p.groupCredits==1&&errors==1);}
 {Creature actor;Player p;actor.ai.script.legacy=true;errors=0;assert(actor.HandleSpellClick(&p));assert(p.nativeCredits==1&&p.groupCredits==1&&p.taxis==1&&errors==1);}
 errors=0;
 for(bool ai:{false,true})for(int mode=0;mode<4;++mode){Creature actor;Player p,peer;p.peer=&peer;actor.IsAIEnabled=ai;if(mode==1)p.condition=false;if(mode==2)p.requirements=false;if(mode==3)p.reject=true;actor.HandleSpellClick(&p);assert(p.nativeCredits==uint32(mode==0)&&!p.groupCredits&&!peer.groupCredits&&!peer.nativeCredits&&p.taxis==uint32(mode==0));}
 {Creature actor;Player p;p.defer=true;actor.HandleSpellClick(&p);assert(!p.nativeCredits&&!p.taxis);p.queued();assert(p.nativeCredits==1&&p.taxis==1&&!p.groupCredits);}
 {Creature actor;Player p;p.taxiReject=true;actor.HandleSpellClick(&p);assert(p.nativeCredits==1&&!p.taxis);}// Native effect order, not a claim of flight success.
 {Creature actor;Unit pet;actor.HandleSpellClick(&pet);assert(pet.casts==1);}
 assert(!errors);
 std::cout<<"PASS native full HandleSpellClick/SmartAI dispatch and real personal90/taxi123 effects: explicit old rejected-condition/failed-cast/group/dangling-link counterfactuals; repaired primary/missing AI/requirements/conditions/cast rejection/deferred actual hit/personal-only credit. Native taxi rejection still follows native effect90; no successful flight/client claim. Cast completion/taxi/conditions/SAI row availability are adapters.\n";
}
