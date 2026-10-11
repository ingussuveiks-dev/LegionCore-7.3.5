#include "ScriptMgr.h"
#include "SmartAI.h"
#include "Player.h"
#include "TemporarySummon.h"
#include "ObjectAccessor.h"
#include <map>
#include <mutex>

namespace BloodDebt
{
Player* Owner(Creature* npc)
{
    TempSummon* summon = npc->ToTempSummon();
    Unit* owner = summon ? summon->GetSummoner() : nullptr;
    return owner ? owner->ToPlayer() : nullptr;
}
bool Wanted(Player* player)
{
    if (!player || player->GetMapId() != 1220 || player->GetQuestStatus(40229) != QUEST_STATUS_NONE) return false;
    auto status = player->GetQuestStatus(39426);
    return status == QUEST_STATUS_INCOMPLETE || status == QUEST_STATUS_COMPLETE || status == QUEST_STATUS_REWARDED;
}
bool InArea(Player* player)
{
    auto area = player->GetAreaId();
    return area == 7725 || area == 7810 || area == 7829;
}
Creature* Existing(Player* player, bool clear = false)
{
    auto guids = *player->GetSummonList(96453);
    for (ObjectGuid const& guid : guids)
        if (Creature* npc = ObjectAccessor::GetCreature(*player,guid))
            if (Owner(npc) == player)
            {
                if (clear) npc->DespawnOrUnsummon();
                else if (npc->IsAlive() && !npc->IsDespawn()) return npc;
            }
    return nullptr;
}
void Recover(Player* player)
{
    if (Wanted(player) && InArea(player) && player->IsAlive() && !player->IsBeingTeleported() &&
        !player->isInFlight() && !player->GetVehicle() && !Existing(player))
        player->CastSpell(player,191109,true); // Native personal Navarrogg, including summon properties.
}
}

struct npc_blood_debt_navarrogg : SmartAI
{
    npc_blood_debt_navarrogg(Creature* npc) : SmartAI(npc) { }
    uint32 checkTimer = 1000;
    void IsSummonedBy(Unit* summoner) override
    {
        Player* player = summoner->ToPlayer();
        if (!BloodDebt::Wanted(player)) { me->DespawnOrUnsummon(); return; }
        // spell_area and quest acceptance both cast the non-aura summon.
        auto guids = *player->GetSummonList(96453);
        for (ObjectGuid const& guid : guids)
            if (Creature* other = ObjectAccessor::GetCreature(*player,guid))
                if (other != me && BloodDebt::Owner(other) == player && other->IsAlive() && !other->IsDespawn())
                { me->DespawnOrUnsummon(); return; }
        SmartAI::IsSummonedBy(summoner); // Personal visibility, follow, combat and quest handoff remain in SmartAI.
    }
    void UpdateAI(uint32 diff) override
    {
        if (checkTimer <= diff)
        {
            checkTimer = 1000;
            if (!BloodDebt::Wanted(BloodDebt::Owner(me))) { me->DespawnOrUnsummon(); return; }
        }
        else checkTimer -= diff;
        SmartAI::UpdateAI(diff);
    }
};

class player_blood_debt_navarrogg : public PlayerScript
{
    std::mutex mutex;
    std::map<ObjectGuid,uint32> timers;
public:
    player_blood_debt_navarrogg() : PlayerScript("player_blood_debt_navarrogg") { }
    void OnLogin(Player* player) override { BloodDebt::Recover(player); }
    void OnLogout(Player* player) override
    {
        { std::lock_guard<std::mutex> lock(mutex); timers.erase(player->GetGUID()); }
        BloodDebt::Existing(player,true);
    }
    void OnMapChanged(Player* player) override { OnLogout(player); BloodDebt::Recover(player); }
    void OnUpdate(Player* player, uint32 diff) override
    {
        if (!BloodDebt::Wanted(player))
        {
            bool serviced;
            { std::lock_guard<std::mutex> lock(mutex); serviced = timers.erase(player->GetGUID()) != 0; }
            if (serviced) BloodDebt::Existing(player,true);
            return;
        }
        {
            std::lock_guard<std::mutex> lock(mutex);
            auto& timer = timers[player->GetGUID()];
            if (timer > diff) { timer -= diff; return; }
            timer = 5000;
        }
        BloodDebt::Recover(player);
    }
};

void AddSC_highmountain_blood_debt()
{
    RegisterCreatureAI(npc_blood_debt_navarrogg);
    new player_blood_debt_navarrogg();
}
