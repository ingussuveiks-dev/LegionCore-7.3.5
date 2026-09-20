////////////////////////////////////////////////////////////////////////////////
///
///  MILLENIUM-STUDIO
///  Copyright 2015 Millenium-studio SARL
///  All Rights Reserved.
///
////////////////////////////////////////////////////////////////////////////////

#include "grimrail_depot.hpp"

/*
static BossScenarios const g_BossScenarios[] =
{
    { GrimrailDepotData::DataRocketsparkEncounter,   eGrimrailDepotScenarios::ScenarioBossRocketsparkAndBorka },
    { GrimrailDepotData::DataNitroggThundertower,    eGrimrailDepotScenarios::ScenarioBossNitroggThundertower },
    { GrimrailDepotData::DataSkyLordTovra,           eGrimrailDepotScenarios:: },
    { 0, 0 }
};
*/

class instance_grimrail_depot : public InstanceMapScript
{
public:

    instance_grimrail_depot() : InstanceMapScript("instance_grimrail_depot", 1208) { }

    struct instance_grimrail_depotAI : public InstanceScript
    {
        instance_grimrail_depotAI(InstanceMap* map) : InstanceScript(map) {}

        InstanceScript* m_Instance = this;

        ObjectGuid m_RocketsparkGUID;
        ObjectGuid m_BorkaGUID;
        ObjectGuid m_SkylordTovraGUID;
        ObjectGuid m_SkylordTovraDragonGUID;
        ObjectGuid m_RocketsparkAndBurkaEntrance;
        ObjectGuid m_RocketsparkAndBurkaExit;
        ObjectGuid m_NitrogThunderPowerGUID;
        ObjectGuid m_NitroggTurretGUID;
        ObjectGuid m_NitroggAssaultFlapsGUID;
        ObjectGuid m_NitroggInvisibleDoorGUID;
        ObjectGuid m_SkylordTovraDoorGUID;
        ObjectGuid m_TrainDoorGUID[6];
        uint32 m_TrainDoorCount;
        uint32 m_TrainDoorNumber;

        void Initialize() override
        {
            SetBossNumber(3);
            //LoadScenariosInfos(g_BossScenarios, instance->IsChallengeMode() ? eGrimrailDepotScenarios::IronDocksChallengeID : eIronDocksScenario::IronDocksScenarioID);

            m_RocketsparkGUID.Clear();
            m_BorkaGUID.Clear();
            m_NitrogThunderPowerGUID.Clear();
            m_SkylordTovraGUID.Clear();
            m_SkylordTovraDragonGUID.Clear();
            m_NitroggTurretGUID.Clear();
            m_NitroggAssaultFlapsGUID.Clear();
            m_NitroggInvisibleDoorGUID.Clear();
            m_SkylordTovraDoorGUID.Clear();
            m_RocketsparkAndBurkaEntrance.Clear();
            m_RocketsparkAndBurkaExit.Clear();
            for (ObjectGuid& guid : m_TrainDoorGUID)
                guid.Clear();
            m_TrainDoorCount = 0;
            m_TrainDoorNumber = 0;
        }

        void OnCreatureCreate(Creature* l_Creature) override
        {
            if (l_Creature)
            {
                switch (l_Creature->GetEntry())
                {
                    case GrimrailDepotBosses::BossRocketSpark:
                        m_RocketsparkGUID = l_Creature->GetGUID();
                        break;
                    case GrimrailDepotBosses::BossBorkatheBrute:
                        m_BorkaGUID = l_Creature->GetGUID();
                        break;
                    case GrimrailDepotBosses::BossNitroggThundertower:
                        m_NitrogThunderPowerGUID = l_Creature->GetGUID();
                        break;
                    case GrimrailDepotBosses::BossSkylordTovra:
                        m_SkylordTovraGUID = l_Creature->GetGUID();
                        break;
                    case GrimrailDepotCreatures::CreatureIronTurretNitrogg:
                        m_NitroggTurretGUID = l_Creature->GetGUID();
                        break;
                    case GrimrailDepotCreatures::CreatureSkylordTovraDragon:
                        m_SkylordTovraDragonGUID = l_Creature->GetGUID();
                        break;
                    default:
                        break;
                }
            }
        }

        void OnUnitDeath(Unit* p_Unit) override
        {
            Creature* l_Creature = p_Unit->ToCreature();
            if (!l_Creature)
                return;

            switch (l_Creature->GetEntry())
            {
            case GrimrailDepotBosses::BossSkylordTovra:
            {
                if (l_Creature->GetMap()->IsHeroic())
                    DoCompleteAchievement(GrimrailDepotAchivement::AchievementGrimrailDepotHeroic);
                else
                    DoCompleteAchievement(GrimrailDepotAchivement::AchievementGrimrailDepotNormal);
                break;
            }
            default:
                break;
            }
        }

        void OnGameObjectCreate(GameObject* p_Go) override
        {
            switch (p_Go->GetEntry())
            {
            case GrimrailDepotGobjects::GameObjectIronWroughtGate:
                m_RocketsparkAndBurkaExit = p_Go->GetGUID();
                break;
            case GrimrailDepotGobjects::GameObjectSpikedGate:
                m_RocketsparkAndBurkaEntrance = p_Go->GetGUID();
                break;
            case GrimrailDepotGobjects::GameObjectAssaultFlaps:
                m_NitroggAssaultFlapsGUID = p_Go->GetGUID();
                break;
            case GrimrailDepotGobjects::GameObjectInvisibleDoor:
                m_NitroggInvisibleDoorGUID = p_Go->GetGUID();
                break;
            case GrimrailDepotGobjects::GameObjectSpikedGateSkylordTovra:
                m_SkylordTovraDoorGUID = p_Go->GetGUID();
                break;
            case GrimrailDepotGobjects::GameObjectIronWroughtGate01OnTrain:
            case GrimrailDepotGobjects::GameObjectIronWroughtGate02OnTrain:
            case GrimrailDepotGobjects::GameObjectIronWroughtGate03OnTrain:
            case GrimrailDepotGobjects::GameObjectIronWroughtGate04OnTrain:
            case GrimrailDepotGobjects::GameObjectIronWroughtGate05OnTrain:
            case GrimrailDepotGobjects::GameObjectIronWroughtGate06OnTrain:
                m_TrainDoorGUID[p_Go->GetEntry() == GrimrailDepotGobjects::GameObjectIronWroughtGate01OnTrain ? 0 :
                    p_Go->GetEntry() == GrimrailDepotGobjects::GameObjectIronWroughtGate02OnTrain ? 1 :
                    p_Go->GetEntry() == GrimrailDepotGobjects::GameObjectIronWroughtGate03OnTrain ? 2 :
                    p_Go->GetEntry() == GrimrailDepotGobjects::GameObjectIronWroughtGate04OnTrain ? 3 :
                    p_Go->GetEntry() == GrimrailDepotGobjects::GameObjectIronWroughtGate05OnTrain ? 4 : 5] = p_Go->GetGUID();
                break;
            default:
                break;
            }
        }

        void SetData(uint32 type, uint32 data) override
        {
            if (type == GrimrailDepotData::DataIronWroughtGateOnTrainCount)
                m_TrainDoorCount = data;
            else if (type == GrimrailDepotData::DataIronWroughtGateOnTrainDoorNumber)
                m_TrainDoorNumber = data;
        }

        uint32 GetData(uint32 type) const override
        {
            if (type == GrimrailDepotData::DataIronWroughtGateOnTrainCount)
                return m_TrainDoorCount;
            if (type == GrimrailDepotData::DataIronWroughtGateOnTrainDoorNumber)
                return m_TrainDoorNumber;
            return 0;
        }

        ObjectGuid GetGuidData(uint32 p_Identifier) const override
        {
            switch (p_Identifier)
            {
                case GrimrailDepotData::DataRocketspark:
                    return m_RocketsparkGUID;
                    break;
                case GrimrailDepotData::DataBorka:
                    return m_BorkaGUID;
                    break;
                case GrimrailDepotData::DataNitroggThundertower:
                    return m_NitrogThunderPowerGUID;
                    break;
                case GrimrailDepotData::DataSkyLordTovra:
                    return m_SkylordTovraGUID;
                    break;
                case GrimrailDepotData::DataNitroggTurret:
                    return m_NitroggTurretGUID;
                    break;
                case GrimrailDepotData::DataAssaultFlaps:
                    return m_NitroggAssaultFlapsGUID;
                    break;
                case GrimrailDepotData::DataInvisibleDoor:
                    return m_NitroggInvisibleDoorGUID;
                    break;
                case GrimrailDepotData::DataSkyLordTovraDragon:
                    return m_SkylordTovraDragonGUID;
                    break;
                case GrimrailDepotData::DataSpikedGateSkylordTovraDoor:
                    return m_SkylordTovraDoorGUID;
                    break;
                case GrimrailDepotData::DataSpikedGateBoss1:
                    return m_RocketsparkAndBurkaEntrance;
                    break;
                case GrimrailDepotData::DataIronWroughtGate:
                    return m_RocketsparkAndBurkaExit;
                case GrimrailDepotData::DataIronWroughtGate01OnTrain:
                case GrimrailDepotData::DataIronWroughtGate02OnTrain:
                case GrimrailDepotData::DataIronWroughtGate03OnTrain:
                case GrimrailDepotData::DataIronWroughtGate04OnTrain:
                case GrimrailDepotData::DataIronWroughtGate05OnTrain:
                case GrimrailDepotData::DataIronWroughtGate06OnTrain:
                    return m_TrainDoorGUID[p_Identifier - GrimrailDepotData::DataIronWroughtGate01OnTrain];
                default:
                    break;
            }
            return ObjectGuid::Empty;
        }
    };

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_grimrail_depotAI(map);
    }

};

void AddSC_instance_grimrail_depot()
{
    new instance_grimrail_depot();
}
