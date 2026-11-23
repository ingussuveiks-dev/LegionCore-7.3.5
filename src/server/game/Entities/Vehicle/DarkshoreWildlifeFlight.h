#ifndef DarkshoreWildlifeFlight_h__
#define DarkshoreWildlifeFlight_h__

#include "Define.h"

namespace DarkshoreWildlifeFlight
{
constexpr uint32 QuestId = 13885;
constexpr uint32 Map = 1;
constexpr uint32 Orseus = 34392;
constexpr uint32 OrseusSpawn = 122516;
constexpr uint32 Hippogryph = 34375;
constexpr uint32 Kathrena = 34376;
constexpr uint32 Summon = 65372;
constexpr uint32 SummonProperties = 64;
constexpr uint32 ProtectWildlife = 65373;
constexpr int8 RiderSeat = 0;
constexpr int8 KathrenaSeat = 1;

inline uint32 CreditForWildlife(uint32 entry)
{
    switch (entry)
    {
        case 2165:
            return 34323;
        case 34318:
        case 34396:
            return 34324;
        case 2070:
        case 2071:
        case 2237:
            return 34325;
        default:
            return 0;
    }
}
}

#endif
