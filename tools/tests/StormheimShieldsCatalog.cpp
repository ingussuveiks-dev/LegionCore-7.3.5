#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
#include <set>
int main(int argc,char**argv){
 if(argc!=2)return 1;
 DB2Storage<GameObjectDisplayInfoEntry> objects("GameObjectDisplayInfo.db2",GameobjectDisplayInfoLoadInfo::Instance());
 DB2Storage<CreatureDisplayInfoEntry> displays("CreatureDisplayInfo.db2",CreatureDisplayInfoLoadInfo::Instance());
 DB2Storage<CreatureModelDataEntry> models("CreatureModelData.db2",CreatureModelDataLoadInfo::Instance());
 DB2Storage<QuestLineXQuestEntry> lines("QuestLineXQuest.db2",QuestLineXQuestLoadInfo::Instance());
 if(!objects.Load(argv[1],0)||!displays.Load(argv[1],0)||!models.Load(argv[1],0)||!lines.Load(argv[1],0))return 2;
 for(auto id:{27378,35192}){auto d=objects.LookupEntry(id);if(!d||!d->FileDataID)return 3;std::cout<<"shield display"<<id<<" file="<<d->FileDataID<<'\n';}
 for(auto id:{34520,30776}){auto d=displays.LookupEntry(id);if(!d)return 4;auto m=models.LookupEntry(d->ModelID);if(!m||!m->FileDataID)return 4;std::cout<<"giver display"<<id<<" model="<<m->ID<<" file="<<m->FileDataID<<'\n';}
 std::set<unsigned> found;for(auto q:lines)if(q->QuestID==39062||q->QuestID==39063||q->QuestID==39154||q->QuestID==38878){bool horde=q->QuestID==39154||q->QuestID==38878;bool delivery=q->QuestID==39062||q->QuestID==39154;if(q->QuestLineID!=(horde?137:136)||q->OrderIndex!=(delivery?3:4))return 5;found.insert(q->QuestID);}if(found.size()!=4)return 5;
 std::cout<<"PASS: native shield/giver models and both chapter136/137 delivery/shield order. Text-to-object placement follows objective order as a repair choice, not recovered client sniff evidence.\n";
}
