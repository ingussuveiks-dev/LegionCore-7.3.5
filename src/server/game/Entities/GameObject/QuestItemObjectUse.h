#ifndef TRINITY_QUEST_ITEM_OBJECT_USE_H
#define TRINITY_QUEST_ITEM_OBJECT_USE_H

#include "Define.h"

class Creature;
class GameObject;
class Item;
class Player;

namespace StagnantOasis
{
constexpr uint32 Quest = 877;
constexpr uint32 Fissure = 3737;
constexpr uint32 FissureSpawn = 32492;
constexpr uint32 Seeds = 5068;
constexpr uint32 PlantSeeds = 5206;
}

namespace AmaniCatacombs
{
constexpr uint32 Quest = 9193;
constexpr uint32 Remains = 181148;
constexpr uint32 Torch = 22755;
constexpr uint32 BurnRemains = 28451;
constexpr uint32 Entrance = 4071;

void RestoreIncompleteQuest(Player& player);
}

namespace RavagerCage
{
constexpr uint32 Quest = 9582;
constexpr uint32 Cage = 181849;
constexpr uint32 CageSpawn = 7903;
constexpr uint32 Key = 23925;
constexpr uint32 OpenCage = 30767;
constexpr uint32 Ravager = 17556;
constexpr uint32 RavagerSpawn = 35799;
}

namespace ProphecyOfAkida
{
constexpr uint32 Quest = 9544;
constexpr uint32 Cage = 181714;
constexpr uint32 Captive = 17375;
constexpr uint32 Key = 23801;
constexpr uint32 OpenCage = 30406;
constexpr uint32 CaptiveReady = 1;

bool IsPlacedPair(GameObject const& cage, Creature const& captive);
Creature* FindCaptive(GameObject const& cage);
}

namespace SavingPrincessStillpine
{
constexpr uint32 Quest = 9667;
constexpr uint32 Cage = 181928;
constexpr uint32 CageSpawn = 19182;
constexpr uint32 Princess = 17682;
constexpr uint32 PrincessSpawn = 17225;
constexpr uint32 Key = 24099;
constexpr uint32 OpenCage = 31003;
constexpr uint32 ReadyForRescue = 1;

Creature* FindPrincess(GameObject const& cage);
}

namespace BloodcupBraid
{
constexpr uint32 Quest = 13875;
constexpr uint32 Plant = 194997;
constexpr uint32 Flower = 46315;
constexpr uint32 HairBraid = 46316;
constexpr uint32 Weave = 65001;

bool CanGather(GameObject const& plant, Player const& player);
}

namespace QuestItemObjectUse
{
struct Binding
{
    uint32 ObjectEntry;
    uint32 Spawn;
    uint32 MapId;
    uint32 AreaId;
    uint32 Quest;
    uint32 ItemEntry;
    uint32 Spell;
};

Binding const* ForObject(uint32 entry);
Binding const* ForSpell(uint32 spellId);
Item* FindUsableItem(Player const& player, Binding const& binding);
bool CanUse(GameObject const* object, Player const* player, Binding const& binding);
}

#endif
