#pragma once

class GameObject;
class Player;

namespace QuestLootChest
{
    bool IsUnavailable(GameObject const& object, Player* player);
}
