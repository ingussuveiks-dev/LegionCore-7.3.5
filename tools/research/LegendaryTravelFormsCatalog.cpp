// Read the matching 7.3.5 spell/form records with worldserver's native DB2 reader.
#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
#include <set>

int main(int argc, char** argv)
{
    if (argc != 2) return 1;
    DB2Storage<SpellEntry> spells("Spell.db2", SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2", SpellEffectLoadInfo::Instance());
    DB2Storage<SpellMiscEntry> misc("SpellMisc.db2", SpellMiscLoadInfo::Instance());
    DB2Storage<SpellDurationEntry> durations("SpellDuration.db2", SpellDurationLoadInfo::Instance());
    DB2Storage<SpellAuraOptionsEntry> options("SpellAuraOptions.db2", SpellAuraOptionsLoadInfo::Instance());
    DB2Storage<SpellInterruptsEntry> interrupts("SpellInterrupts.db2", SpellInterruptsLoadInfo::Instance());
    DB2Storage<SpellShapeshiftFormEntry> forms("SpellShapeshiftForm.db2", SpellShapeshiftFormLoadInfo::Instance());
    if (!spells.Load(argv[1], 0) || !effects.Load(argv[1], 0) || !misc.Load(argv[1], 0) ||
        !durations.Load(argv[1], 0) || !options.Load(argv[1], 0) || !interrupts.Load(argv[1], 0) || !forms.Load(argv[1], 0)) return 2;
    std::set<uint32> wanted{241331,241332,241846,241849,242066,242070,242105,242111};
    for (auto id : wanted)
    {
        auto spell = spells.LookupEntry(id);
        if (!spell) return 3;
        std::cout << "SPELL\t" << id << '\t' << spell->Name->Str[0] << '\t' << spell->Description->Str[0] << '\n';
    }
    for (auto e : effects) if (wanted.count(e->SpellID))
        std::cout << "EFFECT\t" << e->SpellID << '\t' << unsigned(e->DifficultyID) << '\t' << e->EffectIndex << '\t'
            << e->Effect << '\t' << e->EffectAura << '\t' << e->EffectMiscValue[0] << '\t' << e->EffectBasePoints << '\t' << e->EffectTriggerSpell << '\t' << e->EffectAuraPeriod << '\n';
    for (auto m : misc) if (wanted.count(m->SpellID))
    {
        auto d = durations.LookupEntry(m->DurationIndex);
        std::cout << "DURATION\t" << m->SpellID << '\t' << unsigned(m->DifficultyID) << '\t' << (d ? d->Duration : 0) << '\n';
    }
    for (auto o : options) if (wanted.count(o->SpellID))
        std::cout << "OPTIONS\t" << o->SpellID << '\t' << o->CumulativeAura << '\t' << o->ProcTypeMask << '\n';
    for (auto i : interrupts) if (wanted.count(i->SpellID))
        std::cout << "INTERRUPTS\t" << i->SpellID << '\t' << i->AuraInterruptFlags[0] << '\t' << i->AuraInterruptFlags[1] << '\n';
    for (uint32 id : {37,38,39,40}) if (auto f = forms.LookupEntry(id))
        std::cout << "FORM\t" << id << '\t' << f->Name->Str[0] << '\t' << f->Flags << '\t' << f->CreatureDisplayID[0] << '\t' << f->CreatureDisplayID[1] << '\t' << f->MountTypeID << '\n';
    for (auto id : {242066,241846,242070,241849,242105,242111})
    {
        bool durationOK=false, effectOK=false, optionsOK=id!=242066&&id!=241846;
        for (auto m : misc) if (m->SpellID==id && !m->DifficultyID)
            if (auto d=durations.LookupEntry(m->DurationIndex)) durationOK=d->Duration==((id==242066||id==241846)?60000:-1);
        for (auto o : options) if (o->SpellID==id && o->CumulativeAura==5) optionsOK=true;
        for (auto e : effects) if (e->SpellID==id && !e->DifficultyID && !e->EffectIndex)
        {
            if (id==242105||id==242111) effectOK=e->EffectAura==226 && e->EffectAuraPeriod==500;
            else if (id==242070||id==241849) effectOK=e->EffectAura==36 && e->EffectMiscValue[0]==(id==242070?38:40);
            else effectOK=e->EffectAura==31 && e->EffectBasePoints==5;
        }
        if (!durationOK||!effectOK||!optionsOK) return 4;
    }
    for (uint32 id : {37,38,39,40})
    {
        auto f=forms.LookupEntry(id);
        if (!f || !f->CreatureDisplayID[0] || !f->MountTypeID) return 5;
    }
    std::cout << "PASS\tNative 60-second five-stack speed buffs, infinite forms, half-second controllers, form 38/40 effects, and all four model/mount records\n";
    return 0;
}
