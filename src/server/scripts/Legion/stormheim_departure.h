#pragma once

class Player;
class Creature;
class Quest;

namespace StormheimDeparture
{
    void Hello(Player* player, Creature* creature);
    bool Select(Player* player, Creature* creature, unsigned int menu, unsigned int option);
    bool Accept(Player* player, Creature* creature, Quest const* quest);
}
