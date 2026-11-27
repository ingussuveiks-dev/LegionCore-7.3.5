/*
    Scenarios Instance Scripts
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "Player.h"
#include "Scenario.h"
#include "ScenarioMgr.h"
#include "MapManager.h"
#include "TemporarySummon.h"
#include "Vehicle.h"
#include "Group.h"
#include "ObjectAccessor.h"
#include "GameObject.h"
#include "SmartScriptMgr.h"
#include "VioletHoldBarrier.h"
#include "bilaal_death.h"
#include "bilaal_portal.h"
#include "faronaar_ritual.h"
#include "../../Monk/Intro/chen_rescue.h"
#include "../../Monk/Intro/jorvinax_defeat.h"
#include "../../Monk/Intro/chen_presentation.h"
#include "../../Monk/Intro/hait_opening.h"
#include "../../Monk/Intro/nine_czi_arrival.h"
#include "../../Monk/Intro/nine_czi_rescue.h"

//rogue dreadblades 1545
struct instance_dreadblades : public InstanceScript
{
    instance_dreadblades(InstanceMap* map) : InstanceScript(map) {}
    
    WorldLocation loc_res_pla;

    WorldLocation* GetClosestGraveYard(float /*x*/, float /*y*/, float /*z*/) override
    {
        Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId());
        if (!scenario || scenario->GetScenarioId() != 1012)
            return nullptr;
        // Native names use displayed stages (one-based). Keep Eliza's beach
        // after final pickup too: the completed sentinel is step5, not step0.
        uint32 step = scenario->GetCurrentStep();
        uint32 graveyardId = step >= 4 ? 5361 : step >= 2 ? 5360 : step == 1 ? 5359 : 5299;
        WorldSafeLocsEntry const* gy = sWorldSafeLocsStore.LookupEntry(graveyardId);
        if (!gy || gy->ID != graveyardId || gy->MapID != 1545 ||
            (gy->Loc.X == 0.0f && gy->Loc.Y == 0.0f && gy->Loc.Z == 0.0f) ||
            !MapManager::IsValidMapCoord(1545, gy->Loc.X, gy->Loc.Y, gy->Loc.Z, gy->Loc.O * float(M_PI) / 180.0f))
        {
            TC_LOG_ERROR("scripts", "Dreadblades graveyard: invalid native WorldSafeLocs %u for instance %u", graveyardId, instance->GetInstanceId());
            return nullptr;
        }
        loc_res_pla.WorldRelocate(1545, gy->Loc.X, gy->Loc.Y, gy->Loc.Z, gy->Loc.O * float(M_PI) / 180.0f);
        return &loc_res_pla;
    }
};

// mage violethold 1494
struct instance_violethold_mage : public InstanceScript
{
    instance_violethold_mage(InstanceMap* map) : InstanceScript(map) {}

    void OnGameObjectCreate(GameObject* object) override
    {
        InstanceScript::OnGameObjectCreate(object);
        if (!VioletHoldBarrier::IsBarrier(object))
            return;

        object->SetFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_NOT_SELECTABLE);
        _barriers.insert(object->GetGUID());
    }

    void OnGameObjectRemove(GameObject* object) override
    {
        InstanceScript::OnGameObjectRemove(object);
        if (VioletHoldBarrier::IsBarrier(object))
            _barriers.erase(object->GetGUID());
    }

    void OnCreatureCreate(Creature* creature) override
    {
        InstanceScript::OnCreatureCreate(creature);
        if (creature->GetEntry() == 102785 && creature->GetDBTableGUIDLow() == 376617)
            _kathranatir = creature->GetGUID();
    }

    void OnCreatureRemove(Creature* creature) override
    {
        InstanceScript::OnCreatureRemove(creature);
        if (creature->GetGUID() == _kathranatir)
            _kathranatir.Clear();
    }

    void OnCreatureDamageTaken(Creature* creature, Unit* attacker, uint32& damage) override
    {
        if (creature->GetGUID() != _kathranatir)
            return;

        Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId());
        if (!scenario || scenario->GetScenarioId() != VioletHoldBarrier::Scenario)
            return;

        // Preserve the two native health triggers at 85% (Tomb of Ice) and
        // 10% (scripted escape). This hook receives the final damage and uses
        // the same attacker-relative health as Unit::DealDamage/ModifyHealth.
        uint32 const step = scenario->GetCurrentStep();
        int32 const minimumPercent = step < 3 ? 85 : 10;
        uint64 const minimum = std::max<uint64>(1, creature->CountPctFromMaxHealth(minimumPercent, attacker));
        uint64 const health = creature->GetHealth(attacker);
        damage = health > minimum ? uint32(std::min<uint64>(damage, health - minimum)) : 0;
    }

    void Update(uint32 diff) override
    {
        if (_kathranatir.IsEmpty() && _barriers.empty())
            return;

        if (_openingCheck > diff)
        {
            _openingCheck -= diff;
            return;
        }
        _openingCheck = 500;

        Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId());
        if (!scenario || scenario->GetScenarioId() != VioletHoldBarrier::Scenario)
            return;

        if (scenario->GetCurrentStep() >= VioletHoldBarrier::FinalStage)
        {
            auto barriers = std::move(_barriers);
            _barriers.clear();
            for (ObjectGuid const& guid : barriers)
                if (GameObject* object = instance->GetGameObject(guid))
                    object->Delete();
            return;
        }

        if (scenario->GetCurrentStep() == 1)
        {
            auto const* progress = scenario->GetAchievementMgr().GetCriteriaProgressMap();
            auto const crossing = progress->find(51519);
            if (crossing == progress->end() || !crossing->second.Counter)
                for (auto const& reference : instance->GetPlayers())
                {
                    Player* player = reference.getSource();
                    // Use the player's current world position. A mage who
                    // Blinked before the stage update can already be across.
                    if (VioletHoldBarrier::IsParticipant(player) && VioletHoldBarrier::IsInCourtyard(player->GetPosition()))
                    {
                        scenario->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 48802, 0, 0, nullptr, player);
                        break;
                    }
                }
            return;
        }

        if (scenario->GetCurrentStep() != 0)
            return;

        // The existing health event applies the Tomb of Ice to the dreadlord.
        // Meryl may be his victim at that moment. Use a quest participant as
        // the reference for the shared scenario criterion once the aura lands.
        Creature* kathranatir = instance->GetCreature(_kathranatir);
        if (!kathranatir || !kathranatir->IsAlive() || !kathranatir->HasAura(211589, _kathranatir))
            return;

        for (auto const& reference : instance->GetPlayers())
        {
            Player* player = reference.getSource();
            if (VioletHoldBarrier::IsParticipant(player) && player->IsWithinDistInMap(kathranatir, 80.0f))
            {
                scenario->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 48801, 0, 0, nullptr, player);
                break;
            }
        }
    }

    WorldLocation loc_res_pla;

    WorldLocation* GetClosestGraveYard(float x, float y, float z) override
    {
        loc_res_pla.WorldRelocate(1494, x, y, z);

        uint32 graveyardId = 5294;

        if (WorldSafeLocsEntry const* gy = sWorldSafeLocsStore.LookupEntry(graveyardId))
        {
            loc_res_pla.WorldRelocate(gy->MapID, gy->Loc.X, gy->Loc.Y, gy->Loc.Z);
        }
        return &loc_res_pla;
    }

private:
    GuidUnorderedSet _barriers;
    ObjectGuid _kathranatir;
    uint32 _openingCheck = 0;
};

struct npc_violet_hold_forge_lift : ScriptedAI
{
    explicit npc_violet_hold_forge_lift(Creature* creature) : ScriptedAI(creature) { }

    void IsSummonedBy(Unit* summoner) override
    {
        Player* player = summoner->ToPlayer();
        if (me->GetUInt32Value(UNIT_FIELD_CREATED_BY_SPELL) != 203356 ||
            !VioletHoldBarrier::IsParticipant(player) || player->GetVehicle() || !me->GetVehicleKit())
            return;

        Scenario* scenario = sScenarioMgr->GetScenario(me->GetInstanceId());
        if (!scenario || scenario->GetScenarioId() != VioletHoldBarrier::Scenario ||
            (scenario->GetCurrentStep() != 2 && scenario->GetCurrentStep() != 3))
            return;

        _rider = player->GetGUID();
        _ground = me->GetPosition();
        me->SetReactState(REACT_PASSIVE);
        me->SetCanFly(true);
        me->SetDisableGravity(true);
        me->DespawnOrUnsummon(12000);
        _events.ScheduleEvent(BOARDING_TIMEOUT, 2500);
        player->EnterVehicle(me, 0, true);
    }

    void PassengerBoarded(Unit* passenger, int8 seat, bool apply) override
    {
        if (seat != 0 || passenger->GetGUID() != _rider)
            return;

        _events.Reset();
        if (!apply)
            return;

        // Let the native seat-entry spline finish before moving the vehicle.
        _events.ScheduleEvent(RISE, 1000);
        _events.ScheduleEvent(DESCEND, 5700);
        _events.ScheduleEvent(EXIT_FALLBACK, 8000);
    }

    void UpdateAI(uint32 diff) override
    {
        _events.Update(diff);
        switch (_events.ExecuteEvent())
        {
            case BOARDING_TIMEOUT:
                TC_LOG_WARN("scripts", "Violet Hold Forge lift: the activating mage did not board seat 0");
                break;
            case RISE:
                if (GetRider())
                    me->GetMotionMaster()->MovePoint(RISE, _ground.GetPositionX(), _ground.GetPositionY(),
                        _ground.GetPositionZ() + 4.0f, false, 3.5f);
                break;
            case DESCEND:
                if (GetRider())
                    me->GetMotionMaster()->MovePoint(LANDING_POINT, _ground, false, 3.5f);
                break;
            case EXIT_FALLBACK:
                if (Unit* rider = GetRider())
                    rider->ExitVehicle(&_ground);
                break;
            default:
                break;
        }
    }

    void MovementInform(uint32 type, uint32 id) override
    {
        if (type == POINT_MOTION_TYPE && id == LANDING_POINT)
            if (Unit* rider = GetRider())
                rider->ExitVehicle(&_ground);
    }

private:
    enum : uint32 { BOARDING_TIMEOUT = 1, RISE, DESCEND, EXIT_FALLBACK, LANDING_POINT };

    Unit* GetRider() const
    {
        Unit* passenger = me->GetVehicleKit()->GetPassenger(0);
        return passenger && passenger->GetGUID() == _rider ? passenger : nullptr;
    }

    EventMap _events;
    ObjectGuid _rider;
    Position _ground;
};

//mage & priest nexus 1583
struct instance_priest_mage_nexus_art : public InstanceScript
{
    instance_priest_mage_nexus_art(InstanceMap* map) : InstanceScript(map) {}

    BilaalDeath::State bilaal;
    BilaalPortal::State portal;
    void CreatureDies(Creature* creature, Unit* killer) override { bilaal.Death(instance, creature, killer); }
    void SetGuidData(uint32 type, ObjectGuid guid) override
    {
        if (type == 104546) portal.Arrived(instance, bilaal, guid);
    }
    void Update(uint32 diff) override
    {
        bilaal.Update(instance, diff);
        portal.Update(instance, bilaal, diff);
    }

    WorldLocation loc_res_pla;

    WorldLocation* GetClosestGraveYard(float x, float y, float z) override
    {
        loc_res_pla.WorldRelocate(1583, x, y, z);

        uint32 graveyardId = 5442;

        if (WorldSafeLocsEntry const* gy = sWorldSafeLocsStore.LookupEntry(graveyardId))
        {
            loc_res_pla.WorldRelocate(gy->MapID, gy->Loc.X, gy->Loc.Y, gy->Loc.Z);
        }
        return &loc_res_pla;
    }
};

//nightborne open quest scenario 1840
struct instance_nightborne_sunwell : public InstanceScript
{
    instance_nightborne_sunwell(InstanceMap* map) : InstanceScript(map) {}

    WorldLocation loc_res_pla;

    WorldLocation* GetClosestGraveYard(float x, float y, float z) override
    {
        loc_res_pla.WorldRelocate(1840, x, y, z);

        uint32 graveyardId = 6303;

        if (WorldSafeLocsEntry const* gy = sWorldSafeLocsStore.LookupEntry(graveyardId))
        {
            loc_res_pla.WorldRelocate(gy->MapID, gy->Loc.X, gy->Loc.Y, gy->Loc.Z);
        }
        return &loc_res_pla;
    }
};

// frostmage
struct instance_faronaar_mage : public InstanceScript
{
    instance_faronaar_mage(InstanceMap* map) : InstanceScript(map) {}

    void OnCreatureCreate(Creature* creature) override
    {
        InstanceScript::OnCreatureCreate(creature);
        _ritual.Observe(creature);
    }

    void OnPlayerLeave(Player* player) override { _ritual.Leave(*player); }
    void Update(uint32 diff) override { _ritual.Update(*instance, diff); }

    uint32 GetData(uint32 type) const override
    {
        return type == FaronaarRitual::FocusReadyData ? _ritual.IsFocusReady() : InstanceScript::GetData(type);
    }

    void SetData(uint32 type, uint32 value) override
    {
        if (type == FaronaarRitual::FocusReadyData && value == 1)
            _ritual.EnableFocus(*instance);
    }

    WorldLocation loc_res_pla;

    WorldLocation* GetClosestGraveYard(float x, float y, float z) override
    {
        loc_res_pla.WorldRelocate(1616, x, y, z);

        uint32 graveyardId = 5511;

        if (WorldSafeLocsEntry const* gy = sWorldSafeLocsStore.LookupEntry(graveyardId))
        {
            loc_res_pla.WorldRelocate(gy->MapID, gy->Loc.X, gy->Loc.Y, gy->Loc.Z);
        }
        return &loc_res_pla;
    }

private:
    FaronaarRitual::State _ritual;
};

// feraldruid
struct instance_ruins_of_falanaar_feral : public InstanceScript
{
    instance_ruins_of_falanaar_feral(InstanceMap* map) : InstanceScript(map) {}

    WorldLocation loc_res_pla;

    WorldLocation* GetClosestGraveYard(float x, float y, float z) override
    {
        loc_res_pla.WorldRelocate(1612, x, y, z);

        uint32 graveyardId = 5514;

        if (WorldSafeLocsEntry const* gy = sWorldSafeLocsStore.LookupEntry(graveyardId))
        {
            loc_res_pla.WorldRelocate(gy->MapID, gy->Loc.X, gy->Loc.Y, gy->Loc.Z);
        }
        return &loc_res_pla;
    }
};

// Monk order hall introduction - map 1014, scenario 943
struct instance_monk_intro : public InstanceScript
{
    instance_monk_intro(InstanceMap* map) : InstanceScript(map), chenRescue(map), jorvinax(&chenRescue), chenPresentation(&chenRescue), nineArrival(&chenRescue), nineRescue(new MonkNineRescue::State(&chenRescue)), haitOpening(new MonkHaitOpening::State(map)) { }

    MonkChenRescue::State chenRescue;
    MonkJorvinax::State jorvinax;
    MonkChenPresentation::State chenPresentation;
    MonkNineArrival::State nineArrival;
    std::unique_ptr<MonkNineRescue::State> nineRescue;
    std::unique_ptr<MonkHaitOpening::State> haitOpening;
    void OnCreatureDamageTaken(Creature* actor, Unit*, uint32& damage) override { haitOpening->Protect(actor, damage); }
    void OnCreatureCreate(Creature* actor) override { InstanceScript::OnCreatureCreate(actor); chenRescue.Observe(actor); nineArrival.Observe(actor); nineRescue->Observe(actor); haitOpening->Observe(actor); }
    void OnGameObjectCreate(GameObject* door) override { InstanceScript::OnGameObjectCreate(door); haitOpening->ObserveDoor(door); }
    void OnPlayerEnter(Player*) override { haitOpening->RestoreCourtyardDoor(); }
    void onScenarionNextStep(uint32) override { haitOpening->RestoreCourtyardDoor(); }
    void CreatureDies(Creature* actor, Unit* killer) override { haitOpening->Death(actor, killer); nineRescue->Death(actor, killer); chenRescue.Death(actor, killer); jorvinax.Death(actor, killer); }
    ObjectGuid GetGuidData(uint32 type) const override
    {
        if (type == MonkChenRescue::ClaimData) return chenRescue.claim;
        if (type == MonkHaitOpening::AcceptedData) return haitOpening->owner;
        if (type == MonkChenArrival::ClaimData) return chenRescue.arrivalClaim;
        if (type == MonkChenArrival::PlayerData) return chenRescue.arrivalPlayer;
        return InstanceScript::GetGuidData(type);
    }
    void SetGuidData(uint32 type, ObjectGuid value) override
    {
        if (type == MonkChenRescue::RequestData) chenRescue.Request(value);
        if (type == MonkChenArrival::RouteData) chenRescue.Routed(value);
        if (type == MonkChenArrival::RequestData) chenRescue.Arrived(value);
        if (type == MonkChenRoutes::EscortData) chenRescue.RouteRequest(value);
        if (type == MonkChenRoutes::HaitData) chenRescue.RouteRequest(value);
        if (type == MonkJorvinax::ArrivalData) jorvinax.Arrival(value);
        if (type == MonkNineArrival::RequestData) nineArrival.Request(value);
        if (type == MonkNineRescue::RequestData) nineRescue->Request(value);
        if (type == MonkHaitOpening::AcceptData) haitOpening->Accept(value);
    }
    void Update(uint32 diff) override { haitOpening->Update(diff); nineRescue->Update(diff); nineArrival.Update(diff); chenPresentation.UpdateRescue(diff); chenPresentation.Update(diff); chenRescue.Update(diff); chenPresentation.UpdateEscortSpeech(diff); jorvinax.Update(diff); chenPresentation.Update(diff, true); }
};

//116849
struct npc_stormstout_brewer_q45404 : public ScriptedAI
{
    npc_stormstout_brewer_q45404(Creature* creature) : ScriptedAI(creature) 
    {
        me->SetReactState(REACT_PASSIVE);
        detoxed = false;
    }

    bool detoxed;

    void SpellHit(Unit* caster, const SpellInfo* spell) override
    {
        if ((spell->Id == 218164 || spell->Id == 115450) && !detoxed)
        {
            if (auto plr = caster->ToPlayer())
            {
                detoxed = true;
                Talk(0);
                plr->UpdateAchievementCriteria(CRITERIA_TYPE_KILL_CREATURE, 117025, 1, 0, plr, true);
                me->GetMotionMaster()->MoveRandom(12.f);
                me->DespawnOrUnsummon(7000);
            }
        }
    }
};

void AddSC_instance_class_art_scenarios()
{
    RegisterInstanceScript(instance_dreadblades, 1545);
    RegisterInstanceScript(instance_violethold_mage, 1494);
    RegisterCreatureAI(npc_violet_hold_forge_lift);
    RegisterInstanceScript(instance_priest_mage_nexus_art, 1583);
    RegisterInstanceScript(instance_nightborne_sunwell, 1840);
    RegisterInstanceScript(instance_faronaar_mage, 1616);
    RegisterInstanceScript(instance_ruins_of_falanaar_feral, 1612);
    RegisterInstanceScript(instance_monk_intro, 1014);
    RegisterCreatureAI(npc_stormstout_brewer_q45404);
}
