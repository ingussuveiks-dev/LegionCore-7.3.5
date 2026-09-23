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
#include "MotionMaster.h"
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
    // CUSTOM placement: only the deck origin is from WorldSafeLocs. NPC offsets
    // and the departure flight are approximations, NOT Blizzard sniff coordinates.
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
            _visibilityTimer(0), _lastCombatStep(0xFF), _exitSpawned(false) { }

        bool _alliance;
        uint32 _setupTimer;
        bool _setupComplete;
        uint32 _visibilityTimer;
        uint8 _lastCombatStep;
        bool _exitSpawned;
        ObjectGuid _transportGuid;
        std::vector<ObjectGuid> _passengerGuids;

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

            if (!_setupComplete)
            {
                SpawnStaticPassengers(transport);
                SpawnExit(); // The original tutorial also permits skipping lessons.
            }
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
            if (Player* player = GetPlayer())
                player->SendActionButtons(1);
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
        player->SEND_GOSSIP_MENU(29277, creature->GetGUID());
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
        player->SEND_GOSSIP_MENU(creature->GetEntry() == NPC_ALLIANCE_EXIT ? 29837 : 30357, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action) override
    {
        player->CLOSE_GOSSIP_MENU();
        if (sender != GOSSIP_SENDER_MAIN || action != GOSSIP_ACTION_INFO_DEF + 1 || player->GetVehicle())
            return true;

        // A separate world-space bird avoids moving a static gunship passenger.
        player->CombatStop(true);
        // Fill any newly learned active class spells into vacant real slots. The
        // tutorial overlay remains visible until OnMapChanged restores this bar.
        for (auto const& spell : player->GetSpellMapConst())
            player->AddSpellToActionBarIfAppropriate(spell.first, false);
        player->SummonCreature(creature->GetEntry(), creature->GetPosition(), TEMPSUMMON_TIMED_DESPAWN, 60000, 4933);
        return true;
    }

    struct DepartureAI : public ScriptedAI
    {
        DepartureAI(Creature* creature) : ScriptedAI(creature) { }
        ObjectGuid rider;
        uint32 timer = 0;
        uint8 phase = 0;

        void IsSummonedBy(Unit* summoner) override
        {
            // Static transport passengers have no player summoner.
            if (!summoner || !summoner->ToPlayer())
                return;
            rider = summoner->GetGUID();
            me->SetReactState(REACT_PASSIVE);
            me->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            // Vehicle 4933 belongs to the Horde tutorial exit bird in this DB.
            // Reuse its passenger layout for the Alliance model as well.
            if (!me->GetVehicleKit() && !me->CreateVehicleKit(4933, me->GetEntry()))
                return;
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
                // Map::SummonCreature inherits its summoner's transport.
                if (Transport* transport = me->GetTransport())
                    transport->RemovePassenger(me);
                if (Transport* transport = player->GetTransport())
                    transport->RemovePassenger(player);
                player->EnterVehicle(me, 0, true);
                // VehicleJoinEvent is asynchronous; check after the next updates.
                phase = 1;
                timer = 1000;
            }
            else if (phase == 1)
            {
                if (player->GetVehicleBase() != me)
                {
                    phase = 3;
                    timer = 1;
                    return;
                }
                // CUSTOM short takeoff route, not an official sniffed spline.
                me->GetMotionMaster()->MovePoint(1, me->GetPositionX() + 60.0f,
                    me->GetPositionY() + 40.0f, me->GetPositionZ() + 45.0f, false);
                phase = 2;
                timer = 10000;
            }
            else if (phase == 2)
            {
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

        bool surrendered = false;

        void DamageTaken(Unit* attacker, uint32& damage, DamageEffectType /*type*/) override
        {
            if (me->GetMapId() != MAP_ALLIANCE && me->GetMapId() != MAP_HORDE)
                return;
            if (surrendered)
            {
                damage = 0;
                return;
            }
            // CUSTOM 15% surrender threshold; the video confirms yielding, not
            // the exact health value. Clamp lethal hits before awarding progress.
            if (damage < me->GetHealth() && me->GetHealth() - damage > me->CountPctFromMaxHealth(15))
                return;
            damage = 0;
            surrendered = true;
            me->RemoveAllAuras();
            me->CombatStop(true);
            me->SetReactState(REACT_PASSIVE);
            me->setFaction(35);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetStandState(UNIT_STAND_STATE_KNEEL);
            me->MonsterSay("I yield!", LANG_UNIVERSAL, ObjectGuid::Empty);
            Player* player = attacker ? attacker->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr;
            if (!player)
                if (TempSummon* summon = me->ToTempSummon())
                    player = summon->GetSummoner() ? summon->GetSummoner()->ToPlayer() : nullptr;

            if (player)
                player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, CRITERIA_SPAR_COMPLETE);
            me->DespawnOrUnsummon(5000);
        }

        void UpdateAI(uint32 /*diff*/) override
        {
            if (surrendered || !UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_boost_sparring_opponentAI(creature);
    }
};

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
