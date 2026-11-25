#include <unordered_set>

#include "Config.h"
#include "DB2Stores.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "MMapFactory.h"
#include "MMapManager.h"
#include "Map.h"
#include "MapManager.h"
#include "MySQLThreading.h"
#include "ObjectMgr.h"
#include "VMapFactory.h"
#include "VMapManager2.h"

#include <cstdlib>
#include <iostream>

struct BaseMapsTag
{
    using Member = std::vector<Map*> MapManager::*;
    friend Member GetMember(BaseMapsTag);
};

template<class Tag, typename Tag::Member Value>
struct TestAccess
{
    friend typename Tag::Member GetMember(Tag) { return Value; }
};

template struct TestAccess<BaseMapsTag, &MapManager::i_maps>;

int main(int argc, char** argv)
{
    std::string error;
    auto config = std::getenv("LEGION_GRAVEYARD_TEST_CONFIG");
    if (argc != 2 || !config || !sConfigMgr->LoadInitial(config, {}, error))
        return 1;
    if (!sMapStore.Load(argv[1], LOCALE_enUS) || !sAreaTableStore.Load(argv[1], LOCALE_enUS) ||
        !sWorldSafeLocsStore.Load(argv[1], LOCALE_enUS))
        return 2;

    MySQL::Library_Init();
    WorldDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("WorldDatabaseInfo", ""), 0, 1);
    if (WorldDatabase.Open())
        return 3;
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    auto readOnly = WorldDatabase.Query("SELECT @@session.transaction_read_only");
    if (!readOnly || !readOnly->Fetch()[0].GetBool())
        return 4;
    sObjectMgr->LoadGraveyardZones();

    constexpr uint32 continent = 1220;
    static_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{continent, {}}});
    MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{continent, {}}});
    unsigned verified = 0;
    {
        Map map(continent, 300, 0, DIFFICULTY_NONE);
        auto& mapSlot = (sMapMgr->*GetMember(BaseMapsTag{})).at(continent);
        mapSlot = &map;
        struct Coast
        {
            float x, y;
            uint32 graveyard;
        };
        // Sea creature placements on the eastern Broken Shore and off Azsuna.
        for (auto coast : {Coast{-1360, 1432, 5975}, Coast{-1470, 5784, 5612}})
        {
            bool matched = false;
            std::map<uint32, unsigned> zones, results;
            for (int dx = -400; dx <= 400 && !matched; dx += 20)
            {
                for (int dy = -400; dy <= 400 && !matched; dy += 20)
                {
                    float x = coast.x + dx, y = coast.y + dy;
                    uint32 zone = map.GetZoneId(x, y, 0);
                    ++zones[zone];
                    if (zone != 7656)
                        continue;
                    auto horde = sObjectMgr->GetClosestGraveYard(x, y, 0, continent, HORDE);
                    auto alliance = sObjectMgr->GetClosestGraveYard(x, y, 0, continent, ALLIANCE);
                    ++results[horde ? horde->ID : 0];
                    if (horde && horde == alliance && horde->ID == coast.graveyard && horde->MapID == continent)
                    {
                        std::cout << "PASS native Great Sea terrain " << x << ' ' << y << " -> " << horde->ID << " for both factions\n";
                        ++verified;
                        matched = true;
                    }
                }
            }
            if (!matched)
            {
                std::cerr << "No sample selected graveyard " << coast.graveyard << ". Terrain zones:";
                for (auto const& zone : zones)
                    std::cerr << ' ' << zone.first << '=' << zone.second;
                std::cerr << ". Graveyards:";
                for (auto const& result : results)
                    std::cerr << ' ' << result.first << '=' << result.second;
                std::cerr << '\n';
            }
        }
        mapSlot = nullptr;
    }
    WorldDatabase.Close();
    MySQL::Library_End();
    return verified == 2 ? 0 : 5;
}
