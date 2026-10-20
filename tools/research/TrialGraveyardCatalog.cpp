// Packed native DB2 reader; distinguish exits from internal checkpoints.
#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
#include <iomanip>
#include <string>

int main(int argc, char** argv)
{
    if (argc != 2) return 1;
#define STORE(T,N) DB2Storage<T##Entry> N(#T ".db2",T##LoadInfo::Instance()); if (!N.Load(argv[1],0)) return 2
    STORE(Map, maps);
    STORE(AreaTable, areas);
    STORE(WorldSafeLocs, locations);
#undef STORE
    auto raid = maps.LookupEntry(1648);
    auto zone = areas.LookupEntry(8440);
    auto exterior = locations.LookupEntry(5779);
    if (!raid || !zone || zone->ContinentID != 1648 || zone->ParentAreaID ||
        !exterior || exterior->MapID != 1220 ||
        std::string(exterior->Name->Str[0]) != "Helheim Raid - Exit Target - MRN") return 3;
    for (uint32 id : {5775u, 5839u, 5827u})
        if (!locations.LookupEntry(id) || locations.LookupEntry(id)->MapID != 1648) return 4;
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "RAID id=" << raid->ID << " name=" << raid->MapName->Str[0]
        << " corpseMap=" << raid->CorpseMapID << " corpse=" << raid->CorpsePos.X
        << ',' << raid->CorpsePos.Y << " parent=" << raid->ParentMapID << '\n';
    for (auto area : areas)
        if (area->ContinentID == 1648 || area->ID == 7541)
            std::cout << "AREA id=" << area->ID << " map=" << area->ContinentID
                << " parent=" << area->ParentAreaID << " faction=" << unsigned(area->FactionGroupMask)
                << " name=" << area->AreaName->Str[0] << '\n';
    for (auto location : locations)
    {
        float dx = location->Loc.X - exterior->Loc.X, dy = location->Loc.Y - exterior->Loc.Y;
        if (location->MapID == 1648 || (location->MapID == 1220 && dx*dx + dy*dy < 500*500))
            std::cout << "LOC id=" << location->ID << " map=" << location->MapID
                << " pos=" << location->Loc.X << ',' << location->Loc.Y << ',' << location->Loc.Z
                << " degrees=" << location->Loc.O << " name=" << location->Name->Str[0] << '\n';
    }
}
