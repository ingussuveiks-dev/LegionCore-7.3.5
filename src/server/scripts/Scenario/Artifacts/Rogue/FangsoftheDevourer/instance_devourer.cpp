/*
    Rogue: Devourer
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "MapManager.h"
#include "Scenario.h"
#include "ScenarioMgr.h"


class instance_devourer : public InstanceMapScript
{
public:
    instance_devourer() : InstanceMapScript("instance_devourer", 1607) { }

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_devourer_InstanceMapScript(map);
    }

    struct instance_devourer_InstanceMapScript : public InstanceScript
    {
        instance_devourer_InstanceMapScript(InstanceMap* map) : InstanceScript(map) {}
        
        WorldLocation loc_res_pla;  // for respawn
        std::list<Creature*> trashGUIDconteiner;
        uint32 outrotimer;
        
        void Initialize() override
        {
            trashGUIDconteiner.clear();
            outrotimer = 2000;
        }
        
        void OnCreatureCreate(Creature* creature) override
        {
            if (creature->GetEntry() == 98308 || creature->GetEntry() == 98307 || creature->GetEntry() == 54638)
                trashGUIDconteiner.push_back(creature);
        }
        
        void OnCreatureRemove(Creature* creature) override
        {
            if (creature->GetEntry() == 98308 || creature->GetEntry() == 98307 || creature->GetEntry() == 54638)
                trashGUIDconteiner.remove(creature);
        }
        
        
        WorldLocation* GetClosestGraveYard(float /*x*/, float /*y*/, float /*z*/) override
        {
            Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId());
            if (!scenario || scenario->GetScenarioId() != 1078)
                return nullptr;
            // Keep the earned floor boundaries, including the locked stage2
            // escape, and retain the last cemetery at completed sentinel7.
            uint32 step = scenario->GetCurrentStep();
            uint32 graveyardId = step >= 5 ? 5470 : step >= 3 ? 5469 : step >= 1 ? 5468 : 5467;
            WorldSafeLocsEntry const* gy = sWorldSafeLocsStore.LookupEntry(graveyardId);
            if (!gy || gy->ID != graveyardId || gy->MapID != 1607 ||
                (gy->Loc.X == 0.0f && gy->Loc.Y == 0.0f && gy->Loc.Z == 0.0f) ||
                !MapManager::IsValidMapCoord(1607, gy->Loc.X, gy->Loc.Y, gy->Loc.Z, gy->Loc.O * float(M_PI) / 180.0f))
            {
                TC_LOG_ERROR("scripts", "Devourer graveyard: invalid native WorldSafeLocs %u for instance %u", graveyardId, instance->GetInstanceId());
                return nullptr;
            }
            loc_res_pla.WorldRelocate(1607, gy->Loc.X, gy->Loc.Y, gy->Loc.Z, gy->Loc.O * float(M_PI) / 180.0f);
            return &loc_res_pla;
        }
        
        void Update(uint32 diff) override
        {
            if (getScenarionStep() == 7)
                if (outrotimer <= diff)
                {
                    if (!trashGUIDconteiner.empty())
                        for (auto trash : trashGUIDconteiner)
                        {
                            if (trash && urand(0, 1) == 1)
                                trash->CastSpell(trash, (trash->GetEntry() == 98308 ? 209814 : 209813));
                        }
                    outrotimer = 2000;
                }
                else outrotimer -= diff;
        }
    };
};

void AddSC_instance_devourer()
{
    new instance_devourer();
}
