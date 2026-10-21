/*
    Dungeon : Dark Heart Thicket 100-110
*/

#include "darkheart_thicket.h"
#include <set>
#include <sstream>

DoorData const doorData[] =
{
    {GO_GLAIDALIS_FIRE_DOOR,     DATA_GLAIDALIS,     DOOR_TYPE_ROOM,     BOUNDARY_NONE},
    {GO_DRESARON_FIRE_DOOR,      DATA_DRESARON,      DOOR_TYPE_ROOM,     BOUNDARY_NONE},
    {GO_OAKHEART_DOOR,           DATA_OAKHEART,      DOOR_TYPE_ROOM,     BOUNDARY_NONE},
    {0,                        0,                  DOOR_TYPE_ROOM,     BOUNDARY_NONE},
};

class instance_darkheart_thicket : public InstanceMapScript
{
public:
    instance_darkheart_thicket() : InstanceMapScript("instance_darkheart_thicket", 1466) {}

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_darkheart_thicket_InstanceMapScript(map);
    }

    struct instance_darkheart_thicket_InstanceMapScript : public InstanceScript
    {
        instance_darkheart_thicket_InstanceMapScript(InstanceMap* map) : InstanceScript(map)
        {
            SetHeaders(DataHeader);
            SetBossNumber(MAX_ENCOUNTER);
        }

        WorldLocation loc_res_pla;

        ObjectGuid MalfurionGUID;
        ObjectGuid MalfurionCageGUID;
        ObjectGuid InvisDoorGUID;
        ObjectGuid DresaronGUID;
        ObjectGuid OakheartGUID;
        std::set<uint64> slainDwellers;

        void Initialize() override
        {
            LoadDoorData(doorData);
        }

        bool SetBossState(uint32 type, EncounterState state) override
        {
            if (!InstanceScript::SetBossState(type, state))
                return false;
            
            return true;
        }

        void OnCreatureCreate(Creature* creature) override
        {
            switch (creature->GetEntry())
            {
                case NPC_OAKHEART:
                    OakheartGUID = creature->GetGUID();
                    if (GetData(DATA_OAKHEART_AWAKENED) && creature->IsAIEnabled)
                        creature->AI()->DoAction(ACTION_AWAKEN_OAKHEART);
                    break;
                case NPC_NIGHTMARE_DWELLER:
                    if (!creature->IsAlive())
                        RecordDwellerDeath(creature);
                    break;
                case NPC_MALFURION_STORMRAGE:    
                    MalfurionGUID = creature->GetGUID();
                    break;
                case NPC_DRESARON:
                    DresaronGUID = creature->GetGUID();
                    break;
                case NPC_NIGHTMARE_BINDINGS:
                    MalfurionCageGUID = creature->GetGUID();
                    break;
                default:
                    break;
            }
        }

        void RecordDwellerDeath(Creature* creature)
        {
            // Database spawn IDs remain stable across corpse/grid reloads.
            // Temporary summons and duplicate death notifications do not count.
            uint64 spawn = creature->GetDBTableGUIDLow();
            if (!spawn || slainDwellers.size() >= 5 || !slainDwellers.insert(spawn).second)
                return;
            if (Creature* oakheart = instance->GetCreature(OakheartGUID))
            {
                // Optional native visual is independent of essential progression.
                creature->CastSpell(oakheart, 204852, true);
                if (slainDwellers.size() == 5 && oakheart->IsAIEnabled)
                    oakheart->AI()->DoAction(ACTION_AWAKEN_OAKHEART);
            }
            SaveToDB();
        }

        void OnUnitDeath(Unit* unit) override
        {
            if (unit->GetEntry() == NPC_NIGHTMARE_DWELLER)
                if (Creature* creature = unit->ToCreature())
                    RecordDwellerDeath(creature);
        }

        void WriteSaveDataMore(std::ostringstream& data) override
        {
            data << " D1 " << slainDwellers.size();
            for (uint64 spawn : slainDwellers)
                data << ' ' << spawn;
        }

        void ReadSaveDataMore(std::istringstream& data) override
        {
            slainDwellers.clear();
            std::string version;
            uint32 count = 0;
            if (!(data >> version >> count) || version != "D1" || count > 5)
                return; // Old boss-only saves remain readable.
            std::set<uint64> loaded;
            for (uint32 i = 0; i < count; ++i)
            {
                uint64 spawn = 0;
                if (!(data >> spawn) || !spawn || !loaded.insert(spawn).second)
                    return;
            }
            slainDwellers = std::move(loaded);
        }

        void OnGameObjectCreate(GameObject* go) override
        {
            switch (go->GetEntry())
            {
                case GO_GLAIDALIS_FIRE_DOOR:
                case GO_DRESARON_FIRE_DOOR:
                case GO_OAKHEART_DOOR:
                    AddDoor(go, true);
                    break;
                case GO_GLAIDALIS_INVIS_DOOR:
                    InvisDoorGUID = go->GetGUID();
                    break;
                default:
                    break;
            }
        }

        void SetData(uint32 type, uint32 data) override
        {
            /*switch (type)
            {
                default:
                    break;
            }*/
        }

        ObjectGuid GetGuidData(uint32 type) const override
        {
            switch (type)
            {
                case NPC_MALFURION_STORMRAGE:   
                    return MalfurionGUID;
                case NPC_DRESARON:
                    return DresaronGUID;
                case NPC_OAKHEART:
                    return OakheartGUID;
                case NPC_NIGHTMARE_BINDINGS:
                    return MalfurionCageGUID;
                case GO_GLAIDALIS_INVIS_DOOR:
                    return InvisDoorGUID;
            }
            return ObjectGuid::Empty;
        }

        uint32 GetData(uint32 type) const override
        {
            if (type == DATA_OAKHEART_AWAKENED)
                return slainDwellers.size() == 5 || GetBossState(DATA_OAKHEART) == DONE;
            return 0;
        }

        WorldLocation* GetClosestGraveYard(float x, float y, float z) override
        {
            loc_res_pla.WorldRelocate(1466, x, y, z);

            uint32 graveyardId = 5334;

            if (GetBossState(DATA_GLAIDALIS) == DONE)
                graveyardId = 5874;

            if (graveyardId)
            {
                if (WorldSafeLocsEntry const* gy = sWorldSafeLocsStore.LookupEntry(graveyardId))
                {
                    loc_res_pla.WorldRelocate(gy->MapID, gy->Loc.X, gy->Loc.Y, gy->Loc.Z);
                }
            }
            return &loc_res_pla;
        }

        /* void Update(uint32 diff) 
        {
            // Challenge
            InstanceScript::Update(diff);
        } */
    };
};

void AddSC_instance_darkheart_thicket()
{
    new instance_darkheart_thicket();
}
