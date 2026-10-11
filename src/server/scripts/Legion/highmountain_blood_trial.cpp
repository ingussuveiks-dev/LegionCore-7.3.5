#include "ScriptMgr.h"
#include "CombatAI.h"
#include "GameObject.h"
#include "Player.h"
#include "TemporarySummon.h"
#include "ObjectAccessor.h"
#include "QuestData.h"
#include "CreatureTextMgr.h"

namespace BloodTrial
{
constexpr uint32 QuestId = 39455;
bool Active(Player* player)
{
    return player && player->IsAlive() && player->GetMapId() == 1220 &&
        !player->IsBeingTeleported() && !player->isInFlight() && !player->IsFlying() && !player->GetVehicle() &&
        player->GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE;
}
bool Done(Player* player, uint32 entry) { return player->GetQuestObjectiveData(QuestId,entry) > 0; }
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
            if (Owner(creature) == player && creature->IsAlive()) return creature;
    return nullptr;
}
void Clear(Player* player)
{
    for (uint32 entry : {97544u,97539u})
    {
        auto guids = *player->GetSummonList(entry);
        for (ObjectGuid const& guid : guids)
            if (Creature* creature = ObjectAccessor::GetCreature(*player,guid))
                if (Owner(creature) == player) creature->DespawnOrUnsummon();
    }
}
void Credit(Player* player, uint32 entry, ObjectGuid guid)
{
    if (!Active(player) || Done(player,entry)) return;
    if (Quest const* quest = sQuestDataStore->GetQuestTemplate(QuestId))
        for (QuestObjective const& objective : quest->GetObjectives())
            if (objective.ObjectID == entry)
            {
                player->SetQuestObjectiveData(quest,&objective,1);
                player->SendQuestUpdateAddCredit(quest,guid,objective,1);
                if (player->CanCompleteQuest(QuestId)) player->CompleteQuest(QuestId);
                return;
            }
}
}

class go_blood_trial_altar : public GameObjectScript
{
public:
    go_blood_trial_altar() : GameObjectScript("go_blood_trial_altar") { }
    bool OnGossipHello(Player* player, GameObject* go) override
    {
        if (!BloodTrial::Active(player) || BloodTrial::Done(player,97544) || go->GetEntry() != 244564 ||
            !go->isSpawned() || !player->InSamePhase(go) || !player->canSeeOrDetect(go) ||
            !player->IsWithinDistInMap(go,6.0f) || !player->IsWithinLOSInMap(go) || BloodTrial::Existing(player,97544)) return true;

        BloodTrial::Clear(player); // Remove this player's old sacrifice before retry.
        // Native Witch entry 97539 has a feign-dead presentation in the live template.
        Creature* witch = player->SummonCreature(97539,go->GetPositionX(),go->GetPositionY(),go->GetPositionZ(),go->GetOrientation(),
            TEMPSUMMON_TIMED_DESPAWN,120000,player->GetGUID());
        if (!witch) return true;
        witch->SetReactState(REACT_PASSIVE);
        witch->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
        // Native spell 192443 summons 97544 with wild SummonProperties 3640.
        player->CastSpell(go->GetPositionX(),go->GetPositionY(),go->GetPositionZ(),192443,true);
        if (!BloodTrial::Existing(player,97544)) { BloodTrial::Clear(player); return true; }
        BloodTrial::Credit(player,244564,go->GetGUID());
        return true; // No shared goober cooldown, summon or generic/group credit.
    }
};

struct npc_blood_trial_warmonger : AggressorAI
{
    npc_blood_trial_warmonger(Creature* creature) : AggressorAI(creature) { }
    std::weak_ptr<uint8> lifetime;
    Position origin;
    ObjectGuid sacrifice;
    bool surrendered = false;

    void Cancel()
    {
        if (Creature* witch = ObjectAccessor::GetCreature(*me,sacrifice)) witch->DespawnOrUnsummon();
        me->DespawnOrUnsummon();
    }

    void IsSummonedBy(Unit* summoner) override
    {
        Player* player = summoner->ToPlayer();
        if (!BloodTrial::Active(player) || BloodTrial::Done(player,97544)) { me->DespawnOrUnsummon(); return; }
        player->AddQuestDelayedEvent(BloodTrial::QuestId,0,[] { });
        lifetime = player->getQuestStatus(BloodTrial::QuestId)->ScriptLifetime;
        if (Creature* witch = BloodTrial::Existing(player,97539)) sacrifice = witch->GetGUID();
        origin = me->GetPosition();
        me->AddPlayerInPersonnalVisibilityList(player->GetGUID());
        // Template immunity to NPCs also excluded the challenger's pets.
        me->RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_PACIFIED);
        me->SetNoCallAssistance(true);
        me->m_CanCallAssistance = false;
        me->SetReactState(REACT_DEFENSIVE);
        Talk(1,player->GetGUID());
        AttackStart(player);
    }
    bool CanAIAttack(Unit const* target) const override
    {
        return !surrendered && target && BloodTrial::Owner(me) &&
            target->GetCharmerOrOwnerPlayerOrPlayerItself() == BloodTrial::Owner(me);
    }
    void MoveInLineOfSight(Unit*) override { } // Never recruit nearby spectators into the duel.
    void EnterEvadeMode() override { Cancel(); }
    void DamageTaken(Unit* attacker, uint32& damage, DamageEffectType) override
    {
        Player* player = BloodTrial::Owner(me);
        if (surrendered || lifetime.expired() || !BloodTrial::Active(player) || !BloodTrial::Done(player,244564) ||
            !attacker || attacker->GetCharmerOrOwnerPlayerOrPlayerItself() != player ||
            !player->InSamePhase(me) || player->GetDistance(me) > 40.0f || me->GetExactDist(&origin) > 40.0f)
        { damage = 0; return; }
        if (!damage) return;
        // DamageTaken precedes the core's division by this multiplier. Compare
        // the damage the death check will receive against target-relative HP.
        double multiplier = me->GetHealthMultiplierForTarget(attacker);
        double finalDamage = multiplier > 0 ? damage / multiplier : damage;
        if (finalDamage < me->GetHealth(attacker)) return;
        damage = 0;
        surrendered = true;
        me->InterruptNonMeleeSpells(true);
        me->CombatStop(true);
        me->RemoveAllAuras();
        me->SetReactState(REACT_PASSIVE);
        me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE);
        player->RemoveAurasDueToSpell(195789,me->GetGUID());
        if (attacker != player) attacker->RemoveAurasDueToSpell(195789,me->GetGUID());
        BloodTrial::Credit(player,97544,me->GetGUID());
        if (Creature* torok = me->FindNearestCreature(93836,60.0f,true))
            if (player->InSamePhase(torok) && player->canSeeOrDetect(torok))
                sCreatureTextMgr->SendChat(torok,0,player->GetGUID(),CHAT_MSG_MONSTER_WHISPER);
        me->DespawnOrUnsummon(3000);
    }
    void UpdateAI(uint32 diff) override
    {
        if (surrendered) return;
        Player* player = BloodTrial::Owner(me);
        if (lifetime.expired() || !BloodTrial::Active(player) || !BloodTrial::Done(player,244564) ||
            !player->InSamePhase(me) || player->GetDistance(me) > 40.0f || me->GetExactDist(&origin) > 40.0f)
        { Cancel(); return; }
        // Preserve existing Burning Slash and the core's ordinary combat scheduling.
        AggressorAI::UpdateAI(diff);
    }
};

class player_blood_trial : public PlayerScript
{
public:
    player_blood_trial() : PlayerScript("player_blood_trial") { }
    void OnLogout(Player* player) override
    {
        auto status = player->GetQuestStatus(BloodTrial::QuestId);
        if (status == QUEST_STATUS_INCOMPLETE || status == QUEST_STATUS_COMPLETE) BloodTrial::Clear(player);
    }
    void OnMapChanged(Player* player) override { OnLogout(player); }
};

void AddSC_highmountain_blood_trial()
{
    new go_blood_trial_altar();
    RegisterCreatureAI(npc_blood_trial_warmonger);
    new player_blood_trial();
}
