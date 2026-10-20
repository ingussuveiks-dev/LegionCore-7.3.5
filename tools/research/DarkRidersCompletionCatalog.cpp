#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
int main(int argc,char** argv){
 if(argc!=2)return 1;
 DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
 if(!effects.Load(argv[1],0))return 2;
 for(auto e:effects)if(e->SpellID==204819||e->SpellID==232961)
  std::cout<<"effect "<<e->SpellID<<' '<<e->EffectIndex<<' '<<e->Effect<<' '<<e->EffectMiscValue[0]<<' '<<e->EffectItemType<<'\n';
}
