#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
int main(int argc, char** argv)
{
    if (argc != 2) return 1;
    DB2Storage<ConversationLineEntry> lines("ConversationLine.db2", ConversationLineLoadInfo::Instance());
    DB2Storage<BroadcastTextEntry> texts("BroadcastText.db2", BroadcastTextLoadInfo::Instance());
    if (!lines.Load(argv[1], 0) || !texts.Load(argv[1], 0)) return 2;
    auto line = lines.LookupEntry(5378);
    if (!line) { std::cout << "Missing line5378\n"; return 3; }
    std::cout << "5378: BroadcastTextID=" << line->BroadcastTextID << ", VisualKit=" << line->SpellVisualKitID
        << ", AnimKit=" << line->AnimKitID << ", SpeechType=" << unsigned(line->SpeechType) << '\n';
    if (line->BroadcastTextID != 98424) return 4;
    std::cout << "Base BroadcastText98424 present=" << bool(texts.LookupEntry(98424)) << "; effective Hotfix row checked by runner.\n";
    std::cout << "PASS: native sparse ConversationLine loader resolves Vethir conversation2549 line5378. Client playback remains pending.\n";
}
