#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    DB2Storage<CreatureDisplayInfoEntry> actors("CreatureDisplayInfo.db2",CreatureDisplayInfoLoadInfo::Instance());
    DB2Storage<CreatureModelDataEntry> models("CreatureModelData.db2",CreatureModelDataLoadInfo::Instance());
    DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
    DB2Storage<FactionTemplateEntry> factions("FactionTemplate.db2",FactionTemplateLoadInfo::Instance());
    if(!actors.Load(argv[1],0)||!models.Load(argv[1],0)||!spells.Load(argv[1],0)||!factions.Load(argv[1],0))return 2;
    for(uint32 id:{70104u,68635u}){auto a=actors.LookupEntry(id);if(!a)return 3;auto m=models.LookupEntry(a->ModelID);if(!m||!m->FileDataID)return 4;std::cout<<"display"<<id<<" model="<<a->ModelID<<" file="<<m->FileDataID<<'\n';}
    for(uint32 id:{181705u,181709u,192299u,192298u}){auto s=spells.LookupEntry(id);if(!s)return 5;std::cout<<"spell"<<id<<" name="<<s->Name->Str[0]<<'\n';}
    auto legacy=spells.LookupEntry(81040);std::cout<<"legacy81040 present="<<bool(legacy);if(legacy)std::cout<<" name="<<legacy->Name->Str[0];std::cout<<'\n';
    auto faction=factions.LookupEntry(16);if(!faction)return 6;std::cout<<"Guarm factionTemplate16 faction="<<faction->Faction<<'\n';if(faction->Faction!=14)return 7;
    std::cout<<"PASS: native Guarm/Ashildir model chain, actual disguise/reaction/controller/summon names and Guarm faction14. Visual playback and neutral passage still need the client.\n";
}
