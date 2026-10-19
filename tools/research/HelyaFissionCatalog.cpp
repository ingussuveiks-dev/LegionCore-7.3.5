// Read original runtime tables with the core's packed DB2 reader. This records
// client data and incoming references; it cannot establish a server AI timer.
#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
#define STORE(T,N) DB2Storage<T##Entry> N(#T ".db2",T##LoadInfo::Instance());if(!N.Load(argv[1],0))return 2
    STORE(Spell,spells);
    STORE(SpellEffect,effects);
    STORE(SpellAuraOptions,options);
    STORE(SpellMisc,misc);
    STORE(SpellDuration,durations);
    STORE(SpellAuraRestrictions,restrictions);
    STORE(JournalEncounterSection,sections);
    auto fission=spells.LookupEntry(158534);if(!fission)return 3;
    std::cout<<"SPELL "<<fission->ID<<" "<<fission->Name->Str[0]<<" description="<<fission->Description->Str[0]<<"\n";
    unsigned incoming=0;
    for(auto e:effects){
        if(e->SpellID==158534||e->EffectTriggerSpell==158534||e->EffectBasePoints==158534||e->EffectMiscValue[0]==158534||e->EffectMiscValue[1]==158534){
            std::cout<<"EFFECT row="<<e->ID<<" spell="<<e->SpellID<<" diff="<<e->DifficultyID<<" index="<<e->EffectIndex<<" type="<<e->Effect<<" aura="<<e->EffectAura<<" base="<<e->EffectBasePoints<<" target="<<e->ImplicitTarget[0]<<','<<e->ImplicitTarget[1]<<" period="<<e->EffectAuraPeriod<<" trigger="<<e->EffectTriggerSpell<<"\n";
            if(e->SpellID!=158534)++incoming;
        }
    }
    for(auto a:options)if(a->SpellID==158534||a->SpellID==193367)
        std::cout<<"OPTIONS spell="<<a->SpellID<<" diff="<<unsigned(a->DifficultyID)<<" stacks="<<a->CumulativeAura<<" procMask="<<a->ProcTypeMask<<" chance="<<unsigned(a->ProcChance)<<" charges="<<a->ProcCharges<<" ppm="<<a->SpellProcsPerMinuteID<<"\n";
    for(auto m:misc)if(m->SpellID==158534){auto d=durations.LookupEntry(m->DurationIndex);if(!d)return 4;std::cout<<"MISC spell="<<m->SpellID<<" diff="<<unsigned(m->DifficultyID)<<" duration="<<d->Duration<<" maxDuration="<<d->MaxDuration<<"\n";}
    for(auto r:restrictions)if(r->CasterAuraSpell==158534||r->TargetAuraSpell==158534||r->ExcludeCasterAuraSpell==158534||r->ExcludeTargetAuraSpell==158534)
        std::cout<<"RESTRICTION spell="<<r->SpellID<<" diff="<<unsigned(r->DifficultyID)<<" caster="<<r->CasterAuraSpell<<" target="<<r->TargetAuraSpell<<" excludedCaster="<<r->ExcludeCasterAuraSpell<<" excludedTarget="<<r->ExcludeTargetAuraSpell<<"\n";
    for(auto s:sections)if(s->SpellID==158534)
        std::cout<<"JOURNAL section="<<s->ID<<" encounter="<<s->JournalEncounterID<<" parent="<<s->ParentSectionID<<" difficultyMask="<<unsigned(s->DifficultyMask)<<" title="<<s->Title->Str[0]<<" body="<<s->BodyText->Str[0]<<"\n";
    std::cout<<"INCOMING_EFFECT_FIELD_MATCHES "<<incoming<<"\n";
    return 0;
}
