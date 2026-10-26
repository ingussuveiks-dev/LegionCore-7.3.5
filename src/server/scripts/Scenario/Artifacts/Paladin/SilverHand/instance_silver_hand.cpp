/*
    Paladin Specialization: Holy
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "Scenario.h"
#include "ScenarioMgr.h"


class instance_silver_hand : public InstanceMapScript
{
public:
    instance_silver_hand() : InstanceMapScript("instance_silver_hand", 1539) {}

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_silver_hand_InstanceMapScript(map);
    }

    struct instance_silver_hand_InstanceMapScript : public InstanceScript
    {
        instance_silver_hand_InstanceMapScript(InstanceMap* map) : InstanceScript(map) {}

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

        WorldLocation* GetClosestGraveYard(float x, float y, float z) override
        {
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

void AddSC_instance_silver_hand()
{
    new instance_silver_hand();
}
