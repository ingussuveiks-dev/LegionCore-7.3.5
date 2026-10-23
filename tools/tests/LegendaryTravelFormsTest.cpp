// Execute the extracted production controllers AND full shapeshift handler.
// The fixture supplies engine state, not a second copy of the repair logic.
#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <set>
using uint8=uint8_t; using uint32=uint32_t;
#include "Forms.inc"
enum Powers {POWER_MANA=0, POWER_RAGE=1, POWER_FOCUS=2, POWER_ENERGY=3};
enum AuraEffectHandleModes {AURA_EFFECT_HANDLE_REAL=1};
constexpr int SPELL_AURA_CLONE_CASTER=1, SPELL_AURA_MOD_SHAPESHIFT=36, SPELL_AURA_PERIODIC_DUMMY=226;
constexpr int MECHANIC_SNARE=1, MECHANIC_ROOT=2, CLASS_DRUID=11, BASE_ATTACK=0, INVENTORY_SLOT_BAG_0=0, EQUIPMENT_SLOT_MAINHAND=15, EFFECT_0=0;
struct ObjectGuid {static int Empty;}; int ObjectGuid::Empty=0;
struct Item {};
struct Aura {unsigned stacks=5; unsigned GetStackAmount()const{return stacks;}};
struct AuraEffect;
struct Unit {
 std::map<uint32,Aura> auras; ShapeshiftForm form=FORM_NONE;
 Powers power=POWER_FOCUS; bool combat=false,alive=true,player=true;
 unsigned casts=0, removals=0, model=1234, restores=0, inits=0, boosts=0;
 Aura* GetAura(uint32 id){auto i=auras.find(id);return i==auras.end()?nullptr:&i->second;}
 bool HasAura(uint32 id){return GetAura(id)!=nullptr;}
 bool isInCombat()const{return combat;} bool IsAlive()const{return alive;}
 ShapeshiftForm GetShapeshiftForm()const{return form;}
 uint32 GetModelForForm(ShapeshiftForm f){return f==FORM_NONE?0:(f==FORM_WISP_FORM||f==FORM_WISP_FORM_2?10045:38633);}
 void SetShapeshiftForm(ShapeshiftForm f){form=f;}
 void CastSpell(Unit* target,uint32 id,bool){assert(target==this);++casts;auras[id]={};form=id==242070?FORM_SPIRIT_OWL_FORM_2:FORM_WISP_FORM_2;model=GetModelForForm(form);}
 void RemoveAurasDueToSpell(uint32 id);
 void RemoveAurasByType(int type,int=0,void const* =nullptr){if(type==SPELL_AURA_MOD_SHAPESHIFT){auras.erase(242070);auras.erase(241849);}}
 bool HasAuraType(int type){return type==SPELL_AURA_MOD_SHAPESHIFT&&(HasAura(242070)||HasAura(241849));}
 void RemoveAurasWithMechanic(uint32){} bool IsPolymorphed(){return false;} uint32 getTransForm(){return 0;}
 void SetDisplayId(uint32 id){model=id;} Powers GetPowerType(){return power;}
 void SetFieldPowerType(Powers p){power=p;} void SetPowerType(Powers p){power=p;}
 int getClass(){return 3;} void RemoveMovementImpairingEffects(){}
 void RestoreDisplayId(){++restores;model=form==FORM_NONE?1234:GetModelForForm(form);}
 void AddSpellCooldown(uint32,int,double){} void SendSpellCooldown(uint32,int,int){}
 bool IsPlayer(){return player;} Unit* ToPlayer(){return this;} void InitDataForForm(){++inits;}
 bool CanUseAttackType(int){return true;} Item* GetItemByPos(int,int){return nullptr;}
 void _ApplyWeaponDamage(int,Item*,bool){} void AddTemporarySpell(int){} void RemoveTemporarySpell(int){}
};
double getPreciseTime(){return 0;}
unsigned unknownForms=0;
#define TC_LOG_ERROR(...) (++unknownForms)
struct AuraApplication {Unit* owner;Unit* GetTarget()const{return owner;}int GetRemoveMode()const{return 0;}};
struct SpellShapeshiftFormEntry {int PresetSpellID[8]{};};
struct FormStore {SpellShapeshiftFormEntry entry;SpellShapeshiftFormEntry* LookupEntry(ShapeshiftForm){return &entry;}} sSpellShapeshiftFormStore;
struct AuraEffect {
 ShapeshiftForm effectForm; int GetMiscValue()const{return effectForm;} void const* GetBase()const{return this;}
 void HandleShapeshiftBoosts(Unit* owner,bool)const{++owner->boosts;}
 void HandleAuraModShapeshift(AuraApplication const*,uint8,bool)const;
};
#include "Shapeshift.inc"
void Unit::RemoveAurasDueToSpell(uint32 id)
{
 if(!auras.erase(id))return;
 ++removals;
 if(id==242070||id==241849){AuraEffect effect{form};AuraApplication app{this};effect.HandleAuraModShapeshift(&app,AURA_EFFECT_HANDLE_REAL,false);}
}
struct AuraScript;
struct PeriodicHook {
 std::function<void(AuraScript*)> callback;
 template<class C> void operator+=(void(C::*method)(AuraEffect const*)) {callback=[method](AuraScript* s){(static_cast<C*>(s)->*method)(nullptr);};}
};
struct RemoveHook {
 std::function<void(AuraScript*)> callback;
 template<class C> void operator+=(void(C::*method)(AuraEffect const*,AuraEffectHandleModes)) {callback=[method](AuraScript* s){(static_cast<C*>(s)->*method)(nullptr,AURA_EFFECT_HANDLE_REAL);};}
};
struct AuraScript {Unit* ownerUnit=nullptr;Unit* casterUnit=nullptr;PeriodicHook OnEffectPeriodic;RemoveHook AfterEffectRemove;Unit* GetUnitOwner(){return ownerUnit;}Unit* GetCaster(){return casterUnit;}virtual void Register()=0;};
#define PrepareAuraScript(C) public:
#define AuraEffectPeriodicFn(C,...) &C
#define AuraEffectRemoveFn(C,...) &C
#include "Controllers.inc"

template<class Script> void Lifecycle(bool baseline,uint32 speed,uint32 travel)
{
 Unit owner; Script script;script.ownerUnit=&owner;script.casterUnit=&owner;script.Register();
 assert(script.OnEffectPeriodic.callback);
 auto tick=[&]{script.OnEffectPeriodic.callback(&script);};
 tick();assert(owner.casts==0);
 owner.auras[speed].stacks=4;tick();assert(owner.casts==0);
 owner.auras[speed].stacks=5;owner.combat=true;tick();assert(owner.casts==0);
 owner.combat=false;tick();assert(owner.casts==1&&owner.HasAura(travel));
 tick();assert(owner.casts==1);
 owner.auras.erase(speed);tick();
 if(baseline){assert(owner.HasAura(travel));assert(!script.AfterEffectRemove.callback);return;}
 assert(!owner.HasAura(travel)&&owner.form==FORM_NONE&&owner.model==1234);
 assert(owner.power==POWER_FOCUS&&owner.restores==1);
 tick();assert(owner.removals==1);
 owner.auras[speed].stacks=5;tick();assert(owner.casts==2);
 owner.combat=true;tick();assert(!owner.HasAura(travel)&&owner.HasAura(speed));
 owner.combat=false;tick();assert(owner.casts==3);
 owner.auras[speed].stacks=4;tick();assert(!owner.HasAura(travel));
 owner.form=FORM_CAT;owner.model=9999;owner.auras[speed].stacks=5;tick();assert(owner.casts==3&&owner.model==9999);
 owner.form=FORM_NONE;owner.alive=false;tick();assert(owner.casts==3);
 owner.alive=true;tick();assert(owner.casts==4);
 assert(script.AfterEffectRemove.callback);script.AfterEffectRemove.callback(&script);
 assert(!owner.HasAura(travel)&&owner.form==FORM_NONE&&owner.HasAura(speed));
 script.AfterEffectRemove.callback(&script);assert(owner.removals==4);
 // The periodic callback belongs to the aura owner even if no caster resolves.
 script.casterUnit=nullptr;tick();assert(owner.casts==5);
 owner.alive=false;tick();assert(!owner.HasAura(travel));
 script.ownerUnit=nullptr;tick();script.AfterEffectRemove.callback(&script);
 // Missing own stacks must never cancel the other legendary's form.
 Unit other;other.form=FORM_WISP_FORM_2;other.auras[travel==242070?241849:242070]={};
 script.ownerUnit=&other;tick();assert(other.auras.size()==1);
}
int main(int argc,char**)
{
 bool baseline=argc>1;
 Lifecycle<spell_item_vigilance_perch>(baseline,242066,242070);
 Lifecycle<spell_item_sentinels_eternal_refuge>(baseline,241846,241849);
 unknownForms=0;
 for(auto form:{FORM_SPIRIT_OWL_FORM,FORM_SPIRIT_OWL_FORM_2,FORM_WISP_FORM,FORM_WISP_FORM_2}) {
  Unit owner;AuraEffect effect{form};AuraApplication app{&owner};
  effect.HandleAuraModShapeshift(&app,0,true);assert(owner.model==1234);
  effect.HandleAuraModShapeshift(&app,AURA_EFFECT_HANDLE_REAL,true);
  assert(owner.form==form&&owner.model!=1234&&owner.power==POWER_FOCUS);
  effect.HandleAuraModShapeshift(&app,AURA_EFFECT_HANDLE_REAL,false);
  assert(owner.form==FORM_NONE&&owner.model==1234&&owner.restores==1&&owner.power==POWER_FOCUS);
 }
 if(baseline){assert(unknownForms==6);std::cout<<"PASS: baseline reproduces both orphaned forms and missing 37/38/40 admission\n";return 0;}
 assert(unknownForms==0);
 Unit npc;npc.player=false;AuraEffect invalid{static_cast<ShapeshiftForm>(250)};AuraApplication app{&npc};
 invalid.HandleAuraModShapeshift(&app,AURA_EFFECT_HANDLE_REAL,true);assert(unknownForms==1);
 std::cout<<"PASS: both production controllers, hook registration, expiry/stack loss/combat/death/unequip/null owner/other form, full native shapeshift model+focus restoration, and unknown-form diagnostic\n";
}
