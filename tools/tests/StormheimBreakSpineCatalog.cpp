#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "QuestDef.h"
#include <iostream>
static_assert(QUEST_OBJECTIVE_FLAG_TRACKED_ON_MINIMAP == 1, "Native tracking flag changed");
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    DB2Storage<QuestLineXQuestEntry> line("QuestLineXQuest.db2",QuestLineXQuestLoadInfo::Instance());
    DB2Storage<CreatureDisplayInfoEntry> displays("CreatureDisplayInfo.db2",CreatureDisplayInfoLoadInfo::Instance());
    DB2Storage<CreatureModelDataEntry> models("CreatureModelData.db2",CreatureModelDataLoadInfo::Instance());
    DB2Storage<FactionTemplateEntry> factions("FactionTemplate.db2",FactionTemplateLoadInfo::Instance());
    DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
    if(!line.Load(argv[1],0)||!displays.Load(argv[1],0)||!models.Load(argv[1],0)||!factions.Load(argv[1],0)||!spells.Load(argv[1],0))return 2;
    unsigned found=0;
    for(auto q:line)if(q->QuestID>=40002&&q->QuestID<=40005){
        if(q->QuestLineID!=138)return 3;
        std::cout<<"quest "<<q->QuestID<<" line "<<q->QuestLineID<<" order "<<unsigned(q->OrderIndex)<<'\n';++found;
    }
    if(found!=4)return 3;
    for(unsigned id:{63482u,64046u,64645u,65514u}){
        auto display=displays.LookupEntry(id);if(!display)return 4;
        auto model=models.LookupEntry(display->ModelID);if(!model||!model->FileDataID)return 5;
        std::cout<<"display "<<id<<" model "<<model->ID<<" file "<<model->FileDataID<<'\n';
    }
    auto enemy=factions.LookupEntry(14);if(!enemy)return 6;
    for(unsigned id:{1u,2u}){
        auto player=factions.LookupEntry(id);if(!player||!enemy->IsHostileTo(*player))return 7;
        std::cout<<"faction14 hostile to "<<id<<'\n';
    }
    for(unsigned id:{210535u,210537u,205462u,210541u,187706u,209263u,209470u,198511u,198557u,198559u}){
        auto spell=spells.LookupEntry(id);if(!spell)return 8;
        std::cout<<"spell "<<id<<" "<<spell->Name->Str[0]<<'\n';
    }
    std::cout<<"native TRACKED_ON_MINIMAP=1; all four kill objectives remain required\n";
}
