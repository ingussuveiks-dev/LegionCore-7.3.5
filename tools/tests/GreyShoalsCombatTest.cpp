#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
using uint32=uint32_t;
#include "TargetFlags.inc"
static void Require(bool ok,char const* why){if(!ok)throw std::runtime_error(why);}
struct ObjectGuid {static constexpr int Empty=0;};
enum {SPELL_CAST_OK,SMART_TARGET_SELF=1,SMART_TARGET_VICTIM=2,SMART_TARGET_HOSTILE_RANDOM=5,SELECT_TARGET_RANDOM=0};
struct Unit;struct Player;
struct WorldObject {
 int guid;float x;bool unit=true;
 bool IsUnit(){return unit;}WorldObject* ToGameObject(){return nullptr;}WorldObject* ToCorpse(){return nullptr;}
};
struct Selector {Unit* target=nullptr;Unit* SelectTarget(int,int){return target;}};
struct Unit:WorldObject {
 Unit(int id,float pos){guid=id;x=pos;}
 Unit* victim=nullptr;Selector selector;
 int GetGUID(){return guid;}Player* ToPlayer(){return nullptr;}bool IsCreature(){return true;}
 Unit* getVictim(){return victim;}Selector* AI(){return &selector;}
};
struct Player:Unit {int GetSelection(){return 0;}};
namespace ObjectAccessor {Unit* GetUnit(Unit&,int){return nullptr;}}
struct SpellInfo {
 uint32 mask;
 uint32 GetExplicitTargetMask(){return mask;}
 int CheckExplicitTarget(Unit*,Unit*){return SPELL_CAST_OK;}
};
struct SpellCastTargets {
 Unit* target=nullptr;bool dst=false,src=false;float destination=0,source=0;
 Unit* GetUnitTarget()const{return target;}WorldObject* GetObjectTarget()const{return target;}
 void RemoveObjectTarget(){target=nullptr;}void SetUnitTarget(Unit* p){target=p;}
 bool HasDst()const{return dst;}bool HasSrc()const{return src;}
 void SetDst(WorldObject const& p){dst=true;destination=p.x;}void SetSrc(WorldObject const& p){src=true;source=p.x;}
 void RemoveDst(){dst=false;}void RemoveSrc(){src=false;}
};
struct Spell {
 Unit* m_caster;SpellInfo* m_spellInfo;SpellCastTargets m_targets;Unit* m_originalTarget=nullptr;int m_originalTargetGUID=0;
 void InitExplicitTargets(SpellCastTargets const&);
};
#include "InitExplicitTargets.inc"
struct SmartScript {
 Unit* me;
 WorldObject* GetBaseObject(){return me;}
 std::vector<WorldObject*> GetTargets(int type){std::vector<WorldObject*> result;auto l=&result;switch(type){
#include "SmartTargets.inc"
 default:throw std::runtime_error("Unsupported fixture target");
 }return result;}
};
int main(){try{
 Unit caster(1,10),victim(2,20),random(3,30);caster.victim=&victim;caster.selector.target=&random;SmartScript smart{&caster};
 for(auto test:std::vector<std::pair<int,Unit*>>{{SMART_TARGET_SELF,&caster},{SMART_TARGET_VICTIM,&victim},{SMART_TARGET_HOSTILE_RANDOM,&random}}){
  auto selected=smart.GetTargets(test.first);Require(selected.size()==1&&selected[0]==test.second,"Native SmartAI target selection changed");
 }
 // Masks come from the separately executed typed native DB2/target-mask catalog:
 // self Battle Rage/Flurry0; victim bite/slash and random bolt128; mist64.
 for(auto test:std::vector<std::pair<uint32,Unit*>>{{0,&caster},{TARGET_FLAG_UNIT_ENEMY,&victim},{TARGET_FLAG_UNIT_ENEMY,&random},{TARGET_FLAG_DEST_LOCATION,&random}}){
  SpellInfo info{test.first};Spell spell{&caster,&info};SpellCastTargets input;input.target=test.second;spell.InitExplicitTargets(input);
  Require(spell.m_originalTarget==test.second&&spell.m_originalTargetGUID==test.second->guid,"Original target lost");
  if(test.first==TARGET_FLAG_DEST_LOCATION){
   Require(!spell.m_targets.target&&spell.m_targets.HasDst()&&spell.m_targets.destination==random.x,"Mist must use the selected hostile position, not caster fallback");
  }else if(test.first){Require(spell.m_targets.target==test.second&&!spell.m_targets.HasDst(),"Hostile unit target lost");}
  else Require(!spell.m_targets.target&&!spell.m_targets.HasDst(),"Caster-centered spell retained redundant explicit target");
 }
 caster.selector.target=nullptr;caster.victim=nullptr;
 Require(smart.GetTargets(SMART_TARGET_VICTIM).empty()&&smart.GetTargets(SMART_TARGET_HOSTILE_RANDOM).empty(),"Missing combat target invented");
 SpellInfo destination{TARGET_FLAG_DEST_LOCATION};Spell fallback{&caster,&destination};fallback.InitExplicitTargets({});
 Require(fallback.m_targets.HasDst()&&fallback.m_targets.destination==caster.x,"Native no-object destination fallback changed");
 std::cout<<"PASS: actual SmartAI self/victim/random cases and native InitExplicitTargets; hostile mist position survives redundant unit-target removal. No full cast/hit or client claim.\n";
 return 0;
}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}
