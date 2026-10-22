#include <cassert>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <set>
#include <vector>
using uint32=uint32_t;using uint8=uint8_t;
enum {HUNTER_EXPERT_TRAPPER=199543,HUNTER_SUPER_STICKY_TAR=201158,IN_MILLISECONDS=1000,EFFECT_0=0,SPELL_AURA_MOD_DECREASE_SPEED=33};
int errors=0,rolls=0;bool successfulRoll=true;
bool roll_chance_i(int percent){assert(percent==20);++rolls;return successfulRoll;}
struct Unit {std::set<int> auras;int casts=0;Unit* lastTarget=nullptr;
 bool HasAura(int id){return auras.count(id)!=0;}
 void CastSpell(Unit* target,int id,bool triggered){assert(id==HUNTER_SUPER_STICKY_TAR&&triggered);lastTarget=target;++casts;target->auras.insert(id);}};
struct SpellInfo{};
struct AuraEffect {int index=0;int GetEffIndex(){return index;}};
#include "Hooks.inc"
#define TC_LOG_ERROR(...) (++errors)
struct AuraScript;
struct Aura {Unit* owner=nullptr;Unit* caster=nullptr;bool removed=false;SpellInfo info;SpellInfo* m_spellInfo=&info;std::vector<AuraScript*> m_loadedScripts;
 Unit* GetUnitOwner(){return owner;}bool IsRemoved(){return removed;}void CallScriptEffectUpdateHandlers(uint32,AuraEffect*);};
struct Application {Unit* target;Unit* GetTarget(){return target;}};
struct Handler {int index;std::function<void(AuraScript*,uint32,AuraEffect*)> callback;
 bool IsEffectAffected(SpellInfo*,int effect){return effect==index;}void Call(AuraScript* script,uint32 diff,AuraEffect* effect){callback(script,diff,effect);}};
struct Hook:std::vector<Handler>{void operator+=(Handler h){push_back(h);}};
template<class T> Handler MakeHandler(void(T::*fn)(uint32,AuraEffect*),int index,int aura){assert(aura==SPELL_AURA_MOD_DECREASE_SPEED);return {index,[fn](AuraScript* s,uint32 d,AuraEffect* e){(static_cast<T*>(s)->*fn)(d,e);}};}
struct AuraScript {
 Aura* m_aura=nullptr;Application* m_auraApplication=nullptr;int m_currentScriptState=SPELL_SCRIPT_STATE_NONE;Hook OnEffectUpdate;
 virtual bool Validate(SpellInfo const*){return true;}virtual void Register(){}
 bool ValidateSpellInfo(std::initializer_list<int>){return true;}
 Unit* GetCaster(){return m_aura->caster;}Unit* GetTarget()const;Unit* GetUnitOwner()const;
 void _PrepareScriptCall(int state){m_currentScriptState=state;m_auraApplication=nullptr;}void _FinishScriptCall(){m_currentScriptState=SPELL_SCRIPT_STATE_NONE;}
};
#include "Target.inc"
#include "Owner.inc"
#include "Dispatch.inc"
#define PrepareAuraScript(name) public:
#define AuraEffectUpdateFn(fn,index,aura) MakeHandler(&fn,index,aura)
#include "Trap.inc"
int main(){
 Unit hunter,target;hunter.auras.insert(HUNTER_EXPERT_TRAPPER);Aura aura;aura.owner=&target;aura.caster=&hunter;
 spell_hun_tar_trap_aura script;script.m_aura=&aura;script.Register();aura.m_loadedScripts={&script};AuraEffect effect;
 aura.CallScriptEffectUpdateHandlers(999,&effect);assert(hunter.casts==0&&rolls==0&&errors==0);
 aura.CallScriptEffectUpdateHandlers(1,&effect);
 if(errors||hunter.casts!=1){std::cerr<<"FAIL: actual EFFECT_UPDATE must obtain the unit owner and apply sticky tar; errors="<<errors<<" casts="<<hunter.casts<<'\n';return 1;}
 assert(hunter.lastTarget==&target&&rolls==1&&script.m_currentScriptState==SPELL_SCRIPT_STATE_NONE);
 for(int tick=0;tick<1000;++tick)aura.CallScriptEffectUpdateHandlers(50,&effect);
 assert(hunter.casts==1&&rolls==1&&errors==0); // Never stack over existing sticky tar.
 target.auras.clear();hunter.auras.clear();aura.CallScriptEffectUpdateHandlers(1000,&effect);assert(hunter.casts==1&&rolls==1);
 hunter.auras.insert(HUNTER_EXPERT_TRAPPER);successfulRoll=false;aura.CallScriptEffectUpdateHandlers(1000,&effect);assert(hunter.casts==1&&rolls==2);
 successfulRoll=true;aura.CallScriptEffectUpdateHandlers(3500,&effect);assert(hunter.casts==2&&rolls==3);target.auras.clear();
 aura.CallScriptEffectUpdateHandlers(499,&effect);assert(hunter.casts==2);aura.CallScriptEffectUpdateHandlers(1,&effect);assert(hunter.casts==3);
 target.auras.clear();aura.caster=nullptr;aura.CallScriptEffectUpdateHandlers(1000,&effect);assert(hunter.casts==3);
 aura.caster=&hunter;aura.owner=nullptr;aura.CallScriptEffectUpdateHandlers(1000,&effect);assert(hunter.casts==3&&errors==0);
 aura.owner=&target;effect.index=1;aura.CallScriptEffectUpdateHandlers(1000,&effect);assert(hunter.casts==3);effect.index=0;
 aura.removed=true;aura.CallScriptEffectUpdateHandlers(1000,&effect);assert(hunter.casts==3&&errors==0);
 std::cout<<"PASS: real Tar Trap EFFECT_UPDATE registration, native aura dispatcher/getters, owner target, one-second timer/remainder, talent/chance/existing-aura gates, missing owner/caster, wrong effect and removed aura\n";
}
