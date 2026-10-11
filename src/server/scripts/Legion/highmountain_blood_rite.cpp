#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "CombatAI.h"
#include "Player.h"
#include "TemporarySummon.h"
#include "ObjectAccessor.h"
#include "QuestData.h"
#include "CreatureTextMgr.h"

namespace BloodRite
{
constexpr uint32 QuestId = 39860;
Position const PortalPosition = {5103.12f,5073.15f,627.398f,5.72733f};
bool Active(Player* player)
{
    return player && player->IsAlive() && player->GetMapId() == 1220 &&
        !player->IsBeingTeleported() && !player->isInFlight() && !player->GetVehicle() &&
        player->GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE;
}
Player* Owner(Creature* creature)
{
    TempSummon* summon = creature->ToTempSummon();
    Unit* owner = summon ? summon->GetSummoner() : nullptr;
    return owner ? owner->ToPlayer() : nullptr;
}
Creature* Existing(Player* player, uint32 entry)
{
    auto guids = *player->GetSummonList(entry);
    for (ObjectGuid const& guid : guids)
        if (Creature* creature = ObjectAccessor::GetCreature(*player,guid))
            if (Owner(creature) == player && creature->IsAlive() && !creature->IsDespawn()) return creature;
    return nullptr;
}
void ClearEntry(Player* player, uint32 entry)
{
    auto guids = *player->GetSummonList(entry);
    for (ObjectGuid const& guid : guids)
        if (Creature* creature = ObjectAccessor::GetCreature(*player,guid))
            if (Owner(creature) == player) creature->DespawnOrUnsummon();
}
void ReturnGuide(Player* player)
{
    if (!player || !player->IsAlive() || player->GetMapId() != 1220 || player->GetAreaId() != 7807 ||
        player->IsBeingTeleported()) return;
    auto status = player->GetQuestStatus(QuestId);
    auto next = player->GetQuestStatus(39381);
    if ((status == QUEST_STATUS_COMPLETE || (status == QUEST_STATUS_REWARDED &&
        (next == QUEST_STATUS_NONE || next == QUEST_STATUS_INCOMPLETE))) && !Existing(player,96286))
        player->CastSpell(player,192536,true); // Native personal Navarrogg, existing turn-in/escape SmartAI.
}
void Begin(Player* player, Creature* navarrogg)
{
    if (!Active(player) || !player->InSamePhase(navarrogg) || !player->canSeeOrDetect(navarrogg) ||
        !player->IsWithinDistInMap(navarrogg,6.0f) || !player->IsWithinLOSInMap(navarrogg) ||
        Existing(player,97418)) return;
    Creature* marker = navarrogg->FindNearestCreature(97675,20.0f,true);
    if (!marker || !player->InSamePhase(marker)) return;
    for (uint32 entry : {97675u,97440u}) ClearEntry(player,entry);
    player->CastSpell(player,190902,true); // Restore Oakin after death/relog before starting the scene.
    if (!Existing(player,97440)) return;
    Creature* portal = player->SummonCreature(97675,marker->GetPositionX(),marker->GetPositionY(),marker->GetPositionZ(),marker->GetOrientation(),
        TEMPSUMMON_TIMED_DESPAWN,600000,player->GetGUID());
    if (!portal) return;
    portal->CastSpell(portal,192682,true);
    Creature* boss = player->SummonCreature(97418,marker->GetPositionX(),marker->GetPositionY(),marker->GetPositionZ(),marker->GetOrientation(),
        TEMPSUMMON_TIMED_DESPAWN,600000,player->GetGUID());
    if (!boss) { portal->DespawnOrUnsummon(); return; }
    sCreatureTextMgr->SendChat(navarrogg,0,player->GetGUID(),CHAT_MSG_MONSTER_WHISPER);
    if (!player->GetQuestObjectiveData(QuestId,95259))
        if (Quest const* quest = sQuestDataStore->GetQuestTemplate(QuestId))
            for (QuestObjective const& objective : quest->GetObjectives())
                if (objective.ObjectID == 95259)
                {
                    player->SetQuestObjectiveData(quest,&objective,1);
                    player->SendQuestUpdateAddCredit(quest,navarrogg->GetGUID(),objective,1);
                    break;
                }
}
}

struct npc_blood_rite_navarrogg : ScriptedAI
{
    npc_blood_rite_navarrogg(Creature* creature) : ScriptedAI(creature) { }
    void MoveInLineOfSight(Unit* who) override
    {
        if (Player* player = who->ToPlayer())
        {
            if (!player->IsWithinDistInMap(me,6.0f) || !player->InSamePhase(me) || !player->canSeeOrDetect(me)) return;
            BloodRite::ReturnGuide(player);
            BloodRite::Begin(player,me);
        }
    }
};

struct npc_blood_rite_marakhan : AggressorAI
{
    npc_blood_rite_marakhan(Creature* creature) : AggressorAI(creature) { }
    std::weak_ptr<uint8> lifetime;
    ObjectGuid portal, oakin;
    uint32 stage = 0, timer = 1500;
    bool fighting = false;
    bool Valid(Player* player) const
    {
        return !lifetime.expired() && BloodRite::Active(player) && player->InSamePhase(me) &&
            player->GetDistance(me) <= 60.0f && me->GetExactDist(&BloodRite::PortalPosition) <= 60.0f;
    }
    void RemoveActor(ObjectGuid guid)
    {
        if (Creature* actor = ObjectAccessor::GetCreature(*me,guid)) actor->DespawnOrUnsummon();
    }
    void Cancel() { RemoveActor(portal); RemoveActor(oakin); me->DespawnOrUnsummon(); }
    void IsSummonedBy(Unit* summoner) override
    {
        Player* player = summoner->ToPlayer();
        if (!BloodRite::Active(player)) { me->DespawnOrUnsummon(); return; }
        player->AddQuestDelayedEvent(BloodRite::QuestId,0,[] { });
        lifetime = player->getQuestStatus(BloodRite::QuestId)->ScriptLifetime;
        if (Creature* actor = BloodRite::Existing(player,97675)) portal = actor->GetGUID();
        if (Creature* actor = BloodRite::Existing(player,97440)) oakin = actor->GetGUID();
        me->AddPlayerInPersonnalVisibilityList(player->GetGUID());
        me->SetNoCallAssistance(true);
        me->m_CanCallAssistance = false;
        me->SetReactState(REACT_PASSIVE);
        me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
        me->SetVisible(false);
    }
    bool CanAIAttack(Unit const* target) const override
    {
        return fighting && target && BloodRite::Owner(me) &&
            target->GetCharmerOrOwnerPlayerOrPlayerItself() == BloodRite::Owner(me);
    }
    void MoveInLineOfSight(Unit*) override { }
    void EnterEvadeMode() override { Cancel(); }
    void DamageTaken(Unit* attacker, uint32& damage, DamageEffectType) override
    {
        Player* player = BloodRite::Owner(me);
        if (!fighting || !Valid(player) || !attacker || attacker->GetCharmerOrOwnerPlayerOrPlayerItself() != player)
            damage = 0;
    }
    void JustDied(Unit*) override
    {
        RemoveActor(portal);
        if (Creature* actor = ObjectAccessor::GetCreature(*me,oakin)) actor->DespawnOrUnsummon(30000);
        // KillRewarder grants native monster credit on a 50 ms player event.
        // Keep the corpse available and recover the guide after that event.
        if (Player* player = BloodRite::Owner(me))
            player->AddQuestDelayedEvent(BloodRite::QuestId,100,[player] { BloodRite::ReturnGuide(player); });
        me->DespawnOrUnsummon(30000);
    }
    void UpdateAI(uint32 diff) override
    {
        Player* player = BloodRite::Owner(me);
        if (!Valid(player)) { Cancel(); return; }
        if (fighting) { AggressorAI::UpdateAI(diff); return; }
        if (timer > diff) { timer -= diff; return; }
        switch (stage++)
        {
            case 0: me->SetVisible(true); timer = 2000; break;
            case 1: Talk(0,player->GetGUID()); timer = 2500; break;
            case 2:
                // Never select another player's companion with a nearest-creature search.
                if (Creature* actor = ObjectAccessor::GetCreature(*me,oakin))
                    if (BloodRite::Owner(actor) == player && actor->IsAlive()) me->CastSpell(actor,212758,true);
                timer = 3000;
                break;
            case 3: Talk(1,player->GetGUID()); timer = 3000; break;
            case 4:
                fighting = true;
                me->setFaction(14);
                me->RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_PACIFIED);
                me->SetReactState(REACT_DEFENSIVE);
                AttackStart(player);
                break;
        }
    }
};

class player_blood_rite : public PlayerScript
{
public:
    player_blood_rite() : PlayerScript("player_blood_rite") { }
    void OnLogin(Player* player) override { BloodRite::ReturnGuide(player); }
    void OnLogout(Player* player) override
    {
        for (uint32 entry : {97418u,97675u,97440u}) BloodRite::ClearEntry(player,entry);
    }
    void OnMapChanged(Player* player) override { OnLogout(player); BloodRite::ReturnGuide(player); }
};

void AddSC_highmountain_blood_rite()
{
    RegisterCreatureAI(npc_blood_rite_navarrogg);
    RegisterCreatureAI(npc_blood_rite_marakhan);
    new player_blood_rite();
}
