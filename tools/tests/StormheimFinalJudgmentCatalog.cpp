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
    DB2Storage<MapEntry> maps("Map.db2",MapLoadInfo::Instance());
    DB2Storage<ConversationLineEntry> lines("ConversationLine.db2",ConversationLineLoadInfo::Instance());
    if(!actors.Load(argv[1],0)||!models.Load(argv[1],0)||!spells.Load(argv[1],0)||!maps.Load(argv[1],0))return 2;
    for(uint32 id:{64593u,68635u,65123u,61276u}){auto row=actors.LookupEntry(id);if(!row)return 3;auto model=models.LookupEntry(row->ModelID);if(!model||!model->FileDataID)return 4;std::cout<<"display"<<id<<" model="<<row->ModelID<<" file="<<model->FileDataID<<'\n';}
    for(uint32 id:{89319u,100044u,209164u,209363u,208544u,209310u,209241u}){auto row=spells.LookupEntry(id);if(!row)return 5;std::cout<<"spell"<<id<<" name="<<row->Name->Str[0]<<'\n';}
    auto helheim=maps.LookupEntry(1463);if(!helheim||helheim->InstanceType!=0)return 6;
    std::cout<<"map1463 outdoor type="<<unsigned(helheim->InstanceType)<<'\n';
    if(!lines.Load(argv[1],0))return 7;
    uint32 ids[]={3955,3957,3958,3959,3960},texts[]={98468,114753,98471,98472,98473};
    if(lines.LookupEntry(3956))return 8;
    for(unsigned i=0;i<5;++i){auto row=lines.LookupEntry(ids[i]);if(!row||row->BroadcastTextID!=texts[i]||row->NextConversationLineID!=(i==4?0:ids[i+1]))return 9;std::cout<<"line "<<row->ID<<" text="<<row->BroadcastTextID<<" next="<<row->NextConversationLineID<<'\n';}
    DB2Storage<SoundKitEntry> sounds("SoundKit.db2",SoundKitLoadInfo::Instance());
    if(!sounds.Load(argv[1],0))return 10;
    for(uint32 id:{53045u,53048u,53050u,53053u,53054u,53055u,69871u})if(!sounds.LookupEntry(id))return 11;
    std::cout<<"PASS: native five-line finale chain, removed3956, current114753 and all seven voice kits.\n";
    std::cout<<"PASS: native DB2Storage Faljar/Ashildir displays/models, copied spell records, detection/intro and outdoor Helheim. No rendered combat/cinematic certification.\n";
}
