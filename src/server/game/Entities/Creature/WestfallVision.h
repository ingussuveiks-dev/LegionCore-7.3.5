#ifndef WESTFALL_VISION_H
#define WESTFALL_VISION_H

#include "Define.h"

class Player;

namespace WestfallVision
{
constexpr uint32 Quest = 26320;
constexpr uint32 Map = 36;
constexpr uint32 Incense = 58147;
constexpr uint32 Summon = 79586;
constexpr uint32 Ride = 79587;
constexpr uint32 Credit = 79620;
constexpr uint32 Camera = 42693;
constexpr uint32 Vehicle = 916;
constexpr uint32 Properties = 827;

bool CanStart(Player* player);
}

#endif
