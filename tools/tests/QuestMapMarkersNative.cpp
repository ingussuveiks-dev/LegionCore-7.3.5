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

#ifdef CHECK_VISION_QUEST_POI
    auto map = sWorldMapAreaStore.LookupEntry(756);
    Require(map && map->MapID == 36 && map->AreaID == 1581, "The matching client map catalog must identify the Deadmines page");
    auto markers = sQuestDataStore->GetQuestPOIVector(26320);
    Require(markers && markers->size() == 4, "The native Vision map blobs must be present");
    uint32 dungeon = 0;
    uint32 returnToMarshal = 0;
    for (auto const& marker : *markers)
    {
        if (marker.points.size() != 1)
            continue;
        auto const& point = marker.points.front();
        if (point.X == -13 && point.Y == -381 && marker.MapID == 36 && marker.WorldMapAreaID == 756 && marker.Floor == 1 && marker.ObjectiveIndex == 0)
            ++dungeon;
        if (point.X == -10509 && point.Y == 1045 && marker.MapID == 0 && marker.WorldMapAreaID == 39 && marker.ObjectiveIndex == -1)
            ++returnToMarshal;
    }
    Require(dungeon == 2 && returnToMarshal == 1, "The focus belongs on Deadmines floor one and the return marker belongs in Westfall");
    std::cout << "CASE both Vision focus blobs lead to the Deadmines, and the Marshal marker stays in Westfall\n";
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
