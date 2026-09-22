/*
 * Level 100 character boost tutorial (Legion, client 7.3.5).
 *
 * The client ships the class-specific scenarios and both faction gunship
 * maps, but the original server-side passengers and encounter controller
 * are not part of the public 7.3.5 database.  Keep the scenario criteria in
 * DB2 and supply only the missing server-controlled actors and waves here.
 */

#include "ScriptMgr.h"
#include "Creature.h"
#include "InstanceScript.h"
#include "Player.h"
#include "Scenario.h"
#include "ScenarioMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "TemporarySummon.h"
#include "Transport.h"

namespace BoostExperience
{
enum Maps : uint32
{
    MAP_ALLIANCE = 1554,
    MAP_HORDE    = 1557
};

enum GameObjects : uint32
{
    GO_DAWN_BLADE = 246606,
    GO_WARBRINGER = 246607
};

enum Creatures : uint32
{
    NPC_ALLIANCE_TRAINER = 100448,
    NPC_HORDE_TRAINER    = 105317,
    NPC_ALLIANCE_EXIT    = 107543,
    NPC_HORDE_EXIT       = 112565,
    NPC_ALLIANCE_SPAR    = 109010,
    NPC_HORDE_SPAR       = 111995,

    NPC_TRAINING_DUMMY_1 = 100648,
    NPC_TRAINING_DUMMY_2 = 101824,
    NPC_TRAINING_DUMMY_3 = 103703,

    NPC_LEGION_IMP       = 102658,
    NPC_LEGION_INFERNAL  = 112639,
    NPC_LEGION_BAT       = 111776
};

enum Spells : uint32
{
    SPELL_START_TRAINING = 219615,
    SPELL_LEAVE_TUTORIAL = 219912
};

enum Criteria : uint32
{
    CRITERIA_SPAR_COMPLETE = 48772
};

// WorldSafeLocs 5219 and 5752 are transport-local boost spawn positions.
Position const AllianceDeck = { 11.1372f, -0.363736f, 20.6586f, 2.937f };
Position const HordeDeck    = { 0.720768f, 1.68496f, 34.501f, 6.2787f };

uint32 GetScenarioFor(Player const* player)
{
    switch (player->getClass())
    {
        case CLASS_PALADIN:      return 1073;
        case CLASS_HUNTER:       return player->GetSpecializationId() == 255 ? 1214 : 1083;
        case CLASS_WARLOCK:      return 1084;
        case CLASS_PRIEST:       return 1090;
        case CLASS_SHAMAN:       return player->GetSpecializationId() == 263 ? 1182 : 1091;
        case CLASS_WARRIOR:      return 1093;
        case CLASS_DEATH_KNIGHT: return 1094;
        case CLASS_MONK:         return 1095;
        case CLASS_DRUID:        return player->GetSpecializationId() == 102 ? 1181 : 1096;
        case CLASS_MAGE:         return 1132;
        case CLASS_ROGUE:        return 1133;
        default:                 return 0;
    }
}

Position Offset(Position const& base, float x, float y, float z = 0.0f, float orientation = 0.0f)
{
    return Position(base.GetPositionX() + x, base.GetPositionY() + y,
        base.GetPositionZ() + z, orientation);
}
}

using namespace BoostExperience;

class instance_boost_experience : public InstanceMapScript
{
public:
    instance_boost_experience(char const* name, uint32 mapId, bool alliance) :
        InstanceMapScript(name, mapId), _alliance(alliance) { }

    struct instance_boost_experience_InstanceScript : public InstanceScript
    {
        instance_boost_experience_InstanceScript(InstanceMap* map, bool alliance) :
            InstanceScript(map), _alliance(alliance), _setupTimer(500), _setupComplete(false),
            _lastCombatStep(0xFF), _exitSpawned(false) { }

        bool _alliance;
        uint32 _setupTimer;
        bool _setupComplete;
        uint8 _lastCombatStep;
        bool _exitSpawned;
        ObjectGuid _transportGuid;

        Position const& Deck() const { return _alliance ? AllianceDeck : HordeDeck; }
        uint32 TransportEntry() const { return _alliance ? GO_DAWN_BLADE : GO_WARBRINGER; }

        Player* GetPlayer() const
        {
            Map::PlayerList const& players = instance->GetPlayers();
            if (players.isEmpty())
                return nullptr;

            return players.begin()->getSource();
        }

        Transport* GetGunship()
        {
            if (!_transportGuid.IsEmpty())
                if (GameObject* object = instance->GetGameObject(_transportGuid))
                    return object->ToTransport();

            if (GameObject* object = GetGameObjectByEntry(TransportEntry()))
                return object->ToTransport();

            return nullptr;
        }

        TempSummon* SummonPassenger(Transport* transport, uint32 entry, Position const& position,
            Unit* summoner = nullptr, TempSummonType type = TEMPSUMMON_MANUAL_DESPAWN, uint32 duration = 0)
        {
            return transport->SummonPassenger(entry, position, type, nullptr, duration, summoner);
        }

        void SpawnStaticPassengers(Transport* transport)
        {
            Position const& deck = Deck();
            uint32 trainer = _alliance ? NPC_ALLIANCE_TRAINER : NPC_HORDE_TRAINER;
            uint32 allianceEntourage[] = { 95093, 105628, 107953, 107541 };
            uint32 hordeEntourage[] = { 112038, 112002, 112026, 112025 };
            uint32 const* entourage = _alliance ? allianceEntourage : hordeEntourage;

            if (TempSummon* creature = SummonPassenger(transport, trainer, Offset(deck, 5.0f, 0.0f, 0.0f, 3.14f)))
                creature->SetReactState(REACT_PASSIVE);

            uint32 dummies[] = { NPC_TRAINING_DUMMY_1, NPC_TRAINING_DUMMY_2, NPC_TRAINING_DUMMY_3 };
            for (uint8 index = 0; index < 3; ++index)
                if (TempSummon* dummy = SummonPassenger(transport, dummies[index],
                    Offset(deck, -6.0f, -4.0f + 4.0f * index, 0.0f, 0.0f)))
                    dummy->SetReactState(REACT_PASSIVE);

            uint8 index = 0;
            for (uint8 entourageIndex = 0; entourageIndex < 4; ++entourageIndex)
            {
                uint32 entry = entourage[entourageIndex];
                if (TempSummon* creature = SummonPassenger(transport, entry,
                    Offset(deck, 8.0f, -6.0f + 4.0f * index++, 0.0f, 3.14f)))
                    creature->SetReactState(REACT_PASSIVE);
            }
        }

        void PutPlayerOnTransport(Player* player, Transport* transport)
        {
            if (player->GetTransport() == transport)
                return;

            Position const& deck = Deck();
            float x = deck.GetPositionX();
            float y = deck.GetPositionY();
            float z = deck.GetPositionZ();
            float orientation = deck.GetOrientation();
            transport->CalculatePassengerPosition(x, y, z, &orientation);

            player->NearTeleportTo(x, y, z, orientation);
            player->m_movementInfo.transport.Guid = transport->GetGUID();
            player->m_movementInfo.transport.Pos.Relocate(deck);
            player->m_movementInfo.transport.VehicleSeatIndex = -1;
            transport->AddPassenger(player);
        }

        bool Setup()
        {
            Player* player = GetPlayer();
            Transport* transport = GetGunship();
            if (!player || !transport)
                return false;

            uint32 scenarioId = GetScenarioFor(player);
            if (!scenarioId)
            {
                TC_LOG_ERROR("scripts", "Boost tutorial has no scenario for class %u", player->getClass());
                return false;
            }

            transport->EnableMovement(true);
            PutPlayerOnTransport(player, transport);

            Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId());
            if (!scenario)
                scenario = sScenarioMgr->AddScenario(instance, scenarioId);

            if (!scenario)
                return false;

            SpawnStaticPassengers(transport);
            scenario->SendStepUpdate(player, true);
            setScenarioStep(scenario->GetCurrentStep());
            TC_LOG_INFO("scripts", "Started boost tutorial scenario %u for %s on map %u",
                scenarioId, player->GetName(), instance->GetId());
            return true;
        }

        void OnGameObjectCreate(GameObject* gameObject) override
        {
            InstanceScript::OnGameObjectCreate(gameObject);
            if (gameObject->GetEntry() == TransportEntry())
                _transportGuid = gameObject->GetGUID();
        }

        void OnPlayerEnter(Player* /*player*/) override
        {
            _setupTimer = 100;
        }

        void Update(uint32 diff) override
        {
            if (_setupComplete)
                return;

            if (_setupTimer > diff)
            {
                _setupTimer -= diff;
                return;
            }

            _setupComplete = Setup();
            _setupTimer = _setupComplete ? 0 : 500;
        }

        void StartCombat(TempSummon* summon, Player* player)
        {
            if (!summon || !player)
                return;

            summon->setFaction(14);
            summon->SetReactState(REACT_AGGRESSIVE);
            summon->AI()->AttackStart(player);
        }

        void SpawnSparringWave(uint8 count)
        {
            Transport* transport = GetGunship();
            Player* player = GetPlayer();
            if (!transport || !player)
                return;

            uint32 entry = _alliance ? NPC_ALLIANCE_SPAR : NPC_HORDE_SPAR;
            Position const& deck = Deck();
            for (uint8 index = 0; index < count; ++index)
                StartCombat(SummonPassenger(transport, entry,
                    Offset(deck, -2.0f, -2.5f + 5.0f * index, 0.0f, 0.0f), player,
                    TEMPSUMMON_CORPSE_TIMED_DESPAWN, 15000), player);
        }

        void SpawnLegionWave()
        {
            Transport* transport = GetGunship();
            Player* player = GetPlayer();
            if (!transport || !player)
                return;

            Position const& deck = Deck();
            for (uint8 index = 0; index < 10; ++index)
                StartCombat(SummonPassenger(transport, NPC_LEGION_IMP,
                    Offset(deck, -8.0f + float(index % 5) * 3.0f, -5.0f + float(index / 5) * 10.0f),
                    player, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 15000), player);

            StartCombat(SummonPassenger(transport, NPC_LEGION_INFERNAL, Offset(deck, -11.0f, 0.0f),
                player, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 15000), player);
            StartCombat(SummonPassenger(transport, NPC_LEGION_BAT, Offset(deck, 0.0f, 8.0f, 3.0f),
                player, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 15000), player);
        }

        void SpawnExit()
        {
            if (_exitSpawned)
                return;

            if (Transport* transport = GetGunship())
            {
                uint32 entry = _alliance ? NPC_ALLIANCE_EXIT : NPC_HORDE_EXIT;
                if (TempSummon* creature = SummonPassenger(transport, entry, Offset(Deck(), 12.0f, 0.0f, 0.0f, 3.14f)))
                {
                    creature->SetReactState(REACT_PASSIVE);
                    _exitSpawned = true;
                }
            }
        }

        void onScenarionNextStep(uint32 newStep) override
        {
            if (_lastCombatStep == newStep)
                return;

            Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId());
            if (!scenario)
                return;

            uint8 stepCount = scenario->GetStepCount(false);
            if (stepCount < 4)
                return;
            if (newStep == stepCount - 4)
                SpawnSparringWave(1);
            else if (newStep == stepCount - 3)
                SpawnSparringWave(2);
            else if (newStep == stepCount - 2)
                SpawnLegionWave();
            else if (newStep >= uint32(stepCount - 1))
                SpawnExit();
            else
                return;

            _lastCombatStep = uint8(newStep);
        }
    };

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_boost_experience_InstanceScript(map, _alliance);
    }

private:
    bool _alliance;
};

class npc_boost_trainer : public CreatureScript
{
public:
    npc_boost_trainer() : CreatureScript("npc_boost_trainer") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        player->ADD_GOSSIP_ITEM(GossipOptionNpc::None, "I am ready to begin training.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        player->SEND_GOSSIP_MENU(19768, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* /*creature*/, uint32 /*sender*/, uint32 /*action*/) override
    {
        player->CLOSE_GOSSIP_MENU();
        player->CastSpell(player, SPELL_START_TRAINING, true);
        return true;
    }
};

class npc_boost_exit : public CreatureScript
{
public:
    npc_boost_exit() : CreatureScript("npc_boost_exit") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        player->ADD_GOSSIP_ITEM(GossipOptionNpc::None, "Take me to the Broken Shore.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        player->SEND_GOSSIP_MENU(creature->GetEntry() == NPC_ALLIANCE_EXIT ? 20085 : 20459, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* /*creature*/, uint32 /*sender*/, uint32 /*action*/) override
    {
        player->CLOSE_GOSSIP_MENU();
        player->CastSpell(player, SPELL_LEAVE_TUTORIAL, true);
        return true;
    }
};

struct npc_boost_training_dummy : public ScriptedAI
{
    npc_boost_training_dummy(Creature* creature) : ScriptedAI(creature)
    {
        me->SetReactState(REACT_PASSIVE);
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/) override
    {
        if (me->GetMapId() == MAP_ALLIANCE || me->GetMapId() == MAP_HORDE)
            damage = 0;
    }
};

class npc_boost_sparring_opponent : public CreatureScript
{
public:
    npc_boost_sparring_opponent() : CreatureScript("npc_boost_sparring_opponent") { }

    struct npc_boost_sparring_opponentAI : public ScriptedAI
    {
        npc_boost_sparring_opponentAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            if (me->GetMapId() == MAP_ALLIANCE || me->GetMapId() == MAP_HORDE)
                me->setFaction(14);
        }

        void IsSummonedBy(Unit* summoner) override
        {
            if (Player* player = summoner ? summoner->ToPlayer() : nullptr)
                AttackStart(player);
        }

        void JustDied(Unit* killer) override
        {
            Player* player = killer ? killer->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr;
            if (!player)
                if (TempSummon* summon = me->ToTempSummon())
                    player = summon->GetSummoner() ? summon->GetSummoner()->ToPlayer() : nullptr;

            if (player)
                player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, CRITERIA_SPAR_COMPLETE);
        }

        void UpdateAI(uint32 /*diff*/) override
        {
            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_boost_sparring_opponentAI(creature);
    }
};

void AddSC_boost_experience()
{
    new instance_boost_experience("instance_boost_experience_alliance", MAP_ALLIANCE, true);
    new instance_boost_experience("instance_boost_experience_horde", MAP_HORDE, false);
    new npc_boost_trainer();
    new npc_boost_exit();
    RegisterCreatureAI(npc_boost_training_dummy);
    new npc_boost_sparring_opponent();
}
