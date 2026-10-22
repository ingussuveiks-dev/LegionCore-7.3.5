#include <cassert>
#include <cstdint>
#include <iostream>
using uint32=uint32_t;
enum {GAMEOBJECT_TYPE_CHEST=3,GAMEOBJECT_TYPE_FISHINGHOLE=25,GAMEOBJECT_TYPE_GATHERING_NODE=50,GAMEOBJECT_TYPE_CHALLENGE_MODE_REWARD=51};
struct GameObjectTemplate {
    uint32 type=GAMEOBJECT_TYPE_CHEST;
    struct {uint32 chestLoot=0;} chest,fishingHole,gatheringNode,challengeModeReward;
    uint32 GetLootId() const;
};
#include "GetLootId.inc"
int main(){
    GameObjectTemplate chest;
    assert(chest.GetLootId()==0); // Native SendLoot skips FillLoot for ID zero.
    chest.chest.chestLoot=245479;
    assert(chest.GetLootId()==245479);
    GameObjectTemplate unrelated;
    unrelated.chest.chestLoot=2265;
    assert(unrelated.GetLootId()==2265);
    std::cout<<"PASS: production chest selector resolves Mara reward table after repair\n";
}
