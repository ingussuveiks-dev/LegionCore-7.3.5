#include <cstdint>
#include <functional>
#include <iostream>
#include <mutex>
#include <stack>
#include <stdexcept>
#include <string>
#include <vector>
using uint8=uint8_t;
constexpr uint8 SPELL_SCRIPT_STATE_END=4;
#include "AuraHooks.inc"
#define ASSERT(x) do { if (!(x)) throw std::runtime_error(#x); } while (false)
#define TC_LOG_ERROR(...) throw std::runtime_error("PreventDefaultAction outside an effect hook")
constexpr int EFFECT_0=0, SPELL_AURA_DUMMY=4, PROC_HIT_CRITICAL=2;
struct SpellInfo { unsigned Id; } immolate{157736}, unrelated{123};
struct DamageInfo { SpellInfo const* spell; SpellInfo const* GetSpellInfo(){return spell;} };
struct ProcEventInfo {
    DamageInfo* damage=nullptr; int hit=0;
    DamageInfo* GetDamageInfo(){return damage;} int GetHitMask(){return hit;}
};
struct AuraApplication {};
struct AuraEffect { uint8 index=0; uint8 GetEffIndex() const{return index;} };
struct Unit { int fragments=0; std::function<void()> onCast; void CastSpell(Unit*,unsigned id,bool triggered) {
    ASSERT(id==193540 && triggered); ++fragments; if(onCast)onCast();
} } caster;
bool rollSuccess=true;
bool roll_chance_i(int chance){ASSERT(chance==50);return rollSuccess;}
class AuraScript;
struct CheckHandler {
    std::function<bool(AuraScript*,ProcEventInfo&)> fn;
    bool Call(AuraScript* s,ProcEventInfo& e){return fn(s,e);}
};
struct AreaHandler {
    std::function<bool(AuraScript*,Unit*)> fn;
    bool Call(AuraScript* s,Unit* u){return fn(s,u);}
};
struct EffectHandler {
    std::function<void(AuraScript*,AuraEffect const*,ProcEventInfo&)> fn;
    bool IsEffectAffected(SpellInfo const*,uint8 index){return index==EFFECT_0;}
    void Call(AuraScript* s,AuraEffect const* a,ProcEventInfo& e){fn(s,a,e);}
};
template<class T> CheckHandler MakeCheck(bool(T::*f)(ProcEventInfo&)) {
    return {[f](AuraScript* s,ProcEventInfo& e){return (static_cast<T*>(s)->*f)(e);}};
}
template<class T> EffectHandler MakeEffect(void(T::*f)(AuraEffect const*,ProcEventInfo&),int,int) {
    return {[f](AuraScript* s,AuraEffect const* a,ProcEventInfo& e){(static_cast<T*>(s)->*f)(a,e);}};
}
template<class T> struct Hooks:std::vector<T> { void operator+=(T h){this->push_back(h);} };
class AuraScript {
public:
    uint8 m_currentScriptState=0; AuraApplication const* m_auraApplication=nullptr; bool m_defaultActionPrevented=false;
    #include "AuraState.inc"
    std::mutex i_scriptStatesLock;
    Hooks<CheckHandler> DoCheckProc; Hooks<AreaHandler> DoCheckAreaTarget; Hooks<EffectHandler> OnEffectProc;
    void _PrepareScriptCall(AuraScriptHookType,AuraApplication const* =nullptr);
    void _FinishScriptCall(); bool _IsDefaultActionPrevented(); void PreventDefaultAction();
    Unit* GetTarget(){return &caster;} virtual void Register(){}
};
#include "AuraLifecycle.inc"
class Aura {
public:
    bool removed=false; std::vector<AuraScript*> m_loadedScripts; SpellInfo const* m_spellInfo=&immolate;
    bool IsRemoved() const{return removed;}
    bool CallScriptCheckAreaTargetHandlers(Unit*);
    bool CallScriptCheckProcHandlers(AuraApplication const*,ProcEventInfo&);
    bool CallScriptEffectProcHandlers(AuraEffect const*,AuraApplication const*,ProcEventInfo&);
};
#include "AuraDispatch.inc"
#define PrepareAuraScript(T) public:
#define AuraCheckProcFn(F) MakeCheck(&F)
#define AuraEffectProcFn(F,I,A) MakeEffect(&F,I,A)
#include "ImmolateProc.inc"
void clean(AuraScript const& s){ASSERT(s.m_currentScriptState==0 && !s.m_auraApplication && !s.m_defaultActionPrevented && s.m_scriptStates.empty());}
int main() try {
    spell_warl_immolate_proc script; script.Register(); Aura aura; aura.m_loadedScripts={&script};
    AuraApplication outer, nested; AuraEffect effect; DamageInfo hit{&immolate}, other{&unrelated};
    ProcEventInfo critical{&hit,PROC_HIT_CRITICAL}, ordinary{&hit,0}, wrong{&other,PROC_HIT_CRITICAL}, energize;
    bool nestedRestored=false;
    caster.onCast=[&]{
        ASSERT(script.m_currentScriptState==AURA_SCRIPT_HOOK_EFFECT_PROC && script.m_auraApplication==&outer);
        ASSERT(!aura.CallScriptCheckProcHandlers(&nested,energize));
        nestedRestored=script.m_currentScriptState==AURA_SCRIPT_HOOK_EFFECT_PROC && script.m_auraApplication==&outer;
    };
    // Production Immolate handler casts 193540, which re-enters its own rejected
    // proc check. The outer dispatcher must still see PreventDefaultAction.
    for(int i=0;i<100;++i){
        ASSERT(aura.CallScriptCheckProcHandlers(&outer,critical));clean(script);
        ASSERT(aura.CallScriptEffectProcHandlers(&effect,&outer,critical));ASSERT(nestedRestored);clean(script);
    }
    ASSERT(caster.fragments==100);
    // Ordinary/failed checks must not accumulate state across successive ticks.
    for(int i=0;i<1000;++i){ASSERT(!aura.CallScriptCheckProcHandlers(&outer,ordinary));clean(script);}
    ASSERT(!aura.CallScriptCheckProcHandlers(&outer,wrong));clean(script);
    rollSuccess=false;ASSERT(!aura.CallScriptCheckProcHandlers(&outer,critical));clean(script);rollSuccess=true;
    AuraScript a,b; Aura multi; multi.m_loadedScripts={&a,&b}; int later=0;
    a.DoCheckProc += CheckHandler{[](AuraScript*,ProcEventInfo&){return true;}};
    b.DoCheckProc += CheckHandler{[](AuraScript*,ProcEventInfo&){return false;}};
    b.DoCheckProc += CheckHandler{[&](AuraScript*,ProcEventInfo&){++later;return true;}};
    ASSERT(!multi.CallScriptCheckProcHandlers(&outer,critical));ASSERT(later==0);clean(a);clean(b);
    b.DoCheckProc.clear();ASSERT(multi.CallScriptCheckProcHandlers(&outer,critical));clean(a);clean(b);
    // The other rejecting dispatcher shares the same lifecycle obligation.
    b.DoCheckAreaTarget += AreaHandler{[](AuraScript*,Unit*){return false;}};
    ASSERT(!multi.CallScriptCheckAreaTargetHandlers(&caster));clean(a);clean(b);
    b._PrepareScriptCall(AURA_SCRIPT_HOOK_EFFECT_PERIODIC,&outer);b.PreventDefaultAction();
    ASSERT(!multi.CallScriptCheckAreaTargetHandlers(&caster));
    ASSERT(b.m_currentScriptState==AURA_SCRIPT_HOOK_EFFECT_PERIODIC && b.m_auraApplication==&outer && b._IsDefaultActionPrevented());
    b._FinishScriptCall();clean(b);
    multi.removed=true;ASSERT(!multi.CallScriptCheckAreaTargetHandlers(&caster));
    ASSERT(!multi.CallScriptEffectProcHandlers(&effect,&outer,critical));clean(a);clean(b);
    std::cout<<"PASS: production aura lifecycle, rejecting proc/area dispatchers, nested Immolate energize, exact fragment counts, short-circuit and repeated calls\n";
} catch(std::exception const& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
