#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    DB2Storage<GameObjectDisplayInfoEntry> objects("GameObjectDisplayInfo.db2",GameobjectDisplayInfoLoadInfo::Instance());
    DB2Storage<PhaseEntry> phases("Phase.db2",PhaseLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    if(!objects.Load(argv[1],0)||!phases.Load(argv[1],0)||!effects.Load(argv[1],0))return 2;
    auto go=objects.LookupEntry(35704);if(!go||go->FileDataID!=1361813)return 3;
    std::cout<<"Aegis display35704 file "<<go->FileDataID<<" minZ "<<go->GeoBoxMin.Z<<" maxZ "<<go->GeoBoxMax.Z<<'\n';
    for(unsigned phase:{3554u,4591u,4592u}){if(!phases.LookupEntry(phase))return 4;std::cout<<"native phase "<<phase<<'\n';}
    unsigned credits=0;for(auto effect:effects)if((effect->Effect==90||effect->Effect==134)&&effect->EffectMiscValue[0]==110409){++credits;std::cout<<"credit110409 spell "<<effect->SpellID<<" effect "<<effect->Effect<<'\n';}
    std::cout<<"native direct/group spell credit effects for110409 "<<credits<<'\n';
}
