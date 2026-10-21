#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
#include <map>
#include <utility>

int main(int argc, char** argv)
{
    if (argc != 2) return 1;
    DB2Storage<QuestLineEntry> lines("QuestLine.db2", QuestLineLoadInfo::Instance());
    DB2Storage<QuestLineXQuestEntry> quests("QuestLineXQuest.db2", QuestLineXQuestLoadInfo::Instance());
    if (!lines.Load(argv[1], 0) || !quests.Load(argv[1], 0)) return 2;
    std::map<int, std::pair<int, int>> expected{{3465, {247, 23}}, {3467, {207, 4}}, {3888, {274, 2}}};
    unsigned count = 0;
    for (auto q : quests)
        if (q->QuestID == 44089)
        {
            auto i = expected.find(q->ID);
            if (i == expected.end() || i->second != std::make_pair(int(q->QuestLineID), int(q->OrderIndex)) || !lines.LookupEntry(q->QuestLineID)) return 3;
            std::cout << "L\t" << q->ID << '\t' << q->QuestLineID << '\t' << q->QuestID << '\t' << int(q->OrderIndex) << '\n';
            ++count;
        }
    return count == expected.size() ? 0 : 4;
}
