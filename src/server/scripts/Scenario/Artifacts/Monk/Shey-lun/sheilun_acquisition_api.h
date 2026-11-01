#ifndef SHEILUN_ACQUISITION_API_H
#define SHEILUN_ACQUISITION_API_H

class Player;
class Creature;
class Unit;

namespace SheilunAcquisition
{
void Started(Player* player);
void BattleAlly(Creature* boss, Creature* ally);
void Defeated(Creature* boss, Unit* killer);
void Register();
}
#endif
