#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    DB2Storage<ConversationLineEntry> lines("ConversationLine.db2",ConversationLineLoadInfo::Instance());
    DB2Storage<CreatureDisplayInfoEntry> actors("CreatureDisplayInfo.db2",CreatureDisplayInfoLoadInfo::Instance());
    DB2Storage<CreatureModelDataEntry> models("CreatureModelData.db2",CreatureModelDataLoadInfo::Instance());
    DB2Storage<SoundKitEntry> sounds("SoundKit.db2",SoundKitLoadInfo::Instance());
    DB2Storage<FactionTemplateEntry> factions("FactionTemplate.db2",FactionTemplateLoadInfo::Instance());
    DB2Storage<FactionEntry> reputation("Faction.db2",FactionLoadInfo::Instance());
    if(!lines.Load(argv[1],0)||!actors.Load(argv[1],0)||!models.Load(argv[1],0)||!sounds.Load(argv[1],0)||!factions.Load(argv[1],0)||!reputation.Load(argv[1],0))return 2;
    for(uint32 id:{4748u,7170u}){auto l=lines.LookupEntry(id);if(!l||l->BroadcastTextID!=(id==4748?98489:120831)||l->NextConversationLineID!=(id==4748?7170:0)||l->SpellVisualKitID||l->AnimKitID)return 3;std::cout<<"line"<<id<<" broadcast="<<l->BroadcastTextID<<" next="<<l->NextConversationLineID<<" additional="<<l->AdditionalDuration<<" visualKit=0 animKit=0\n";}
    for(uint32 id:{65043u,68635u}){auto a=actors.LookupEntry(id);if(!a)return 4;auto m=models.LookupEntry(a->ModelID);if(!m||!m->FileDataID)return 5;std::cout<<"display"<<id<<" model="<<a->ModelID<<" file="<<m->FileDataID<<" gender="<<unsigned(a->Gender)<<'\n';}
    for(uint32 id:{52999u,53000u,70180u}){if(!sounds.LookupEntry(id))return 6;std::cout<<"sound"<<id<<" exists=1\n";}
    auto friendly=factions.LookupEntry(190);if(!friendly)return 7;
    for(uint32 id:{1u,2u}){auto player=factions.LookupEntry(id);if(!player||friendly->IsHostileTo(*player))return 8;}
    std::cout<<"Ashildir factionTemplate190 faction="<<friendly->Faction<<" hostileToAlliance=0 hostileToHorde=0\n";
    auto faction=reputation.LookupEntry(friendly->Faction);if(!faction||faction->CanHaveReputation())return 9;
    std::cout<<"Ashildir faction148 CanHaveReputation=0\n";
    std::cout<<"PASS: native sparse2252 line4748 and next7170, Helya/Ashildir models/gender and three sound kits. World explicitly sends only4748; client playback/next-line semantics remain unverified.\n";
}
