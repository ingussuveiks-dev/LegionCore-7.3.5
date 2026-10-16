#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    DB2Storage<CreatureDisplayInfoEntry> actors("CreatureDisplayInfo.db2",CreatureDisplayInfoLoadInfo::Instance());
    DB2Storage<CreatureModelDataEntry> models("CreatureModelData.db2",CreatureModelDataLoadInfo::Instance());
    DB2Storage<GameObjectDisplayInfoEntry> objects("GameObjectDisplayInfo.db2",GameobjectDisplayInfoLoadInfo::Instance());
    DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
    DB2Storage<ConversationLineEntry> lines("ConversationLine.db2",ConversationLineLoadInfo::Instance());
    DB2Storage<BroadcastTextEntry> texts("BroadcastText.db2",BroadcastTextLoadInfo::Instance());
    if(!actors.Load(argv[1],0)||!models.Load(argv[1],0)||!objects.Load(argv[1],0)||!spells.Load(argv[1],0)||!lines.Load(argv[1],0)||!texts.Load(argv[1],0))return 2;
    for(uint32 id:{65043u,328u,27823u,28213u}){auto a=actors.LookupEntry(id);if(!a)return 3;auto m=models.LookupEntry(a->ModelID);if(!m||!m->FileDataID)return 4;std::cout<<"display"<<id<<" model="<<a->ModelID<<" file="<<m->FileDataID<<'\n';}
    auto altar=objects.LookupEntry(7786);if(!altar||!altar->FileDataID)return 5;std::cout<<"altar7786 file="<<altar->FileDataID<<'\n';
    auto spell=spells.LookupEntry(212898);if(!spell)return 6;std::cout<<"spell212898 name="<<spell->Name->Str[0]<<'\n';
    for(uint32 id:{4747u,4748u}){auto l=lines.LookupEntry(id);if(!l||l->BroadcastTextID!=(id==4747?98487:98489))return 7;auto t=texts.LookupEntry(l->BroadcastTextID);if(!t){std::cout<<"line"<<id<<" base-only broadcast="<<l->BroadcastTextID<<" next="<<l->NextConversationLineID<<" visualKit="<<l->SpellVisualKitID<<" animKit="<<l->AnimKitID<<" additional="<<l->AdditionalDuration<<" speech="<<unsigned(l->SpeechType)<<"\n";continue;}std::cout<<"line"<<id<<" broadcast="<<l->BroadcastTextID<<" next="<<l->NextConversationLineID<<" visualKit="<<l->SpellVisualKitID<<" animKit="<<l->AnimKitID<<" text="<<t->Text->Str[0]<<" sound="<<t->SoundEntriesID[0]<<'\n';}
    DB2Storage<SoundKitEntry> sounds("SoundKit.db2",SoundKitLoadInfo::Instance());if(!sounds.Load(argv[1],0)||!sounds.LookupEntry(52998))return 9;
    std::cout<<"PASS: actual native DB2Storage altar/credit/Sylvanas/Helya actor models, conversation spell and lines4747/4748. Conversation playback remains a client check.\n";
}
