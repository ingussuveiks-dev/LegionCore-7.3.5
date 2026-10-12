#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "Player.h"
#include "TemporarySummon.h"
#include "ObjectAccessor.h"
#include "QuestData.h"
#include "CreatureTextMgr.h"

namespace SkyhornRitual
{
constexpr uint32 QuestId = 39387;
Position const Start = {4171.37f,4398.75f,660.976f,0.414413f};
Position const Destination = {4087.64f,4379.32f,670.626f,1.57f};
bool Active(Player* player)
{
    return player && player->IsAlive() && player->GetMapId() == 1220 &&
        player->GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE &&
        !player->IsBeingTeleported() && !player->isInFlight() && !player->IsFlying() && !player->GetVehicle();
}
Player* Owner(Creature* npc)
{
    auto summon = npc->ToTempSummon();
    auto owner = summon ? summon->GetSummoner() : nullptr;
    return owner ? owner->ToPlayer() : nullptr;
}
bool Interact(Player* player, Creature* npc)
{
    return Active(player) && npc->IsAlive() && !npc->IsDespawn() &&
        player->InSamePhase(npc) && player->canSeeOrDetect(npc) &&
        player->IsWithinDistInMap(npc,6.0f) && player->IsWithinLOSInMap(npc);
}
void Credit(Player* player, ObjectGuid guid, uint32 id)
{
    if (!Active(player)) return;
    if (Quest const* quest = sQuestDataStore->GetQuestTemplate(QuestId))
        for (auto const& objective : quest->GetObjectives())
            if (objective.ID == id && !player->GetQuestObjectiveData(quest,objective.StorageIndex))
            {
                // ObjectID 98773 occurs twice: meeting and witnessing are different objectives.
                player->SetQuestObjectiveData(quest,&objective,1);
                player->SendQuestUpdateAddCredit(quest,guid,objective,1);
                if (player->CanCompleteQuest(QuestId)) player->CompleteQuest(QuestId);
                return;
            }
}
Creature* Existing(Player* player)
{
    auto guids = *player->GetSummonList(98773);
    for (auto const& guid : guids)
        if (Creature* npc = ObjectAccessor::GetCreature(*player,guid))
            if (Owner(npc) == player && npc->IsAlive() && !npc->IsDespawn() && npc->AI()->GetData(QuestId)) return npc;
    return nullptr;
}
}

struct npc_skyhorn_ritual_ritual : ScriptedAI
{
    npc_skyhorn_ritual_ritual(Creature* npc) : ScriptedAI(npc) { }
    std::weak_ptr<uint8> lifetime;
    uint32 stage = 0, timer = 120000;
    bool arrived = false;
    uint32 GetData(uint32 id) const override { return id == SkyhornRitual::QuestId && !lifetime.expired(); }
    void IsSummonedBy(Unit* summoner) override
    {
        Player* player = summoner->ToPlayer();
        if (!SkyhornRitual::Active(player) || player->GetDistance(me) > 6.0f ||
            me->GetExactDist(&SkyhornRitual::Start) > 6.0f || SkyhornRitual::Existing(player))
        { me->DespawnOrUnsummon(); return; }
        player->AddQuestDelayedEvent(SkyhornRitual::QuestId,0,[] { });
        lifetime = player->getQuestStatus(SkyhornRitual::QuestId)->ScriptLifetime;
        me->AddPlayerInPersonnalVisibilityList(player->GetGUID());
        me->SetReactState(REACT_PASSIVE);
        me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
        me->SetUInt32Value(UNIT_FIELD_NPC_FLAGS,0);
        me->SetWalk(true);
        sCreatureTextMgr->SendChat(me,0,player->GetGUID(),CHAT_MSG_MONSTER_WHISPER);
        me->GetMotionMaster()->MovePoint(1,SkyhornRitual::Destination);
    }
    void MovementInform(uint32 type, uint32 id) override
    {
        if (!arrived && type == POINT_MOTION_TYPE && id == 1 && me->GetExactDist(&SkyhornRitual::Destination) < 3.0f)
        { arrived = true; timer = 2000; }
    }
    void UpdateAI(uint32 diff) override
    {
        Player* player = SkyhornRitual::Owner(me);
        if (!me->ToTempSummon()) return;
        if (lifetime.expired() || !SkyhornRitual::Active(player) || !player->InSamePhase(me) ||
            !player->canSeeOrDetect(me) || player->GetDistance(me) > 40.0f)
        { me->DespawnOrUnsummon(); return; }
        if (!arrived)
        {
            if (timer <= diff) me->DespawnOrUnsummon(); else timer -= diff;
            return;
        }
        Creature* mayla = me->FindNearestCreature(93826,10.0f,true);
        if (!mayla || !player->InSamePhase(mayla) || !player->canSeeOrDetect(mayla))
            mayla = me->FindNearestCreature(108434,10.0f,true);
        if (!mayla || !mayla->IsAlive() || !player->InSamePhase(mayla) || !player->canSeeOrDetect(mayla) ||
            player->GetDistance(me) > 15.0f || player->GetDistance(mayla) > 15.0f || !player->IsWithinLOSInMap(mayla))
        { me->DespawnOrUnsummon(); return; }
        if (timer > diff) { timer -= diff; return; }
        me->SetFacingToObject(mayla);
        switch (stage++)
        {
            case 0: sCreatureTextMgr->SendChat(me,3,player->GetGUID(),CHAT_MSG_MONSTER_WHISPER); break;
            case 1: sCreatureTextMgr->SendChat(me,1,player->GetGUID(),CHAT_MSG_MONSTER_WHISPER); break;
            case 2:
                sCreatureTextMgr->SendChat(me,2,player->GetGUID(),CHAT_MSG_MONSTER_WHISPER);
                me->HandleEmoteCommand(66); // Preserve the existing ritual's emote.
                break;
            case 3:
                SkyhornRitual::Credit(player,me->GetGUID(),280509); // Hidden optional wait objective first.
                SkyhornRitual::Credit(player,me->GetGUID(),280508); // Required witness objective completes the quest.
                me->DespawnOrUnsummon(); return;
        }
        timer = 5000;
    }
};

struct npc_skyhorn_ritual_guide : npc_skyhorn_ritual_ritual
{
    npc_skyhorn_ritual_guide(Creature* npc) : npc_skyhorn_ritual_ritual(npc) { }
    void MoveInLineOfSight(Unit* who) override
    {
        if (me->ToTempSummon()) return;
        if (Player* player = who->ToPlayer())
            if (SkyhornRitual::Interact(player,me)) SkyhornRitual::Credit(player,me->GetGUID(),280358);
    }
    void sGossipHello(Player* player) override { MoveInLineOfSight(player); }
    void sGossipSelect(Player* player, uint32 menu, uint32 option) override
    {
        if (me->ToTempSummon()) return;
        if (menu != 18944 || option != 0 || !SkyhornRitual::Interact(player,me) || SkyhornRitual::Existing(player)) return;
        SkyhornRitual::Credit(player,me->GetGUID(),280358);
        if (!SkyhornRitual::Active(player)) return;
        player->PlayerTalkClass->SendCloseGossip();
        player->SummonCreature(98773,me->GetPositionX(),me->GetPositionY(),me->GetPositionZ(),me->GetOrientation(),
            TEMPSUMMON_TIMED_DESPAWN,300000,player->GetGUID());
    }
};

void AddSC_highmountain_skyhorn_ritual()
{
    RegisterCreatureAI(npc_skyhorn_ritual_guide);
}
