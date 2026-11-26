#define LOST_ARMY_NATIVE_SUPPORT_ONLY
#include "LostArmyLootNative.cpp"

#ifdef CHECK_LEGACY_QUEST_POI
extern DB2Storage<QuestPOIBlobEntry> sQuestPOIBlobStore;
extern DB2Storage<QuestPOIPointEntry> sQuestPOIPointStore;
#endif

namespace
{
#ifdef CHECK_LEGACY_QUEST_POI
void CheckRestoredClientPoints()
{
    constexpr uint32 records[] = {
        421374, 421375, 421370, 421371, 421372, 421373, 422898, 422899,
        422900, 422901, 418539, 418540, 422939, 422940, 422941,
        57906, 61584, 345570, 407544, 422949, 422950, 422951, 422952,
        418516, 418517, 391652, 391653, 391654, 391655,
        419377, 419378, 419362, 419363, 423216,
    };
    std::unordered_map<int32, std::vector<QuestPOIPointEntry const*>> clientPoints;
    for (auto point : sQuestPOIPointStore)
        if (std::find(std::begin(records), std::end(records), point->ID) != std::end(records))
            clientPoints[point->ID].push_back(point);

    size_t total = 0;
    for (uint32 record : records)
    {
        auto client = sQuestPOIBlobStore.LookupEntry(record);
        Require(client && client->NumPoints && clientPoints[record].size() == client->NumPoints,
            "The matching client blob must have all its original points");
        auto serverBlobs = sQuestDataStore->GetQuestPOIVector(client->QuestID);
        Require(serverBlobs, "The restored existing quest must have server marker data");
        auto server = std::find_if(serverBlobs->begin(), serverBlobs->end(), [client](QuestPOI const& blob)
        {
            return blob.MapID == client->MapID && blob.WorldMapAreaID == client->WorldMapAreaID && blob.Floor == client->Floor &&
                blob.ObjectiveIndex == client->ObjectiveIndex && blob.PlayerConditionID == client->PlayerConditionID;
        });
        Require(server != serverBlobs->end() && server->points.size() == client->NumPoints,
            "The matching server blob must carry the complete coordinate set");
        for (size_t i = 0; i < server->points.size(); ++i)
            Require(server->points[i].X == clientPoints[record][i]->X && server->points[i].Y == clientPoints[record][i]->Y,
                "Every restored coordinate and its order must match the extracted client data");
        total += server->points.size();
    }
    Require(total == 56, "The restoration must account for all 56 exact client points");
    std::cout << "CASE 34 server marker groups from 13 quests match all 56 client coordinates and their order\n";
}
#endif

void CheckMapMarkers()
{
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
#ifdef CHECK_LEGACY_QUEST_POI
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE quest_poi AS SELECT * FROM legion_world.quest_poi WHERE QuestID IN "
        "(13841,13842,14375,14434,25804,25932,26118,26397,26398,27542,47709,49772,49933,26320,26520,50319)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE quest_poi_points AS SELECT * FROM legion_world.quest_poi_points WHERE QuestID IN "
        "(13841,13842,14375,14434,25804,25932,26118,26397,26398,27542,47709,49772,49933,26320,26520,50319)");
#else
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE quest_poi AS SELECT * FROM legion_world.quest_poi WHERE QuestID IN (26320,26520,50319)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE quest_poi_points AS SELECT * FROM legion_world.quest_poi_points WHERE QuestID IN (26320,26520,50319)");
#endif
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

#ifdef CHECK_LEGACY_QUEST_POI
    CheckRestoredClientPoints();
#endif

#ifdef CHECK_OSLOW_QUEST_POI
    auto redridge = sWorldMapAreaStore.LookupEntry(36);
    Require(redridge && redridge->MapID == 0 && redridge->AreaID == 44, "The matching client catalog must identify the Redridge page");
    auto oslow = sQuestDataStore->GetQuestPOIVector(26520);
    Require(oslow, "The two primary Foreman Oslow marker groups must load");
    bool rescue = false;
    bool turnIn = false;
    for (auto const& marker : *oslow)
    {
        if (marker.MapID != 0 || marker.WorldMapAreaID != 36 || marker.Floor != 0 || marker.points.size() != 1)
            continue;
        auto const& point = marker.points.front();
        if (marker.ObjectiveIndex == 0 && marker.QuestObjectiveID == 252598 && point.X == -9274 && point.Y == -2288)
            rescue = true;
        if (marker.ObjectiveIndex == -1 && point.X == -9219 && point.Y == -2222)
            turnIn = true;
    }
    Require(rescue && turnIn, "Redridge markers must lead to Foreman Oslow and Magistrate Solomon");
    std::cout << "CASE the actual Foreman and Magistrate marker points loaded on the Redridge page\n";
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
