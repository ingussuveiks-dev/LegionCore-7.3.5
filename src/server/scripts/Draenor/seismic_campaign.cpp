#include "garrison_campaign_actors.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include <mutex>

namespace SeismicCampaign
{
enum : uint32
{
    Intro = 34026,
    Data = 34027,
    Workshop = 34028,
    Prototype = 34029,
    Engineer = 34030,
    Mines = 34031,
    Information = 34032,
    Home = 34033,
    Machines = 34048
};

void FinishRide(Player* player, bool home)
{
    if (!player || !player->IsInWorld() || !player->IsAlive() || player->isInCombat() || player->GetMapId() != 1116)
        return;
    if (home)
    {
        if (!CampaignRecovery::Active(player, Home) || !player->GetQuestObjectiveData(Home, 272438) ||
            player->GetDistance2d(6281, 2224) > 35)
            return;
        float z = player->GetMap()->GetHeight(1786, 104, 1000.0f);
        if (z > INVALID_HEIGHT)
            player->TeleportTo(1116, 1786, 104, z + 0.5f, 0);
    }
    else if (player->GetQuestStatus(Prototype) == QUEST_STATUS_INCOMPLETE && player->GetDistance2d(8195, -589) < 40)
    {
        // Native scene 11679 records Hansel at this cave position.
        if (player->TeleportTo(1116, 6280.62f, 2224.26f, 136.651f, 4.9243f))
            player->KilledMonsterCredit(77311);
    }
}
} // namespace SeismicCampaign

class player_seismic_campaign : public PlayerScript
{
public:
    player_seismic_campaign() : PlayerScript("player_seismic_campaign")
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
        using namespace SeismicCampaign;
        using CampaignRecovery::Active;
        if (player->GetTeamId() != TEAM_ALLIANCE || (player->GetMapId() != 1116 && !player->GetMap()->IsGarrison()))
            return;
        std::lock_guard<std::mutex> lock(mutex);
        auto& state = states[player->GetGUID()];
        state.timer += diff;
        if (state.timer < 2000)
            return;
        state.timer = 0;
        state.Begin();
        bool done = player->GetQuestRewardStatus(Home);
        if (!done)
        {
            if (Active(player, Intro) || player->GetQuestRewardStatus(Intro))
                if (!player->GetQuestRewardStatus(Workshop))
                    state.At(player, 1, 77217, 1773, 105, false, false, true);
            if (player->GetQuestStatus(Data) == QUEST_STATUS_INCOMPLETE)
                state.At(player, 2, 77225, 1775, 105);
            if (Active(player, Workshop) || Active(player, Prototype))
                state.At(player, 3, 77160, 8201, -630, false, false, true);
            if (player->GetQuestStatus(Prototype) == QUEST_STATUS_INCOMPLETE)
                state.At(player, 4, 227231, 8195, -589, true);
            if (Active(player, Prototype) || player->GetQuestRewardStatus(Prototype))
                state.At(player, 5, 77160, 6280.62f, 2224.26f, false, false, true);
            if (Active(player, Engineer))
                state.At(player, 6, 77161, 6291, 2077, false, false, true);
            if (player->GetQuestRewardStatus(Engineer))
                state.At(player, 7, 77167, 6268, 2204, false, false, true);

            if (player->GetQuestStatus(Mines) == QUEST_STATUS_INCOMPLETE)
            {
                static float const sites[][2] = {{6054, 2098}, {6112, 2114}, {6257, 2263}, {6136, 2344}};
                uint32 count = player->GetQuestObjectiveData(Mines, 272271);
                if (count < 4)
                    state.At(player, 20 + count, 227183, sites[count][0], sites[count][1], true);
            }
            if (player->GetQuestStatus(Machines) == QUEST_STATUS_INCOMPLETE)
            {
                static float const sites[][2] = {{5904, 2119}, {6187, 2154}, {6213, 2295}, {6044, 2436}, {5809, 2242}};
                uint32 count = player->GetQuestObjectiveData(Machines, 272273);
                if (count < 5)
                    state.At(player, 30 + count, 227172, sites[count][0], sites[count][1], true);
            }
            // Blackhammer 77175 and his item 110453 already have a real spawn
            // and quest loot. Do not give the information item from an NPC.
            if (Active(player, Home))
            {
                state.At(player, 8, 227270, 6273, 2227, true);
                state.At(player, 9, 77161, 1786, 107, false, false, true);
                state.At(player, 10, 77160, 1784, 104, false, false, true);
            }
            if (player->IsAlive() && !player->isInFlight())
            {
                Discover(player, Workshop, 77160, 8201, -630);
                Discover(player, Engineer, 77161, 6291, 2077);
                Discover(player, Home, 77160, 6281, 2224);
            }
        }
        state.End(player);
    }

private:
    void Discover(Player* player, uint32 quest, uint32 entry, float x, float y)
    {
        if (player->GetQuestStatus(quest) != QUEST_STATUS_INCOMPLETE || player->GetDistance2d(x, y) > 12)
            return;
        if (Creature* creature = player->FindNearestCreature(entry, 12))
            if (CampaignRecovery::Owned(creature, player) && player->IsWithinLOSInMap(creature))
                player->KilledMonsterCredit(entry);
    }
    std::mutex mutex;
    std::map<ObjectGuid, CampaignRecovery::Encounter> states;
};

class npc_seismic_tremor_tracker : public CreatureScript
{
public:
    npc_seismic_tremor_tracker() : CreatureScript("npc_seismic_tremor_tracker")
    {
    }
    struct AI : ScriptedAI
    {
        AI(Creature* creature) : ScriptedAI(creature), summons(me)
        {
        }
        SummonList summons;
        ObjectGuid owner;
        std::set<ObjectGuid> living;
        uint32 wave = 0, delay = 0, timeout = 0;
        Player* Owner()
        {
            return ObjectAccessor::GetPlayer(*me, owner);
        }
        void Stop()
        {
            wave = delay = timeout = 0;
            living.clear();
            summons.DespawnAll();
        }
        void Start(Player* player)
        {
            if (wave || !CampaignRecovery::Owned(me, player) ||
                player->GetQuestStatus(SeismicCampaign::Data) != QUEST_STATUS_INCOMPLETE)
                return;
            owner = player->GetGUID();
            timeout = 180000;
            SpawnWave();
        }
        void SpawnWave()
        {
            Player* player = Owner();
            if (!player)
            {
                Stop();
                return;
            }
            ++wave;
            for (uint32 i = 0; i < 3; ++i)
                if (Creature* grunt =
                        me->SummonCreature(77244, me->GetPositionX() + 8 + i * 2, me->GetPositionY() + 6 - i * 3,
                                           me->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN, 180000, owner))
                {
                    summons.Summon(grunt);
                    living.insert(grunt->GetGUID());
                    grunt->setFaction(14);
                    grunt->SetReactState(REACT_AGGRESSIVE);
                    if (grunt->IsAIEnabled)
                        grunt->AI()->AttackStart(player);
                }
            // Partial summon failure must not turn a two/one-opponent wave into success.
            if (living.size() != 3)
                Stop();
        }
        void SummonedCreatureDies(Creature* creature, Unit*) override
        {
            if (!wave || !living.erase(creature->GetGUID()) || !living.empty())
                return;
            if (wave == 1)
                delay = 2000;
            else
            {
                if (Player* player = Owner())
                    if (player->IsAlive() && player->GetDistance(me) < 60 &&
                        player->GetQuestStatus(SeismicCampaign::Data) == QUEST_STATUS_INCOMPLETE)
                        player->KilledMonsterCredit(77225);
                Stop();
            }
        }
        void SummonedCreatureDespawn(Creature* creature) override
        {
            // A disappearance is not a kill. Cancel so the owner can retry.
            if (living.erase(creature->GetGUID()))
                Stop();
        }
        void UpdateAI(uint32 diff) override
        {
            if (!wave)
                return;
            Player* player = Owner();
            if (!player || !player->IsAlive() || player->GetDistance(me) > 60 ||
                player->GetQuestStatus(SeismicCampaign::Data) != QUEST_STATUS_INCOMPLETE || timeout <= diff)
            {
                Stop();
                return;
            }
            timeout -= diff;
            if (delay)
            {
                if (delay <= diff)
                {
                    delay = 0;
                    SpawnWave();
                }
                else
                    delay -= diff;
            }
        }
    };
    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (player->IsAlive() && creature->IsAIEnabled)
            static_cast<AI*>(creature->AI())->Start(player);
        return true;
    }
    CreatureAI* GetAI(Creature* creature) const override
    {
        return new AI(creature);
    }
};

class go_seismic_campaign_object : public GameObjectScript
{
public:
    go_seismic_campaign_object() : GameObjectScript("go_seismic_campaign_object")
    {
    }
    bool OnGossipHello(Player* player, GameObject* object) override
    {
        using namespace SeismicCampaign;
        if (object->GetOwnerGUID() != player->GetGUID() || !player->IsAlive() || player->isInCombat())
            return true;
        switch (object->GetEntry())
        {
        case 227183:
            return player->GetQuestStatus(Mines) != QUEST_STATUS_INCOMPLETE;
        case 227172:
            return player->GetQuestStatus(Machines) != QUEST_STATUS_INCOMPLETE;
        case 227231:
            return player->GetQuestStatus(Prototype) != QUEST_STATUS_INCOMPLETE || player->HasAura(158645);
        case 227270:
            return !CampaignRecovery::Active(player, Home) || !player->GetQuestObjectiveData(Home, 272438) ||
                   player->HasAura(158317);
        default:
            return true;
        }
    }
};

// The original journeys are client scenes, not vehicles. Removal on scene
// completion or the finite timeout reaches the same guarded arrival handler.
class aura_seismic_mole_ride : public SpellScriptLoader
{
public:
    aura_seismic_mole_ride() : SpellScriptLoader("aura_seismic_mole_ride")
    {
    }
    class Script : public AuraScript
    {
        PrepareAuraScript(Script);
        void Arrive(AuraEffect const*, AuraEffectHandleModes)
        {
            SeismicCampaign::FinishRide(GetTarget()->ToPlayer(), GetId() == 158317);
        }
        void Register() override
        {
            AfterEffectRemove +=
                AuraEffectRemoveFn(Script::Arrive, EFFECT_ALL, SPELL_AURA_ACTIVATE_SCENE, AURA_EFFECT_HANDLE_REAL);
        }
    };
    AuraScript* GetAuraScript() const override
    {
        return new Script();
    }
};

void AddSC_seismic_campaign()
{
    new player_seismic_campaign();
    new npc_seismic_tremor_tracker();
    new go_seismic_campaign_object();
    new aura_seismic_mole_ride();
}
