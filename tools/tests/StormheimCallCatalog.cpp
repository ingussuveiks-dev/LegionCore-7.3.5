#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
#include <set>

int main(int argc, char** argv)
{
    if (argc != 2) return 1;
#define STORE(T, N, F, L) DB2Storage<T> N(F ".db2", L::Instance()); if (!N.Load(argv[1], 0)) return 2
    STORE(QuestLineXQuestEntry, links, "QuestLineXQuest", QuestLineXQuestLoadInfo);
    STORE(CreatureDisplayInfoEntry, displays, "CreatureDisplayInfo", CreatureDisplayInfoLoadInfo);
    STORE(CreatureModelDataEntry, models, "CreatureModelData", CreatureModelDataLoadInfo);
    STORE(AreaTableEntry, areas, "AreaTable", AreaTableLoadInfo);
#undef STORE
    std::set<unsigned> found;
    for (auto link : links)
        if (link->QuestID == 44720 || link->QuestID == 44771)
        {
            if (link->QuestLineID != 280 || link->OrderIndex != (link->QuestID == 44720 ? 0 : 1)) return 3;
            found.insert(link->QuestID);
            std::cout << "line " << link->QuestID << " " << link->QuestLineID << " " << unsigned(link->OrderIndex) << '\n';
        }
    if (found.size() != 2) return 4;
    for (unsigned id : {59729u, 76630u})
    {
        auto display = displays.LookupEntry(id);
        auto model = display ? models.LookupEntry(display->ModelID) : nullptr;
        if (!model || !model->FileDataID) return 5;
        std::cout << "display " << id << " model " << model->ID << " file " << model->FileDataID << '\n';
    }
    for (unsigned id : {7505u, 7803u})
    {
        auto area = areas.LookupEntry(id);
        if (!area || area->ContinentID != 1220) return 6;
        std::cout << "area " << id << " " << area->AreaName->Str[0] << '\n';
    }
    std::cout << "PASS: native quest-line order, both NPC display/model records and destination areas. Client pending.\n";
}
