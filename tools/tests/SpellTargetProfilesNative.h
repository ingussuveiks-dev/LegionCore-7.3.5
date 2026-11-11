// Original hook body on real native SpellInfo is the pinned counterfactual.
struct TargetHookBefore:SpellScript::TargetHook{
 using SpellScript::TargetHook::TargetHook;
#include "TargetHookBefore.inc"
};
static void VerifySpellTargetProfiles(){
 auto heal=sSpellMgr->GetSpellInfo(122147);auto parent=sSpellMgr->GetSpellInfo(122193);Require(heal&&parent,"Native Mending profiles absent");
 TargetHookBefore oldArea(EFFECT_0,TARGET_UNIT_SRC_AREA_ENTRY,true),oldSingle(EFFECT_0,TARGET_UNIT_TARGET_ANY,false);
 Require(oldArea.GetAffectedEffectsMask(heal)==0&&oldSingle.GetAffectedEffectsMask(heal)==1,"Pinned base-only hook counterfactual missing");
 SpellScript::TargetHook area(EFFECT_0,TARGET_UNIT_SRC_AREA_ENTRY,true),single(EFFECT_0,TARGET_UNIT_TARGET_ANY,false);
 Require(area.GetAffectedEffectsMask(heal)==1&&single.GetAffectedEffectsMask(heal)==1,"Startup must validate both authored profiles");
 uint32 cases=0;
 for(auto difficulty:{DIFFICULTY_NONE,DIFFICULTY_10_N,DIFFICULTY_25_N,DIFFICULTY_10_HC,DIFFICULTY_25_HC,DIFFICULTY_MYTHIC_RAID}){
  auto effect=heal->GetEffect(EFFECT_0,difficulty);bool areaProfile=effect->TargetB.GetTarget()==TARGET_UNIT_SRC_AREA_ENTRY;
  Require(area.IsEffectAffectedForDifficulty(heal,EFFECT_0,difficulty,TARGET_UNIT_SRC_AREA_ENTRY)==areaProfile,"Runtime area hook ignored the actual difficulty/fallback");
  Require(single.IsEffectAffectedForDifficulty(heal,EFFECT_0,difficulty)==!areaProfile,"Explicit hook leaked into another difficulty");
  Require(!area.IsEffectAffectedForDifficulty(heal,EFFECT_0,difficulty,TARGET_UNIT_SRC_AREA_ALLY),"Area hook leaked into the other target stream");
  Require(!area.IsEffectAffectedForDifficulty(heal,EFFECT_1,difficulty),"Hook accepted a missing effect");
  SpellScript::TargetHook all(EFFECT_ALL,TARGET_UNIT_TARGET_ANY,false),first(EFFECT_FIRST_FOUND,TARGET_UNIT_TARGET_ANY,false);
  Require(all.IsEffectAffectedForDifficulty(parent,EFFECT_1,difficulty)&&!all.IsEffectAffectedForDifficulty(parent,EFFECT_0,difficulty),"ALL mask lost the matching actual effect");
  Require(first.IsEffectAffectedForDifficulty(parent,EFFECT_1,difficulty)&&!first.IsEffectAffectedForDifficulty(parent,EFFECT_0,difficulty),"FIRST_FOUND did not select the first matching actual effect");++cases;
 }
 std::cout<<"PASS native target-hook profiles:"<<cases<<"real difficulty/fallback cases,startup union,runtime effect/target stream,ALL/FIRST_FOUND and pinned base-only rejection\n";
}
