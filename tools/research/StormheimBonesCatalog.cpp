#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    DB2Storage<ConversationLineEntry> lines("ConversationLine.db2",ConversationLineLoadInfo::Instance());
    DB2Storage<AnimKitEntry> kits("AnimKit.db2",AnimKitLoadInfo::Instance());
    if(!lines.Load(argv[1],0)||!kits.Load(argv[1],0))return 2;
    for(int id:{4740,4741,4742,4743})
    {
        auto line=lines.LookupEntry(id);if(!line)return 3;
        std::cout<<id<<" "<<line->BroadcastTextID<<" "<<line->SpellVisualKitID<<" "<<line->AdditionalDuration<<" "<<line->NextConversationLineID<<" "<<line->AnimKitID<<" "<<unsigned(line->SpeechType)<<"\n";
    }
    std::cout<<"invalid_anim_kit192171 "<<(kits.LookupEntry(192171)?1:0)<<"\n";
    return kits.LookupEntry(192171)?4:0;
}
