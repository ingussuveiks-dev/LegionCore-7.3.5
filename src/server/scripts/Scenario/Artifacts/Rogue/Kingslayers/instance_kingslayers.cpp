/*
    Rogue: Kingslayers
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "KingslayersGuardProgress.h"
#include "Map.h"
#include "MapManager.h"
#include "Scenario.h"
#include "ScenarioMgr.h"


class instance_kingslayers : public InstanceMapScript
{
public:
    instance_kingslayers() : InstanceMapScript("instance_kingslayers", 1620) { }

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_kingslayers_InstanceMapScript(map);
    }

    struct instance_kingslayers_InstanceMapScript : public InstanceScript
    {
        instance_kingslayers_InstanceMapScript(InstanceMap* map) : InstanceScript(map) {}

        struct GuardState
        {
            ObjectGuid guid;
            bool defeated = false;
        };
        mutable GuardState altheaGuards[3];

        uint32 GuardSlot(Creature* creature) const
        {
            if (creature && instance->GetId() == 1620 && creature->GetMap() == instance)
                for (uint32 i = 0; i < 3; ++i)
                    if (creature->GetDBTableGUIDLow() == KingslayersGuards::Spawns[i] && creature->GetEntry() == KingslayersGuards::Entries[i])
                        return i;
            return 3;
        }

        void OnCreatureCreateForScript(Creature* creature) override
        {
            InstanceScript::OnCreatureCreateForScript(creature);
            uint32 slot = GuardSlot(creature);
            if (slot < 3)
            {
                altheaGuards[slot].guid = creature->GetGUID();
                if (creature->IsAlive())
                    altheaGuards[slot].defeated = false;
            }
        }

        void OnCreatureRemoveForScript(Creature* creature) override
        {
            uint32 slot = GuardSlot(creature);
            if (slot < 3 && altheaGuards[slot].guid == creature->GetGUID() && creature->IsAlive())
                altheaGuards[slot].defeated = false;
            InstanceScript::OnCreatureRemoveForScript(creature);
        }

        void CreatureDiesForScript(Creature* creature, Unit* killer) override
        {
            // Unit::Kill delivers this even when a guard's current AI is absent.
            // Disappearance, a spatial query and arbitrary SetData are not death.
            uint32 slot = GuardSlot(creature);
            if (slot < 3 && altheaGuards[slot].guid == creature->GetGUID() && !creature->IsAlive())
                altheaGuards[slot].defeated = true;
            InstanceScript::CreatureDiesForScript(creature, killer);
        }

        ObjectGuid GetGuidData(uint32 type) const override
        {
            for (uint32 i = 0; i < 3; ++i)
                if (type == KingslayersGuards::Spawns[i])
                    return altheaGuards[i].guid;
            return InstanceScript::GetGuidData(type);
        }

        uint32 GetData(uint32 type) const override
        {
            bool all = true;
            for (uint32 i = 0; i < 3; ++i)
            {
                if (Creature* creature = instance->GetCreature(altheaGuards[i].guid))
                    if (creature->IsAlive())
                        altheaGuards[i].defeated = false; // Same-GUID native respawn is a new living guard.
                if (type == KingslayersGuards::Spawns[i])
                    return altheaGuards[i].defeated;
                all = all && altheaGuards[i].defeated;
            }
            if (type == KingslayersGuards::AllDefeated)
                return all;
            return InstanceScript::GetData(type);
        }
        
        WorldLocation loc_res_pla;  // for respawn

        WorldLocation* GetClosestGraveYard(float /*x*/, float /*y*/, float /*z*/) override
        {
            Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId());
            if (!scenario || scenario->GetScenarioId() != 1123)
                return nullptr;
            // Keep the native stage8-10 cemetery after the completed sentinel10.
            uint32 step = scenario->GetCurrentStep();
            uint32 graveyardId = step >= 7 ? 5530 : step >= 5 ? 5529 : step >= 2 ? 5528 : 5518;
            WorldSafeLocsEntry const* gy = sWorldSafeLocsStore.LookupEntry(graveyardId);
            if (!gy || gy->ID != graveyardId || gy->MapID != 1620 ||
                (gy->Loc.X == 0.0f && gy->Loc.Y == 0.0f && gy->Loc.Z == 0.0f) ||
                !MapManager::IsValidMapCoord(1620, gy->Loc.X, gy->Loc.Y, gy->Loc.Z, gy->Loc.O * float(M_PI) / 180.0f))
            {
                TC_LOG_ERROR("scripts", "Kingslayers graveyard: invalid native WorldSafeLocs %u for instance %u", graveyardId, instance->GetInstanceId());
                return nullptr;
            }
            loc_res_pla.WorldRelocate(1620, gy->Loc.X, gy->Loc.Y, gy->Loc.Z, gy->Loc.O * float(M_PI) / 180.0f);
            return &loc_res_pla;
        }
        
        void Update(uint32 diff) override {}
    };
};

void AddSC_instance_kingslayers()
{
    new instance_kingslayers();
}
