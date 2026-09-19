/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "noodle_time.h"

#include <array>
#include <map>
#include <set>
#include <vector>

namespace
{
enum NoodleEvents
{
    EVENT_START_ROUND = 1,
    EVENT_SPAWN_CUSTOMER,
    EVENT_END_ROUND,
    EVENT_RESTART_ROUND
};

enum CustomerKind
{
    CUSTOMER_MERCHANT,
    CUSTOMER_SCHOLAR,
    CUSTOMER_FARMHAND
};

Position const NoodleStandPosition = { -269.45f, 604.40f, 167.55f, 6.20f };
Position const CustomerSpawnPosition = { -288.00f, 625.00f, 167.55f, 5.40f };
Position const CustomerQueuePosition = { -271.20f, 611.50f, 167.55f, 4.80f };
Position const FoodPositions[] =
{
    { -268.20f, 606.20f, 168.15f, 4.70f },
    { -267.20f, 606.20f, 168.15f, 4.70f },
    { -266.20f, 606.20f, 168.15f, 4.70f },
    { -265.20f, 606.20f, 168.15f, 4.70f }
};
Position const SeatPositions[] =
{
    { -258.413f, 597.519f, 167.548f, 1.33318f },
    { -263.689f, 601.899f, 167.548f, 1.33318f },
    { -252.592f, 598.724f, 167.548f, 1.33318f },
    { -257.276f, 602.899f, 167.548f, 1.33318f }
};

uint32 const NormalCustomers[] =
{
    NPC_MERCHANT_MALE, NPC_MERCHANT_FEMALE,
    NPC_SCHOLAR_MALE, NPC_SCHOLAR_FEMALE,
    NPC_FARMHAND_MALE, NPC_FARMHAND_FEMALE
};

uint32 const ImpatientCustomers[] =
{
    NPC_IMPATIENT_MERCHANT_MALE, NPC_IMPATIENT_MERCHANT_FEMALE,
    NPC_IMPATIENT_SCHOLAR_MALE, NPC_IMPATIENT_SCHOLAR_FEMALE,
    NPC_IMPATIENT_FARMHAND_MALE, NPC_IMPATIENT_FARMHAND_FEMALE
};

uint32 const HungryCustomers[] =
{
    NPC_HUNGRY_MERCHANT_MALE, NPC_HUNGRY_MERCHANT_FEMALE,
    NPC_HUNGRY_SCHOLAR_MALE, NPC_HUNGRY_SCHOLAR_FEMALE,
    NPC_HUNGRY_FARMHAND_MALE, NPC_HUNGRY_FARMHAND_FEMALE
};

CustomerKind GetCustomerKind(uint32 entry)
{
    switch (entry)
    {
        case NPC_SCHOLAR_MALE:
        case NPC_SCHOLAR_FEMALE:
        case NPC_IMPATIENT_SCHOLAR_MALE:
        case NPC_IMPATIENT_SCHOLAR_FEMALE:
        case NPC_HUNGRY_SCHOLAR_MALE:
        case NPC_HUNGRY_SCHOLAR_FEMALE:
            return CUSTOMER_SCHOLAR;
        case NPC_FARMHAND_MALE:
        case NPC_FARMHAND_FEMALE:
        case NPC_IMPATIENT_FARMHAND_MALE:
        case NPC_IMPATIENT_FARMHAND_FEMALE:
        case NPC_HUNGRY_FARMHAND_MALE:
        case NPC_HUNGRY_FARMHAND_FEMALE:
            return CUSTOMER_FARMHAND;
        default:
            return CUSTOMER_MERCHANT;
    }
}

uint32 GetPreferredFood(uint32 entry)
{
    switch (GetCustomerKind(entry))
    {
        case CUSTOMER_SCHOLAR:
            return NPC_NOODLE_SOUP;
        case CUSTOMER_FARMHAND:
            return NPC_PANDAREN_TREASURES_NOODLE_SOUP;
        default:
            return NPC_DRAGONS_NEST_NOODLE_SOUP;
    }
}

bool IsImpatient(uint32 entry)
{
    return entry >= NPC_IMPATIENT_MERCHANT_MALE && entry <= NPC_IMPATIENT_FARMHAND_FEMALE;
}

bool IsHungry(uint32 entry)
{
    return entry == NPC_HUNGRY_MERCHANT_MALE || entry == NPC_HUNGRY_MERCHANT_FEMALE ||
        (entry >= NPC_HUNGRY_SCHOLAR_MALE && entry <= NPC_HUNGRY_FARMHAND_FEMALE);
}
}

class instance_noodle_time : public InstanceMapScript
{
public:
    instance_noodle_time() : InstanceMapScript("instance_noodle_time", MAP_NOODLE_TIME) { }

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_noodle_time_InstanceMapScript(map);
    }

    struct instance_noodle_time_InstanceMapScript : public InstanceScript
    {
        struct CustomerState
        {
            uint8 SeatIndex = 0xFF;
            uint8 Servings = 0;
            uint32 TimeLeft = 40000;
            bool Seated = false;
            bool Hungry = false;
            bool Impatient = false;
        };

        instance_noodle_time_InstanceMapScript(InstanceMap* map) : InstanceScript(map) { }

        EventMap Events;
        std::array<ObjectGuid, 4> Seats;
        std::array<ObjectGuid, 4> SeatOccupants;
        std::map<ObjectGuid, CustomerState> Customers;
        std::set<ObjectGuid> FoodBowls;
        ObjectGuid SungshinGuid;
        ObjectGuid NoodleStandGuid;
        ObjectGuid SelectedCustomer;
        uint32 SelectedFood = 0;
        uint32 ScenarioId = 0;
        uint32 Score = 0;
        uint32 ServedCustomers = 0;
        uint8 EscapedCustomers = 0;
        bool RoundActive = false;
        bool RoundComplete = false;

        void Initialize() override
        {
            Events.Reset();
            Seats.fill(ObjectGuid::Empty);
            SeatOccupants.fill(ObjectGuid::Empty);
            Customers.clear();
            FoodBowls.clear();
            SungshinGuid.Clear();
            NoodleStandGuid.Clear();
            SelectedCustomer.Clear();
            SelectedFood = 0;
            ScenarioId = 0;
            Score = 0;
            ServedCustomers = 0;
            EscapedCustomers = 0;
            RoundActive = false;
            RoundComplete = false;
        }

        void OnPlayerEnter(Player* player) override
        {
            ScenarioId = player->GetScenarioId();

            if (!NoodleStandGuid)
                if (Creature* stand = instance->SummonCreature(NPC_NOODLE_STAND, NoodleStandPosition))
                {
                    NoodleStandGuid = stand->GetGUID();
                    player->EnterVehicle(stand, 0, true);
                }

            SpawnSeats();

            if (ScenarioId == SCENARIO_NOODLE_TIME)
                Events.ScheduleEvent(EVENT_START_ROUND, 1500);
        }

        void OnCreatureCreate(Creature* creature) override
        {
            switch (creature->GetEntry())
            {
                case NPC_SUNGSHIN_IRONPAW:
                    SungshinGuid = creature->GetGUID();
                    creature->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                    break;
                case NPC_NOODLE_STAND:
                    NoodleStandGuid = creature->GetGUID();
                    creature->SetReactState(REACT_PASSIVE);
                    break;
                case NPC_SEAT:
                case NPC_NOODLE_SOUP:
                case NPC_DRAGONS_NEST_NOODLE_SOUP:
                case NPC_PANDAREN_TREASURES_NOODLE_SOUP:
                    creature->SetReactState(REACT_PASSIVE);
                    creature->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
                    break;
                default:
                    if (IsNoodleCustomer(creature->GetEntry()))
                    {
                        creature->SetReactState(REACT_PASSIVE);
                        creature->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
                    }
                    break;
            }
        }

        void SetData(uint32 type, uint32 /*data*/) override
        {
            switch (type)
            {
                case DATA_START_TUTORIAL:
                    if (ScenarioId == SCENARIO_SECRET_INGREDIENT && !RoundActive && !RoundComplete)
                    {
                        DoUpdateAchievementCriteria(CRITERIA_TYPE_KILL_CREATURE, NPC_TUTORIAL_CREDIT, 1);
                        StartRound();
                    }
                    break;
                case DATA_COOK_NOODLE_SOUP:
                    SpawnFood(NPC_NOODLE_SOUP);
                    break;
                case DATA_COOK_DRAGONS_NEST_SOUP:
                    SpawnFood(NPC_DRAGONS_NEST_NOODLE_SOUP);
                    break;
                case DATA_COOK_PANDAREN_TREASURES_SOUP:
                    SpawnFood(NPC_PANDAREN_TREASURES_NOODLE_SOUP);
                    break;
                case DATA_CLEAR_CUSTOMERS:
                    ClearWaitingCustomers();
                    break;
                case DATA_CLEAR_FOOD:
                    ClearFood();
                    break;
                default:
                    break;
            }
        }

        void SetGuidData(uint32 type, ObjectGuid guid) override
        {
            switch (type)
            {
                case DATA_SELECT_CUSTOMER:
                    if (Customers.count(guid) && !Customers[guid].Seated)
                    {
                        SelectedCustomer = guid;
                        if (Creature* customer = instance->GetCreature(guid))
                            customer->HandleEmoteCommand(EMOTE_ONESHOT_WAVE);
                    }
                    break;
                case DATA_SELECT_SEAT:
                    SeatCustomer(guid);
                    break;
                case DATA_SELECT_FOOD:
                    SelectFood(guid);
                    break;
                case DATA_SERVE_CUSTOMER:
                    ServeCustomer(guid);
                    break;
                default:
                    break;
            }
        }

        void Update(uint32 diff) override
        {
            Events.Update(diff);
            while (uint32 eventId = Events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_START_ROUND:
                        StartRound();
                        break;
                    case EVENT_SPAWN_CUSTOMER:
                        SpawnCustomer();
                        if (RoundActive)
                            Events.ScheduleEvent(EVENT_SPAWN_CUSTOMER, 7000);
                        break;
                    case EVENT_END_ROUND:
                        if (RoundActive)
                            CompleteRound();
                        break;
                    case EVENT_RESTART_ROUND:
                        StartRound();
                        break;
                    default:
                        break;
                }
            }

            if (!RoundActive)
                return;

            std::vector<ObjectGuid> expired;
            for (auto& itr : Customers)
            {
                CustomerState& state = itr.second;
                if (state.TimeLeft <= diff)
                    expired.push_back(itr.first);
                else
                    state.TimeLeft -= diff;
            }

            for (ObjectGuid const& guid : expired)
                CustomerEscaped(guid);
        }

    private:
        bool IsNoodleCustomer(uint32 entry) const
        {
            return (entry >= NPC_MERCHANT_MALE && entry <= NPC_HUNGRY_MERCHANT_FEMALE) ||
                (entry >= NPC_HUNGRY_SCHOLAR_MALE && entry <= NPC_HUNGRY_FARMHAND_FEMALE);
        }

        void SpawnSeats()
        {
            for (uint8 i = 0; i < Seats.size(); ++i)
            {
                if (Seats[i] && instance->GetCreature(Seats[i]))
                    continue;

                if (Creature* seat = instance->SummonCreature(NPC_SEAT, SeatPositions[i]))
                    Seats[i] = seat->GetGUID();
            }
        }

        void StartRound()
        {
            if (RoundActive || RoundComplete)
                return;

            ClearCustomers(true);
            ClearFood();
            Score = 0;
            ServedCustomers = 0;
            EscapedCustomers = 0;
            SelectedFood = 0;
            SelectedCustomer.Clear();
            RoundActive = true;

            if (Creature* sungshin = instance->GetCreature(SungshinGuid))
                sungshin->AI()->Talk(0);

            Events.ScheduleEvent(EVENT_SPAWN_CUSTOMER, 1000);
            Events.ScheduleEvent(EVENT_END_ROUND, 300000);
        }

        void SpawnCustomer()
        {
            uint32 entry = 0;
            uint32 roll = urand(0, 99);
            if (roll < 20)
                entry = ImpatientCustomers[urand(0, sizeof(ImpatientCustomers) / sizeof(uint32) - 1)];
            else if (roll < 40)
                entry = HungryCustomers[urand(0, sizeof(HungryCustomers) / sizeof(uint32) - 1)];
            else
                entry = NormalCustomers[urand(0, sizeof(NormalCustomers) / sizeof(uint32) - 1)];

            if (Creature* customer = instance->SummonCreature(entry, CustomerSpawnPosition))
            {
                CustomerState state;
                state.Hungry = IsHungry(entry);
                state.Impatient = IsImpatient(entry);
                state.TimeLeft = state.Impatient ? 20000 : 40000;
                Customers[customer->GetGUID()] = state;
                customer->GetMotionMaster()->MovePoint(1, CustomerQueuePosition);
            }
        }

        void SpawnFood(uint32 entry)
        {
            if (!RoundActive || FoodBowls.size() >= 4)
                return;

            Position const& position = FoodPositions[FoodBowls.size()];
            if (Creature* food = instance->SummonCreature(entry, position))
                FoodBowls.insert(food->GetGUID());
        }

        void SelectFood(ObjectGuid guid)
        {
            if (!FoodBowls.erase(guid))
                return;

            if (Creature* food = instance->GetCreature(guid))
            {
                SelectedFood = food->GetEntry();
                food->DespawnOrUnsummon();
            }
        }

        void SeatCustomer(ObjectGuid seatGuid)
        {
            if (!RoundActive || !SelectedCustomer)
                return;

            uint8 seatIndex = 0xFF;
            for (uint8 i = 0; i < Seats.size(); ++i)
                if (Seats[i] == seatGuid)
                {
                    seatIndex = i;
                    break;
                }

            if (seatIndex == 0xFF || SeatOccupants[seatIndex])
                return;

            auto itr = Customers.find(SelectedCustomer);
            Creature* customer = instance->GetCreature(SelectedCustomer);
            if (itr == Customers.end() || !customer || itr->second.Seated)
            {
                SelectedCustomer.Clear();
                return;
            }

            CustomerState& state = itr->second;
            state.Seated = true;
            state.SeatIndex = seatIndex;
            state.TimeLeft = state.Impatient ? 15000 : 30000;
            SeatOccupants[seatIndex] = SelectedCustomer;
            customer->GetMotionMaster()->MovePoint(2, SeatPositions[seatIndex]);
            SelectedCustomer.Clear();
        }

        void ServeCustomer(ObjectGuid guid)
        {
            if (!RoundActive || !SelectedFood)
                return;

            auto itr = Customers.find(guid);
            Creature* customer = instance->GetCreature(guid);
            if (itr == Customers.end() || !customer || !itr->second.Seated)
                return;

            CustomerState& state = itr->second;
            bool preferred = SelectedFood == GetPreferredFood(customer->GetEntry());
            SelectedFood = 0;
            ++state.Servings;

            if (state.Hungry && state.Servings == 1)
            {
                AddScore(preferred ? 600 : 300);
                state.TimeLeft = state.Impatient ? 15000 : 30000;
                return;
            }

            AddScore(preferred ? (state.Hungry ? 1000 : 975) : 500);
            ++ServedCustomers;
            FinishCustomer(guid);

            if (ScenarioId == SCENARIO_SECRET_INGREDIENT && ServedCustomers >= 5)
                CompleteRound();
        }

        void AddScore(uint32 amount)
        {
            Score += amount;
        }

        void FinishCustomer(ObjectGuid guid)
        {
            auto itr = Customers.find(guid);
            if (itr == Customers.end())
                return;

            if (itr->second.SeatIndex < SeatOccupants.size())
                SeatOccupants[itr->second.SeatIndex].Clear();

            if (Creature* customer = instance->GetCreature(guid))
            {
                customer->HandleEmoteCommand(EMOTE_ONESHOT_CHEER);
                customer->DespawnOrUnsummon(1500);
            }

            Customers.erase(itr);
        }

        void CustomerEscaped(ObjectGuid guid)
        {
            auto itr = Customers.find(guid);
            if (itr == Customers.end())
                return;

            if (itr->second.SeatIndex < SeatOccupants.size())
                SeatOccupants[itr->second.SeatIndex].Clear();

            if (Creature* customer = instance->GetCreature(guid))
                customer->DespawnOrUnsummon();
            Customers.erase(itr);

            if (++EscapedCustomers < 5)
                return;

            RoundActive = false;
            Events.CancelEvent(EVENT_SPAWN_CUSTOMER);
            Events.CancelEvent(EVENT_END_ROUND);
            ClearCustomers(true);
            ClearFood();
            if (Creature* sungshin = instance->GetCreature(SungshinGuid))
                sungshin->AI()->Talk(2);
            Events.ScheduleEvent(EVENT_RESTART_ROUND, 3000);
        }

        void ClearWaitingCustomers()
        {
            std::vector<ObjectGuid> waiting;
            for (auto const& itr : Customers)
                if (!itr.second.Seated)
                    waiting.push_back(itr.first);

            for (ObjectGuid const& guid : waiting)
            {
                if (Creature* customer = instance->GetCreature(guid))
                    customer->DespawnOrUnsummon();
                Customers.erase(guid);
            }
            SelectedCustomer.Clear();
        }

        void ClearCustomers(bool includeSeated)
        {
            std::vector<ObjectGuid> removed;
            for (auto const& itr : Customers)
                if (includeSeated || !itr.second.Seated)
                    removed.push_back(itr.first);

            for (ObjectGuid const& guid : removed)
            {
                if (Creature* customer = instance->GetCreature(guid))
                    customer->DespawnOrUnsummon();
                Customers.erase(guid);
            }
            SeatOccupants.fill(ObjectGuid::Empty);
            SelectedCustomer.Clear();
        }

        void ClearFood()
        {
            for (ObjectGuid const& guid : FoodBowls)
                if (Creature* food = instance->GetCreature(guid))
                    food->DespawnOrUnsummon();
            FoodBowls.clear();
            SelectedFood = 0;
        }

        void CompleteRound()
        {
            if (!RoundActive || RoundComplete)
                return;

            RoundActive = false;
            RoundComplete = true;
            Events.CancelEvent(EVENT_SPAWN_CUSTOMER);
            Events.CancelEvent(EVENT_END_ROUND);
            ClearCustomers(true);
            ClearFood();

            DoUpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, CRITERIA_NOODLE_TIME_COMPLETE, 1);
            if (ScenarioId == SCENARIO_NOODLE_TIME)
                DoUpdateAchievementCriteria(CRITERIA_TYPE_KILL_CREATURE, NPC_TOTAL_SCORE, Score);

            if (Creature* sungshin = instance->GetCreature(SungshinGuid))
                sungshin->AI()->Talk(1);

            instance->ApplyOnEveryPlayer([](Player* player)
            {
                if (player->GetVehicle())
                    player->ExitVehicle();
            });
        }
    };
};

void AddSC_instance_noodle_time()
{
    new instance_noodle_time();
}
