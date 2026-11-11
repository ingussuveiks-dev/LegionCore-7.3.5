// Matching local26972+read-only hotfix spell profiles;not gameplay proof.
#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);
 for(uint32 id:{105984u,105992u,122193u,122147u}){
  auto info=sSpellMgr->GetSpellInfo(id);Require(info,"Requested pending spell absent from matching runtime data");
  if(auto raw=sSpellStore.LookupEntry(id))std::cout<<"NATIVE_DESCRIPTION id="<<id<<' '<<raw->Description->Str[0]<<'\n';
  for(auto difficulty:{DIFFICULTY_NONE,DIFFICULTY_10_N,DIFFICULTY_10_HC}){
   auto misc=info->GetMisc(difficulty);std::cout<<"PROFILE id="<<id<<" difficulty="<<uint32(difficulty)<<" attributes=";
   for(auto flag:misc->MiscData.Attributes)std::cout<<flag<<',';std::cout<<'\n';
   for(uint32 i=0;i<MAX_SPELL_EFFECTS;++i){auto e=info->GetEffect(i,difficulty);if(e&&e->Effect)std::cout<<"EFFECT id="<<id<<" difficulty="<<uint32(difficulty)<<" index="<<i<<" type="<<e->Effect<<" aura="<<e->ApplyAuraName<<" amount="<<e->BasePoints<<" trigger="<<e->TriggerSpell<<" targetA="<<e->TargetA.GetTarget()<<" targetB="<<e->TargetB.GetTarget()<<" radius="<<e->CalcRadius()<<'\n';}
  }
 }
 auto rows=WorldDatabase.Query("SELECT spell_id,ScriptName FROM spell_script_names WHERE spell_id IN(105984,105992,122193,122147) ORDER BY spell_id,ScriptName");if(rows)do{auto f=rows->Fetch();std::cout<<"BINDING "<<f[0].GetInt32()<<' '<<f[1].GetString()<<'\n';}while(rows->NextRow());
 CloseAltheaNative();std::cout<<"PASS selected OPEN spell catalog readback;no casts,character writes or full-sequence verification\n";return 0;
 }catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
