#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "Player.h"
#include "TemporarySummon.h"
#include "ObjectAccessor.h"
#include "QuestData.h"
#include "CreatureTextMgr.h"

namespace UnexpectedAllies
{
constexpr uint32 QuestId = 39456;
Position const Start = {4123.89f,4475.31f,660.195f,1.0246f};
Position const Destination = {4083.05f,4388.5f,670.709f,4.71f};
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
                // ObjectID 99619 occurs twice: meeting and witnessing are different objectives.
                player->SetQuestObjectiveData(quest,&objective,1);
                player->SendQuestUpdateAddCredit(quest,guid,objective,1);
                if (player->CanCompleteQuest(QuestId)) player->CompleteQuest(QuestId);
                return;
            }
}
Creature* Existing(Player* player)
{
    auto guids = *player->GetSummonList(99624);
    for (auto const& guid : guids)
        if (Creature* npc = ObjectAccessor::GetCreature(*player,guid))
            if (Owner(npc) == player && npc->IsAlive() && !npc->IsDespawn() && npc->AI()->GetData(QuestId)) return npc;
    return nullptr;
}
}

struct npc_unexpected_allies_guide : ScriptedAI
{
    npc_unexpected_allies_guide(Creature* npc) : ScriptedAI(npc) { }
    void MoveInLineOfSight(Unit* who) override
    {
        if (Player* player = who->ToPlayer())
            if (UnexpectedAllies::Interact(player,me)) UnexpectedAllies::Credit(player,me->GetGUID(),280678);
    }
    void sGossipHello(Player* player) override { MoveInLineOfSight(player); }
    void sGossipSelect(Player* player, uint32 menu, uint32 option) override
    {
        if (menu != 18971 || option != 0 || !UnexpectedAllies::Interact(player,me) || UnexpectedAllies::Existing(player)) return;
        UnexpectedAllies::Credit(player,me->GetGUID(),280678);
        if (!UnexpectedAllies::Active(player)) return;
        player->PlayerTalkClass->SendCloseGossip();
        player->SummonCreature(99624,me->GetPositionX(),me->GetPositionY(),me->GetPositionZ(),me->GetOrientation(),
            TEMPSUMMON_TIMED_DESPAWN,300000,player->GetGUID());
    }
};

struct npc_unexpected_allies_ritual : ScriptedAI
{
    npc_unexpected_allies_ritual(Creature* npc) : ScriptedAI(npc) { }
    std::weak_ptr<uint8> lifetime;
    uint32 stage = 0, timer = 120000;
    bool arrived = false;
    uint32 GetData(uint32 id) const override { return id == UnexpectedAllies::QuestId && !lifetime.expired(); }
    void IsSummonedBy(Unit* summoner) override
    {
        Player* player = summoner->ToPlayer();
        if (!UnexpectedAllies::Active(player)) { me->DespawnOrUnsummon(); return; }
        player->AddQuestDelayedEvent(UnexpectedAllies::QuestId,0,[] { });
        lifetime = player->getQuestStatus(UnexpectedAllies::QuestId)->ScriptLifetime;
        me->AddPlayerInPersonnalVisibilityList(player->GetGUID());
        me->SetReactState(REACT_PASSIVE);
        me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
        me->SetWalk(true);
        sCreatureTextMgr->SendChat(me,0,player->GetGUID(),CHAT_MSG_MONSTER_WHISPER);
        me->GetMotionMaster()->MovePoint(1,UnexpectedAllies::Destination);
    }
    void MovementInform(uint32 type, uint32 id) override
    {
        if (!arrived && type == POINT_MOTION_TYPE && id == 1 && me->GetExactDist(&UnexpectedAllies::Destination) < 3.0f)
        { arrived = true; timer = 2000; }
    }
    void UpdateAI(uint32 diff) override
    {
        Player* player = UnexpectedAllies::Owner(me);
        if (lifetime.expired() || !UnexpectedAllies::Active(player) || !player->InSamePhase(me) ||
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
            case 0: sCreatureTextMgr->SendChat(me,1,player->GetGUID(),CHAT_MSG_MONSTER_WHISPER); break;
            case 1: sCreatureTextMgr->SendChat(me,2,player->GetGUID(),CHAT_MSG_MONSTER_WHISPER); break;
            case 2: sCreatureTextMgr->SendChat(mayla,50,player->GetGUID(),CHAT_MSG_MONSTER_WHISPER); break;
            case 3: sCreatureTextMgr->SendChat(mayla,51,player->GetGUID(),CHAT_MSG_MONSTER_WHISPER); break;
            case 4: sCreatureTextMgr->SendChat(mayla,52,player->GetGUID(),CHAT_MSG_MONSTER_WHISPER); break;
            case 5: sCreatureTextMgr->SendChat(me,3,player->GetGUID(),CHAT_MSG_MONSTER_WHISPER); break;
            case 6:
                UnexpectedAllies::Credit(player,me->GetGUID(),280680); // Hidden optional wait objective first.
                UnexpectedAllies::Credit(player,me->GetGUID(),280843); // Required witness objective completes the quest.
                me->DespawnOrUnsummon(); return;
        }
        timer = 5000;
    }
};

void AddSC_highmountain_unexpected_allies()
{
    RegisterCreatureAI(npc_unexpected_allies_guide);
    RegisterCreatureAI(npc_unexpected_allies_ritual);
}
