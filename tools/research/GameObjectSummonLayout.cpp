#include "MovementPackets.h"
#include "GameObject.h"
#include <cstddef>
#include <iostream>

struct GameObjectLayout : GameObject
{
    static std::size_t SummonerOffset() { return offsetof(GameObjectLayout, m_summonerGUID); }
};

int main()
{
    std::cout << "SUMMONER_OFFSET\t" << GameObjectLayout::SummonerOffset() << '\t' << sizeof(ObjectGuid) << '\n';
}
