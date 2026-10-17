#include "ScriptMgr.h"
#include "SmartAI.h"
#include "Player.h"
#include "GossipDef.h"
#include "TemporarySummon.h"
#include "Vehicle.h"
#include "MoveSpline.h"
#include "MotionMaster.h"
#include "stormheim_batrider_path.h"
#include <map>

namespace StormheimBatRider
{
// A forcibly killed vehicle no longer receives AI updates. Finish its recovery
// on the player's event queue, using only a GUID and a bounded exit-spline wait.
void RecoverAfterDeath(ObjectGuid guid,uint32 attempts=50)
{
    Player* p=ObjectAccessor::FindPlayer(guid);
    if(!p || !p->IsInWorld() || !p->IsAlive() || p->GetMapId()!=1220 ||
        p->GetInstanceId() || p->IsBeingTeleported() || p->GetVehicle() || p->GetTransport())return;
    if(p->movespline->Finalized()){p->NearTeleportTo(Source);return;}
    if(attempts)p->AddDelayedEvent(100,[guid,attempts]{RecoverAfterDeath(guid,attempts-1);});
}
}

struct npc_stormheim_foothold_batrider : SmartAI
{
    explicit npc_stormheim_foothold_batrider(Creature* c):SmartAI(c){}
    enum class Stage { Idle, Boarding, Flying, Exiting, Finished };
    Stage stage=Stage::Idle;
    ObjectGuid rider;
    std::map<ObjectGuid,ObjectGuid> flights;
    uint32 elapsed=0;
    uint32 point=0;
    bool tookOff=false, recover=false;

    Player* Rider() { return ObjectAccessor::FindPlayer(rider); }
    void RestoreControl(Player* p)
    {
        if(!p)return;
        if(p->GetUnitBeingMoved()==me)p->SetClientControl(p,true);
        else if(p->GetViewpoint()==me)p->SetViewpoint(me,false);
    }
    void BeginExit(Player* p,bool arrived)
    {
        if(stage==Stage::Finished || stage==Stage::Exiting)return;
        stage=Stage::Exiting;
        recover=tookOff && !arrived;
        elapsed=0;
        me->StopMoving();
        if(p && p->GetVehicleBase()==me)
            // The aura-removal path ignores ExitVehicle's optional position.
            // Arrive at the landing first and let native exit use that origin.
            p->ExitVehicle();
        RestoreControl(p);
    }
    void sGossipSelect(Player* p,uint32 menu,uint32 option) override
    {
        if(menu!=18762 || option!=0){SmartAI::sGossipSelect(p,menu,option);return;}
        if(stage!=Stage::Idle || me->GetEntry()!=97250 || me->GetDBTableGUIDLow()!=340420 ||
            !p || !p->IsInWorld() || !p->IsAlive() || p->GetTeam()!=HORDE ||
            p->GetMapId()!=1220 || p->GetInstanceId() || p->GetZoneId()!=7541 ||
            p->GetMap()!=me->GetMap() || !me->IsAlive() ||
            !me->HasFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_GOSSIP) ||
            !p->InSamePhase(me) || !p->IsWithinDistInMap(me,INTERACTION_DISTANCE) ||
            p->IsBeingTeleported() || p->GetVehicle() || p->GetTransport() ||
            p->isInFlight() || p->isInCombat() || p->GetCharmerGUID())return;
        auto& gossip=p->PlayerTalkClass->GetGossipMenu();
        if(gossip.GetMenuId()!=18762 || !gossip.GetItem(0))return;
        auto it=flights.find(p->GetGUID());
        if(it!=flights.end())
        {
            if(ObjectAccessor::GetCreature(*me,it->second))return;
            flights.erase(it);
        }
        // Keep the public quest ender in place. A viewer-scoped copy uses the
        // same native model/vehicle/seat and never awards quest credit.
        Creature* transport=me->SummonCreature(97250,me->GetPosition(),TEMPSUMMON_TIMED_DESPAWN,90000,0,p->GetGUID());
        if(!transport)return;
        flights[p->GetGUID()]=transport->GetGUID();
        transport->AI()->SetGUID(p->GetGUID());
        p->PlayerTalkClass->SendCloseGossip();
    }
    void SetGUID(ObjectGuid const& guid,int32 /*id*/=0) override
    {
        if(stage!=Stage::Idle || me->GetDBTableGUIDLow() || me->GetEntry()!=97250)return;
        Player* p=ObjectAccessor::GetPlayer(*me,guid);
        if(!p || p->GetVehicle() || p->GetTransport() || !me->GetVehicleKit())
        {me->DespawnOrUnsummon();return;}
        rider=guid;
        stage=Stage::Boarding;
        me->RemoveFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_GOSSIP|UNIT_NPC_FLAG_QUESTGIVER|UNIT_NPC_FLAG_SPELLCLICK);
        me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_IMMUNE_TO_PC|UNIT_FLAG_IMMUNE_TO_NPC);
        me->SetReactState(REACT_PASSIVE);
        me->setActive(true);
        p->StopMoving();
        p->EnterVehicle(me,0,true);
    }
    void PassengerBoarded(Unit* passenger,int8 seat,bool apply) override
    {
        SmartAI::PassengerBoarded(passenger,seat,apply);
        if(!apply)
        {
            RestoreControl(passenger->ToPlayer());
            if(passenger->GetGUID()==rider && stage==Stage::Flying)
            {
                // Native RemovePassenger invokes this before clearing the
                // player's vehicle pointer. Do not recursively ExitVehicle.
                stage=Stage::Exiting;recover=true;elapsed=0;me->StopMoving();
            }
        }
    }
    void JustDied(Unit* killer) override
    {
        if(stage!=Stage::Idle)
        {
            BeginExit(Rider(),false);
            if(tookOff)StormheimBatRider::RecoverAfterDeath(rider);
        }
        SmartAI::JustDied(killer);
    }
    void SummonedCreatureDespawn(Creature* summon) override
    {
        for(auto it=flights.begin();it!=flights.end();)
            if(it->second==summon->GetGUID())it=flights.erase(it);else ++it;
        SmartAI::SummonedCreatureDespawn(summon);
    }
    void MoveNext()
    {
        me->GetMotionMaster()->MovePoint(100+point,StormheimBatRider::Flight[point],false,24.0f);
    }
    void MovementInform(uint32 type,uint32 id) override
    {
        if(stage!=Stage::Flying || type!=POINT_MOTION_TYPE || id!=100+point ||
            !me->movespline->Finalized() || me->GetExactDist(&StormheimBatRider::Flight[point])>2.0f)return;
        if(++point<StormheimBatRider::Flight.size())MoveNext();
        else BeginExit(Rider(),true);
    }
    void UpdateAI(uint32 diff) override
    {
        if(stage==Stage::Idle){SmartAI::UpdateAI(diff);return;}
        if(stage==Stage::Finished)return;
        elapsed+=diff;
        Player* p=Rider();
        if(!p || !p->IsInWorld() || !p->IsAlive() || p->GetMap()!=me->GetMap() || p->IsBeingTeleported())
        {
            BeginExit(p,false);
            RestoreControl(p);
            stage=Stage::Finished;
            me->DespawnOrUnsummon(1000);
            return;
        }
        if(stage==Stage::Boarding)
        {
            if(p->GetVehicleBase()!=me || !me->GetVehicleKit() || me->GetVehicleKit()->GetPassenger(0)!=p || !p->movespline->Finalized())
            {if(elapsed>=10000)BeginExit(p,false);return;}
            me->SetCanFly(true);me->SetDisableGravity(true);
            me->GetVehicleKit()->RelocatePassengers();
            p->SendMovementFlagUpdate(true);
            p->SetClientControl(me,false);
            tookOff=true;stage=Stage::Flying;elapsed=0;
            MoveNext();
        }
        else if(stage==Stage::Flying)
        {
            if(p->GetVehicleBase()!=me){BeginExit(p,false);return;}
            if(elapsed>=60000){BeginExit(p,false);return;}
            // A missing movement callback may be retried only at its actual
            // finished endpoint; a timer alone never counts as arrival.
            if(me->movespline->Finalized())MovementInform(POINT_MOTION_TYPE,100+point);
        }
        else if(stage==Stage::Exiting)
        {
            RestoreControl(p);
            // Let the native transport-exit spline finish (pet-frame cleanup
            // uses UNIT_EXITING_VEHICLE) before any recovery teleport/despawn.
            if(p->GetVehicleBase()==me){if(elapsed>=5000)p->ExitVehicle();return;}
            if(!p->GetVehicle() && !p->GetTransport() && !p->movespline->Finalized())
            {if(elapsed<5000)return;p->StopMoving();}
            if(recover && !p->GetVehicle() && !p->GetTransport())p->NearTeleportTo(StormheimBatRider::Source);
            stage=Stage::Finished;
            me->DespawnOrUnsummon(1000);
        }
    }
};

void AddSC_stormheim_batrider()
{
    TC_LOG_INFO("server.loading","Stormheim: Foothold Bat-Rider return flight, vehicle4387 seat15956");
    RegisterCreatureAI(npc_stormheim_foothold_batrider);
}
