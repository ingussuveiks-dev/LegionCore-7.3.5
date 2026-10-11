#include "ScriptMgr.h"
#include "SmartAI.h"
#include "Player.h"
#include "TemporarySummon.h"
#include "ObjectAccessor.h"

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
    bool pending = false;
    std::weak_ptr<uint8> lifetime;
    void SetData(uint32 id, uint32 value) override
    {
        if (id == RockEscape::QuestId) { pending = value != 0; return; }
        SmartAI::SetData(id,value);
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
        player->AddQuestDelayedEvent(RockEscape::QuestId,0,[] { });
        lifetime = player->getQuestStatus(RockEscape::QuestId)->ScriptLifetime;
        ObjectGuid guid = me->GetGUID();
        player->PlayerTalkClass->SendCloseGossip();
        player->CastSpell(player,190967,true);
        player->AddQuestDelayedEvent(RockEscape::QuestId,3000,[player,guid]
        {
            Creature* npc = ObjectAccessor::GetCreature(*player,guid);
            if (!npc || !RockEscape::CanStart(player,npc)) { RockEscape::Reset(player,guid); return; }
            // TeleportTo can defer movement until a player update/client acknowledgement.
            // Keep the guide until arrival is observed; failure must remain retryable.
            if (!player->TeleportTo(1220,&RockEscape::Destination)) { RockEscape::Reset(player,guid); return; }
            RockEscape::Arrival(player,guid,50);
        });
    }
};

void AddSC_highmountain_rock_escape()
{
    RegisterCreatureAI(npc_rock_escape_navarrogg);
}
