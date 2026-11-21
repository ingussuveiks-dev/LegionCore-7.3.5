#ifndef ANOWYN_CRYSTAL_H
#define ANOWYN_CRYSTAL_H

#include "Define.h"

namespace AnowynCrystal
{
constexpr uint32 Quest = 9169;
constexpr uint32 Entry = 181359;
constexpr uint32 Orb = 23191;
constexpr uint32 Opening = 29049;
constexpr uint32 Lock = 1648;

constexpr bool IsQuestSpawn(uint32 map, uint64 spawn)
{
    return map == 530 && spawn == 12008;
}
}

#endif
