#ifndef STORMHEIM_LIFEBLOOD_H
#define STORMHEIM_LIFEBLOOD_H

class Player;
class GameObject;
namespace StormheimLifeblood
{
void Offer(Player* player, GameObject* altar);
bool Select(Player* player, GameObject* altar, unsigned menu, unsigned option);
}
#endif
