#ifndef WestfallStew_h__
#define WestfallStew_h__

#include "Define.h"
#include <list>

class Creature;
class Player;
class WorldObject;

namespace WestfallStew
{
constexpr uint32 Quest = 26271;
constexpr uint32 Item = 57991;
constexpr uint32 Offer = 79450;
constexpr uint32 FullBelly = 79451;
constexpr uint32 Pot = 42617;
constexpr uint32 SummonProperties = 2461;
constexpr uint32 SentinelHill = 108;
constexpr float FeedingRange = 15.0f;

bool IsActive(Player const* owner);
std::list<Creature*> FindHungryResidents(WorldObject const& source, Player const& owner);
}

#endif
