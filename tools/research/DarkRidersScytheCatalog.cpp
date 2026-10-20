#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
int main(int argc,char** argv){
 if(argc!=2)return 1;
 DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
 if(!effects.Load(argv[1],0))return 2;
 for(auto e:effects)if(e->SpellID==199727)
  std::cout<<"effect "<<e->SpellID<<' '<<e->EffectIndex<<' '<<e->Effect<<' '<<e->ImplicitTarget[0]<<' '<<e->ImplicitTarget[1]<<'\n';
}
