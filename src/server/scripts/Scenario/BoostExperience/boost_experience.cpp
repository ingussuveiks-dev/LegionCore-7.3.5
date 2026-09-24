/*
 * Level 100 character boost tutorial (Legion, client 7.3.5).
 *
 * The client ships the class-specific scenarios and both faction gunship
 * maps, but the original server-side passengers and encounter controller
 * are not part of the public 7.3.5 database.  Keep the scenario criteria in
 * DB2 and supply only the missing server-controlled actors and waves here.
 */

#include <algorithm>
#include <cmath>

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
#include "MotionMaster.h"
#include "MoveSpline.h"
#include "ObjectAccessor.h"
#include "Vehicle.h"
#include "QuestData.h"

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

    NPC_TRAINING_DUMMY   = 107104,

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

enum InstanceData : uint32
{
    DATA_SPAR_SURRENDER = 1
};

enum class CombatStage : uint8
{
    None,
    OneOpponent,
    TwoOpponents,
    LegionAttack,
    Exit
};

constexpr CombatStage GetCombatStage(uint32 step, uint32 stepCount)
{
    if (stepCount < 4)
        return CombatStage::None;
    if (step == stepCount - 4)
        return CombatStage::OneOpponent;
    if (step == stepCount - 3)
        return CombatStage::TwoOpponents;
    if (step == stepCount - 2)
        return CombatStage::LegionAttack;
    return step >= stepCount - 1 ? CombatStage::Exit : CombatStage::None;
}

constexpr bool ShouldSparringOpponentSurrender(uint64 health, uint64 maxHealth, uint32 damage)
{
    return damage && (damage >= health || uint64(health - damage) * 100 <= uint64(maxHealth) * 15);
}

static_assert(GetCombatStage(9, 14) == CombatStage::None, "Ability lesson must precede combat");
static_assert(GetCombatStage(10, 14) == CombatStage::OneOpponent, "First opponent must spawn alone");
static_assert(GetCombatStage(11, 14) == CombatStage::TwoOpponents, "Second wave has two opponents");
static_assert(GetCombatStage(12, 14) == CombatStage::LegionAttack, "Legion attacks after sparring");
static_assert(GetCombatStage(13, 14) == CombatStage::Exit, "Exit follows the Legion attack");
static_assert(ShouldSparringOpponentSurrender(100, 100, 1000), "A one-shot hit must surrender");
static_assert(ShouldSparringOpponentSurrender(100, 100, 85), "The threshold hit must surrender");
static_assert(!ShouldSparringOpponentSurrender(100, 100, 84), "Early damage must not surrender");
static_assert(!ShouldSparringOpponentSurrender(10, 100, 0), "Zero damage must not surrender");
static_assert(ShouldSparringOpponentSurrender(10000, 10000, 20000), "Scaled lethal damage must surrender");
static_assert(!ShouldSparringOpponentSurrender(5000000000ULL, 5000000000ULL, 1), "Health must not truncate to 32 bits");

CreatureAI* CreateBoostSparringAI(Creature* creature);

struct BoostDeckDefenderAI : public ScriptedAI
{
    BoostDeckDefenderAI(Creature* creature) : ScriptedAI(creature) { }

    void UpdateAI(uint32 /*diff*/) override
    {
        if (UpdateVictim())
            DoMeleeAttackIfReady();
    }
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

uint8 GetFirstTargetLesson(uint32 scenarioId)
{
    // The opening lessons for these scenarios summon a pet, enter a form or
    // prepare a weapon before the first target-based ability is introduced.
    switch (scenarioId)
    {
        case 1083: // Beast Mastery Hunter: Call Pet
        case 1084: // Affliction Warlock: Summon Voidwalker
        case 1094: // Unholy Death Knight: Raise Dead
        case 1132: // Frost Mage: Summon Water Elemental
        case 1181: // Balance Druid: Moonkin Form
        case 1182: // Enhancement Shaman: Flametongue
        case 1214: // Survival Hunter: Call Pet
            return 2;
        case 1096: // Feral Druid: Cat Form, then Prowl
        case 1133: // Assassination Rogue: Deadly Poison, then Stealth
            return 3;
        default:
            return 1;
    }
}

void EnsureDoomguardShard(Player* player, uint32 step)
{
    // The earlier Affliction lessons can consume the last shard. The
    // Doomguard lesson requires one and must remain possible on reconnect.
    if (player && player->getClass() == CLASS_WARLOCK && step == 9 &&
        player->GetPower(POWER_SOUL_SHARDS) < 10)
        player->SetPower(POWER_SOUL_SHARDS, 10);
}

Position Offset(Position const& base, float x, float y, float z = 0.0f, float orientation = 0.0f)
{
    // CUSTOM placement in the player's starting frame: x is forward, y is
    // left. The faction safe locations face opposite directions on their
    // respective transports; raw shared X/Y offsets put actors in cabins.
    float const facing = base.GetOrientation();
    float const cosine = std::cos(facing);
    float const sine = std::sin(facing);
    return Position(base.GetPositionX() + x * cosine - y * sine,
        base.GetPositionY() + x * sine + y * cosine,
        base.GetPositionZ() + z, Position::NormalizeOrientation(facing + orientation));
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
            _visibilityTimer(0), _trainingDummySpawned(false), _lastCombatStep(0xFF), _exitSpawned(false) { }

        bool _alliance;
        uint32 _setupTimer;
        bool _setupComplete;
        uint32 _visibilityTimer;
        bool _trainingDummySpawned;
        uint8 _lastCombatStep;
        bool _exitSpawned;
        ObjectGuid _transportGuid;
        ObjectGuid _trainingDummyGuid;
        WorldLocation _graveyard;
        std::vector<ObjectGuid> _passengerGuids;
        std::vector<ObjectGuid> _defenderGuids;
        std::vector<ObjectGuid> _sparringGuids;
        std::vector<ObjectGuid> _creditedSparringGuids;
        std::vector<ObjectGuid> _yieldedSparringGuids;
        std::vector<ObjectGuid> _pendingCombatGuids;
        std::vector<ObjectGuid> _legionGuids;
        std::map<ObjectGuid, ObjectGuid> _combatTargets;
        uint32 _defenderTimer = 0;
        uint32 _secondWaveTimer = 0;

        bool IsActiveSparringOpponent(ObjectGuid guid) const
        {
            if (std::find(_sparringGuids.begin(), _sparringGuids.end(), guid) == _sparringGuids.end() ||
                std::find(_creditedSparringGuids.begin(), _creditedSparringGuids.end(), guid) != _creditedSparringGuids.end())
                return false;

            Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId());
            if (!scenario)
                return false;

            CombatStage stage = GetCombatStage(scenario->GetCurrentStep(), scenario->GetStepCount(false));
            return (stage == CombatStage::OneOpponent && _sparringGuids.size() == 1) ||
                (stage == CombatStage::TwoOpponents && _sparringGuids.size() == 2);
        }

        void OnCreatureDamageTaken(Creature* creature, Unit* attacker, uint32& damage) override
        {
            if (!creature)
                return;

            ObjectGuid guid = creature->GetGUID();
            if (std::find(_yieldedSparringGuids.begin(), _yieldedSparringGuids.end(), guid) != _yieldedSparringGuids.end())
            {
                damage = 0;
                return;
            }

            if (!IsActiveSparringOpponent(guid))
                return;

            // Sparring is completed by the trainee or their pet. Damage from
            // deck actors or unrelated summons must not advance the lesson.
            if (!attacker || attacker->GetCharmerOrOwnerPlayerOrPlayerItself() != GetPlayer())
            {
                damage = 0;
                return;
            }

            // The player can exceed the opponent's entire health pool with a
            // single spell. Clamp that hit before Unit::DealDamage can kill it.
            // Use the same target-relative health as Unit::DealDamage. The
            // unscaled template health can be much larger for level-100 players.
            if (!ShouldSparringOpponentSurrender(creature->GetHealth(attacker), creature->GetMaxHealth(attacker), damage))
                return;

            TC_LOG_INFO("scripts", "Boost tutorial opponent %s surrendered to %s: health %llu/%llu, final damage %u",
                guid.ToString().c_str(), attacker->GetGUID().ToString().c_str(),
                static_cast<unsigned long long>(creature->GetHealth(attacker)),
                static_cast<unsigned long long>(creature->GetMaxHealth(attacker)), damage);
            damage = 0;
            _yieldedSparringGuids.push_back(guid);
            creature->RemoveAllAuras();
            creature->CombatStop(true);
            creature->SetReactState(REACT_PASSIVE);
            creature->setFaction(35);
            creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            creature->SetStandState(UNIT_STAND_STATE_KNEEL);
            creature->MonsterSay("I yield!", LANG_UNIVERSAL, ObjectGuid::Empty);
            creature->DespawnOrUnsummon(5000);
            SetGuidData(DATA_SPAR_SURRENDER, guid);
        }

        void SetGuidData(uint32 type, ObjectGuid guid) override
        {
            if (type != DATA_SPAR_SURRENDER || !IsActiveSparringOpponent(guid))
                return;

            Player* player = GetPlayer();
            Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId());
            if (!player || !scenario)
                return;

            uint32 step = scenario->GetCurrentStep();

            _creditedSparringGuids.push_back(guid);
            player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, CRITERIA_SPAR_COMPLETE);
            TC_LOG_INFO("scripts", "Boost tutorial credited sparring opponent %s for %s at step %u",
                guid.ToString().c_str(), player->GetName(), step);
        }

        void CreatureDies(Creature* creature, Unit* killer) override
        {
            if (!creature)
                return;

            if (IsActiveSparringOpponent(creature->GetGUID()))
            {
                TC_LOG_ERROR("scripts", "Boost tutorial sparring opponent %s died before surrender; crediting fallback",
                    creature->GetGUID().ToString().c_str());
                SetGuidData(DATA_SPAR_SURRENDER, creature->GetGUID());
                return;
            }

            if (std::find(_legionGuids.begin(), _legionGuids.end(), creature->GetGUID()) == _legionGuids.end())
                return;

            // Allies can finish Legion attackers. Their kills still count for
            // the player's defense objective; player and pet kills are already
            // credited by the normal kill path.
            if (killer && !creature->GetLootRecipient() &&
                std::find(_defenderGuids.begin(), _defenderGuids.end(), killer->GetGUID()) != _defenderGuids.end())
                if (Player* player = GetPlayer())
                    player->UpdateAchievementCriteria(CRITERIA_TYPE_KILL_CREATURE, creature->GetEntry(), 1, 0, creature, true);
        }

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

        WorldLocation* GetClosestGraveYard(float /*x*/, float /*y*/, float /*z*/) override
        {
            Transport* transport = GetGunship();
            if (!transport)
                return nullptr;

            Position const& deck = Deck();
            float x = deck.GetPositionX();
            float y = deck.GetPositionY();
            float z = deck.GetPositionZ();
            float orientation = deck.GetOrientation();
            transport->CalculatePassengerPosition(x, y, z, &orientation);
            _graveyard.WorldRelocate(instance->GetId(), x, y, z, orientation);
            return &_graveyard;
        }

        TempSummon* SummonPassenger(Transport* transport, uint32 entry, Position const& position,
            Unit* summoner = nullptr, TempSummonType type = TEMPSUMMON_MANUAL_DESPAWN, uint32 duration = 0)
        {
            TempSummon* passenger = transport->SummonPassenger(entry, position, type, nullptr, duration, summoner);
            if (!passenger)
            {
                TC_LOG_ERROR("scripts", "Boost tutorial failed to summon passenger %u on transport %s",
                    entry, transport->GetGUID().ToString().c_str());
                return nullptr;
            }

            // Transport::SummonPassenger registers runtime summons as static
            // passengers. Moving transports may unload that collection when
            // their current grid is considered inactive, even while a player
            // is attached to the transport. The scenario then remains active
            // but its trainer, dummies and entourage all disappear. Move the
            // summon to the regular passenger collection, which is retained
            // for the lifetime of the runtime summon.
            transport->RemovePassenger(passenger);
            transport->AddPassenger(passenger);
            passenger->m_movementInfo.transport.Pos.Relocate(position);
            passenger->m_movementInfo.transport.VehicleSeatIndex = -1;
            _passengerGuids.push_back(passenger->GetGUID());

            // Instance transports do not inherit the player's phase set.  A
            // boost character usually has Legion intro phases, so passengers
            // created with the transport's empty phase set existed server-side
            // but were invisible to the player.
            if (Player* player = GetPlayer())
            {
                passenger->SetPhaseMask(player->GetPhaseMask(), false);
                passenger->SetPhaseId(player->GetPhases(), false);
                passenger->setIgnorePhaseIdCheck(true);
                passenger->UpdateObjectVisibility();
            }

            return passenger;
        }

        void SpawnStaticPassengers(Transport* transport)
        {
            Position const& deck = Deck();
            uint32 trainer = _alliance ? NPC_ALLIANCE_TRAINER : NPC_HORDE_TRAINER;
            uint32 allianceEntourage[] = { 95093, 105628, 107953, 107541 };
            uint32 hordeEntourage[] = { 112038, 112002, 112026, 112025 };
            uint32 const* entourage = _alliance ? allianceEntourage : hordeEntourage;

            if (TempSummon* creature = SummonPassenger(transport, trainer, Offset(deck, 5.0f, 0.0f, 0.0f, 3.14f)))
            {
                creature->SetReactState(REACT_PASSIVE);
                _defenderGuids.push_back(creature->GetGUID());
            }

            uint8 index = 0;
            for (uint8 entourageIndex = 0; entourageIndex < 4; ++entourageIndex)
            {
                uint32 entry = entourage[entourageIndex];
                if (TempSummon* creature = SummonPassenger(transport, entry,
                    Offset(deck, 8.0f, -6.0f + 4.0f * index++, 0.0f, 3.14f)))
                {
                    creature->SetReactState(REACT_PASSIVE);
                    _defenderGuids.push_back(creature->GetGUID());
                }
            }
        }

        void SetTrainingDummyForStep(Player* player, uint32 step, uint8 stepCount)
        {
            // Retail keeps one target dummy for all target-based ability
            // lessons. It disappears when the solo sparring opponent arrives.
            bool shouldSpawn = player && stepCount >= 5 &&
                step >= GetFirstTargetLesson(GetScenarioFor(player)) && step < uint32(stepCount - 4);

            if (_trainingDummySpawned == shouldSpawn)
                return;

            if (!_trainingDummyGuid.IsEmpty())
            {
                if (Creature* dummy = instance->GetCreature(_trainingDummyGuid))
                    dummy->DespawnOrUnsummon();

                _passengerGuids.erase(std::remove(_passengerGuids.begin(), _passengerGuids.end(),
                    _trainingDummyGuid), _passengerGuids.end());
                _trainingDummyGuid.Clear();
            }

            _trainingDummySpawned = false;
            if (!shouldSpawn)
                return;

            // Keep the target on the lesson deck beside the trainer, facing
            // across the lesson area. Both factions use the same local layout.
            Position const dummyPosition = Offset(Deck(), 5.0f, -8.0f, 0.0f, 1.5708f);
            if (Transport* transport = GetGunship())
                if (TempSummon* dummy = SummonPassenger(transport, NPC_TRAINING_DUMMY, dummyPosition))
                {
                    dummy->SetReactState(REACT_PASSIVE);
                    // Fear still applies for the lesson criterion, but its
                    // fleeing motion must not move a shipboard training dummy.
                    dummy->SetControlled(true, UNIT_STATE_ROOT);
                    _trainingDummyGuid = dummy->GetGUID();
                    _trainingDummySpawned = true;
                    _visibilityTimer = 500;
                }
        }

        void PutPlayerOnTransport(Player* player, Transport* transport)
        {
            Position const& deck = Deck();
            float x = deck.GetPositionX();
            float y = deck.GetPositionY();
            float z = deck.GetPositionZ();
            float orientation = deck.GetOrientation();
            transport->CalculatePassengerPosition(x, y, z, &orientation);

            // A character that logs out aboard the gunship is restored as an
            // existing passenger with the last client-reported local offset.
            // Do not keep that stale offset: every new tutorial instance must
            // start at the retail WorldSafeLoc pose, facing down the deck.
            if (Transport* currentTransport = player->GetTransport())
                if (currentTransport != transport)
                    currentTransport->RemovePassenger(player);

            if (player->GetTransport() != transport)
                transport->AddPassenger(player);

            player->m_movementInfo.transport.Guid = transport->GetGUID();
            player->m_movementInfo.transport.Pos.Relocate(deck);
            player->m_movementInfo.transport.VehicleSeatIndex = -1;
            player->NearTeleportTo(x, y, z, orientation);
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

            if (!_setupComplete)
                SpawnStaticPassengers(transport);

            SetTrainingDummyForStep(player, scenario->GetCurrentStep(), scenario->GetStepCount(false));
            EnsureDoomguardShard(player, scenario->GetCurrentStep());
            scenario->SendStepUpdate(player, true);
            player->SendActionButtons(1);
            setScenarioStep(scenario->GetCurrentStep());
            TC_LOG_INFO("scripts", "Started boost tutorial scenario %u for %s on map %u (player phases: %u, transport phases: %u)",
                scenarioId, player->GetName(), instance->GetId(), uint32(player->GetPhases().size()), uint32(transport->GetPhases().size()));
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
            if (_secondWaveTimer)
            {
                if (_secondWaveTimer > diff)
                    _secondWaveTimer -= diff;
                else
                {
                    _secondWaveTimer = 0;
                    if (Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId()))
                        if (GetCombatStage(scenario->GetCurrentStep(), scenario->GetStepCount(false)) == CombatStage::TwoOpponents)
                            SpawnSparringWave(2);
                }
            }

            if (_defenderTimer)
            {
                if (_defenderTimer > diff)
                    _defenderTimer -= diff;
                else
                {
                    _defenderTimer = 1000;
                    for (ObjectGuid const& defenderGuid : _defenderGuids)
                    {
                        Creature* defender = instance->GetCreature(defenderGuid);
                        if (!defender || !defender->IsAlive() || !defender->AI() || defender->getVictim())
                            continue;

                        Creature* nextTarget = nullptr;
                        uint32 leastSupport = uint32(_defenderGuids.size()) + 1;
                        for (ObjectGuid const& legionGuid : _legionGuids)
                            if (Creature* attacker = instance->GetCreature(legionGuid))
                                if (attacker->IsAlive() && attacker->IsInWorld())
                                {
                                    if (_combatTargets[legionGuid] == defenderGuid)
                                    {
                                        nextTarget = attacker;
                                        break;
                                    }
                                    // Once an ally's own pair is defeated, help
                                    // the remaining fight without all selecting
                                    // the first enemy in the spawn list.
                                    uint32 support = 0;
                                    for (ObjectGuid const& allyGuid : _defenderGuids)
                                        if (Creature* ally = instance->GetCreature(allyGuid))
                                            if (ally->getVictim() == attacker)
                                                ++support;
                                    if (support < leastSupport)
                                    {
                                        leastSupport = support;
                                        nextTarget = attacker;
                                    }
                                }
                        if (nextTarget)
                            defender->AI()->AttackStart(nextTarget);
                    }
                }
            }

            // Transport::SummonPassenger queues creatures with AddToMapWait.
            // Their AI is initialized only when the map processes that queue.
            if (!_pendingCombatGuids.empty())
            {
                if (Player* player = GetPlayer())
                    _pendingCombatGuids.erase(std::remove_if(_pendingCombatGuids.begin(), _pendingCombatGuids.end(),
                        [this, player](ObjectGuid const& guid)
                        {
                            Creature* summon = instance->GetCreature(guid);
                            if (!summon || !summon->IsInWorld() || !summon->AI())
                                return false;

                            // Runtime transport summons can receive a generic summon AI
                            // when the template script binding is missing or stale.
                            // Install the tutorial AI explicitly before combat begins.
                            if (std::find(_sparringGuids.begin(), _sparringGuids.end(), guid) != _sparringGuids.end())
                            {
                                CreatureAI* sparringAI = CreateBoostSparringAI(summon);
                                if (!summon->AIM_Initialize(sparringAI))
                                {
                                    delete sparringAI;
                                    return false;
                                }
                            }

                            Unit* target = ObjectAccessor::GetUnit(*summon, _combatTargets[guid]);
                            if (!target || !target->IsAlive())
                                target = player;
                            summon->AddThreat(target, 100.0f);
                            summon->AI()->AttackStart(target);
                            TC_LOG_INFO("scripts", "Boost tutorial attacker %s assigned to %s",
                                guid.ToString().c_str(), target->GetGUID().ToString().c_str());
                            if (Creature* defender = target->ToCreature())
                                if (defender->AI() && !defender->getVictim())
                                    defender->AI()->AttackStart(summon);
                            return true;
                        }), _pendingCombatGuids.end());
                else
                    _pendingCombatGuids.clear();
            }

            // Runtime passengers are queued for addition to the map. Their
            // first visibility update can therefore run before they are in
            // world, and transport passengers are skipped by the ordinary
            // grid notifier afterwards. Synchronize them once the add queue
            // has completed so the client receives their create packets.
            if (_visibilityTimer)
            {
                if (_visibilityTimer > diff)
                    _visibilityTimer -= diff;
                else
                {
                    _visibilityTimer = 0;
                    if (Player* player = GetPlayer())
                    {
                        for (ObjectGuid const& guid : _passengerGuids)
                        {
                            Creature* passenger = instance->GetCreature(guid);
                            if (!passenger || !passenger->IsInWorld())
                            {
                                TC_LOG_ERROR("scripts", "Boost tutorial passenger %s was not added to map %u instance %u",
                                    guid.ToString().c_str(), instance->GetId(), instance->GetInstanceId());
                                continue;
                            }

                            bool alreadyVisible = player->HaveAtClient(passenger);
                            if (!alreadyVisible)
                            {
                                passenger->SendUpdateToPlayer(player);
                                player->AddClient(passenger->GetGUID());
                                player->SendInitialVisiblePackets(passenger);
                            }

                            TC_LOG_INFO("scripts", "Boost tutorial passenger %u %s for %s (distance %.2f, client visible %u -> %u)",
                                passenger->GetEntry(), passenger->GetGUID().ToString().c_str(), player->GetName(),
                                player->GetDistance(passenger), uint32(alreadyVisible), uint32(player->HaveAtClient(passenger)));
                        }
                    }
                }
            }

            if (!_setupTimer)
                return;

            if (_setupTimer > diff)
            {
                _setupTimer -= diff;
                return;
            }

            if (Setup())
            {
                _setupComplete = true;
                _setupTimer = 0;
                _visibilityTimer = 500;
            }
            else
                _setupTimer = 500;
        }

        void StartCombat(TempSummon* summon, Unit* target)
        {
            if (!summon || !target)
                return;

            summon->setFaction(14);
            summon->SetReactState(REACT_DEFENSIVE);
            _combatTargets[summon->GetGUID()] = target->GetGUID();
            _pendingCombatGuids.push_back(summon->GetGUID());
        }

        void ClearSparringWave()
        {
            for (ObjectGuid const& guid : _sparringGuids)
            {
                if (Creature* opponent = instance->GetCreature(guid))
                    if (std::find(_yieldedSparringGuids.begin(), _yieldedSparringGuids.end(), guid) == _yieldedSparringGuids.end())
                        opponent->DespawnOrUnsummon();

                _passengerGuids.erase(std::remove(_passengerGuids.begin(), _passengerGuids.end(), guid),
                    _passengerGuids.end());
                _pendingCombatGuids.erase(std::remove(_pendingCombatGuids.begin(), _pendingCombatGuids.end(), guid),
                    _pendingCombatGuids.end());
                _combatTargets.erase(guid);
            }
            _sparringGuids.clear();
            _creditedSparringGuids.clear();
        }

        void SpawnSparringWave(uint8 count)
        {
            Transport* transport = GetGunship();
            Player* player = GetPlayer();
            if (!transport || !player)
                return;

            uint32 entry = _alliance ? NPC_ALLIANCE_SPAR : NPC_HORDE_SPAR;
            Position const& deck = Deck();
            ClearSparringWave();
            for (uint8 index = 0; index < count; ++index)
                if (TempSummon* opponent = SummonPassenger(transport, entry,
                    Offset(deck, 4.0f, count == 1 ? -7.0f : -7.0f + 14.0f * index, 0.0f, 3.14f), nullptr,
                    TEMPSUMMON_CORPSE_TIMED_DESPAWN, 15000))
                {
                    _sparringGuids.push_back(opponent->GetGUID());
                    StartCombat(opponent, player);
                    TC_LOG_INFO("scripts", "Boost tutorial sparring %s initialized: level %u, target level %u, health %llu/%llu, multiplier %.6f",
                        opponent->GetGUID().ToString().c_str(), uint32(opponent->getLevel()), uint32(opponent->GetLevelForTarget(player)),
                        static_cast<unsigned long long>(opponent->GetHealth(player)),
                        static_cast<unsigned long long>(opponent->GetMaxHealth(player)), opponent->GetHealthMultiplierForTarget(player));
                }

            TC_LOG_INFO("scripts", "Boost tutorial spawned sparring wave %u/%u for %s",
                uint32(_sparringGuids.size()), uint32(count), player->GetName());
        }

        void SpawnLegionWave()
        {
            Transport* transport = GetGunship();
            Player* player = GetPlayer();
            if (!transport || !player)
                return;

            Position const& deck = Deck();
            _legionGuids.clear();
            _defenderTimer = 1000;
            std::vector<Unit*> targets{ player };
            for (ObjectGuid const& guid : _defenderGuids)
                if (Creature* defender = instance->GetCreature(guid))
                {
                    CreatureAI* defenderAI = new BoostDeckDefenderAI(defender);
                    if (!defender->AIM_Initialize(defenderAI))
                        delete defenderAI;
                    defender->setFaction(player->getFaction());
                    defender->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NON_ATTACKABLE);
                    defender->SetReactState(REACT_AGGRESSIVE);
                    targets.push_back(defender);
                }

            for (uint8 index = 0; index < 10; ++index)
                if (TempSummon* attacker = SummonPassenger(transport, NPC_LEGION_IMP,
                    Offset(deck, 2.0f + float(index % 5) * 2.0f, -8.0f + float(index / 5) * 16.0f, 0.0f, 3.14f),
                    nullptr, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 15000))
                {
                    _legionGuids.push_back(attacker->GetGUID());
                    StartCombat(attacker, targets[(_legionGuids.size() - 1) % targets.size()]);
                }

            TC_LOG_INFO("scripts", "Boost tutorial spawned Legion imps %u/10 for %s",
                uint32(_legionGuids.size()), player->GetName());

            if (TempSummon* attacker = SummonPassenger(transport, NPC_LEGION_INFERNAL, Offset(deck, 3.0f, 4.0f, 0.0f, 3.14f),
                nullptr, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 15000))
            {
                _legionGuids.push_back(attacker->GetGUID());
                StartCombat(attacker, targets[(_legionGuids.size() - 1) % targets.size()]);
            }
            if (TempSummon* attacker = SummonPassenger(transport, NPC_LEGION_BAT, Offset(deck, 0.0f, 8.0f, 3.0f),
                nullptr, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 15000))
            {
                _legionGuids.push_back(attacker->GetGUID());
                StartCombat(attacker, targets[(_legionGuids.size() - 1) % targets.size()]);
            }
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
            if (Player* player = GetPlayer())
            {
                EnsureDoomguardShard(player, newStep);
                player->SendActionButtons(1);
            }
            if (_lastCombatStep == newStep)
                return;

            Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId());
            if (!scenario)
                return;

            uint8 stepCount = scenario->GetStepCount(false);
            SetTrainingDummyForStep(GetPlayer(), newStep, stepCount);
            switch (GetCombatStage(newStep, stepCount))
            {
                case CombatStage::OneOpponent:
                    SpawnSparringWave(1);
                    break;
                case CombatStage::TwoOpponents:
                    _secondWaveTimer = 2000;
                    break;
                case CombatStage::LegionAttack:
                    ClearSparringWave();
                    SpawnLegionWave();
                    break;
                case CombatStage::Exit:
                    _defenderTimer = 0;
                    SpawnExit();
                    break;
                default:
                    return;
            }

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
        player->SEND_GOSSIP_MENU(29277, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* /*creature*/, uint32 sender, uint32 action) override
    {
        if (sender != GOSSIP_SENDER_MAIN || action != GOSSIP_ACTION_INFO_DEF + 1)
            return true;

        player->CLOSE_GOSSIP_MENU();
        player->CastSpell(player, SPELL_START_TRAINING, true);

        // The opening criteria tree contains separate Horde and Alliance
        // children under an ALL operator. The valid faction child reaches 1/1,
        // but the mutually exclusive sibling prevents the generic criteria
        // evaluator from advancing the step. The trainer is the only valid
        // source of the opening spell, so finish this faction gate explicitly.
        if (Scenario* scenario = sScenarioMgr->GetScenario(player->GetInstanceId()))
            if (scenario->GetCurrentStep() == 0 && scenario->GetStepCount(false) > 1)
            {
                scenario->SetCurrentStep(1);
                TC_LOG_INFO("scripts", "Boost tutorial trainer advanced %s to scenario %u step 1",
                    player->GetName(), scenario->GetScenarioId());
            }

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
        player->SEND_GOSSIP_MENU(creature->GetEntry() == NPC_ALLIANCE_EXIT ? 29837 : 30357, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action) override
    {
        player->CLOSE_GOSSIP_MENU();
        if (sender != GOSSIP_SENDER_MAIN || action != GOSSIP_ACTION_INFO_DEF + 1 || player->GetVehicle())
            return true;

        player->CombatStop(true);
        // Fill any newly learned active class spells into vacant real slots. The
        // tutorial overlay remains visible until OnMapChanged restores this bar.
        for (auto const& spell : player->GetSpellMapConst())
            player->AddSpellToActionBarIfAppropriate(spell.first, false);
        // Board the actual gossip bird. A second, identical vehicle left the
        // client showing the rider on deck while the other bird flew away.
        if (creature->AI())
            creature->AI()->SetGUID(player->GetGUID());
        return true;
    }

    struct DepartureAI : public ScriptedAI
    {
        DepartureAI(Creature* creature) : ScriptedAI(creature) { }
        ObjectGuid rider;
        uint32 timer = 0;
        uint8 phase = 0;
        uint32 boardingWait = 0;

        void RestoreRiderControl(Player* player)
        {
            // Seat 16967 does not charm its vehicle, so the generic vehicle
            // removal path does not restore the active mover for this ride.
            if (player->GetUnitBeingMoved() == me)
                player->SetClientControl(player, true);
            else if (player->GetViewpoint() == me)
                player->SetViewpoint(me, false);
        }

        void PassengerBoarded(Unit* passenger, int8 /*seatId*/, bool apply) override
        {
            if (!apply)
                if (Player* player = passenger->ToPlayer())
                    RestoreRiderControl(player);
        }

        void SetGUID(ObjectGuid const& guid, int32 /*id*/ = 0) override
        {
            if (!rider.IsEmpty())
                return;
            rider = guid;
            me->SetReactState(REACT_PASSIVE);
            me->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            // Vehicle 4933 belongs to the Horde tutorial exit bird in this DB.
            // Reuse its passenger layout for the Alliance model as well.
            if (!me->GetVehicleKit() && !me->CreateVehicleKit(4933, me->GetEntry()))
            {
                rider.Clear();
                me->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                return;
            }
            Position worldPosition = me->GetPosition();
            if (Transport* transport = me->GetTransport())
                transport->RemovePassenger(me);
            me->NearTeleportTo(worldPosition);
            me->setActive(true);
            timer = 500;
        }

        void UpdateAI(uint32 diff) override
        {
            if (!timer)
                return;
            if (timer > diff)
            {
                timer -= diff;
                return;
            }
            timer = 0;
            Player* player = ObjectAccessor::GetPlayer(*me, rider);
            if (!player)
                return;
            if (phase == 0)
            {
                me->SetCanFly(true);
                me->SetDisableGravity(true);
                player->StopMoving();
                if (Transport* transport = player->GetTransport())
                    transport->RemovePassenger(player);
                player->EnterVehicle(me, 0, true);
                // Wait for VehicleJoinEvent AND its boarding spline to finish.
                phase = 1;
                boardingWait = 0;
                timer = 100;
            }
            else if (phase == 1)
            {
                if (player->GetVehicleBase() != me || !player->movespline->Finalized())
                {
                    boardingWait += 100;
                    if (boardingWait >= 10000)
                    {
                        TC_LOG_ERROR("scripts", "Boost tutorial boarding did not complete for %s", player->GetName());
                        phase = 3;
                    }
                    timer = 100;
                    return;
                }
                TC_LOG_INFO("scripts", "Boost tutorial departure bird %s boarded by %s; starting flight",
                    me->GetGUID().ToString().c_str(), player->GetName());
                // This seat is a passenger seat, not a controllable vehicle.
                // A farsight field alone leaves the client's active mover on
                // the ship. Use the same handoff as other scripted flights:
                // send control-off and active-mover packets for the bird while
                // the server drives it. SetClientControl also sets viewpoint.
                me->GetVehicleKit()->RelocatePassengers();
                player->SendMovementFlagUpdate(true);
                player->SetClientControl(me, false);
                TC_LOG_INFO("scripts", "Boost tutorial departure handoff for %s: mover %s, vehicle %s",
                    player->GetName(), player->GetUnitBeingMoved()->GetGUID().ToString().c_str(),
                    player->GetVehicleBase()->GetGUID().ToString().c_str());
                // CUSTOM short takeoff route, not an official sniffed spline.
                me->GetMotionMaster()->MovePoint(1, me->GetPositionX() + 60.0f,
                    me->GetPositionY() + 40.0f, me->GetPositionZ() + 45.0f, false);
                phase = 2;
                timer = 10000;
            }
            else if (phase == 2)
            {
                RestoreRiderControl(player);
                uint32 questId = player->GetTeam() == ALLIANCE ? 40518 : 42740;
                if (player->GetQuestStatus(questId) == QUEST_STATUS_NONE && !player->GetQuestRewardStatus(questId))
                    if (Quest const* quest = sQuestDataStore->GetQuestTemplate(questId))
                        if (player->CanAddQuest(quest, false))
                            player->AddQuest(quest, nullptr);
                // 219912 targets passenger 0, not the caster. Casting it on the
                // player silently missed the queue spell and left them stranded.
                if (player->GetVehicleBase() == me)
                    me->CastSpell(me, SPELL_LEAVE_TUTORIAL, true);
                phase = 3;
                timer = 15000;
            }
            else
            {
                // If the Broken Shore queue cannot launch, recover at the faction
                // capital intro instead of leaving a character on a despawned bird.
                RestoreRiderControl(player);
                player->ExitVehicle();
                bool alliance = player->GetTeam() == ALLIANCE;
                player->TeleportTo(alliance ? 0 : 1,
                    alliance ? -8833.38f : 1569.59f,
                    alliance ? 628.62f : -4397.63f,
                    alliance ? 94.00f : 16.06f, alliance ? 1.06f : 0.54f);
                me->DespawnOrUnsummon(1000);
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override { return new DepartureAI(creature); }
};

struct npc_boost_training_dummy : public ScriptedAI
{
    npc_boost_training_dummy(Creature* creature) : ScriptedAI(creature)
    {
        me->AddUnitTypeMask(UNIT_MASK_TRAINING_DUMMY);
        me->SetReactState(REACT_PASSIVE);
    }

    // A training target must not chase attackers or run an evade/home spline
    // when a lesson changes. Its position is maintained by the moving ship.
    void AttackStart(Unit* /*target*/) override { }
    void UpdateAI(uint32 /*diff*/) override { }

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

CreatureAI* BoostExperience::CreateBoostSparringAI(Creature* creature)
{
    return new npc_boost_sparring_opponent::npc_boost_sparring_opponentAI(creature);
}

class player_boost_tutorial_bars : public PlayerScript
{
public:
    player_boost_tutorial_bars() : PlayerScript("player_boost_tutorial_bars") { }
    void OnMapChanged(Player* player) override
    {
        // Outside the tutorial SendActionButtons uses the untouched full layout.
        // This also covers teleports, disconnect recovery and the skip route.
        player->SendActionButtons(1);
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
    new player_boost_tutorial_bars();
}
