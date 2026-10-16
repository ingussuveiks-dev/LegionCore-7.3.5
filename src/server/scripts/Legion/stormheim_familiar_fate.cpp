#include "ScriptMgr.h"
#include "SmartAI.h"
#include "SpellScript.h"
#include "Player.h"
#include "Item.h"
#include "QuestData.h"
#include "ObjectAccessor.h"
#include <list>
#include <set>

namespace StormheimFamiliarFate
{
constexpr uint32 QuestId = 40002, Shard = 129106, Collected = 97982, Freed = 97981;
Position const Circle = {2873.21f, 1207.08f, 188.763f, 5.55298f};
bool Definition()
{
    auto q = sQuestDataStore->GetQuestTemplate(QuestId);
    if (!q || q->StartScript || q->HasSpecialFlag(QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT)) return false;
    uint32 count = 0;
    for (auto const& o : q->GetObjectives())
    {
        if (o.Type != QUEST_OBJECTIVE_MONSTER || o.Amount != 1 || o.Flags ||
            !((o.ObjectID == Collected && o.StorageIndex == 0) || (o.ObjectID == Freed && o.StorageIndex == 1))) return false;
        ++count;
    }
    return count == 2;
}
bool Active(Player* p)
{
    return p && p->IsInWorld() && p->GetMapId() == 1220 && !p->GetInstanceId() && p->GetZoneId() == 7541 &&
        !p->HasPhaseId(8306) && p->IsAlive() && !p->IsBeingTeleported() && Definition() &&
        p->GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE;
}
void Credit(Player* p, uint32 id, ObjectGuid source)
{
    if (!Active(p) || p->GetQuestObjectiveData(QuestId, id)) return;
    auto q = sQuestDataStore->GetQuestTemplate(QuestId);
    for (auto const& o : q->GetObjectives()) if (o.ObjectID == id)
    {
        p->SetQuestObjectiveData(q, &o, 1);
        p->SendQuestUpdateAddCredit(q, source, o, 1);
        if (p->CanCompleteQuest(QuestId)) p->CompleteQuest(QuestId);
        break;
    }
}
void Reconcile(Player* p)
{
    // The native obtain spell runs while storing the item. A loaded bag can
    // be outside the world then; retry only once normal inventory is visible.
    // Possessing the real source item, never a party member's credit, is proof.
    if (Active(p) && !p->GetQuestObjectiveData(QuestId, Collected) && p->GetItemCount(Shard, false))
        Credit(p, Collected, p->GetGUID());
}
bool Identity(Creature* c)
{
    return c && c->IsInWorld() && c->GetEntry() == 91743 && c->GetDBTableGUIDLow() == 291393 &&
        c->GetMapId() == 1220 && !c->GetInstanceId() && c->GetZoneId() == 7541 && !c->HasPhaseId(8306) &&
        c->GetExactDist(&Circle) < 1;
}
bool CanRelease(Player* p, Creature* c)
{
    return Active(p) && Identity(c) && c->IsAlive() && c->HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP) &&
        !p->isInCombat() && !p->isInFlight() && !p->GetVehicle() && !p->GetTransport() &&
        p->InSamePhase(c) && p->canSeeOrDetect(c) && p->IsWithinLOSInMap(c) && c->IsWithinDistInMap(p, INTERACTION_DISTANCE) &&
        p->GetQuestObjectiveData(QuestId, Collected) == 1 && !p->GetQuestObjectiveData(QuestId, Freed) &&
        p->GetItemCount(Shard, false) && p->PlayerTalkClass->GetGossipMenu().GetMenuId() == 18828 &&
        p->PlayerTalkClass->GetGossipMenu().GetItem(0);
}
bool Released(Player* p)
{
    return p && p->IsInWorld() && p->GetMapId() == 1220 && !p->GetInstanceId() && p->GetZoneId() == 7541 &&
        !p->HasPhaseId(8306) && Definition() && (p->GetQuestRewardStatus(QuestId) ||
            ((p->GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE || p->GetQuestStatus(QuestId) == QUEST_STATUS_COMPLETE) &&
                p->GetQuestObjectiveData(QuestId, Freed) == 1));
}
}

struct npc_stormheim_familiar_binding : SmartAI
{
    explicit npc_stormheim_familiar_binding(Creature* c) : SmartAI(c) { }
    std::set<ObjectGuid> hidden;
    uint32 timer = 0;
    void Hide(Player* p)
    {
        if (!StormheimFamiliarFate::Released(p)) return;
        if (hidden.insert(p->GetGUID()).second)
        {
            me->AddToHideList(p->GetGUID());
            p->UpdateVisibilityOf(me);
        }
    }
    void sGossipSelect(Player* p, uint32 sender, uint32 action) override
    {
        using namespace StormheimFamiliarFate;
        if (!Identity(me)) { SmartAI::sGossipSelect(p, sender, action); return; }
        // The old action33 distributes release credit to the entire group and
        // does not recheck the item condition after the menu was opened.
        if (sender != 18828 || action != 0 || !CanRelease(p, me)) return;
        p->PlayerTalkClass->SendCloseGossip();
        Credit(p, Freed, me->GetGUID());
        Hide(p); // Only this player's binding disappears; other owners keep it.
        // Native quest reward/abandonment removes ItemDrop129106. Retain it
        // here, including after completion/reconnect, until that native cleanup.
        // Preserve the public Vethir pose; no unproven model animation is added.
    }
    void UpdateAI(uint32 diff) override
    {
        using namespace StormheimFamiliarFate;
        SmartAI::UpdateAI(diff);
        if (!Identity(me)) return;
        if (timer > diff) { timer -= diff; return; }
        timer = 500;
        // Restore on abandonment, logout/transfer or a new acceptance. A saved
        // completed/rewarded quest hides the same binding again on return.
        for (auto it = hidden.begin(); it != hidden.end();)
        {
            auto p = ObjectAccessor::GetPlayer(*me, *it);
            if (Released(p)) { ++it; continue; }
            me->RemoveFromHideList(*it);
            if (p) p->UpdateVisibilityOf(me);
            it = hidden.erase(it);
        }
        std::list<Player*> nearby;
        me->GetPlayerListInGrid(nearby, 80);
        for (auto p : nearby) Hide(p);
    }
};

class spell_stormheim_familiar_shard : public SpellScript
{
    PrepareSpellScript(spell_stormheim_familiar_shard);
    void Collect(SpellEffIndex index)
    {
        using namespace StormheimFamiliarFate;
        PreventHitDefaultEffect(index); // native134 is a party-credit dispatcher
        auto p = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        auto item = GetCastItem();
        if (Active(p) && GetHitUnit() == p && item && item->GetEntry() == Shard && item->GetOwnerGUID() == p->GetGUID())
            Reconcile(p);
    }
    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_stormheim_familiar_shard::Collect, EFFECT_0, SPELL_EFFECT_KILL_CREDIT2);
    }
};
class player_stormheim_familiar_fate : public PlayerScript
{
public:
    player_stormheim_familiar_fate() : PlayerScript("player_stormheim_familiar_fate") { }
    void OnUpdate(Player* p, uint32) override { StormheimFamiliarFate::Reconcile(p); }
};
void AddSC_stormheim_familiar_fate()
{
    RegisterCreatureAI(npc_stormheim_familiar_binding);
    RegisterSpellScript(spell_stormheim_familiar_shard);
    new player_stormheim_familiar_fate();
}
