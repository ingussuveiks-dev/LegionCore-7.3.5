#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <vector>
using uint8=uint8_t;using uint32=uint32_t;
#include "ReactEnums.inc"
enum{EFFECT_0=0,SPELL_AURA_DUMMY=4,AURA_EFFECT_HANDLE_REAL=1};
struct Player;struct Pet;struct Creature;
struct Unit{
 using ControlList=std::set<uint32>;ControlList m_Controlled;
 bool alive=true;Unit* victim=nullptr;std::vector<uint32> casts;
 virtual ~Unit()=default;virtual Player* ToPlayer(){return nullptr;}
 bool IsAlive()const{return alive;}Unit* getVictim(){return victim;}
 void CastSpell(Unit* target,uint32 id,bool triggered){assert(target==this&&triggered);casts.push_back(id);}
 float GetDistance(Creature*)const;
};
struct CharmInfo{bool returning=false,commandAttack=false;CommandStates command=COMMAND_FOLLOW;bool IsReturning(){return returning;}bool IsCommandAttack(){return commandAttack;}bool HasCommandState(CommandStates value){return command==value;}};
struct Creature:Unit{
 uint32 entry=100324,despawns=0,despawnDelay=0;float distance=0;ReactStates m_reactState=REACT_HELPER;
 bool combat=false,hasCharm=true,meleeRange=false;CharmInfo charm;Unit* owner=nullptr;
 std::vector<std::function<void()>> delayed;
 uint32 GetEntry(){return entry;}bool isInCombat(){return combat;}
 void AddDelayedCombat(uint32 delay,std::function<void()> callback){assert(delay);delayed.push_back(std::move(callback));}
 void SetReactState(ReactStates,uint32=0);ReactStates GetReactState();bool HasReactState(ReactStates value){return m_reactState==value;}
 void DespawnOrUnsummon(uint32 delay){++despawns;despawnDelay=delay;}
 CharmInfo* GetCharmInfo(){return hasCharm?&charm:nullptr;}Unit* GetCharmerOrOwner(){return owner;}
 bool IsWithinMeleeRange(Unit*,float){return meleeRange;}float GetAttackDist(){return 5;}
};
struct Pet:Creature{};
struct Player:Unit{Pet* pet=nullptr;Player* ToPlayer()override{return this;}
#ifdef HATI_OWNER_TEST
 struct ObjectGuid{bool valid;bool IsEmpty()const{return !valid;}bool IsPet()const{return valid;}};
 ObjectGuid GetPetGUID()const{return {pet!=nullptr};}bool IsInWorld()const{return true;}Pet* GetPet()const;
#else
 Pet* GetPet(){return pet;}
#endif
};
float Unit::GetDistance(Creature* actor)const{return actor->distance;}
namespace ObjectAccessor{std::map<uint32,Creature*> actors;Creature* GetCreatureOrPetOrVehicle(Unit&,uint32 id){auto found=actors.find(id);return found==actors.end()?nullptr:found->second;}}
#ifdef HATI_OWNER_TEST
using ObjectGuid=Player::ObjectGuid;
namespace ObjectAccessor{Pet* GetPet(Player const& player,ObjectGuid){return player.pet;}}
#include "PlayerPet.inc"
#endif
#include "React.inc"
struct PetAI{Creature* me;uint32 attacks=0;explicit PetAI(Creature* actor):me(actor){}void AttackStart(Unit*){++attacks;}void OwnerAttacked(Unit*);void OwnerDamagedBy(Unit*);bool CanAttack(Unit*);};
#include "Consumers.inc"
struct SpellInfo{};struct AuraEffect{uint32 index=0;uint32 GetEffIndex(){return index;}};
#include "Hooks.inc"
struct AuraScript;
struct Handler{uint32 index;std::function<void(AuraScript*,uint32,AuraEffect*)> callback;bool IsEffectAffected(SpellInfo*,uint32 value){return index==value;}void Call(AuraScript* script,uint32 diff,AuraEffect* effect){callback(script,diff,effect);}};
struct Hook:std::vector<Handler>{void operator+=(Handler value){push_back(std::move(value));}};
struct IgnoredHook{void operator+=(int){}};
struct Aura{Unit* caster=nullptr;bool removed=false;uint32 custom=211147;SpellInfo info;SpellInfo* m_spellInfo=&info;std::vector<AuraScript*> m_loadedScripts;Unit* GetCaster(){return caster;}uint32 GetCustomData(){return custom;}bool IsRemoved(){return removed;}void CallScriptEffectUpdateHandlers(uint32,AuraEffect*);};
struct AuraScript{Aura* m_aura=nullptr;uint32 state=SPELL_SCRIPT_STATE_NONE;Hook OnEffectUpdate;IgnoredHook AfterEffectApply,AfterEffectRemove;virtual ~AuraScript()=default;virtual void Register(){}virtual bool Load(){return true;}bool _Load(Aura*);Unit* GetCaster()const;Aura* GetAura(){return m_aura;}void _PrepareScriptCall(uint32 value,void* =nullptr){assert(state==SPELL_SCRIPT_STATE_NONE);state=value;}void _FinishScriptCall(){state=SPELL_SCRIPT_STATE_NONE;}};
template<class T>Handler MakeHandler(void(T::*fn)(uint32,AuraEffect*),uint32 index,uint32 type){assert(index==0&&type==4);return {index,[fn](AuraScript* script,uint32 diff,AuraEffect* effect){(static_cast<T*>(script)->*fn)(diff,effect);}};}
#define AuraEffectUpdateFn(fn,index,type) MakeHandler(&fn,index,type)
#define AuraEffectApplyFn(...) 0
#define AuraEffectRemoveFn(...) 0
#include "Caster.inc"
#include "Dispatch.inc"
#include "Bond.inc"
#include "Baseline.inc"
template<class Script>struct Run{
 Script script;Aura aura;AuraEffect effect;Player player;Pet pet;Creature hati,other;Unit target;
 Run(){ObjectAccessor::actors.clear();pet.entry=1;player.pet=&pet;player.m_Controlled={1,2};ObjectAccessor::actors={{1,&pet},{2,&hati}};hati.owner=&player;script.m_aura=&aura;script.Register();assert(script.OnEffectUpdate.size()==1);aura.caster=&player;aura.m_loadedScripts={&script};}
 void Tick(uint32 diff){aura.CallScriptEffectUpdateHandlers(diff,&effect);assert(script.state==SPELL_SCRIPT_STATE_NONE);}
};
uint32 RunReactionCases(){
 uint32 cases=0,broken=0;uint32 entries[]={100324,106548,106549,106550,106551};
 for(auto entry:entries)for(uint32 before=0;before<5;++before)for(uint32 after=0;after<5;++after){
  {Run<BondBaseline> r;r.hati.entry=entry;r.hati.m_reactState=ReactStates(before);r.pet.m_reactState=ReactStates(after);r.Tick(2000);bool mismatch=r.hati.GetReactState()!=ReactStates(after);assert(mismatch==(after==REACT_PASSIVE&&before!=REACT_PASSIVE));broken+=mismatch;}
  {Run<BondFixed> r;r.hati.entry=entry;r.hati.m_reactState=ReactStates(before);r.pet.m_reactState=ReactStates(after);r.Tick(1999);assert(r.hati.GetReactState()==ReactStates(before));r.Tick(1);assert(r.hati.GetReactState()==ReactStates(after));assert(r.hati.delayed.empty()&&r.player.casts.empty()&&!r.hati.despawns);assert(r.player.pet==&r.pet&&r.player.m_Controlled==Unit::ControlList({1,2}));++cases;}
 }
 assert(broken==20);std::cout<<"COUNTERFACTUAL pinned4ea7b499e actual handler/native dispatcher:20 passive transitions rejected across five Hati entries;105 equal/nonzero controls retained.\n";
 for(auto entry:entries){
  Run<BondFixed> r;r.hati.entry=entry;r.pet.m_reactState=REACT_PASSIVE;r.Tick(2000);PetAI ai(&r.hati);
  ai.OwnerAttacked(&r.target);ai.OwnerDamagedBy(&r.target);assert(ai.attacks==0&&!ai.CanAttack(&r.target));
  r.hati.charm.commandAttack=true;assert(ai.CanAttack(&r.target));r.hati.charm.commandAttack=false;r.hati.charm.command=COMMAND_STAY;assert(!ai.CanAttack(&r.target));
  r.pet.m_reactState=REACT_HELPER;r.Tick(2000);ai.OwnerAttacked(&r.target);ai.OwnerDamagedBy(&r.target);assert(ai.attacks==2);
  r.hati.charm.command=COMMAND_FOLLOW;assert(ai.CanAttack(&r.target));r.hati.charm.returning=true;assert(!ai.CanAttack(&r.target));++cases;
 }
 for(uint32 gate=0;gate<9;++gate){
  Run<BondFixed> r;r.pet.m_reactState=REACT_PASSIVE;
  switch(gate){case 0:r.aura.caster=nullptr;break;case 1:r.player.alive=false;break;case 2:r.player.pet=nullptr;break;case 3:r.hati.alive=false;break;case 4:ObjectAccessor::actors.erase(2);r.player.m_Controlled.erase(1);break;case 5:r.effect.index=1;break;case 6:r.aura.removed=true;break;case 7:r.player.m_Controlled.erase(2);r.player.m_Controlled.erase(1);break;case 8:r.hati.entry=99;break;}
  r.Tick(2000);assert(r.hati.GetReactState()==REACT_HELPER);++cases;
 }
 {Run<BondFixed> r;r.player.m_Controlled.erase(2);ObjectAccessor::actors.erase(2);r.pet.m_reactState=REACT_PASSIVE;r.Tick(2000);assert(r.player.casts==std::vector<uint32>{211147});r.player.m_Controlled.insert(2);ObjectAccessor::actors[2]=&r.hati;r.Tick(2000);assert(r.hati.GetReactState()==REACT_PASSIVE&&r.player.casts.size()==1);++cases;}
 {Run<BondFixed> r;r.player.m_Controlled.erase(1);r.player.pet=nullptr;r.pet.m_reactState=REACT_PASSIVE;r.Tick(2000);assert(r.hati.despawns==1&&r.hati.despawnDelay==1000);++cases;}
 {Run<BondFixed> r;r.hati.distance=100.01f;r.Tick(2000);assert(r.hati.despawns==1&&r.hati.despawnDelay==1000);++cases;}
 {Run<BondFixed> r;r.hati.distance=100;r.Tick(2000);assert(!r.hati.despawns);r.pet.m_reactState=REACT_PASSIVE;for(uint32 i=0;i<100;++i)r.Tick(2000);assert(r.hati.GetReactState()==REACT_PASSIVE&&r.player.casts.empty()&&!r.hati.despawns);++cases;}
 {Run<BondFixed> r;r.pet.m_reactState=REACT_PASSIVE;r.Tick(4500);assert(r.hati.GetReactState()==REACT_PASSIVE&&r.script.update==0);r.pet.m_reactState=REACT_DEFENSIVE;r.Tick(1999);assert(r.hati.GetReactState()==REACT_PASSIVE);r.Tick(1);assert(r.hati.GetReactState()==REACT_DEFENSIVE);++cases;}
 {Run<BondFixed> r;r.pet.m_reactState=REACT_PASSIVE;r.other.entry=777;r.other.m_reactState=REACT_AGGRESSIVE;r.player.m_Controlled.insert(3);ObjectAccessor::actors[3]=&r.other;r.Tick(2000);assert(r.hati.GetReactState()==REACT_PASSIVE&&r.other.GetReactState()==REACT_AGGRESSIVE&&!r.other.despawns);++cases;}
 std::cout<<"PASS "<<cases<<" Hati reaction contexts: full actual2s OnUpdate/registration/native aura dispatcher, native zero enum/get/set and PetAI passive consumers, five entries/all25 transitions, explicit attack still permitted, death/missing owner/pet/actor/removed aura/effect gates, unchanged retry/custom summon/despawn/range/timer and primary ownership. Services controlled; actual client combat/stay/portrait/relog/full Titanstrike remain separate.\n";
 return cases;
}
#ifndef HATI_OWNER_TEST
int main(){RunReactionCases();}
#endif
