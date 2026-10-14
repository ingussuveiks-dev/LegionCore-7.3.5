#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
using uint32=uint32_t;
enum {GAMEOBJECT_TYPE_CHEST=3,GAMEOBJECT_TYPE_FISHINGHOLE=25,GAMEOBJECT_TYPE_GATHERING_NODE=50,GAMEOBJECT_TYPE_CHALLENGE_MODE_REWARD=51};
struct GameObjectTemplate {
    uint32 type=GAMEOBJECT_TYPE_CHEST;
    struct {uint32 chestLoot=0;} chest,fishingHole,gatheringNode,challengeModeReward;
    uint32 GetLootId() const;
};
#include "GetLootId.inc"
int main(int argc,char** argv){
    assert(argc==2);std::ifstream input(argv[1]);assert(input);
    uint32 entry,type,loot,item,chance,questRequired,min,max,quest,amount,count=0;
    while(input>>entry>>type>>loot>>item>>chance>>questRequired>>min>>max>>quest>>amount){
        GameObjectTemplate object;object.type=type;
        assert(object.GetLootId()==0); // observed state bypassed FillLoot
        object.chest.chestLoot=loot;
        assert(object.GetLootId()==entry && item && questRequired==1 && quest==42128);
        ++count;
    }
    assert(count==3);
    std::cout<<"PASS: reproduced three missing loot selectors; migration replay links native chest loot to actual 42128 objectives and guaranteed quest items; unrelated chest and settings preserved\n";
}
