#ifndef TRINITY_STAGNANT_OASIS_H
#define TRINITY_STAGNANT_OASIS_H

#include "Define.h"

class GameObject;
class Player;

namespace StagnantOasis
{
constexpr uint32 Quest = 877;
constexpr uint32 Fissure = 3737;
constexpr uint32 FissureSpawn = 32492;
constexpr uint32 Seeds = 5068;
constexpr uint32 PlantSeeds = 5206;

bool CanPlant(GameObject const* fissure, Player const* player);
}

#endif
