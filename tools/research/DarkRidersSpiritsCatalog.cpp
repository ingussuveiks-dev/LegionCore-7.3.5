#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    DB2Storage<SpellMiscEntry> misc("SpellMisc.db2",SpellMiscLoadInfo::Instance());
    DB2Storage<SpellDurationEntry> duration("SpellDuration.db2",SpellDurationLoadInfo::Instance());
    if(!effects.Load(argv[1],0)||!misc.Load(argv[1],0)||!duration.Load(argv[1],0))return 2;
    for(auto e:effects)if(e->SpellID==198721||e->SpellID==198654||e->SpellID==199568)
        std::cout<<"effect "<<e->SpellID<<' '<<e->EffectIndex<<' '<<e->Effect<<' '<<e->EffectAura<<' '
            <<e->EffectBasePoints<<' '<<e->EffectAuraPeriod<<' '<<e->EffectTriggerSpell<<' '<<e->ImplicitTarget[0]<<'\n';
    for(auto m:misc)if(m->SpellID==198654&&m->DifficultyID==0)
        if(auto d=duration.LookupEntry(m->DurationIndex))
            std::cout<<"duration "<<m->SpellID<<' '<<d->Duration<<' '<<d->MaxDuration<<'\n';
}
