#include <cassert>
#include <cstdint>
#include <string>
#include <iostream>
#include <functional>
#include <vector>
using uint32=uint32_t;
using uint8=uint8_t;
struct Unit {int entry=106693,map=1498;bool alive=true,combat=false;void* vehicle=nullptr;
 int GetEntry(){return entry;}int GetMapId(){return map;}bool IsAlive(){return alive;}
 bool isInCombat(){return combat;}void* GetVehicle(){return vehicle;}};
#include "AuraHooks.inc"
int errors=0;
#define TC_LOG_ERROR(...) (++errors)
struct AuraScript;
struct Aura {Unit* owner=nullptr;Unit* caster=nullptr;std::vector<AuraScript*> m_loadedScripts;
 Unit* GetUnitOwner(){return owner;}Unit* GetCaster(){return caster;}void CallScriptAuraUpdateHandlers(uint32 diff);};
struct Application {Unit* target=nullptr;Unit* GetTarget(){return target;}};
struct Handler{std::function<void(AuraScript*,uint32)> f;void Call(AuraScript* script,uint32 diff){f(script,diff);}};
struct Hook:std::vector<Handler> {void operator+=(Handler h){push_back(h);}};
template<class T> Handler MakeAuraHandler(void(T::*fn)(uint32)){return {[fn](AuraScript* s,uint32 d){(static_cast<T*>(s)->*fn)(d);}};}
struct AuraScript {
 Aura* m_aura=nullptr;Application* m_auraApplication=nullptr;int m_currentScriptState=SPELL_SCRIPT_STATE_NONE;
 bool removed=false;Hook OnAuraUpdate;virtual void Register(){}Unit* GetCaster(){return m_aura->GetCaster();}
 void _PrepareScriptCall(int state){m_currentScriptState=state;m_auraApplication=nullptr;}
 void _FinishScriptCall(){m_currentScriptState=SPELL_SCRIPT_STATE_NONE;}
 void Remove(){removed=true;}Unit* GetTarget() const;Unit* GetUnitOwner() const;
};
#include "Target.inc"
#include "Owner.inc"
#include "AuraUpdate.inc"
namespace DarkWhispersIntro {
#include "StopBeam.inc"
}
#define PrepareAuraScript(name) public:
#define AuraUpdateFn(fn) MakeAuraHandler(&fn)
#include "Beam.inc"
int main(){
 Unit skull,casters[3];Aura auras[3];aura_dark_whispers_ritual_beam scripts[3];
 for(int i=0;i<3;++i){casters[i].entry=106698;auras[i].owner=&skull;auras[i].caster=&casters[i];scripts[i].m_aura=&auras[i];scripts[i].Register();auras[i].m_loadedScripts={&scripts[i]};}
 // The real UPDATE hook has no target application. Old GetTarget logged an
 // error and returned null on every tick; helper-only tests missed this.
 AuraScript old;old.m_aura=&auras[0];old._PrepareScriptCall(AURA_SCRIPT_HOOK_ON_UPDATE);assert(!old.GetTarget()&&errors==1);old._FinishScriptCall();errors=0;
 for(int tick=0;tick<10000;++tick)for(auto& aura:auras)aura.CallScriptAuraUpdateHandlers(50);
 assert(errors==0);for(auto& script:scripts)assert(!script.removed);
 casters[0].combat=true;for(auto& aura:auras)aura.CallScriptAuraUpdateHandlers(50);
 assert(scripts[0].removed&&!scripts[1].removed&&!scripts[2].removed&&errors==0);
 casters[1].alive=false;scripts[1].Update(50);assert(scripts[1].removed);
 auras[2].caster=nullptr;scripts[2].Update(50);assert(scripts[2].removed&&errors==0);
 Unit other;other.entry=1;Aura foreign{&other,nullptr};aura_dark_whispers_ritual_beam script;script.m_aura=&foreign;script.Update(50);assert(!script.removed);
 std::cout<<"PASS: actual production AuraScript UPDATE callback and native GetTarget/GetUnitOwner getters; no application, 30000 updates, per-caster removal and no hook errors\n";
}
