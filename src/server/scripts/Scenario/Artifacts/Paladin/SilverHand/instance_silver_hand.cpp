/*
    Paladin Specialization: Holy
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "Scenario.h"
#include "ScenarioMgr.h"
#include "MapManager.h"
#include "silver_hand_first_fight.h"
#include "../../Priest/slaghammer_handoff.h"


class instance_silver_hand : public InstanceMapScript
{
public:
    instance_silver_hand() : InstanceMapScript("instance_silver_hand", 1539) {}

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_silver_hand_InstanceMapScript(map);
    }

    struct instance_silver_hand_InstanceMapScript : public InstanceScript, public SlaghammerHandoff::Instance
    {
        instance_silver_hand_InstanceMapScript(InstanceMap* map) : InstanceScript(map) {}

        void OnPlayerEnter(Player* player) override
        {
            slaghammer.Rejoin(player);
        }

        void OnCreatureCreate(Creature* creature) override
        {
            switch (creature->GetEntry())
            {
                case 106094:
                    if (creature->HasAura(211502))
                        creature->SetVisible(false);
                    break;
            }
        }

        WorldLocation loc_res_pla;

        void CreatureDies(Creature* creature, Unit* /*killer*/) override
        {
            SilverHandFirstFight::DeliverDeath(creature);
        }

        WorldLocation* GetClosestGraveYard(float x, float y, float z) override
        {
            Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId());
            if (scenario && scenario->GetScenarioId() == 1037)
            {
                // Keep the outdoor start until actual49022 accepts tomb entry.
                // Only49058 at6->7 acknowledges the passage into Zakajz's prison.
                uint32 step = scenario->GetCurrentStep();
                uint32 safeId = step < 4 ? 5339 : step < 7 ? 5335 : 5705;
                WorldSafeLocsEntry const* safe = sWorldSafeLocsStore.LookupEntry(safeId);
                if (!safe || safe->ID != safeId || safe->MapID != 1539 || !(safe->Loc.X || safe->Loc.Y || safe->Loc.Z) ||
                    !MapManager::IsValidMapCoord(1539, safe->Loc.X, safe->Loc.Y, safe->Loc.Z, safe->Loc.O * float(M_PI) / 180.0f))
                {
                    TC_LOG_ERROR("scripts", "Stromkar graveyard: invalid native WorldSafeLocs %u for instance %u", safeId, instance->GetInstanceId());
                    return nullptr;
                }
                loc_res_pla.WorldRelocate(1539, safe->Loc.X, safe->Loc.Y, safe->Loc.Z, safe->Loc.O * float(M_PI) / 180.0f);
                return &loc_res_pla;
            }
            if (scenario && scenario->GetScenarioId() == 991 && scenario->GetCurrentStep() >= 2)
            {
                // Native48121 acknowledges actual tomb entry at1->2. Only the
                // final passage guard's48393 at6->7 unlocks the prison checkpoint.
                uint32 safeId = scenario->GetCurrentStep() < 7 ? 5335 : 5351;
                WorldSafeLocsEntry const* safe = sWorldSafeLocsStore.LookupEntry(safeId);
                if (!safe || safe->ID != safeId || safe->MapID != 1539 || !(safe->Loc.X || safe->Loc.Y || safe->Loc.Z) ||
                    !MapManager::IsValidMapCoord(1539, safe->Loc.X, safe->Loc.Y, safe->Loc.Z, safe->Loc.O * float(M_PI) / 180.0f))
                {
                    TC_LOG_ERROR("scripts", "Shadow graveyard: invalid native WorldSafeLocs %u for instance %u", safeId, instance->GetInstanceId());
                    return nullptr;
                }
                loc_res_pla.WorldRelocate(1539, safe->Loc.X, safe->Loc.Y, safe->Loc.Z, safe->Loc.O * float(M_PI) / 180.0f);
                return &loc_res_pla;
            }
            if (scenario && scenario->GetScenarioId() == 1092)
            {
                // 50766 admits the crypt,50897 the prison. After50912 the
                // escape barrier closes behind the party; recover upstairs.
                uint32 step = scenario->GetCurrentStep();
                uint32 safeId = step < 2 ? 5488 : step >= 5 && step < 7 ? 5694 : 5693;
                WorldSafeLocsEntry const* safe = sWorldSafeLocsStore.LookupEntry(safeId);
                if (!safe || safe->ID != safeId || safe->MapID != 1539 || !(safe->Loc.X || safe->Loc.Y || safe->Loc.Z) ||
                    !MapManager::IsValidMapCoord(1539, safe->Loc.X, safe->Loc.Y, safe->Loc.Z, safe->Loc.O * float(M_PI) / 180.0f))
                {
                    TC_LOG_ERROR("scripts", "Silver Hand graveyard: invalid native WorldSafeLocs %u for instance %u", safeId, instance->GetInstanceId());
                    return nullptr;
                }
                loc_res_pla.WorldRelocate(1539, safe->Loc.X, safe->Loc.Y, safe->Loc.Z, safe->Loc.O * float(M_PI) / 180.0f);
                return &loc_res_pla;
            }
            // Other1539 scenarios retain their existing general safe location.
            loc_res_pla.WorldRelocate(1539, x, y, z);

            uint32 graveyardId = 5281;

            if (graveyardId)
            {
                if (WorldSafeLocsEntry const* gy = sWorldSafeLocsStore.LookupEntry(graveyardId))
                {
                    loc_res_pla.WorldRelocate(gy->MapID, gy->Loc.X, gy->Loc.Y, gy->Loc.Z);
                }
            }
            return &loc_res_pla;
        }
        
        GuidSet _finaleActorsStarted;
        bool _finaleDiscoveryPending = false;

        void QueueFinaleDiscovery()
        {
            if (_finaleDiscoveryPending)
                return;

            _finaleDiscoveryPending = true;
            TC_LOG_WARN("scripts", "Silver Hand finale: missing actors; retry discovery in1000ms");
            AddDelayedEvent(1000, [this]() -> void
            {
                _finaleDiscoveryPending = false;
                Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId());
                if (!scenario || scenario->GetScenarioId() != 1092 || scenario->GetCurrentStep() != 10)
                {
                    TC_LOG_WARN("scripts", "Silver Hand finale: dropping stale actor discovery");
                    return;
                }
                onScenarionNextStep(10);
            });
        }

        void CastBlindingLight(Creature* boss)
        {
            SpellCastResult result = boss->CastSpell(boss, 215260);
            boss->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_IMMUNE_TO_PC| UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_ATTACKABLE_1);
            if (result == SPELL_CAST_OK)
                return;

            ObjectGuid guid = boss->GetGUID();
            TC_LOG_WARN("scripts", "Silver Hand finale: Blinding Light failed with result %u; retry in1000ms", uint32(result));
            AddDelayedEvent(1000, [this, guid]() -> void
            {
                Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId());
                Creature* boss = instance->GetCreature(guid);
                if (!scenario || scenario->GetScenarioId() != 1092 || scenario->GetCurrentStep() != 10 ||
                    !boss || boss->GetGUID() != guid || !boss->IsInWorld() || boss->IsPreDelete() || !boss->IsAlive() ||
                    boss->GetEntry() != 106719 || boss->GetMapId() != 1539 || boss->GetMap() != instance)
                {
                    _finaleActorsStarted.erase(guid);
                    TC_LOG_WARN("scripts", "Silver Hand finale: dropping stale Blinding Light retry");
                    return;
                }
                CastBlindingLight(boss);
            });
        }

        void onScenarionNextStep(uint32 newStep) override
        {
            // This map also hosts the Priest and Warrior artifact scenarios.
            Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId());
            if (!scenario || scenario->GetScenarioId() != 1092)
                return;

            if (newStep == 8)
            {
                Map::PlayerList const &PlList = instance->GetPlayers();
                if (PlList.isEmpty())
                    return;
                
                for (Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
                {
                    Player* player = i->getSource();
                    if (!player || !player->CanContact())
                        continue;

                    if (Creature* travar = player->FindNearestCreature(106676, 70.0f, true))
                    {
                        travar->AI()->DoAction(true);
                        break;
                    }
                }
            }
            if (newStep == 10)
            {
                Map::PlayerList const &PlList = instance->GetPlayers();

                if (PlList.isEmpty())
                {
                    QueueFinaleDiscovery();
                    return;
                }

                bool travardFound = false;
                bool mordothFound = false;
                for (Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
                {
                    Player* player = i->getSource();
                    if (!player || !player->CanContact())
                        continue;

                    if (!travardFound)
                    {
                        if (Creature* travar = player->FindNearestCreature(106777, 70.0f, true))
                        {
                            travardFound = true;
                            if (_finaleActorsStarted.insert(travar->GetGUID()).second)
                                travar->AI()->Talk(3);
                        }
                    }
                    if (!mordothFound)
                    {
                        if (Creature* boss = player->FindNearestCreature(106719, 70.0f, true))
                        {
                            mordothFound = true;
                            if (_finaleActorsStarted.insert(boss->GetGUID()).second)
                                CastBlindingLight(boss);
                        }
                    }
                    if (travardFound && mordothFound)
                        break;
                }
                if (!travardFound || !mordothFound)
                    QueueFinaleDiscovery();
            }
        }
    };
};

class npc_slaghammer_handoff : public CreatureScript
{
public:
    npc_slaghammer_handoff() : CreatureScript("npc_slaghammer_handoff") { }
    CreatureAI* GetAI(Creature* creature) const override { return new SlaghammerHandoff::AI(creature); }
};

void AddSC_instance_silver_hand()
{
    new instance_silver_hand();
    new npc_slaghammer_handoff();
}
