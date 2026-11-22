#ifndef TRINITY_DOZERCISM_H
#define TRINITY_DOZERCISM_H

#include "Define.h"

class Creature;
class Player;
class WorldObject;

namespace Dozercism
{
constexpr uint32 Quest = 14423;
constexpr uint32 Flaregun = 49350;
constexpr uint32 Flare = 68007;
constexpr uint32 Event = 22429;
constexpr uint32 Priest = 36552;
constexpr uint32 Dozer = 35526;
constexpr uint32 DozerSpawn = 260725;
constexpr uint32 SummonProperties = 64;

bool IsEncounterActor(Creature const* actor);
bool IsAnchor(Creature const* actor);
Creature* FindAnchor(WorldObject const& source, float range);
Creature* FindActor(Player& owner, uint32 entry);
bool CanStart(Player* owner);
Player* GetOwner(Creature const& actor);
bool CanContinue(Creature const& actor);
bool CanParticipate(Creature const& actor, Player const* player);
}

#endif
