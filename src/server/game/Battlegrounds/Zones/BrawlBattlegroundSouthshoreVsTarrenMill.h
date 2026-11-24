#ifndef __BrawlBattlegroundSouthshoreVsTarrenMill
#define __BrawlBattlegroundSouthshoreVsTarrenMill

#include "Battleground.h"

class BrawlBattlegroundSouthshoreVsTarrenMill : public Battleground
{
    std::unordered_map<ObjectGuid, uint8> _honorableKills;
    Milliseconds _battleElapsed = Milliseconds::zero();
    void ResetRank(Player* player);
public:
    BrawlBattlegroundSouthshoreVsTarrenMill();

    void Reset() override;
    bool SetupBattleground() override;
    void PostUpdateImpl(uint32 diff) override;
    void AddPlayer(Player* player) override;
    void RemovePlayer(Player* player, ObjectGuid guid, uint32 team) override;
    WorldSafeLocsEntry const* GetClosestGraveYard(Player* player) override;
    void HandleKillPlayer(Player* player, Player* killer) override;
    bool UpdatePlayerScore(Player* player, uint32 type, uint64 value, bool doAddHonor = true) override;
};

#endif
