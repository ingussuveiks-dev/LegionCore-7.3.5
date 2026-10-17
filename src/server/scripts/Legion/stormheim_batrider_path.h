#pragma once
#include "Position.h"
#include <array>

namespace StormheimBatRider
{
// Reconstructed short return flight, not a sniffed TaxiPath. The source is
// DB340420; the landing is native Forsaken Foothold taxi node1741. Keep the
// intermediate flight above the ridge and descend over the destination.
Position const Source{3764.07f,2918.25f,431.166f,2.37719f};
Position const Landing{3987.54f,2914.29f,241.257f,3.14f};
std::array<Position,4> const Flight{{
    {3764.07f,2918.25f,456.0f,0.0f},
    {4000.0f,2918.25f,456.0f,0.0f},
    {3987.54f,2914.29f,260.0f,0.0f},
    {3987.54f,2914.29f,241.257f,0.0f}
}};
}
