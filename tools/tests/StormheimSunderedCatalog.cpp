#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
#include <map>
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    DB2Storage<SpellMiscEntry> misc("SpellMisc.db2",SpellMiscLoadInfo::Instance());
    DB2Storage<SpellRangeEntry> ranges("SpellRange.db2",SpellRangeLoadInfo::Instance());
    DB2Storage<SpellXSpellVisualEntry> links("SpellXSpellVisual.db2",SpellXSpellVisualLoadInfo::Instance());
    DB2Storage<CreatureDisplayInfoEntry> displays("CreatureDisplayInfo.db2",CreatureDisplayInfoLoadInfo::Instance());
    DB2Storage<CreatureModelDataEntry> models("CreatureModelData.db2",CreatureModelDataLoadInfo::Instance());
    if(!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!misc.Load(argv[1],0)||!ranges.Load(argv[1],0)||!links.Load(argv[1],0)||!displays.Load(argv[1],0)||!models.Load(argv[1],0))return 2;
    std::map<uint32,std::vector<SpellEffectEntry const*>> selected;
    for(auto e:effects)if(e->SpellID==192493||e->SpellID==192495||e->SpellID==192500||e->SpellID==192214||e->SpellID==192485||e->SpellID==192486||e->SpellID==212902||e->SpellID==212903||e->SpellID==212904)selected[e->SpellID].push_back(e);
    for(uint32 id:{192493u,192500u}){if(selected[id].size()!=1)return 3;auto e=selected[id][0];if(e->Effect!=3||e->ImplicitTarget[0]!=(id==192493?25:7))return 4;}
    if(selected[192495].size()!=4)return 5;
    uint32 chains[]={192195,192196,192198,192200};
    for(auto e:selected[192495])if(e->Effect!=203||e->EffectIndex<0||e->EffectIndex>3||e->EffectTriggerSpell!=chains[e->EffectIndex]||e->ImplicitTarget[0]!=1)return 6;
    for(uint32 id:{192214u,192485u,192486u}){if(selected[id].size()!=1||selected[id][0]->Effect!=6||selected[id][0]->EffectAura!=4)return 7;}
    for(uint32 i=0;i<3;++i){auto& es=selected[212902+i];if(es.size()!=1||es[0]->Effect!=219||es[0]->EffectMiscValue[0]!=2253+i||es[0]->ImplicitTarget[0]!=18)return 8;}
    for(auto const& row:selected){auto spell=spells.LookupEntry(row.first);if(!spell)return 9;std::cout<<"spell"<<row.first<<" name="<<spell->Name->Str[0]<<'\n';for(auto e:row.second)std::cout<<" effect="<<e->Effect<<" index="<<e->EffectIndex<<" aura="<<e->EffectAura<<" target="<<e->ImplicitTarget[0]<<" trigger="<<e->EffectTriggerSpell<<" misc="<<e->EffectMiscValue[0]<<'\n';}
    bool found=false;
    for(auto m:misc)if(m->SpellID==192493){auto r=ranges.LookupEntry(m->RangeIndex);if(!r||m->CastingTimeIndex!=16||m->RangeIndex!=245)return 10;std::cout<<"lantern castTimeIndex="<<m->CastingTimeIndex<<" range="<<r->RangeMin[0]<<".."<<r->RangeMax[0]<<","<<r->RangeMin[1]<<".."<<r->RangeMax[1]<<'\n';found=true;}
    if(!found)return 11;
    std::map<uint32,uint32> expected{{192214,49786},{192485,49866},{192486,49867},{192493,49840},{192495,49868}};
    for(auto const& pair:expected){found=false;for(auto l:links)if(l->SpellID==pair.first){if(l->SpellVisualID!=pair.second)return 12;found=true;}if(!found)return 13;std::cout<<"visual spell="<<pair.first<<" visual="<<pair.second<<'\n';}
    auto d=displays.LookupEntry(65186);if(!d)return 14;auto model=models.LookupEntry(d->ModelID);if(!model||!model->FileDataID)return 15;
    std::cout<<"fragment display65186 model="<<d->ModelID<<" file="<<model->FileDataID<<'\n';
    std::cout<<"PASS: native DB2 lantern target/dummy, chain-removal effects, three poses/conversations, actual visuals and fragment model. Client rendering and release motion remain unverified.\n";
}
