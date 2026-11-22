#ifndef TRINITY_QUEST_ITEM_OBJECT_USE_H
#define TRINITY_QUEST_ITEM_OBJECT_USE_H

#include "Define.h"

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
