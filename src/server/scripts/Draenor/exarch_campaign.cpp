#include "garrison_campaign_actors.h"
#include "ScriptedGossip.h"
#include "InstanceScript.h"
#include "LFGMgr.h"
#include "Group.h"
#include "WorldSession.h"
#include <mutex>

namespace ExarchCampaign
{
enum : uint32
{
    Intro = 36163,
    Courage = 36164,
    Will = 36167,
    Faith = 36168,
    Champions = 36169
};
bool AllTrials(Player* player)
{
    return player->GetQuestRewardStatus(Courage) && player->GetQuestRewardStatus(Will) &&
           player->GetQuestRewardStatus(Faith);
}
} // namespace ExarchCampaign

class player_exarch_campaign : public PlayerScript
{
public:
    player_exarch_campaign() : PlayerScript("player_exarch_campaign")
    {
    }
    void OnMapChanged(Player* player) override
    {
        OnLogout(player);
    }
    void OnLogout(Player* player) override
    {
        std::lock_guard<std::mutex> lock(mutex);
        auto it = states.find(player->GetGUID());
        if (it != states.end())
        {
            it->second.Begin();
            it->second.End(player);
            states.erase(it);
        }
    }
    void OnUpdate(Player* player, uint32 diff) override
    {
        using namespace ExarchCampaign;
        using CampaignRecovery::Active;
        if (player->GetTeamId() != TEAM_ALLIANCE || (player->GetMapId() != 1116 && player->GetMapId() != 1374))
            return;
        std::lock_guard<std::mutex> lock(mutex);
        auto& state = states[player->GetGUID()];
        state.timer += diff;
        if (state.timer < 2000)
            return;
        state.timer = 0;
        state.Begin();
        if (player->GetMapId() == 1374)
        {
            if (player->GetQuestStatus(Faith) == QUEST_STATUS_COMPLETE)
                if (Group* group = player->GetGroup())
                    if (group->isLFGGroup())
                        sLFGMgr->FinishDungeon(group->GetGUID(), 870);
            // A dedicated native quest map / LFG 870, never the 1182 dungeon.
            state.At(player, 80, 84803, 1490, 2952);
            state.At(player, 81, 84803, 1875, 2952);
            if (player->GetQuestStatus(Faith) == QUEST_STATUS_INCOMPLETE && player->IsAlive())
            {
                if (!player->GetQuestObjectiveData(Faith, 274534))
                    state.At(player, 82, 84719, 1653, 2952, false, true);
                else if (!player->GetQuestObjectiveData(Faith, 274536))
                    state.At(player, 83, 84814, 1902, 2952, false, true);
            }
        }
        else
        {
            if (Active(player, Intro))
                state.At(player, 1, 73395, 1502, -2151, false, false, true);
            if (player->GetQuestRewardStatus(Intro) && !player->GetQuestRewardStatus(Champions))
            {
                if (!player->GetQuestRewardStatus(Courage))
                    state.At(player, 2, 80078, 1500, -2108, false, false, true);
                if (!player->GetQuestRewardStatus(Will))
                    state.At(player, 3, 80079, 1491, -2113, false, false, true);
                if (!player->GetQuestRewardStatus(Faith))
                    state.At(player, 4, 80073, 1482, -2108, false, false, true);
                // Original turn-in/council POIs. Keep all three trials parallel.
                if (player->IsAlive())
                {
                    state.At(player, 5, 84973, 70.1354f, -2749.33f, false, false, true);
                    state.At(player, 6, 84974, 74.6788f, -2765.3f, false, false, true);
                    state.At(player, 7, 84975, 91, -2770, false, false, true);
                }
            }
            if (player->GetQuestStatus(Courage) == QUEST_STATUS_INCOMPLETE)
            {
                state.At(player, 10, 84368, 6928, 4259);
                if (!player->GetQuestObjectiveData(Courage, 274390))
                    state.At(player, 11, 84364, 6969, 4302, false, true);
            }
            if (player->GetQuestStatus(Will) == QUEST_STATUS_INCOMPLETE)
                state.At(player, 12, 84538, 4286, 6513);
            // Plaguebloom 84403 and Tuulani 79434 already have real placements.
        }
        state.End(player);
    }

private:
    std::mutex mutex;
    std::map<ObjectGuid, CampaignRecovery::Encounter> states;
};

class npc_exarch_campaign_guide : public CreatureScript
{
public:
    npc_exarch_campaign_guide() : CreatureScript("npc_exarch_campaign_guide")
    {
    }
    bool OnGossipHello(Player* player, Creature* creature) override
    {
        using namespace ExarchCampaign;
        switch (creature->GetEntry())
        {
        case 84368:
            if (CampaignRecovery::Owned(creature, player) && player->GetQuestStatus(Courage) == QUEST_STATUS_INCOMPLETE)
                player->KilledMonsterCredit(84368);
            return true;
        case 84538:
            if (CampaignRecovery::Owned(creature, player) && player->GetQuestStatus(Will) == QUEST_STATUS_INCOMPLETE)
                player->KilledMonsterCredit(84538);
            return true;
        case 79434:
            if (player->GetQuestStatus(Faith) != QUEST_STATUS_INCOMPLETE)
                return false;
            player->PrepareQuestMenu(creature->GetGUID());
            player->ADD_GOSSIP_ITEM(GossipOptionNpc::None, "I am ready for the Trial of Faith.", GOSSIP_SENDER_MAIN, 1);
            break;
        case 84803:
            if (player->GetMapId() != 1374 || !CampaignRecovery::Owned(creature, player))
                return false;
            player->ADD_GOSSIP_ITEM(GossipOptionNpc::None, "Return to Soulbinder Tuulani.", GOSSIP_SENDER_MAIN, 2);
            break;
        default:
            return false;
        }
        player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        return true;
    }
    bool OnGossipSelect(Player* player, Creature* creature, uint32, uint32 action) override
    {
        player->PlayerTalkClass->ClearMenus();
        player->CLOSE_GOSSIP_MENU();
        if (!player->IsAlive() || player->isInCombat())
            return true;
        if (action == 1 && creature->GetEntry() == 79434 &&
            player->GetQuestStatus(ExarchCampaign::Faith) == QUEST_STATUS_INCOMPLETE)
        {
            if (player->GetGroup())
            {
                player->GetSession()->SendNotification("The Trial of Faith must be entered alone.");
                return true;
            }
            player->CastSpell(player, 169509, true); // native Tuulani conversation credit
            lfg::LfgDungeonSet dungeon = {870};
            sLFGMgr->JoinLfg(player, lfg::PLAYER_ROLE_DAMAGE, dungeon);
        }
        else if (action == 2 && creature->GetEntry() == 84803 && player->GetMapId() == 1374 &&
                 CampaignRecovery::Owned(creature, player))
            sLFGMgr->TeleportPlayer(player, true);
        return true;
    }
};

class npc_exarch_council_trial : public CreatureScript
{
public:
    npc_exarch_council_trial() : CreatureScript("npc_exarch_council_trial")
    {
    }
    struct AI : ScriptedAI
    {
        AI(Creature* creature) : ScriptedAI(creature)
        {
        }
        bool fighting = false, defeated = false;
        void Reset() override
        {
            fighting = false;
            defeated = false;
            me->setFaction(35);
            me->SetStandState(UNIT_STAND_STATE_STAND);
            me->SetFullHealth();
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
        }
        uint32 GetData(uint32) const override
        {
            return defeated ? 1 : 0;
        }
        void DoAction(int32) override
        {
            fighting = true;
            defeated = false;
            me->setFaction(14);
            me->CombatStop(true);
            me->DeleteThreatList();
            me->SetFullHealth();
            me->SetStandState(UNIT_STAND_STATE_STAND);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_NON_ATTACKABLE);
        }
        void DamageTaken(Unit* attacker, uint32& damage, DamageEffectType) override
        {
            Player* player = attacker->GetCharmerOrOwnerPlayerOrPlayerItself();
            if (!fighting || defeated || !player || !CampaignRecovery::Owned(me, player) ||
                player->GetQuestStatus(ExarchCampaign::Champions) != QUEST_STATUS_INCOMPLETE)
            {
                damage = 0;
                return;
            }
            // Creature AI sees raw damage; compare the same scaled quantity
            // used later by Unit::DealDamage, including lethal one-hit spells.
            float multiplier = me->GetHealthMultiplierForTarget(attacker);
            if (multiplier <= 0)
            {
                damage = 0;
                return;
            }
            double finalDamage = damage / multiplier;
            uint64 health = me->GetHealth(attacker), maximum = me->GetMaxHealth(attacker);
            if (finalDamage < health && health - finalDamage > maximum * 0.10)
                return;
            damage = 0;
            defeated = true;
            fighting = false;
            me->CombatStop(true);
            me->DeleteThreatList();
            me->setFaction(35);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
            me->SetStandState(UNIT_STAND_STATE_KNEEL);
            bool all = true;
            for (uint32 entry : {84973, 84974, 84975})
            {
                std::list<Creature*> council;
                me->GetCreatureListWithEntryInGrid(council, entry, 60);
                bool found = false;
                for (Creature* member : council)
                    if (CampaignRecovery::Owned(member, player) && member->IsAIEnabled && member->AI()->GetData(1))
                        found = true;
                all = all && found;
            }
            if (all)
                player->KilledMonsterCredit(84974);
        }
        void UpdateAI(uint32) override
        {
            if (!fighting)
                return;
            TempSummon* summon = me->ToTempSummon();
            Player* owner = summon ? ObjectAccessor::GetPlayer(*me, summon->GetSummonerGUID()) : nullptr;
            if (!owner || !owner->IsAlive() || owner->GetDistance(me) > 60 ||
                owner->GetQuestStatus(ExarchCampaign::Champions) != QUEST_STATUS_INCOMPLETE)
            {
                EnterEvadeMode();
                return;
            }
            if (UpdateVictim())
                DoMeleeAttackIfReady();
        }
    };
    bool OnGossipHello(Player* player, Creature* creature) override
    {
        player->PrepareQuestMenu(creature->GetGUID());
        if (creature->GetEntry() == 84974 && CampaignRecovery::Owned(creature, player) &&
            ExarchCampaign::AllTrials(player) &&
            player->GetQuestStatus(ExarchCampaign::Champions) == QUEST_STATUS_INCOMPLETE && !player->isInCombat())
            player->ADD_GOSSIP_ITEM(GossipOptionNpc::None, "Begin the Trial of Champions.", GOSSIP_SENDER_MAIN, 1);
        player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        return true;
    }
    bool OnGossipSelect(Player* player, Creature* creature, uint32, uint32 action) override
    {
        player->PlayerTalkClass->ClearMenus();
        player->CLOSE_GOSSIP_MENU();
        if (action != 1 || creature->GetEntry() != 84974 || !CampaignRecovery::Owned(creature, player) ||
            !player->IsAlive() || player->isInCombat() || !ExarchCampaign::AllTrials(player) ||
            player->GetQuestStatus(ExarchCampaign::Champions) != QUEST_STATUS_INCOMPLETE)
            return true;
        std::vector<Creature*> members;
        for (uint32 entry : {84973, 84974, 84975})
        {
            std::list<Creature*> found;
            creature->GetCreatureListWithEntryInGrid(found, entry, 60);
            for (Creature* member : found)
                if (CampaignRecovery::Owned(member, player) && member->IsAIEnabled)
                    members.push_back(member);
        }
        if (members.size() != 3)
            return true;
        for (Creature* member : members)
        {
            member->AI()->DoAction(1);
            member->AI()->AttackStart(player);
        }
        return true;
    }
    CreatureAI* GetAI(Creature* creature) const override
    {
        return new AI(creature);
    }
};

class instance_exarch_trial_of_faith : public InstanceMapScript
{
public:
    instance_exarch_trial_of_faith() : InstanceMapScript("instance_exarch_trial_of_faith", 1374)
    {
    }
    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new InstanceScript(map);
    }
};

void AddSC_exarch_campaign()
{
    new player_exarch_campaign();
    new npc_exarch_campaign_guide();
    new npc_exarch_council_trial();
    new instance_exarch_trial_of_faith();
}
