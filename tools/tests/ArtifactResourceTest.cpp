#include <algorithm>
#include <cassert>
#include <cstdint>
#include <map>
#include <set>
#include <iostream>
using uint8=uint8_t;using uint32=uint32_t;using int8=int8_t;using int32=int32_t;
enum Powers{POWER_RUNES=5,POWER_MAELSTROM=11,POWER_ALTERNATE=10,MAX_POWERS=19};
enum{SPELL_EFFECT_HANDLE_HIT_TARGET=1,SPELL_EFFECT_HANDLE_LAUNCH_TARGET=2,AURA_EFFECT_HANDLE_REAL=1,EFFECT_0=0,EFFECT_1=1,EFFECT_2=2,CRITERIA_TYPE_SCRIPT_EVENT=92,CRITERIA_TYPE_SCRIPT_EVENT_2=93,SKILL_ENGINEERING=202,SPELL_GROUP_ELIXIR_GUARDIAN=1,SPELL_GROUP_ELIXIR_BATTLE=2,SPELL_GROUP_ELIXIR_SHATTRATH=3,SPELL_GROUP_ELIXIR_UNSTABLE=4};
using SpellEffIndex=int;
template<class T>int CalculatePct(T value,int pct){return int(value)*pct/100;}
template<class T>void AddPct(T& value,int pct){value+=value*pct/100;}
struct Player;struct AuraEffect;struct Unit;struct GameObject;
struct WorldObject {virtual ~WorldObject()=default;virtual Unit* ToUnit(){return nullptr;}GameObject* ToGameObject(){return nullptr;}};
struct Aura{uint32 GetId(){assert(false);return 0;}int GetStackAmount(){assert(false);return 0;}};
struct AuraApplication{Unit* target=nullptr;Unit* GetTarget()const{return target;}Aura* GetBase(){assert(false);return nullptr;}};
struct Unit:WorldObject{
 bool alive=true;int power=0,maxPower=0;int energized=0;
 using AuraApplicationMap=std::map<int,AuraApplication*>;AuraApplicationMap auras;
 Unit* ToUnit()override{return this;}virtual Player* ToPlayer(){return nullptr;}
 bool IsAlive(){return alive;}uint32 GetMaxPower(Powers p){assert(p==POWER_ALTERNATE);return maxPower;}int GetPower(Powers p){assert(p==POWER_ALTERNATE);return power;}
 void SetMaxPower(Powers p,int v){assert(p==POWER_ALTERNATE);maxPower=v;}
 void SetPower(Powers p,int v){assert(p==POWER_ALTERNATE);power=v;}
 // Resource storage/log packets are controlled sinks; actual production effect
 // and aura lifecycle are compiled below. No quest counter is synthesized.
 void EnergizeBySpell(Unit* target,uint32 spell,int gain,Powers p){assert(spell==152435&&p==POWER_ALTERNATE);target->power=std::clamp(target->power+gain,0,target->maxPower);++target->energized;}
 bool CanPvPScalar(){return false;}bool HasAura(uint32){return false;}Aura* GetAura(uint32){assert(false);return nullptr;}AuraEffect const* GetAuraEffect(uint32,int){assert(false);return nullptr;}
 int GetEffectiveLevel(){assert(false);return 0;}int GetMaxHealth(){assert(false);return 0;}int getLevel(){assert(false);return 0;}
 void SendEnergizeSpellLog(Unit*,uint32,int,int,Powers){assert(false);}AuraApplicationMap& GetAppliedAuras(){assert(false);return auras;}
 void CastSpell(Unit*,uint32,bool,void* =nullptr){assert(false);}Player* GetPlayerMovingMe(){assert(false);return nullptr;}
};
struct Player:Unit{Player* ToPlayer()override{return this;}bool HasSkill(int){assert(false);return false;}int GetRuneCooldown(int){assert(false);return 0;}int GetRuneBaseCooldown(){assert(false);return 0;}void SetRuneCooldown(int,int){assert(false);}void AddRunePower(int){assert(false);}void UpdateAchievementCriteria(int,int,int){assert(false);}void SaveAddComboPoints(int){assert(false);}};
struct UnitPowerBarEntry{int MaxPower=100,StartPower=0;};
struct BarStore{UnitPowerBarEntry row;UnitPowerBarEntry const* LookupEntry(int id){return id==371||id==376?&row:nullptr;}}sUnitPowerBarStore;
struct AuraEffect{int misc=371;int GetMiscValue()const{return misc;}int GetAmount()const{assert(false);return 0;}void HandleProgressBar(AuraApplication const*,uint8,bool)const;};
struct Effect{int MiscValue=10;float PvPMultiplier=0;int CalcValue(Unit*)const{assert(false);return 0;}};
struct Misc{struct{float Speed=0;}MiscData;};
struct SpellInfo{uint32 Id=152435;int SpellLevel=0;Effect effect;Misc misc;Effect const* Effects[3]{&effect,&effect,&effect};Effect const* GetEffect(int,int)const{return &effect;}Misc const* GetMisc()const{return &misc;}};
struct SpellManager{bool IsSpellMemberOfSpellGroup(uint32,int){assert(false);return false;}void GetSetOfSpellsInSpellGroup(int,std::set<uint32>&){assert(false);}SpellInfo const* GetSpellInfo(uint32){assert(false);return nullptr;}} manager;
auto sSpellMgr=&manager;
namespace Trinity{namespace Containers{uint32 SelectRandomContainerElement(std::set<uint32> const&){assert(false);return 0;}}}
struct Item{};struct SpellImplicitTargetInfo{};
struct Targets{WorldObject* object=nullptr;WorldObject* GetObjectTarget(){return object;}Item* GetItemTarget(){return nullptr;}};
struct Spell{
 SpellInfo info;SpellInfo const* m_spellInfo=&info;Unit* m_caster=nullptr;Unit* unitTarget=nullptr;Targets m_targets;
 int effectHandleMode=SPELL_EFFECT_HANDLE_HIT_TARGET,m_diffMode=0,damage=10,m_addptype=-1,m_addpower=0;void* m_CastItem=nullptr;
 void EffectEnergize(SpellEffIndex);void SelectImplicitTargetObjectTargets(SpellEffIndex,SpellImplicitTargetInfo const&);
 int GetTargetCount(){assert(false);return 0;}void CallScriptObjectTargetSelectHandlers(WorldObject*&,int){}
 void AddUnitTarget(Unit* target,int,bool,bool){unitTarget=target;}void AddGOTarget(void*,int){assert(false);}void AddItemTarget(Item*,int){assert(false);}
 void SelectImplicitChainTargets(int,SpellImplicitTargetInfo const&,WorldObject*,int){}
};
#include "ProgressBar.inc"
#include "ExplicitTarget.inc"
#include "Energize.inc"
int main(){
 for(int bar:{371,376}){
  Player owner,other;Unit pet;AuraEffect aura;aura.misc=bar;AuraApplication app{&owner};
  aura.HandleProgressBar(&app,AURA_EFFECT_HANDLE_REAL,true);assert(owner.maxPower==100&&owner.power==0);
  Spell old;old.m_caster=&pet;old.m_targets.object=&pet;old.SelectImplicitTargetObjectTargets(0,{});assert(old.unitTarget==&pet);old.EffectEnergize(0);assert(!pet.energized&&!owner.power);
  // The original Player path and repaired owner-self path use the same native
  // explicit92 selection/effect. Smart85/128/130 resolution is tested separately
  // against both SQL tuples, including pet/guardian/charm and late changes.
  Spell fixed;fixed.m_caster=&owner;fixed.m_targets.object=&owner;fixed.SelectImplicitTargetObjectTargets(0,{});assert(fixed.unitTarget==&owner);
  for(int i=1;i<=10;++i){fixed.EffectEnergize(0);assert(owner.power==i*10&&owner.energized==i);}
  assert(!pet.power&&!pet.energized&&!other.power&&!other.energized&&fixed.m_addptype==10&&fixed.m_addpower==10);
  fixed.EffectEnergize(0);assert(owner.power==100); // storage clamps; no extra quest/item grant
  owner.power=40;owner.alive=false;fixed.EffectEnergize(0);assert(owner.power==40);owner.alive=true;
  fixed.effectHandleMode=SPELL_EFFECT_HANDLE_LAUNCH_TARGET;fixed.EffectEnergize(0);assert(owner.power==40);
  fixed.effectHandleMode=SPELL_EFFECT_HANDLE_HIT_TARGET;fixed.info.effect.MiscValue=-1;fixed.EffectEnergize(0);assert(owner.power==40);
  fixed.info.effect.MiscValue=MAX_POWERS;fixed.EffectEnergize(0);assert(owner.power==40);fixed.info.effect.MiscValue=10;
  aura.HandleProgressBar(&app,AURA_EFFECT_HANDLE_REAL,false);assert(!owner.power&&!owner.maxPower);fixed.EffectEnergize(0);assert(!owner.power);
  aura.HandleProgressBar(&app,0,true);assert(!owner.maxPower);aura.misc=0;aura.HandleProgressBar(&app,AURA_EFFECT_HANDLE_REAL,true);assert(!owner.maxPower);
  aura.misc=bar;aura.HandleProgressBar(&app,AURA_EFFECT_HANDLE_REAL,true);assert(owner.maxPower==100&&!owner.power);
  fixed.unitTarget=nullptr;fixed.EffectEnergize(0);Spell empty;empty.SelectImplicitTargetObjectTargets(0,{});assert(!empty.unitTarget);
 }
 std::cout<<"PASS: actual explicit target selection, native152435 Energize guards and346 bar apply/remove; old pet loses resource, repaired Player receives10, no inactive bar/dead target/invalid mode or power; native quest credit triggering/client remain separate\n";
}
