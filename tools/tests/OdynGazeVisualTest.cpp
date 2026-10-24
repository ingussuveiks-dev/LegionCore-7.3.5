#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>
using uint32=uint32_t;using int32=int32_t;using uint16=uint16_t;using uint8=uint8_t;
#define TC_LOG_ERROR(...) ((void)0)
enum { DIFFICULTY_NONE=0,MAX_DIFFICULTY=32,SPELL_AURA_OVERRIDE_SPELL_VISUAL=1,
 AREATRIGGER_FIELD_SPELL_ID=0,AREATRIGGER_FIELD_SPELL_FOR_VISUALS=1,AREATRIGGER_FIELD_SPELL_XSPELL_VISUAL_ID=2 };
using Difficulty=uint32;
struct Unit;struct SpellInfo;
struct SpellXSpellVisualEntry {uint32 ID=0,SpellVisualID=0,SpellID=0,DifficultyID=0,CasterPlayerConditionID=0,ViewerPlayerConditionID=0,CasterUnitConditionID=0,ViewerUnitConditionID=0;float Probability=0;uint32 Priority=0,Flags=0;};
using SpellVisualMap=std::vector<std::vector<SpellXSpellVisualEntry const*>>;
struct SpellEntry {uint32 ID;};struct SpellInfoLoadHelper {};
struct SpellInfo {
 uint32 Id=0,_extraSpellXVisualID=0;SpellVisualMap _visuals;
 SpellInfo(SpellInfoLoadHelper const&,SpellEntry const* entry,SpellVisualMap const* visuals):Id(entry->ID),_visuals(*visuals){}
 uint32 GetSpellXSpellVisualId(Unit*,Unit* = nullptr) const;
};
struct SpellMgr {
 std::vector<SpellInfo*> mSpellInfoMap{230830,nullptr};
 ~SpellMgr(){for(auto info:mSpellInfoMap)delete info;}
 uint32 GetSpellInfoStoreSize() const{return uint32(mSpellInfoMap.size());}
 SpellInfo const* GetSpellInfo(uint32) const;
 void Load();
} spellManager;
auto* sSpellMgr=&spellManager;
struct AuraEffect {uint32 identity=0,replacement=0;int32 GetMiscValue() const{return int32(identity);}int32 GetMiscValueB() const{return int32(replacement);}};
struct Unit {
 uint32 difficulty=0,entry=93819;std::vector<AuraEffect*> overrides;
 struct Map {uint32 difficulty=0;Difficulty GetDifficultyID() const{return difficulty;}} map;
 Map* GetMap(){map.difficulty=difficulty;return &map;}
 auto const& GetAuraEffectsByType(uint32) const{return overrides;}
 bool IsUnitMeetCondition(uint32,SpellInfo const* = nullptr) const;
 uint32 GetEntry() const{return entry;}
};
struct Conditions {bool IsPlayerMeetingCondition(Unit*,uint32) const{return false;}} conditions;
auto* sConditionMgr=&conditions;
std::vector<SpellXSpellVisualEntry const*> sSpellXSpellVisualStore;
std::vector<SpellEntry const*> sSpellStore;
#include "GetInfo.inc"
#include "GetVisual.inc"
#include "UnitCondition.inc"
void SpellMgr::Load(){std::unordered_map<uint32,SpellVisualMap> visualsBySpell;std::unordered_map<uint32,SpellInfoLoadHelper> loadData;
#include "LoadVisuals.inc"
}
struct AreaTrigger {
 struct Info {int32 VisualID=0;} atInfo;
 uint32 values[3]{};
 void SetUInt32Value(uint32 field,uint32 value){values[field]=value;}
 void Resolve(SpellInfo const* info,Unit* caster){
#include "Resolve.inc"
 }
};
int main(int argc,char** argv){
 assert(argc==3);std::ifstream input(argv[1]);SpellXSpellVisualEntry row;
 assert(input>>row.ID>>row.SpellVisualID>>row.SpellID>>row.DifficultyID>>row.CasterPlayerConditionID>>row.ViewerPlayerConditionID>>row.CasterUnitConditionID>>row.ViewerUnitConditionID>>row.Probability>>row.Priority>>row.Flags);
 assert(row.ID==111461&&row.SpellVisualID==57404&&row.SpellID==230829&&row.DifficultyID==0&&!row.CasterPlayerConditionID&&!row.ViewerPlayerConditionID);
 SpellEntry old{57404},correct{230829};sSpellStore={&old,&correct};sSpellXSpellVisualStore={&row};spellManager.Load();
 auto original=sSpellMgr->GetSpellInfo(57404);auto fixed=sSpellMgr->GetSpellInfo(230829);assert(original&&fixed&&original->GetSpellXSpellVisualId(nullptr)==0&&fixed->GetSpellXSpellVisualId(nullptr)==111461);
 AreaTrigger baseline;baseline.atInfo.VisualID=57404;baseline.Resolve(nullptr,nullptr);assert(baseline.values[1]==57404&&baseline.values[2]==0);
 std::ifstream visualInput(argv[2]);int32 identity=0;assert(visualInput>>identity&&identity==230829);
 for(uint32 difficulty:{0u,1u,12u,23u})for(uint32 entry:{93819u,104307u,1u}){
  Unit caster;caster.entry=entry;caster.difficulty=difficulty;AreaTrigger area;area.atInfo.VisualID=identity;area.Resolve(nullptr,&caster);
  assert(area.values[0]==0&&area.values[1]==230829&&area.values[2]==111461);
 }
 AreaTrigger noCaster;noCaster.atInfo.VisualID=identity;noCaster.Resolve(nullptr,nullptr);assert(noCaster.values[2]==111461);
 AreaTrigger direct;direct.atInfo.VisualID=57404;direct.Resolve(fixed,nullptr);assert(direct.values[0]==230829&&direct.values[1]==230829&&direct.values[2]==111461);
 AreaTrigger overridden;overridden.atInfo.VisualID=-230829;overridden.Resolve(original,nullptr);assert(overridden.values[0]==57404&&overridden.values[1]==230829&&overridden.values[2]==0);
 spellManager.mSpellInfoMap[57404]=nullptr;AreaTrigger fallback;fallback.atInfo.VisualID=57404;fallback.Resolve(nullptr,nullptr);assert(fallback.values[1]==0&&fallback.values[2]==111461);spellManager.mSpellInfoMap[57404]=const_cast<SpellInfo*>(original);
 std::cout<<"PASS: actual native spell registration/getter/selector and scripted AT visual-field branch reproduce57404 name-space collision ->zero visual; corrected native230829 resolves111461 ->57404 without casting it. Difficulty fallback, caster variants and unchanged direct/override/raw-fallback paths checked; no live AT spawn, packet/rendering or quest progression simulated\n";
}
