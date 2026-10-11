#include "ScriptMgr.h"
#include "SmartAI.h"
#include "Player.h"
#include "TemporarySummon.h"
#include "ObjectAccessor.h"
#include "SpellScript.h"

namespace RockEscape
{
constexpr uint32 QuestId = 39381;
Position const Destination = {5435.1f,4497.69f,642.4f,3.467f};
Player* Owner(Creature* npc)
{
    TempSummon* summon = npc->ToTempSummon();
    Unit* owner = summon ? summon->GetSummoner() : nullptr;
    return owner ? owner->ToPlayer() : nullptr;
}
bool Active(Player* player)
{
    if (!player || !player->IsAlive() || player->GetMapId() != 1220 || player->isInFlight() || player->GetVehicle()) return false;
    auto status = player->GetQuestStatus(QuestId);
    // Native 190967 grants the TALK objective before the delayed teleport.
    // COMPLETE is therefore also a valid retry, until the quest is rewarded.
    return status == QUEST_STATUS_INCOMPLETE || status == QUEST_STATUS_COMPLETE;
}
bool CanStart(Player* player, Creature* npc)
{
    return Active(player) && !player->IsBeingTeleported() && player->GetAreaId() == 7807 &&
        Owner(npc) == player && npc->IsAlive() && !npc->IsDespawn() && player->InSamePhase(npc) &&
        player->canSeeOrDetect(npc) && player->IsWithinDistInMap(npc,6.0f) && player->IsWithinLOSInMap(npc);
}
void Reset(Player* player, ObjectGuid guid)
{
    if (Creature* npc = ObjectAccessor::GetCreature(*player,guid))
        if (Owner(npc) == player) npc->AI()->SetData(QuestId,0);
}
void Arrival(Player* player, ObjectGuid guid, uint32 remaining)
{
    if (!Active(player)) { Reset(player,guid); return; }
    if (!player->IsBeingTeleported() && player->GetExactDist(&Destination) <= 10.0f)
    {
        player->RemoveAurasDueToSpell(190967);
        if (Creature* npc = ObjectAccessor::GetCreature(*player,guid))
            if (Owner(npc) == player) npc->DespawnOrUnsummon();
        return;
    }
    if (!remaining) { Reset(player,guid); return; }
    player->AddQuestDelayedEvent(QuestId,100,[player,guid,remaining] { Arrival(player,guid,remaining-1); });
}
}

struct npc_rock_escape_navarrogg : SmartAI
{
    npc_rock_escape_navarrogg(Creature* creature) : SmartAI(creature) { }
    bool pending = false, departed = false;
    std::shared_ptr<uint8> attempt;
    std::weak_ptr<uint8> lifetime;
    void SetData(uint32 id, uint32 value) override
    {
        if (id == RockEscape::QuestId) { pending = value != 0; if (!pending) attempt.reset(); return; }
        SmartAI::SetData(id,value);
    }
    uint32 GetData(uint32 id) const override
    {
        if (id == RockEscape::QuestId) return pending && !departed && !lifetime.expired();
        return SmartAI::GetData(id);
    }
    void DoAction(int32 id) override
    {
        if (id != RockEscape::QuestId) { SmartAI::DoAction(id); return; }
        if (!GetData(RockEscape::QuestId)) return;
        Player* player = RockEscape::Owner(me);
        if (!RockEscape::CanStart(player,me)) { SetData(RockEscape::QuestId,0); return; }
        departed = true;
        if (!player->TeleportTo(1220,&RockEscape::Destination)) { SetData(RockEscape::QuestId,0); return; }
        RockEscape::Arrival(player,me->GetGUID(),50);
    }
    void IsSummonedBy(Unit* summoner) override
    {
        if (Player* player = summoner->ToPlayer())
        {
            // spell_area can cast the guide summon again when quest status changes.
            auto guids = *player->GetSummonList(96286);
            for (ObjectGuid const& guid : guids)
                if (Creature* other = ObjectAccessor::GetCreature(*player,guid))
                    if (other != me && RockEscape::Owner(other) == player && other->IsAlive() && !other->IsDespawn())
                    { me->DespawnOrUnsummon(); return; }
        }
        SmartAI::IsSummonedBy(summoner); // Preserve personal visibility, follow and conversation.
    }
    void sGossipSelect(Player* player, uint32 menu, uint32 option) override
    {
        if (menu != 18673 || option != 0) { SmartAI::sGossipSelect(player,menu,option); return; }
        if (pending && lifetime.expired()) pending = false;
        if (pending || !RockEscape::CanStart(player,me)) return;
        pending = true;
        departed = false;
        attempt = std::make_shared<uint8>();
        std::weak_ptr<uint8> currentAttempt = attempt;
        player->AddQuestDelayedEvent(RockEscape::QuestId,0,[] { });
        lifetime = player->getQuestStatus(RockEscape::QuestId)->ScriptLifetime;
        ObjectGuid guid = me->GetGUID();
        player->PlayerTalkClass->SendCloseGossip();
        player->CastSpell(player,190967,true);
        // The native aura tick normally dispatches this at 1200 ms. Retain a
        // fallback for a dispelled/missing aura, scoped to this exact interaction.
        player->AddQuestDelayedEvent(RockEscape::QuestId,3000,[player,guid,currentAttempt]
        {
            if (currentAttempt.expired()) return;
            if (Creature* npc = ObjectAccessor::GetCreature(*player,guid))
                if (RockEscape::Owner(npc) == player) npc->AI()->DoAction(RockEscape::QuestId);
        });
    }
};

// 7.3.5 effect 2 references absent spell 190982. Implement
// the quest's transport here instead of attempting to cast an absent ID.
// Effects 0/1/3 (personal credit, stun and 190983 presentation) stay native.
class aura_rock_escape_transport : public AuraScript
{
    PrepareAuraScript(aura_rock_escape_transport);
    void Tick(AuraEffect const*)
    {
        PreventDefaultAction();
        Player* player = GetTarget()->ToPlayer();
        if (!player || !RockEscape::Active(player)) return;
        auto guids = *player->GetSummonList(96286);
        for (ObjectGuid const& guid : guids)
            if (Creature* npc = ObjectAccessor::GetCreature(*player,guid))
                if (RockEscape::Owner(npc) == player && !npc->IsDespawn() && npc->AI()->GetData(RockEscape::QuestId))
                {
                    npc->AI()->DoAction(RockEscape::QuestId);
                    return;
                }
    }
    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(aura_rock_escape_transport::Tick,EFFECT_2,SPELL_AURA_PERIODIC_TRIGGER_SPELL);
    }
};

void AddSC_highmountain_rock_escape()
{
    RegisterCreatureAI(npc_rock_escape_navarrogg);
    RegisterAuraScript(aura_rock_escape_transport);
}
