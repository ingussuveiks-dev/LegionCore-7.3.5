#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
#include <set>
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
#define STORE(T,N) DB2Storage<T##Entry> N(#T ".db2",T##LoadInfo::Instance());if(!N.Load(argv[1],0))return 2
    STORE(Spell,spells);STORE(SpellEffect,effects);STORE(SpellMisc,misc);
    STORE(SpellDuration,durations);STORE(SpellAuraOptions,options);STORE(SpellTargetRestrictions,targets);
    STORE(SpellInterrupts,interrupts);
    std::set<unsigned> ids{232450,232350,232358,231862,228854};
    for(unsigned pass=0;pass<4;++pass)for(auto e:effects)if(ids.count(e->SpellID)&&e->EffectTriggerSpell)ids.insert(e->EffectTriggerSpell);
    for(auto id:ids){auto s=spells.LookupEntry(id);if(!s)return 3;
        std::cout<<"SPELL "<<id<<" "<<s->Name->Str[0]<<" description="<<s->Description->Str[0]<<" aura="<<s->AuraDescription->Str[0]<<"\n";
        for(auto m:misc)if(m->SpellID==id){auto d=durations.LookupEntry(m->DurationIndex);std::cout<<" MISC diff="<<unsigned(m->DifficultyID)<<" duration="<<(d?d->Duration:0)<<" castIndex="<<m->CastingTimeIndex<<"\n";}
        for(auto a:options)if(a->SpellID==id)std::cout<<" OPTIONS diff="<<unsigned(a->DifficultyID)<<" stacks="<<a->CumulativeAura<<" procMask="<<a->ProcTypeMask<<" procChance="<<unsigned(a->ProcChance)<<"\n";
        for(auto t:targets)if(t->SpellID==id)std::cout<<" TARGET diff="<<unsigned(t->DifficultyID)<<" cap="<<unsigned(t->MaxTargets)<<"\n";
        bool hasInterrupts=false;for(auto i:interrupts)if(i->SpellID==id){hasInterrupts=true;std::cout<<" INTERRUPTS diff="<<unsigned(i->DifficultyID)<<" aura="<<i->AuraInterruptFlags[0]<<','<<i->AuraInterruptFlags[1]<<"\n";}if(!hasInterrupts)std::cout<<" INTERRUPTS absent (native zero defaults)\n";
        for(auto e:effects)if(e->SpellID==id)std::cout<<" EFFECT index="<<e->EffectIndex<<" diff="<<e->DifficultyID<<" type="<<e->Effect<<" aura="<<e->EffectAura<<" base="<<e->EffectBasePoints<<" target="<<e->ImplicitTarget[0]<<','<<e->ImplicitTarget[1]<<" period="<<e->EffectAuraPeriod<<" trigger="<<e->EffectTriggerSpell<<" radius="<<e->EffectRadiusIndex[0]<<','<<e->EffectRadiusIndex[1]<<" misc="<<e->EffectMiscValue[0]<<','<<e->EffectMiscValue[1]<<"\n";
    }
}
