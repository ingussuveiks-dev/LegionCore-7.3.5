#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include "VMapManager2.h"
#include "GridMap.h"
#include "ModelIgnoreFlags.h"
#include <cstdlib>
#include <cmath>
#include <filesystem>
#include <iostream>

int main(int argc, char** argv)
{
    if (argc != 2) return 1;
    std::string error;
    auto config = std::getenv("LEGION_DIALOGUE_CONFIG");
    if (!config || !sConfigMgr->LoadInitial(config, {}, error)) return 2;
    MySQL::Library_Init();
    HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo", ""), 1, 1);
    if (HotfixDatabase.Open() || !HotfixDatabase.PrepareStatements()) return 3;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto ro = HotfixDatabase.Query("SELECT @@session.transaction_read_only");
    if (!ro || !ro->Fetch()[0].GetBool()) return 4;
    DB2Storage<WorldSafeLocsEntry> safe("WorldSafeLocs.db2", WorldSafeLocsLoadInfo::Instance());
    if (!safe.Load(argv[1], 0)) return 5;
    safe.LoadFromDB();
    auto point = safe.LookupEntry(5710);
    if (!point || point->MapID != 1514) return 6;
    std::cout.precision(9);
    std::cout << "RETURN_SAFE\t" << point->ID << '\t' << point->MapID << '\t'
        << point->Loc.X << '\t' << point->Loc.Y << '\t' << point->Loc.Z << '\t' << point->Loc.O
        << '\t' << point->Name->Str[0] << '\n';
    auto root = std::filesystem::path(argv[1]).lexically_normal();
    if (root.filename().empty()) root = root.parent_path();
    root = root.parent_path().parent_path();
    VMAP::VMapManager2 vm;
    vm.InitializeThreadUnsafe({{1514, {}}});
    int tx = int(32 - point->Loc.X / 533.33333f), ty = int(32 - point->Loc.Y / 533.33333f);
    GridMap terrain;
    auto file = root / "maps" / ("1514_" + std::to_string(tx) + "_" + std::to_string(ty) + ".map");
    if (terrain.loadData(file.string().c_str()) != GridMap::LoadResult::Ok ||
        vm.loadMap((root / "vmaps").string().append("/").c_str(),1514,tx,ty) != VMAP::VMAP_LOAD_RESULT_OK) return 7;
    for (float dx : {-0.3f,0.f,0.3f}) for (float dy : {-0.3f,0.f,0.3f})
    {
        float x = point->Loc.X + dx, y = point->Loc.Y + dy, z = point->Loc.Z;
        float height = std::max(terrain.getHeight(x,y),vm.getHeight(1514,x,y,z+2.f,10.f));
        bool clear = vm.isInLineOfSight(1514,x,y,z+0.5f,x,y,z+2.f,VMAP::ModelIgnoreFlags::Nothing);
        std::cout << "RETURN_FLOOR\t" << x << '\t' << y << '\t' << height << '\t' << clear << '\n';
        if (!std::isfinite(height) || std::abs(z-height)>1.f || !clear) return 8;
    }
    std::cout << "EOF\tsheilun-return-landing\n";
    HotfixDatabase.Close();
    MySQL::Library_End();
}
