#ifndef ScorpidVenom_h__
#define ScorpidVenom_h__

#include "Define.h"

class Player;

namespace ScorpidVenom
{
constexpr uint32 Quest = 25165;
constexpr uint32 Item = 52505;
constexpr uint32 Scorpid = 3125;
constexpr uint32 Totem = 39236;
constexpr uint32 Envenom = 73672;
constexpr uint32 PlaceTotem = 73673;
constexpr uint32 SummonProperties = 81;
constexpr float Range = 15.0f;

bool CanPlace(Player const* owner);
}

#endif
