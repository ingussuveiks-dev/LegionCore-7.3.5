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
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE quest_poi AS SELECT * FROM legion_world.quest_poi WHERE QuestID IN (26320,26520,32470,32471,47430,50319)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE quest_poi_points AS SELECT * FROM legion_world.quest_poi_points WHERE QuestID IN (26320,26520,32470,32471,47430,50319)");
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
    Require(oslow, "The Foreman Oslow marker groups must load");
    if (sConfigMgr->GetBoolDefault("NativeOslowMarkers.ExpectMissing", false))
    {
        Require(oslow->size() == 7 && std::count_if(oslow->begin(), oslow->end(), [](QuestPOI const& marker)
        {
            return marker.points.empty();
        }) == 4, "The original state must reproduce the missing coordinate groups and obsolete aliases");
        std::cout << "EXPECTED_BROKEN four empty Redridge blobs and a duplicated rescue point\n";
    }
    else
    {
        struct Marker
        {
            int32 Blob;
            int32 Objective;
            int32 QuestObjective;
            int32 Actor;
            int32 Tracking;
            QuestPOIPoint Point;
        };
        Marker const expected[] = {
            {0, -1, 0, 0, 11915, {-9219, -2222}},
            {0, 0, 252598, 341, 431377, {-9274, -2288}},
            {1, 0, 252598, 341, 430276, {-9113, -2064}},
            {2, 0, 252598, 341, 430277, {-9112, -2353}},
            {0, 32, 0, 0, 431539, {-8833, -1937}},
        };
        Require(oslow->size() == std::size(expected), "Only the five Legion groups may be published");
        for (auto const& target : expected)
        {
            auto matches = std::count_if(oslow->begin(), oslow->end(), [&target](QuestPOI const& marker)
            {
                return marker.MapID == 0 && marker.WorldMapAreaID == 36 && marker.Floor == 0 && marker.Flags == 0 &&
                    marker.BlobIndex == target.Blob && marker.ObjectiveIndex == target.Objective &&
                    marker.QuestObjectiveID == target.QuestObjective && marker.QuestObjectID == target.Actor &&
                    marker.SpawnTrackingID == target.Tracking && marker.points.size() == 1 &&
                    marker.points.front().X == target.Point.X && marker.points.front().Y == target.Point.Y;
            });
            Require(matches == 1, "Each original Redridge coordinate must retain its tracking ID and objective");
        }
        std::cout << "CASE all five Legion Redridge marker groups loaded with their original tracking IDs\n";
    }
#endif

#ifdef CHECK_MOONKIN_QUEST_POI
    auto moonglade = sWorldMapAreaStore.LookupEntry(241);
    Require(moonglade && moonglade->MapID == 1 && moonglade->AreaID == 493,
        "The matching client catalog must identify the Moonglade map page");
    auto markers = sQuestDataStore->GetQuestPOIVector(47430);
    if (sConfigMgr->GetBoolDefault("NativeMoonkinMarkers.ExpectMissing", false))
    {
        Require(!markers || markers->empty(), "The old Moonkin Monitoring data must reproduce missing map points");
        std::cout << "EXPECTED_BROKEN no loadable points for the four Moonkin Monitoring actors\n";
    }
    else
    {
        struct Marker
        {
            int32 Objective;
            int32 QuestObjective;
            int32 Actor;
            int32 X;
            int32 Y;
        };
        Marker const expected[] = {
            {-1, 0, 0, 7535, -2428},
            {0, 290339, 122241, 7513, -2429},
            {1, 290344, 122256, 7467, -2360},
            {2, 290345, 122390, 7415, -2414},
        };
        Require(markers, "The Moonkin Monitoring markers must load");
        Require(std::count_if(markers->begin(), markers->end(), [](QuestPOI const& marker) { return !marker.points.empty(); }) ==
            std::size(expected), "All four Moonkin Monitoring points must load");
        for (auto const& target : expected)
        {
            auto count = std::count_if(markers->begin(), markers->end(), [&target](QuestPOI const& marker)
            {
                if (marker.MapID != 1 || marker.WorldMapAreaID != 241 || marker.Floor != 0 || marker.points.size() != 1)
                    return false;
                auto const& point = marker.points.front();
                return marker.ObjectiveIndex == target.Objective && marker.QuestObjectiveID == target.QuestObjective &&
                    marker.QuestObjectID == target.Actor && point.X == target.X && point.Y == target.Y;
            });
            Require(count == 1, "Each Moonglade objective and turn-in marker must match its original actor");
        }
        std::cout << "CASE all three Moonkin Monitoring objective markers and the return marker loaded on the Moonglade page\n";
    }
#endif

#ifdef CHECK_CAMERA_QUEST_POI
    struct Route
    {
        uint32 Quest;
        int32 Map;
        int32 Page;
        int32 Area;
        int32 BoxObjective;
        int32 BeltObjective;
        QuestPOIPoint Giver;
        QuestPOIPoint Box;
        QuestPOIPoint Belt;
    };
    Route const routes[] = {
        {32470, 0, 301, 1519, 269882, 269884, {-8259, 657}, {-8773, 540}, {-8946, 799}},
        {32471, 1, 321, 1637, 669882, 669884, {1665, -4108}, {2245, -4614}, {1791, -4566}},
    };
    bool expectMissing = sConfigMgr->GetBoolDefault("NativeCameraMarkers.ExpectMissing", false);
    for (auto const& route : routes)
    {
        auto mapPage = sWorldMapAreaStore.LookupEntry(route.Page);
        Require(mapPage && mapPage->MapID == route.Map && mapPage->AreaID == route.Area, "The matching faction capital map page is required");
        auto markers = sQuestDataStore->GetQuestPOIVector(route.Quest);
        if (expectMissing)
        {
            Require(!markers || std::none_of(markers->begin(), markers->end(), [](QuestPOI const& marker) { return !marker.points.empty(); }),
                "The old capital data must reproduce missing map points");
            continue;
        }
        Require(markers, "The faction route must have its own map markers");
        auto hasMarker = [&](int32 index, int32 objective, int32 object, QuestPOIPoint const& position)
        {
            return std::count_if(markers->begin(), markers->end(), [&](QuestPOI const& marker)
            {
                return marker.MapID == route.Map && marker.WorldMapAreaID == route.Page && marker.Floor == 0 &&
                    marker.ObjectiveIndex == index && marker.QuestObjectiveID == objective && marker.QuestObjectID == object &&
                    marker.points.size() == 1 && marker.points.front().X == position.X && marker.points.front().Y == position.Y;
            }) == 1;
        };
        Require(hasMarker(-1, 0, 0, route.Giver) && hasMarker(0, route.BoxObjective, 155856, route.Box) &&
            hasMarker(2, route.BeltObjective, 3593, route.Belt), "Giver, loot and vendor markers must match this faction's real sources");
    }
    std::cout << (expectMissing ? "EXPECTED_BROKEN both camera quests are missing map points\n"
        : "CASE both camera routes have their own real giver, loot and vendor markers\n");
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
