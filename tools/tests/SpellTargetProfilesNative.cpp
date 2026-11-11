#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
#include "SpellScript.h"
#include "SpellTargetProfilesNative.h"
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);VerifySpellTargetProfiles();CloseAltheaNative();return 0;
 }catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
