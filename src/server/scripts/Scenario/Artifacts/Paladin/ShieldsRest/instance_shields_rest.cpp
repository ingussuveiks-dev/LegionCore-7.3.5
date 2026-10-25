/*
    Paladin Specialization: Protection
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "shields_rest.h"
#include "Scenario.h"
#include "ScenarioMgr.h"
#include <array>



class instance_shields_rest : public InstanceMapScript
{
public:
    instance_shields_rest() : InstanceMapScript("instance_shields_rest", 1495) {}

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_shields_rest_InstanceMapScript(map);
    }

    struct instance_shields_rest_InstanceMapScript : public InstanceScript
    {
        instance_shields_rest_InstanceMapScript(InstanceMap* map) : InstanceScript(map) {}

        std::list<GameObject*> goDoorList;
        std::list<ObjectGuid> trashList;
        ObjectGuid eventDoorGUID;

        uint8 cryptstalkersDiedCount;
        uint8 windshaperDiedCount;
        uint8 igrimEvent;

        uint8 count_kills;
        std::array<ObjectGuid, 2> magnarGuardGUIDs;
        std::array<bool, 2> magnarGuardDeaths;
        ObjectGuid magnarCompanionGUID;
        ObjectGuid magnarDoorGUID;
        bool magnarGuardCreditSent;
        bool magnarGuardsDelivered;
        bool magnarGuardDispatching;
        uint32 magnarGuardGeneration = 0;
        uint16 scenarioid;

        void Initialize() override
        {
            goDoorList.clear();
            trashList.clear();
            eventDoorGUID.Clear();

            cryptstalkersDiedCount = 0;
            windshaperDiedCount = 0;
            igrimEvent = 0;
            count_kills = 0;
            magnarGuardGUIDs = {};
            magnarGuardDeaths = {};
            magnarCompanionGUID.Clear();
            magnarDoorGUID.Clear();
            magnarGuardCreditSent = false;
            magnarGuardsDelivered = false;
            magnarGuardDispatching = false;
            ++magnarGuardGeneration;
            scenarioid = 0;
        }

        void OnPlyerEnter(Player* player)
        {
            if (!player)
                return;

            player->AddDelayedEvent(3000, [player, this]() -> void
            {
                scenarioid = player->GetScenarioId();
            });
        }

        void OnCreatureCreate(Creature* creature) override
        {
            if (creature->GetEntry() == 96455)
                for (uint32 slot = 0; slot < magnarGuardGUIDs.size(); ++slot)
                    if (creature->GetDBTableGUIDLow() == 361412 + slot)
                    {
                        bool recordedDeath = magnarGuardDeaths[slot] && !creature->IsAlive();
                        magnarGuardGUIDs[slot] = creature->GetGUID();
                        magnarGuardDeaths[slot] = recordedDeath;
                    }
            if (creature->GetEntry() == 96468 && creature->GetDBTableGUIDLow() == 361306)
                magnarCompanionGUID = creature->GetGUID();
            switch (creature->GetEntry())
            {
                case NPC_INVISIBLE_MAN:
                case NPC_ORIK_TRUEHEART_PET:
                case NPC_CATO_PET:
                    trashList.push_back(creature->GetGUID());
                    break;
            }
        }

        void OnGameObjectCreate(GameObject* go) override
        {
            if (go->GetEntry() == 243897 && go->GetDBTableGUIDLow() == 134170)
                magnarDoorGUID = go->GetGUID();
            switch (go->GetEntry())
            {
                case GO_FIRST_DOOR:
                    goDoorList.push_back(go);
                    break;
                case GO_EVENT_DOOR:
                    eventDoorGUID = go->GetGUID();
                    break;
            }
        }

        void OnUnitDeath(Unit* unit) override
        {
            if (unit->GetTypeId() == TYPEID_PLAYER)
            {
                unit->RemoveAurasDueToSpell(SPELL_SUMMON_ORIK);
                unit->RemoveAurasDueToSpell(SPELL_SUMMON_CATO);

                if (getScenarionStep() == DATA_STAGE_3)
                {
                    if (GameObject* go = instance->GetGameObject(eventDoorGUID))
                    {
                        go->ResetDoorOrButton();
                        go->SetLootState(GO_NOT_READY);
                    }
                }
            }

            switch (unit->GetEntry())
            {
                case NPC_INNA_THE_CRYPTSTALKER:
                case NPC_DRAKE_SHAE:
                {
                    cryptstalkersDiedCount++;
                    if (cryptstalkersDiedCount == 2)
                    {
                        if (!goDoorList.empty())
                            for (auto go : goDoorList)
                            {
                                if (go->GetDistance(doorPos[0]) < go->GetDistance(doorPos[1]))
                                    HandleGameObject(go->GetGUID(), true);
                            }
                        if (!trashList.empty())
                            for (auto trashGuid : trashList)
                            {
                                if (trashGuid.GetEntry() == NPC_INVISIBLE_MAN)
                                    if (Creature* wind = instance->GetCreature(trashGuid))
                                        wind->CastSpell(wind, SPELL_SPIRIT_WINDS_AT, true);
                            }
                    }
                    break;
                }
                case NPC_SPECTRAL_WINDSHAPER:
                    windshaperDiedCount++;
                    if (windshaperDiedCount == 3)
                    {
                        if (!trashList.empty())
                            for (auto trashGuid : trashList)
                            {
                                if (trashGuid.GetEntry() == NPC_INVISIBLE_MAN)
                                    if (Creature* wind = instance->GetCreature(trashGuid))
                                    {
                                        wind->RemoveAurasDueToSpell(SPELL_SPIRIT_WINDS_AT);
                                        wind->RemoveAllAreaObjects();
                                    }
                            }
                        if (GetData(DATA_YRGRIM_EVENT) == 2)
                            SetData(DATA_YRGRIM_EVENT, 3);
                    }
                    break;
                case 94813:
                    count_kills++;
                    if (count_kills == 2)
                    {
                        if (GameObject* go = unit->FindNearestGameObject(243541, 70.0f))
                            go->SetPhaseMask(4, true);
                        if (Creature* targ = unit->FindNearestCreature(96468, 100.0f, true))
                            targ->AI()->DoAction(1);
                    }
                    break;
                case 96455:
                    RegisterMagnarGuardDeath(unit);
                    break;
            }
        }

        void RegisterMagnarGuardDeath(Unit* unit)
        {
            Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId());
            if (!unit || !unit->IsInWorld() || unit->IsAlive() || unit->GetMap() != instance ||
                !scenario || scenario->GetScenarioId() != 909 || scenario->GetCurrentStep() > 2)
                return;
            for (uint32 slot = 0; slot < magnarGuardGUIDs.size(); ++slot)
                if (unit->GetGUID() == magnarGuardGUIDs[slot])
                    magnarGuardDeaths[slot] = true;
            TryAdvanceMagnarGuards();
        }

        void TryAdvanceMagnarGuards()
        {
            if (magnarGuardsDelivered || magnarGuardDispatching)
                return;
            Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId());
            if (!scenario || scenario->GetScenarioId() != 909 ||
                (scenario->GetCurrentStep() != 2 && !(magnarGuardCreditSent && scenario->GetCurrentStep() == 3)))
                return;
            if (scenario->GetCurrentStep() == 2)
            {
                if (!magnarGuardDeaths[0] || !magnarGuardDeaths[1])
                    return;
                for (uint32 slot = 0; slot < magnarGuardGUIDs.size(); ++slot)
                    if (Creature* guard = instance->GetCreature(magnarGuardGUIDs[slot]))
                        if (guard->IsAlive())
                        {
                            magnarGuardDeaths[slot] = false;
                            return;
                        }
            }
            Creature* companion = instance->GetCreature(magnarCompanionGUID);
            GameObject* door = instance->GetGameObject(magnarDoorGUID);
            if (!companion || !companion->IsInWorld() || !companion->IsAlive() || companion->IsDespawn() ||
                companion->GetEntry() != 96468 || !companion->IsAIEnabled || !companion->AI() ||
                !door || !door->IsInWorld() || door->GetEntry() != 243897 || !door->InSamePhase(companion))
                return;
            Player* recipient = nullptr;
            for (auto const& reference : instance->GetPlayers())
            {
                Player* player = reference.getSource();
                if (player && player->IsInWorld() && player->CanContact() && !player->isGameMaster() &&
                    !player->IsBeingTeleported() && player->getClass() == CLASS_WARRIOR && player->GetMap() == instance &&
                    player->GetInstanceId() == instance->GetInstanceId() && player->InSamePhase(companion) &&
                    player->IsWithinDistInMap(companion, 150.0f) && !player->GetQuestRewardStatus(39191) &&
                    (player->GetQuestStatus(39191) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(39191) == QUEST_STATUS_COMPLETE) &&
                    (!recipient || companion->GetDistance(player) < companion->GetDistance(recipient)))
                    recipient = player;
            }
            if (!recipient)
                return;
            uint32 generation = magnarGuardGeneration;
            magnarGuardDispatching = true;
            if (scenario->GetCurrentStep() == 2)
            {
                CreatureAI* expectedAI = companion->AI();
                expectedAI->DoAction(2);
                if (generation != magnarGuardGeneration)
                    return;
                scenario = sScenarioMgr->GetScenario(instance->GetInstanceId());
                companion = instance->GetCreature(magnarCompanionGUID);
                if (!scenario || scenario->GetScenarioId() != 909 || scenario->GetCurrentStep() != 2 ||
                    !companion || !companion->IsInWorld() || !companion->IsAlive() || companion->IsDespawn() ||
                    !companion->IsAIEnabled || companion->AI() != expectedAI || !expectedAI->GetData(1059))
                {
                    magnarGuardDispatching = false;
                    return;
                }
                magnarGuardCreditSent = true;
                recipient->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 46358);
            }
            if (generation != magnarGuardGeneration)
                return;
            scenario = sScenarioMgr->GetScenario(instance->GetInstanceId());
            companion = instance->GetCreature(magnarCompanionGUID);
            door = instance->GetGameObject(magnarDoorGUID);
            if (scenario && scenario->GetScenarioId() == 909 && scenario->GetCurrentStep() == 3 &&
                companion && companion->IsInWorld() && companion->IsAlive() && !companion->IsDespawn() &&
                companion->GetEntry() == 96468 && companion->IsAIEnabled && companion->AI() &&
                door && door->IsInWorld() && door->GetEntry() == 243897 && door->InSamePhase(companion))
            {
                magnarGuardsDelivered = true;
                door->SetPhaseMask(4, true);
                companion->AI()->DoAction(2);
            }
            magnarGuardDispatching = false;
        }

        void SetData(uint32 type, uint32 data) override
        {
            switch (type)
            {
                case DATA_YRGRIM_EVENT:
                {
                    igrimEvent = data;
                    if (!trashList.empty())
                        for (auto supportGuid : trashList)
                        {
                            if (supportGuid.GetEntry() == NPC_ORIK_TRUEHEART_PET || supportGuid.GetEntry() == NPC_CATO_PET)
                            {
                                if (Creature* support = instance->GetCreature(supportGuid))
                                {
                                    if (data == 2)
                                        support->CastSpell(support, SPELL_FIERCE_WINDS_STUN, true);
                                    else if (data == 3)
                                        support->RemoveAurasDueToSpell(SPELL_FIERCE_WINDS_STUN);
                                    else if (data == 5)
                                    {
                                        if (GameObject* go = instance->GetGameObject(eventDoorGUID))
                                            go->RemoveFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE);
                                    }
                                }
                            }
                        }
                    break;
                }
                default:
                    break;
            }
        }

        uint32 GetData(uint32 type) const override
        {
            switch (type)
            {
                case 1059:
                    return magnarGuardCreditSent;
                case 1060:
                    return magnarGuardGeneration;
                case DATA_YRGRIM_EVENT:
                    return igrimEvent;
                default:
                    return 0;
            }
        }

        ObjectGuid GetGuidData(uint32 type) const override
        {
            switch (type)
            {
                case 0:
                default:
                    return ObjectGuid::Empty;
            }
        }

        void ProcessEvent(WorldObject* obj, uint32 eventId) override
        {
            switch (eventId)
            {
                case 50566:
                    if (getScenarionStep() == DATA_STAGE_3)
                    {
                        DoUpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 50566); //Step 4
                        if (!goDoorList.empty())
                            for (auto go : goDoorList)
                            {
                                if (go->GetDistance(doorPos[1]) < go->GetDistance(doorPos[0]))
                                    HandleGameObject(go->GetGUID(), true);
                            }
                    }
                    if (GameObject* go = instance->GetGameObject(eventDoorGUID))
                        go->Delete();
                    break;
                case 50545:
                {
                    for (auto supportGuid : trashList)
                    {
                        if (supportGuid.GetEntry() == NPC_ORIK_TRUEHEART_PET || supportGuid.GetEntry() == NPC_CATO_PET)
                            if (Creature* support = instance->GetCreature(supportGuid))
                                support->AI()->DoAction(true);
                    }
                    break;
                }
                default:
                    break;
            }
        }

        void onScenarionNextStep(uint32 newStep) override
        {
            if (scenarioid && scenarioid != 1082)
                return;

            switch (newStep)
            {
                case DATA_STAGE_2:
                    DoCastSpellOnPlayers(SPELL_SUMMON_ORIK);
                    DoCastSpellOnPlayers(SPELL_SUMMON_CATO);
                    break;
            }
        }

        uint32 hunterWindTimer = 0;
        void Update(uint32 diff) override
        {
            if (hunterWindTimer > diff) { hunterWindTimer -= diff; return; }
            hunterWindTimer = 500;
            TryAdvanceMagnarGuards();
            auto scenario = sScenarioMgr->GetScenario(instance->GetInstanceId());
            if (!scenario || scenario->GetScenarioId() != 1068)
                return;
            bool active = scenario->GetCurrentStep() == 1 && windshaperDiedCount < 3;
            for (auto guid : trashList)
                if (auto helper = instance->GetCreature(guid))
                    // Native Hunter corridor emitters; the other invisible
                    // actors are rune/ritual effects and must remain untouched.
                    if (helper->GetDBTableGUIDLow() == 365439 || helper->GetDBTableGUIDLow() == 365440)
                    {
                        if (active && !helper->HasAura(SPELL_SPIRIT_WINDS_AT))
                            helper->CastSpell(helper, SPELL_SPIRIT_WINDS_AT, true);
                        else if (!active && helper->HasAura(SPELL_SPIRIT_WINDS_AT))
                        {
                            helper->RemoveAurasDueToSpell(SPELL_SPIRIT_WINDS_AT);
                            helper->RemoveAllAreaObjects();
                        }
                    }
        }
    };
};

void AddSC_instance_shields_rest()
{
    new instance_shields_rest();
}
