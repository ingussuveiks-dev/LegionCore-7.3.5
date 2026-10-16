#include "ScriptMgr.h"
#include "SmartAI.h"
#include "Player.h"
#include "QuestData.h"

namespace StormheimSalvation
{
constexpr uint32 QuestId = 40005;
Position const Departure = {2490.15f, 948.437f, 241.446f, 0.806109f};
Position const Arrival = {2488.31f, 945.13f, 596.224f, 3.88255f};
Position const Return = {2492.08f, 957.37f, 241.44f, 4.29f};
bool Definition()
{
    auto q = sQuestDataStore->GetQuestTemplate(QuestId);
    if (!q || q->StartScript || q->HasSpecialFlag(QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT)) return false;
    uint32 mask = 0;
    for (auto const& o : q->GetObjectives())
    {
        if (o.Type != QUEST_OBJECTIVE_MONSTER || o.Amount != 1) return false;
        if (o.ObjectID == 92307 && o.StorageIndex == 3 && o.Flags == QUEST_OBJECTIVE_FLAG_TRACKED_ON_MINIMAP && !(mask & 1)) mask |= 1;
        else if (o.ObjectID == 103766 && o.StorageIndex == 0 && o.Flags == 28 && !(mask & 2)) mask |= 2;
        else return false;
    }
    return mask == 3;
}
bool IsBase(Creature* c)
{
    return c && ((c->GetEntry() == 97986 && c->GetDBTableGUIDLow() == 338656) ||
        (c->GetEntry() == 98190 && c->GetDBTableGUIDLow() == 341218));
}
bool CanTravel(Player* p, Creature* c)
{
    bool returning = c && c->GetEntry() == 98190;
    return p && IsBase(c) && p->IsInWorld() && c->IsInWorld() && p->IsAlive() && c->IsAlive() &&
        p->GetMapId() == 1220 && c->GetMapId() == 1220 && !p->GetInstanceId() && !c->GetInstanceId() &&
        p->GetZoneId() == 7541 && c->GetZoneId() == 7541 && !p->HasPhaseId(8306) && !c->HasPhaseId(8306) &&
        c->GetExactDist(returning ? &Arrival : &Departure) < 1 && Definition() &&
        p->GetQuestStatus(QuestId) == (returning ? QUEST_STATUS_COMPLETE : QUEST_STATUS_INCOMPLETE) &&
        !p->IsBeingTeleported() && !p->isInCombat() && !p->isInFlight() && !p->GetVehicle() && !p->GetTransport() &&
        p->InSamePhase(c) && p->canSeeOrDetect(c) && p->IsWithinLOSInMap(c) &&
        p->GetNPCIfCanInteractWith(c->GetGUID(), UNIT_NPC_FLAG_GOSSIP) == c &&
        p->PlayerTalkClass->GetGossipMenu().GetMenuId() == (returning ? 18852 : 18843) && p->PlayerTalkClass->GetGossipMenu().GetItem(0);
}
}

struct npc_stormheim_salvation_vethir : SmartAI
{
    explicit npc_stormheim_salvation_vethir(Creature* c) : SmartAI(c) { }
    void sGossipSelect(Player* p, uint32 sender, uint32 action) override
    {
        using namespace StormheimSalvation;
        if (!IsBase(me)) { SmartAI::sGossipSelect(p, sender, action); return; }
        // Revalidate at selection, not only while preparing the gossip menu.
        // Preserve the installed platform teleport; this is not a vehicle flight.
        if (sender != (me->GetEntry() == 98190 ? 18852u : 18843u) || action != 0 || !CanTravel(p, me)) return;
        p->PlayerTalkClass->SendCloseGossip();
        auto const& destination = me->GetEntry() == 98190 ? Return : Arrival;
        p->TeleportTo(1220, destination.GetPositionX(), destination.GetPositionY(), destination.GetPositionZ(), destination.GetOrientation());
        // Neither a click nor a teleport attempt proves Skovald was defeated.
    }
};

void AddSC_stormheim_salvation()
{
    RegisterCreatureAI(npc_stormheim_salvation_vethir);
}
