#ifndef BrazieBotany_h__
#define BrazieBotany_h__

#include "Define.h"

namespace BrazieBotany
{
constexpr uint32 BasicBotany = 28733;
constexpr uint32 FlowerPower = 28617;
constexpr uint32 GhoulsHateMyGrains = 28744;
constexpr uint32 Quests[] = {BasicBotany, FlowerPower, GhoulsHateMyGrains};
constexpr uint32 Map = 0;
constexpr uint32 Giver = 49687;
constexpr uint32 GiverSpawn = 131600;
constexpr uint32 Board = 49176;
constexpr uint32 SummonBoard = 91683;
constexpr uint32 BoardProperties = 3035;
constexpr uint32 SpitterModel = 207284;
constexpr uint32 SunflowerModel = 207282;
constexpr uint32 FreezyaModel = 207350;
constexpr uint32 RocknutModel = 207285;
constexpr uint32 StranglerModel = 207283;

inline bool IsLivingPlantModel(uint32 entry)
{
    return entry == SpitterModel || entry == SunflowerModel || entry == FreezyaModel ||
        entry == RocknutModel || entry == StranglerModel;
}
}

#endif
