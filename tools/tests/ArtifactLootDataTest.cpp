#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
using uint32=uint32_t;
enum {GAMEOBJECT_TYPE_CHEST=3,GAMEOBJECT_TYPE_FISHINGHOLE=25,GAMEOBJECT_TYPE_GATHERING_NODE=50,GAMEOBJECT_TYPE_CHALLENGE_MODE_REWARD=51};
struct GameObjectTemplate {uint32 type=0;struct {uint32 chestLoot=0;}chest,fishingHole,gatheringNode,challengeModeReward;uint32 GetLootId()const;};
#include "GetLootId.inc"
int main(int argc,char**argv)
{
 assert(argc==2);std::ifstream file(argv[1]);assert(file);std::string line;int count=0;
 while(std::getline(file,line)){std::istringstream row(line);uint32 entry,type,loot,item;row>>entry>>type>>loot>>item;
  assert(row&&type==3&&loot==entry&&item);GameObjectTemplate go;go.type=type;go.chest.chestLoot=loot;
  assert(go.GetLootId()==entry);go.chest.chestLoot=0;assert(go.GetLootId()==0);++count;
 }
 assert(count==8);std::cout<<"PASS: eight real artifact rows resolve through native GameObjectTemplate::GetLootId\n";
}
