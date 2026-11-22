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
constexpr uint32 WovenBraid = 46314;
constexpr uint32 Weave = 65001;

bool CanGather(GameObject const& plant, Player const& player);
}

namespace SamophlangeManual
{
constexpr uint32 Quest = 14006;
constexpr uint32 LegacyQuest = 3924;
constexpr uint32 Cover = 11147;
constexpr uint32 Manual = 11149;
constexpr uint32 Assemble = 13714;
}

namespace ThelgenSeismograph
{
constexpr uint32 Quest = 25734;
constexpr uint32 Seismograph = 203184;
constexpr uint32 Spawn = 37153;
constexpr uint32 Record = 55242;

bool CanRead(GameObject const& seismograph, Player const& player);
}

namespace DarkmoonCrafting
{
constexpr uint16 Faire = 75;
constexpr uint32 Island = 974;

struct MaterialSource
{
    uint32 Object;
    uint32 Quest;
    uint32 Material;
    uint32 Result;
    uint32 Quantity;
};

MaterialSource const* FindMaterialSource(uint32 entry);
bool CanGather(GameObject const& object, Player const& player, MaterialSource const& source);
}

namespace DarkmoonBanner
{
constexpr uint32 Quest = 29520;
constexpr uint32 Kit = 72048;
constexpr uint32 FoldedBanner = 72049;
constexpr uint32 Sew = 101993;
constexpr uint32 Plant = 101992;
constexpr uint32 LooseStones = 209288;
constexpr uint32 RaisedBanner = 179965;
constexpr uint32 Credit = 54545;

bool IsPlantingSpot(GameObject const& object);
bool CanPlant(Player const& player);
}

namespace IronforgeBait
{
constexpr uint32 Quest = 29350;
constexpr uint32 Blueprint = 69979;
constexpr uint32 Assemble = 99474;
constexpr uint32 Trap = 208864;
constexpr uint32 TrapItem = 69980;

bool CanGather(GameObject const& trap, Player const& player);
}

namespace IronforgeRations
{
constexpr uint32 Quest = 29351;
constexpr uint32 Item = 69981;
constexpr uint32 Feed = 99479;
constexpr uint32 WellFed = 99478;
constexpr uint32 Guard = 5595;
constexpr uint32 Credit = 53564;
}

namespace LiberationDay
{
constexpr uint32 Quest = 24575;
constexpr uint32 Key = 49881;
constexpr uint32 OpenChain = 3366;
constexpr uint32 Chain = 201775;
constexpr uint32 Captive = 37694;
}

namespace IncendiciteOre
{
constexpr uint32 Quest = 25735;
constexpr uint32 Vein = 203188;
constexpr uint32 Pick = 55240;
constexpr uint32 Ore = 55241;
constexpr uint32 Mine = 77020;
}

namespace KneeDeep
{
constexpr uint32 Quest = 24678;
constexpr uint32 Torch = 50220;
constexpr uint32 Swing = 70631;

bool CanSwing(Player const* player);
bool IsTunnelVermin(Creature const* creature);
}

namespace KorothBanner
{
constexpr uint32 Quest = 24472;
constexpr uint32 Banner = 49742;
constexpr uint32 Swiped = 69858;

bool IsBanner(GameObject const& object);
bool CanLoot(GameObject const& object, Player const& player);
}

namespace WoodsEndCoffer
{
constexpr uint32 Quest = 24646;
constexpr uint32 Artifact = 50086;
constexpr uint32 Horn = 50134;
constexpr uint32 BlowHorn = 71061;
constexpr uint32 Rally = 71011;
constexpr uint32 SummonTracker = 71008;
constexpr uint32 Retreat = 71018;
constexpr uint32 WarStomp = 71019;
constexpr uint32 Ranger = 38022;
constexpr uint32 Tracker = 38027;
constexpr uint32 Tobias = 38029;

bool IsCoffer(GameObject const& object);
bool CanLoot(GameObject const& object, Player const& player);
bool IsAtCabin(Player const* player);
bool IsTobias(Creature const* creature);
Creature* FindTobias(Player const* player);
bool CanRally(Player* player);
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
    bool SharedSpell = false;
};

Binding const* ForObject(uint32 entry);
Binding const* ForSpell(uint32 spellId, uint32 itemEntry);
Item* FindUsableItem(Player const& player, Binding const& binding);
bool CanUse(GameObject const* object, Player const* player, Binding const& binding);
}

#endif
