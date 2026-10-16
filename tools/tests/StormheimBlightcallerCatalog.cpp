#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    DB2Storage<QuestLineXQuestEntry> lines("QuestLineXQuest.db2",QuestLineXQuestLoadInfo::Instance());
    DB2Storage<CreatureDisplayInfoEntry> displays("CreatureDisplayInfo.db2",CreatureDisplayInfoLoadInfo::Instance());
    DB2Storage<CreatureModelDataEntry> models("CreatureModelData.db2",CreatureModelDataLoadInfo::Instance());
    DB2Storage<FactionTemplateEntry> factions("FactionTemplate.db2",FactionTemplateLoadInfo::Instance());
    DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
    if(!lines.Load(argv[1],0)||!displays.Load(argv[1],0)||!models.Load(argv[1],0)||!factions.Load(argv[1],0)||!spells.Load(argv[1],0))return 2;
    bool found=false;for(auto q:lines)if(q->QuestID==38361){if(q->QuestLineID!=127||q->OrderIndex!=2)return 3;found=true;std::cout<<"quest38361 line127 order2\n";}if(!found)return 3;
    for(unsigned id:{66644u,62752u,62718u,62743u}){auto d=displays.LookupEntry(id);if(!d)return 4;auto m=models.LookupEntry(d->ModelID);if(!m||!m->FileDataID)return 5;std::cout<<"display "<<id<<" model "<<m->ID<<" file "<<m->FileDataID<<'\n';}
    auto enemy=factions.LookupEntry(84);auto giver=factions.LookupEntry(118);auto horde=factions.LookupEntry(2);auto alliance=factions.LookupEntry(1);
    if(!enemy||!giver||!horde||!alliance||!enemy->IsHostileTo(*horde)||enemy->IsHostileTo(*alliance)||giver->IsHostileTo(*horde))return 6;
    std::cout<<"faction84 hostile to Horde2, nonhostile to Alliance1; giver118 nonhostile to Horde2\n";
    for(unsigned id:{145363u,159549u,185574u,185575u,185698u,185684u,213738u,215377u}){auto s=spells.LookupEntry(id);if(!s)return 7;std::cout<<"spell "<<id<<" "<<s->Name->Str[0]<<'\n';}
}
