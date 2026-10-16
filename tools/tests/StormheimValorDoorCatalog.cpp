#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    DB2Storage<QuestLineXQuestEntry> line("QuestLineXQuest.db2",QuestLineXQuestLoadInfo::Instance());
    DB2Storage<CreatureDisplayInfoEntry> displays("CreatureDisplayInfo.db2",CreatureDisplayInfoLoadInfo::Instance());
    DB2Storage<CreatureModelDataEntry> models("CreatureModelData.db2",CreatureModelDataLoadInfo::Instance());
    DB2Storage<FactionTemplateEntry> factions("FactionTemplate.db2",FactionTemplateLoadInfo::Instance());
    DB2Storage<FactionEntry> reputation("Faction.db2",FactionLoadInfo::Instance());
    if(!line.Load(argv[1],0)||!displays.Load(argv[1],0)||!models.Load(argv[1],0)||!factions.Load(argv[1],0)||!reputation.Load(argv[1],0))return 2;
    unsigned found=0;
    for(auto q:line)if(q->QuestID==40078||q->QuestID==40001||q->QuestID==40002){
        std::cout<<"quest "<<q->QuestID<<" line "<<q->QuestLineID<<" order "<<unsigned(q->OrderIndex)<<'\n';++found;
    }
    if(found!=2)return 3;
    for(auto q:line)if(q->QuestID==40001)return 8;
    std::cout<<"quest40001 has no native QuestLineXQuest row; handoff is established by World relations\n";
    auto display=displays.LookupEntry(68213);if(!display)return 4;
    auto model=models.LookupEntry(display->ModelID);if(!model||!model->FileDataID)return 5;
    std::cout<<"Vethir display "<<display->ID<<" scale "<<display->CreatureModelScale<<" model "<<model->ID<<" file "<<model->FileDataID<<" modelScale "<<model->ModelScale<<'\n';
    auto v=factions.LookupEntry(190);if(!v)return 6;
    for(unsigned id:{1u,2u}){ // Native human/orc faction templates, both audited player factions.
        auto p=factions.LookupEntry(id);if(!p||v->IsHostileTo(*p))return 7;
        std::cout<<"faction190 vs "<<id<<" hostile "<<v->IsHostileTo(*p)<<" friendly "<<v->IsFriendlyTo(*p)<<'\n';
    }
    std::cout<<"faction190 reputationFaction "<<v->Faction<<'\n';
    auto rep=reputation.LookupEntry(v->Faction);if(!rep||rep->CanHaveReputation())return 9;
    std::cout<<"faction148 canHaveReputation "<<rep->CanHaveReputation()<<'\n';
    return 0;
}
