#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    DB2Storage<SpellMiscEntry> misc("SpellMisc.db2",SpellMiscLoadInfo::Instance());
    DB2Storage<SpellRangeEntry> ranges("SpellRange.db2",SpellRangeLoadInfo::Instance());
    DB2Storage<SpellCastTimesEntry> times("SpellCastTimes.db2",SpellCastTimesLoadInfo::Instance());
    DB2Storage<SpellXSpellVisualEntry> links("SpellXSpellVisual.db2",SpellXSpellVisualLoadInfo::Instance());
    DB2Storage<CreatureDisplayInfoEntry> displays("CreatureDisplayInfo.db2",CreatureDisplayInfoLoadInfo::Instance());
    DB2Storage<CreatureModelDataEntry> models("CreatureModelData.db2",CreatureModelDataLoadInfo::Instance());
    if(!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!misc.Load(argv[1],0)||!ranges.Load(argv[1],0)||!times.Load(argv[1],0)||!links.Load(argv[1],0)||!displays.Load(argv[1],0)||!models.Load(argv[1],0))return 2;
    for(uint32 id:{192387u,192388u}){
        auto spell=spells.LookupEntry(id);if(!spell)return 3;std::cout<<"spell"<<id<<" name="<<spell->Name->Str[0]<<'\n';
        uint32 count=0;for(auto e:effects)if(e->SpellID==id){++count;if(e->EffectIndex||e->Effect!=(id==192387?3:6)||e->EffectAura!=(id==192387?0:4)||e->ImplicitTarget[0]!=(id==192387?25:1))return 4;}
        if(count!=1)return 5;count=0;
        for(auto l:links)if(l->SpellID==id){if(l->SpellVisualID!=(id==192387?49840:49841))return 6;++count;std::cout<<"visual="<<l->SpellVisualID<<'\n';}if(count!=1)return 7;
    }
    bool found=false;for(auto m:misc)if(m->SpellID==192387){auto time=times.LookupEntry(m->CastingTimeIndex);auto range=ranges.LookupEntry(m->RangeIndex);if(!time||time->Base!=1500||!range||range->RangeMax[0]!=500)return 8;found=true;std::cout<<"lantern cast="<<time->Base<<"ms range="<<range->RangeMax[0]<<'\n';}if(!found)return 9;
    auto display=displays.LookupEntry(68954);if(!display)return 10;auto model=models.LookupEntry(display->ModelID);if(!model||!model->FileDataID)return 11;
    std::cout<<"Drowning Valkyra display68954 model="<<display->ModelID<<" file="<<model->FileDataID<<'\n';
    std::cout<<"PASS: actual native DB2 click DUMMY/ANY25/1500ms, drowning aura/self target, visual49840/49841 and creature display/model. Client lantern/pose/model playback remains unverified.\n";
}
