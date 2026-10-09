#ifndef DRAENOR_CAMPAIGN_RECOVERY_H
#define DRAENOR_CAMPAIGN_RECOVERY_H

#include <cmath>

namespace BloodmaulCampaign
{
    enum Quests { SearchForBwuja = 34309, OutOfTheChains = 34314, GearingUp = 34315, SeekingTruth = 34316, ShadowGate = 34381 };

    inline bool ShouldOfferSeeking(bool geared, bool seekingRewarded, bool seekingAbsent)
    {
        return geared && !seekingRewarded && seekingAbsent;
    }

    inline bool ShouldProvideReceiver(bool seekingActive, bool seekingRewarded, bool gateRewarded)
    {
        return seekingActive || (seekingRewarded && !gateRewarded);
    }

    inline bool AtGround(float playerZ, float groundZ, float invalidHeight)
    {
        return groundZ > invalidHeight && std::abs(playerZ - groundZ) <= 5.0f;
    }

    inline bool InsideBorgalPOI(float x, float y)
    {
        // Native quest_poi_points: 34316, Idx1=1. Keep the polygon rather than
        // crediting the entire cave/zone or a loose bounding rectangle.
        static float const points[][2] = {{7322,4996},{7344,4996},{7378,4996},{7400,5001},
            {7409,5031},{7409,5053},{7387,5075},{7365,5075},{7352,5075},{7331,5075},{7318,5044},{7318,5023}};
        bool inside = false;
        for (unsigned i = 0, j = 11; i < 12; j = i++)
            if ((points[i][1] > y) != (points[j][1] > y) &&
                x < (points[j][0] - points[i][0]) * (y - points[i][1]) / (points[j][1] - points[i][1]) + points[i][0])
                inside = !inside;
        return inside;
    }
}
#endif
