#define LOST_ARMY_NATIVE_SUPPORT_ONLY
#include "LostArmyLootNative.cpp"


namespace
{

void CheckMapMarkers()
{
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE quest_poi AS SELECT * FROM legion_world.quest_poi WHERE QuestID IN (26320,50319)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE quest_poi_points AS SELECT * FROM legion_world.quest_poi_points WHERE QuestID IN (26320,50319)");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sQuestDataStore->LoadQuestPOI();

#ifdef CHECK_HIGHEST_QUEST_POI
    auto highest = sQuestDataStore->GetQuestPOIVector(50319);
    Require(highest && highest->size() == 3, "The highest quest with point rows must retain all three real blobs");
    int32 const points[][2] = {{1601, -4372}, {1601, -4370}, {1564, -4192}};
    for (size_t i = 0; i < highest->size(); ++i)
    {
        auto const& marker = (*highest)[i];
        Require(marker.MapID == 1 && marker.WorldMapAreaID == 321 && marker.points.size() == 1 &&
            marker.points.front().X == points[i][0] && marker.points.front().Y == points[i][1],
            "A highest-quest map blob lost its real Orgrimmar position");
    }
    std::cout << "CASE all three live quest 50319 marker points loaded at the inclusive upper bound\n";
#endif


}
}

int main(int argc, char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    SetUnhandledExceptionFilter(CrashTrace);
    try
    {
        InitializeNative(argc, argv);
        CheckMapMarkers();
        HotfixDatabase.Close();
        WorldDatabase.Close();
        return 0;
    }
    catch (std::exception const& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
