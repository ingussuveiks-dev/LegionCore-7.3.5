#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <set>
#include <stack>
#include <vector>
using uint8=uint8_t;using uint32=uint32_t;
enum{EFFECT_0=0,SPELL_AURA_DUMMY=4};
#include "Hooks.inc"
#include "RemoveEnums.inc"
struct Aura;struct Player;struct Creature;struct Pet;
struct Unit{using ControlList=std::set<uint32>;ControlList m_Controlled;bool alive=true;Aura* bond=nullptr;std::vector<uint32> casts;virtual ~Unit()=default;virtual Player* ToPlayer(){return nullptr;}bool IsAlive(){return alive;}Aura* GetAura(uint32 id){assert(id==197344);return bond;}void CastSpell(Unit* target,uint32 id,bool triggered){assert(target==this&&triggered);casts.push_back(id);}};
struct Creature:Unit{uint32 entry=100324,despawns=0,delay=0;uint32 GetEntry(){return entry;}void DespawnOrUnsummon(uint32 value){++despawns;delay=value;}};
struct Pet:Creature{};
struct ObjectGuid{bool valid,pet;bool IsEmpty()const{return !valid;}bool IsPet()const{return pet;}};
struct Player:Unit{Pet* selected=nullptr;bool inWorld=true,petKind=true,lookup=true;Player* ToPlayer()override{return this;}ObjectGuid GetPetGUID()const{return {selected!=nullptr,petKind};}bool IsInWorld()const{return inWorld;}Pet* GetPet()const;};
namespace ObjectAccessor{std::map<uint32,Creature*> actors;Creature* GetCreature(Unit&,uint32 id){auto found=actors.find(id);return found==actors.end()?nullptr:found->second;}Pet* GetPet(Player const& p,ObjectGuid){return p.lookup?p.selected:nullptr;}}
#include "PlayerPet.inc"
struct SpellInfo{};struct AuraEffect{uint32 index=0;uint32 GetEffIndex()const{return index;}};
struct AuraApplication{AuraRemoveMode mode=AURA_REMOVE_BY_EXPIRE;AuraRemoveMode GetRemoveMode()const{return mode;}};
struct AuraScript;
struct Aura{Unit* caster=nullptr;uint32 custom=197388;SpellInfo info;SpellInfo* m_spellInfo=&info;std::vector<AuraScript*> m_loadedScripts;Unit* GetCaster(){return caster;}uint32 GetCustomData(){return custom;}void CallScriptAfterEffectRemoveHandlers(AuraEffect const*,AuraApplication const*,AuraEffectHandleModes);};
struct AuraScript{
 Aura* m_aura=nullptr;AuraApplication const* m_auraApplication=nullptr;uint8 m_currentScriptState=SPELL_SCRIPT_STATE_NONE;bool m_defaultActionPrevented=false;
 #include "StateStore.inc"
 std::stack<ScriptStateStore> m_scriptStates;std::mutex i_scriptStatesLock;
 using AuraEffectApplicationModeFnType=void(AuraScript::*)(AuraEffect const*,AuraEffectHandleModes);
 struct EffectApplyHandler{AuraEffectApplicationModeFnType pEffectHandlerScript;uint32 index;AuraEffectHandleModes mode;
  bool IsEffectAffected(SpellInfo*,uint32 value){return index==value;}void Call(AuraScript*,AuraEffect const*,AuraEffectHandleModes);};
 struct Hook:std::vector<EffectApplyHandler>{void operator+=(EffectApplyHandler value){push_back(value);}}AfterEffectRemove;
 virtual ~AuraScript()=default;virtual void Register(){}virtual bool Load(){return true;}bool _Load(Aura*);Unit* GetCaster()const;AuraApplication const* GetTargetApplication()const;
 void _PrepareScriptCall(AuraScriptHookType,AuraApplication const*);void _FinishScriptCall();
};
template<class T>AuraScript::EffectApplyHandler Handler(void(T::*fn)(AuraEffect const*,AuraEffectHandleModes),uint32 index,uint32 type,AuraEffectHandleModes mode){assert(index==0&&type==4&&mode==AURA_EFFECT_HANDLE_REAL);return {static_cast<AuraScript::AuraEffectApplicationModeFnType>(fn),index,mode};}
#define AuraEffectRemoveFn(fn,index,type,mode) Handler(&fn,index,type,mode)
#include "Caster.inc"
#include "Load.inc"
#include "RemoveNative.inc"
#include "RemoveDispatch.inc"
#include "BrokenFixed.inc"
#include "BrokenBaseline.inc"
template<class Script>struct Run{
 Script script;Aura aura,bond;AuraEffect effect;AuraApplication app;Player player;Pet pet;Creature hati,other;Unit npc;
 Run(){ObjectAccessor::actors.clear();pet.entry=1;player.selected=&pet;player.bond=&bond;player.m_Controlled={1,2,3};other.entry=777;hati.alive=false;ObjectAccessor::actors={{2,&hati},{3,&other}};aura.caster=&player;assert(script._Load(&aura));script.Register();assert(script.AfterEffectRemove.size()==1);aura.m_loadedScripts={&script};}
 void Remove(AuraEffectHandleModes mode=AURA_EFFECT_HANDLE_REAL){aura.CallScriptAfterEffectRemoveHandlers(&effect,&app,mode);assert(script.m_scriptStates.empty()&&script.m_currentScriptState==SPELL_SCRIPT_STATE_NONE&&!script.m_auraApplication);}
};
int main(){uint32 cases=0,broken=0;
 for(uint32 entry:{100324u,106548u,106549u,106550u,106551u})for(uint32 summon:{197388u,211145u,211146u,211147u,211148u}){
  {Run<BrokenBaseline> r;r.hati.entry=entry;r.bond.custom=summon;r.player.selected=nullptr;r.player.m_Controlled.erase(1);r.Remove();assert(r.player.casts==std::vector<uint32>{summon}&&r.hati.despawns==1);++broken;}
  for(uint32 primary=0;primary<3;++primary){Run<BrokenFixed> r;r.hati.entry=entry;r.bond.custom=summon;
   if(!primary){r.player.selected=nullptr;r.player.m_Controlled.erase(1);}if(primary==2)r.pet.alive=false;
   r.Remove();assert(r.hati.despawns==1&&r.hati.delay==1000&&!r.other.despawns);assert(r.player.casts.size()==(primary?1u:0u));if(primary)assert(r.player.casts[0]==summon);++cases;}
 }
 for(uint32 mode=0;mode<=8;++mode){if(mode==AURA_REMOVE_BY_EXPIRE)continue;Run<BrokenFixed> r;r.app.mode=AuraRemoveMode(mode);r.Remove();assert(r.player.casts.empty()&&!r.hati.despawns);++cases;}
 for(uint32 gate=0;gate<10;++gate){Run<BrokenFixed> r;switch(gate){case 0:r.aura.caster=nullptr;break;case 1:r.player.alive=false;break;case 2:r.hati.alive=true;break;case 3:r.player.bond=nullptr;break;case 4:r.player.petKind=false;break;case 5:r.player.lookup=false;break;case 6:r.player.inWorld=false;break;case 7:r.effect.index=1;break;case 8:r.player.selected=nullptr;break;case 9:r.aura.caster=&r.npc;r.npc.bond=&r.bond;break;}r.Remove();assert(r.player.casts.empty()&&r.npc.casts.empty()&&!r.other.despawns);++cases;}
 for(bool noPet:{false,true}){Run<BrokenFixed> r;r.player.m_Controlled.erase(2);ObjectAccessor::actors.erase(2);if(noPet)r.player.selected=nullptr;r.Remove();assert(r.player.casts.size()==(noPet?0u:1u));++cases;}
 for(bool aliveFirst:{false,true}){Run<BrokenFixed> r;r.other.entry=106548;if(aliveFirst){ObjectAccessor::actors[2]=&r.other;ObjectAccessor::actors[3]=&r.hati;}r.Remove();assert(r.player.casts.empty()&&!r.other.despawns&&r.hati.despawns==(aliveFirst?0u:1u));++cases;}
 {Run<BrokenFixed> r;r.other.entry=106548;r.other.alive=false;r.Remove();assert(r.hati.despawns==1&&r.other.despawns==1&&r.player.casts.size()==1);++cases;}
 {Run<BrokenFixed> r;r.player.m_Controlled.insert(99);r.Remove();assert(r.hati.despawns==1&&r.player.casts.size()==1&&!r.other.despawns);++cases;}
 {Run<BrokenFixed> r;r.Remove(AURA_EFFECT_HANDLE_CHANGE_AMOUNT);assert(r.player.casts.empty()&&!r.hati.despawns);++cases;}
 // Primary recall after a refused expiry uses the retained ordinary Bond poll.
 // Its current full native suite is run by the PowerShell parent; no direct grant here.
 assert(broken==25);
 std::cout<<"PASS "<<cases<<" Broken Bond removal contexts; pinnedfbaa14ff8 incorrectly summons25 no-primary controls. Actual full callback/Register/native after-remove dispatcher/mode handler/state stack/getters/Player::GetPet; corpse cleanup and selected-dead-primary policy retained. Services controlled; no client death/revive claim.\n";
}
