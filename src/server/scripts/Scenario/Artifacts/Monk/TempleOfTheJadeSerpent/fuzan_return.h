#ifndef FUZAN_RETURN_H
#define FUZAN_RETURN_H

#include "fuzan_return_route.h"
#include "Vehicle.h"
#include "MotionMaster.h"
#include "MoveSpline.h"
#include "Spline.h"
#include "ModelIgnoreFlags.h"
#include "GossipDef.h"
#include "ScriptedGossip.h"
#include "Item.h"
#include "WorldSession.h"
#include <map>
#include <mutex>

// A labelled functional departure reconstruction. No recovered retail spline
// or continuous retail arrival footage is available. Keep native boarding and
// credit, then use the original SmartAI's ground pad for safe map admission.
namespace FuZanReturn
{
constexpr uint32 QuestId=42765, VehicleEntry=109441, FlightPoint=0x465a5201;
inline Position Landing() { return Position(901.691f,3605.27f,193.116f,6.26855f); }
enum class Leg : uint8 { Summon, Board, Credit, Flight, Exit, Transfer, Abort };
struct Trip
{
    ObjectGuid owner, source, vehicle, item;
    std::shared_ptr<QuestCancellationReceipt const> quest;
    uint64 epoch=0;
    uint32 instance=0, elapsed=0, retry=0;
    uint8 tries=0;
    Leg leg=Leg::Summon;
    Position ground;
    bool summoning=false, credit=false, event=false;
};
static std::mutex tripLock;
static std::map<ObjectGuid,Trip> trips;
static uint64 nextEpoch=0;
inline bool Read(ObjectGuid owner,Trip& trip)
{
    std::lock_guard<std::mutex> guard(tripLock);
    auto it=trips.find(owner);if(it==trips.end())return false;trip=it->second;return true;
}
template<class Fn> inline bool Edit(Trip const& trip,Fn fn)
{
    std::lock_guard<std::mutex> guard(tripLock);
    auto it=trips.find(trip.owner);if(it==trips.end()||it->second.epoch!=trip.epoch)return false;
    fn(it->second);return true;
}
inline void Erase(Trip const& trip)
{
    std::lock_guard<std::mutex> guard(tripLock);
    auto it=trips.find(trip.owner);if(it!=trips.end()&&it->second.epoch==trip.epoch)trips.erase(it);
}
inline Creature* Actor(Player* player,ObjectGuid guid)
{
    return player&&player->IsInWorld()?ObjectAccessor::GetCreature(*player,guid):nullptr;
}
inline bool Local(Player* player,Trip const& trip)
{
    return player&&player->IsInWorld()&&player->GetMapId()==1625&&player->GetInstanceId()==trip.instance;
}
inline bool ItemOwned(Player* player,Trip const& trip)
{
    Item* item=player?player->GetItemByGuid(trip.item):nullptr;
    return item&&item->GetEntry()==128938&&item->GetOwnerGUID()==trip.owner&&item->GetCount();
}
inline bool QuestOwned(Player* player,Trip const& trip)
{
    auto quest=player?player->GetQuestCancellationReceipt(QuestId):nullptr;
    return quest&&trip.quest&&quest==trip.quest&&!trip.quest->load(std::memory_order_acquire)&&
        player->GetQuestStatus(QuestId)==QUEST_STATUS_INCOMPLETE;
}
inline void Restore(Player* player,Trip const& trip)
{
    if(auto view=player->GetViewpoint())if(view->GetGUID()==trip.vehicle)player->SetViewpoint(view,false);
    if(auto mover=player->GetUnitBeingMoved())if(mover->GetGUID()==trip.vehicle)player->SetClientControl(player,true);
}
inline void Clear(Player* player,Trip const& trip)
{
    Erase(trip);Restore(player,trip);
    if(auto vehicle=player->GetVehicleBase())if(vehicle->GetGUID()==trip.vehicle)player->ExitVehicle();
    if(auto actor=Actor(player,trip.vehicle))actor->DespawnOrUnsummon(2000);
    player->RemoveAurasDueToSpell(217818);
}
inline bool FlightClear(Player* player,std::vector<G3D::Vector3> const& path)
{
    if(!player||path.size()<2||path.size()>16)return false;
    for(auto const& p:path)if(!FuZanReturnRoute::Valid(p))return false;
    Movement::SplineBase spline;spline.init_spline(path.data(),int(path.size()),Movement::SplineBase::ModeCatmullrom);
    unsigned budget=0;
    for(int segment=spline.first();segment<spline.last();++segment)
    {
        float length=spline.SegLength(segment);if(!std::isfinite(length)||length<0.f||length>2048.f)return false;
        int samples=std::max(16,int(std::ceil(length*2.f)));if((budget+=samples)>8192)return false;
        G3D::Vector3 previous;
        for(int sample=0;sample<=samples;++sample)
        {
            G3D::Vector3 p;spline.evaluate_percent(segment,float(sample)/samples,p);
            for(float dx:{-2.f,0.f,2.f})for(float dy:{-2.f,0.f,2.f})
            {
                float height=player->GetMap()->GetHeight(player->GetPhases(),p.x+dx,p.y+dy,p.z+4.f,true,10000.f);
                if(!std::isfinite(height)||height< -10000.f||height>p.z+.05f){TC_LOG_ERROR("scripts","Fu Zan: blocked departure sample (%f,%f,%f), ground=%f",p.x+dx,p.y+dy,p.z,height);return false;}
                if(sample)for(float dz:{.1f,2.f,4.f})
                    if(!player->GetMap()->isInLineOfSight(previous.x+dx,previous.y+dy,previous.z+dz,
                        p.x+dx,p.y+dy,p.z+dz,player->GetPhases(),VMAP::ModelIgnoreFlags::Nothing)){TC_LOG_ERROR("scripts","Fu Zan: blocked departure corridor (%f,%f,%f)",p.x+dx,p.y+dy,p.z+dz);return false;}
            }
            previous=p;
        }
    }
    return true;
}
inline void Request(Player* player,Creature* source,uint32 menu,uint32 option)
{
    if(!player||!player->PlayerTalkClass)return;
    auto const& gossip=player->PlayerTalkClass->GetGossipMenu();
    auto const& items=gossip.GetMenuItems();auto found=items.find(option);
    bool offered=menu==19932&&!option&&gossip.GetMenuId()==menu&&found!=items.end()&&
        found->second.OptionNpc==GossipOptionNpc::None&&!found->second.Sender&&!found->second.Action&&
        !found->second.IsCoded&&!found->second.BoxMoney;
    player->PlayerTalkClass->ClearMenus();player->CLOSE_GOSSIP_MENU();
    if(!offered||!source||source->GetEntry()!=109440||!source->ToTempSummon()||!source->IsAlive()||
        player->GetMap()!=source->GetMap()||player->GetMapId()!=1625||!player->IsAlive()||!player->CanContact()||
        player->getClass()!=CLASS_MONK||!player->InSamePhase(source)||!player->CanTargetQuestObject(source)||
        !player->canSeeOrDetect(source)||!player->IsWithinDistInMap(source,INTERACTION_DISTANCE)||
        !player->IsWithinLOSInMap(source)||player->GetVehicle()||player->waitOnSeat||player->GetTransport()||
        player->isInFlight()||player->isInCombat()||player->HasPendingTeleport())return;
    auto scenario=sScenarioMgr->GetScenario(player->GetInstanceId());
    Item* item=player->GetItemByEntry(128938);
    if(!scenario||scenario->GetScenarioId()!=1137||(scenario->GetCurrentStep()!=8&&!scenario->IsCompleted(false))||!item||
        item->GetOwnerGUID()!=player->GetGUID()||!item->GetCount())return;
    Trip trip;trip.owner=player->GetGUID();trip.source=source->GetGUID();trip.item=item->GetGUID();
    trip.instance=player->GetInstanceId();trip.ground=source->GetPosition();trip.event=scenario->IsCompleted(false);
    auto questTemplate=sQuestDataStore->GetQuestTemplate(QuestId);
    trip.credit=questTemplate&&player->GetQuestObjectiveData(questTemplate,2)>0;
    trip.quest=player->GetQuestCancellationReceipt(QuestId);
    std::lock_guard<std::mutex> guard(tripLock);
    if(trips.count(trip.owner))return;
    trip.epoch=++nextEpoch;trips.emplace(trip.owner,trip);
}
inline void RegisterVehicle(Creature* vehicle,Player* player)
{
    Trip trip;if(!player||!Read(player->GetGUID(),trip)||!Local(player,trip)||!ItemOwned(player,trip)||
        trip.leg!=Leg::Summon||!trip.summoning||!trip.vehicle.IsEmpty())return;
    Edit(trip,[&](Trip& t){t.vehicle=vehicle->GetGUID();t.leg=Leg::Board;t.retry=0;t.tries=0;});
    vehicle->setActive(true);
}
inline void Boarded(Creature* vehicle,Player* player,int8 seat,bool apply)
{
    Trip trip;if(!player||seat!=0||!Read(player->GetGUID(),trip)||trip.vehicle!=vehicle->GetGUID())return;
    if(apply&&Local(player,trip)&&trip.leg==Leg::Board)
        Edit(trip,[](Trip& t){t.leg=Leg::Credit;t.retry=0;});
    else if(!apply&&trip.leg!=Leg::Exit&&trip.leg!=Leg::Transfer)
    {Restore(player,trip);Edit(trip,[](Trip& t){t.leg=Leg::Abort;});}
}
inline void FlightEnded(Creature* vehicle)
{
    std::lock_guard<std::mutex> guard(tripLock);
    for(auto& pair:trips)if(pair.second.vehicle==vehicle->GetGUID()&&pair.second.leg==Leg::Flight)pair.second.leg=Leg::Exit;
}
inline void Update(Player* player,uint32 diff)
{
    Trip trip;if(!player||!Read(player->GetGUID(),trip))return;
    if(!Local(player,trip)) {Clear(player,trip);return;}
    if(trip.leg==Leg::Transfer)return;
    Edit(trip,[=](Trip& t){t.elapsed+=diff;t.retry=t.retry>diff?t.retry-diff:0;});
    if(!player->IsAlive()||player->HasPendingTeleport()||player->isInFlight()||player->GetTransport()||
        (player->GetVehicleBase()&&player->GetVehicleBase()->GetGUID()!=trip.vehicle)) {Clear(player,trip);return;}
    if(!ItemOwned(player,trip)||trip.elapsed>55000)
    {
        if(trip.leg!=Leg::Abort)TC_LOG_ERROR("scripts","Fu Zan: return timed out or owned artifact disappeared; safe source recovery");
        Edit(trip,[](Trip& t){t.leg=Leg::Abort;});trip.leg=Leg::Abort;
    }
    Creature* vehicle=Actor(player,trip.vehicle);
    if(trip.leg==Leg::Abort)
    {
        Restore(player,trip);
        if(player->GetVehicleBase()){player->ExitVehicle();return;}
        if(!player->movespline->Finalized())return;
        if(player->GetUnitBeingMoved()==player)
            player->NearTeleportTo(trip.ground.GetPositionX(),trip.ground.GetPositionY(),trip.ground.GetPositionZ(),trip.ground.GetOrientation());
        Clear(player,trip);return;
    }
    if(trip.leg==Leg::Summon||trip.leg==Leg::Board)
    {
        if(trip.retry)return;
        if(trip.tries>=5)
        {TC_LOG_ERROR("scripts","Fu Zan: return boarding failed; source retained for retry");Edit(trip,[](Trip& t){t.leg=Leg::Abort;});return;}
        Edit(trip,[](Trip& t){++t.tries;t.retry=1000;t.summoning=false;});
        if(trip.leg==Leg::Summon)
        {
            if(auto source=Actor(player,trip.source))player->CastSpell(source,217818,true);
        }
        else if(vehicle&&!player->GetVehicle()&&!player->waitOnSeat&&player->movespline->Finalized()&&
            vehicle->GetVehicleKit()&&vehicle->GetVehicleKit()->HasEmptySeat(0))player->CastSpell(vehicle,46598,true);
        return;
    }
    if(!vehicle&&trip.leg!=Leg::Exit)
    {TC_LOG_ERROR("scripts","Fu Zan: return vehicle disappeared; safe source recovery");Edit(trip,[](Trip& t){t.leg=Leg::Abort;});return;}
    if(trip.leg==Leg::Credit)
    {
        if(!vehicle||player->GetVehicleBase()!=vehicle||!vehicle->GetVehicleKit()||
            vehicle->GetVehicleKit()->GetPassenger(0)!=player||!player->movespline->Finalized()||player->IsMapObjectArrivalProtected())return;
        auto path=FuZanReturnRoute::Departure({vehicle->GetPositionX(),vehicle->GetPositionY(),vehicle->GetPositionZ()});
        if(!FlightClear(player,path))
        {TC_LOG_ERROR("scripts","Fu Zan: departure collision check failed; flight withheld");Edit(trip,[](Trip& t){t.leg=Leg::Abort;});return;}
        // Only the actual accepted seat callback unlocks native credit. A
        // repeated cast cannot summon a second vehicle or replay this receipt.
        player->CastSpell(player,217818,true);
        Trip current;if(!Read(player->GetGUID(),current)||!current.event)return;
        Edit(trip,[](Trip& t){t.leg=Leg::Flight;});
        player->SetClientControl(vehicle,false);vehicle->SetDisableGravity(true);vehicle->SetCanFly(true);vehicle->SetSpeed(MOVE_FLIGHT,2.f);
        vehicle->GetMotionMaster()->MoveSmoothFlyPath(FlightPoint,path.data(),uint32(path.size()));
        return;
    }
    if(trip.leg!=Leg::Exit)return;
    Restore(player,trip);
    if(player->GetVehicleBase()){player->ExitVehicle();return;}
    if(!player->movespline->Finalized()||player->GetUnitBeingMoved()!=player||trip.retry)return;
    Edit(trip,[](Trip& t){t.leg=Leg::Transfer;});
    if(vehicle)vehicle->DespawnOrUnsummon(2000);
    player->RemoveAurasDueToSpell(217818);
    player->CastSpell(player,217820,true);
    if(!player->HasPendingTeleport()&&Local(player,trip))
    {TC_LOG_ERROR("scripts","Fu Zan: native return transfer failed; retry pending");Edit(trip,[](Trip& t){t.leg=Leg::Exit;t.retry=1000;});}
}
class ReturnActor : public CreatureScript
{
public:
    ReturnActor():CreatureScript("npc_fuzan_return_guide"){}
    struct AI : SmartAI
    {
        explicit AI(Creature* c):SmartAI(c){}
        void sGossipSelect(Player* player,uint32 menu,uint32 option) override { Request(player,me,menu,option); }
    };
    CreatureAI* GetAI(Creature* c)const override{return new AI(c);}
    bool OnGossipSelect(Player*,Creature*,uint32,uint32)override{return true;}
};
class ReturnVehicle : public CreatureScript
{
public:
    ReturnVehicle():CreatureScript("npc_fuzan_return_vehicle"){}
    struct AI : ScriptedAI
    {
        explicit AI(Creature* c):ScriptedAI(c){}
        void IsSummonedBy(Unit* owner)override{RegisterVehicle(me,owner?owner->ToPlayer():nullptr);}
        void PassengerBoarded(Unit* who,int8 seat,bool apply)override{Boarded(me,who?who->ToPlayer():nullptr,seat,apply);}
        void MovementInform(uint32 type,uint32 point)override{if(type==EFFECT_MOTION_TYPE&&point==FlightPoint)FlightEnded(me);}
    };
    CreatureAI* GetAI(Creature* c)const override{return new AI(c);}
};
class spell_fuzan_return_boarding : public SpellScript
{
    PrepareSpellScript(spell_fuzan_return_boarding);
    Trip trip;
    SpellCastResult Check()
    {
        auto player=GetCaster()->ToPlayer();
        return player&&Read(player->GetGUID(),trip)&&Local(player,trip)&&ItemOwned(player,trip)&&
            (trip.leg==Leg::Summon||trip.leg==Leg::Credit)?SPELL_CAST_OK:SPELL_FAILED_BAD_TARGETS;
    }
    void Hit(SpellEffIndex effect)
    {
        auto player=GetCaster()->ToPlayer();Trip current;bool allowed=false;
        if(player&&Read(player->GetGUID(),current)&&current.epoch==trip.epoch&&Local(player,current)&&ItemOwned(player,current))
        {
            if(effect==EFFECT_0)
                Edit(current,[&](Trip& t){if(t.leg==Leg::Summon&&t.vehicle.IsEmpty()&&!t.summoning){t.summoning=true;allowed=true;}});
            else if(GetHitPlayer()==player&&current.leg==Leg::Credit&&player->GetVehicleBase()&&
                player->GetVehicleBase()->GetGUID()==current.vehicle&&player->GetVehicle()->GetPassenger(0)==player)
            {
                if(effect==EFFECT_1)allowed=!current.event;
                else if(effect==EFFECT_2&&!current.credit&&QuestOwned(player,current))
                    Edit(current,[&](Trip& t){t.credit=true;allowed=true;});
            }
        }
        if(!allowed)PreventHitDefaultEffect(effect);
    }
    void Finished()
    {
        auto player=GetCaster()->ToPlayer();Trip current;
        if(!player||!Read(player->GetGUID(),current)||current.epoch!=trip.epoch||current.leg!=Leg::Credit||current.event||
            !player->GetVehicleBase()||player->GetVehicleBase()->GetGUID()!=current.vehicle)return;
        Edit(current,[](Trip& t){t.event=true;});
        player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,52751);
    }
    void Register()override
    {
        OnCheckCast+=SpellCheckCastFn(spell_fuzan_return_boarding::Check);
        OnEffectHit+=SpellEffectFn(spell_fuzan_return_boarding::Hit,EFFECT_0,SPELL_EFFECT_SUMMON);
        OnEffectHitTarget+=SpellEffectFn(spell_fuzan_return_boarding::Hit,EFFECT_1,SPELL_EFFECT_APPLY_AURA);
        OnEffectHitTarget+=SpellEffectFn(spell_fuzan_return_boarding::Hit,EFFECT_2,SPELL_EFFECT_KILL_CREDIT);
        AfterCast+=SpellCastFn(spell_fuzan_return_boarding::Finished);
    }
};
class spell_fuzan_return_transfer : public SpellScript
{
    PrepareSpellScript(spell_fuzan_return_transfer);
    void Hit(SpellEffIndex effect)
    {
        auto player=GetCaster()->ToPlayer();Trip trip;
        if(!player||GetHitPlayer()!=player||!Read(player->GetGUID(),trip)||!Local(player,trip)||
            trip.leg!=Leg::Transfer||!trip.event||player->GetVehicle()||!player->movespline->Finalized())
        {PreventHitDefaultEffect(effect);return;}
        // The native air destination requires an unrecovered arrival flight.
        // Reconstruct the departure only; retain the original SmartAI ground
        // pad instead of admitting the player unmounted above the hall roof.
        auto landing=Landing();if(auto dest=GetHitDest())dest->WorldRelocate(1514,landing.GetPositionX(),landing.GetPositionY(),landing.GetPositionZ(),landing.GetOrientation());
    }
    void Register()override{OnEffectHitTarget+=SpellEffectFn(spell_fuzan_return_transfer::Hit,EFFECT_0,SPELL_EFFECT_TELEPORT_L);}
};
class PlayerEvents : public PlayerScript
{
public:
    PlayerEvents():PlayerScript("player_fuzan_return"){}
    void OnUpdate(Player* player,uint32 diff)override{Update(player,diff);}
    void OnMapChanged(Player* player)override{Trip trip;if(Read(player->GetGUID(),trip)&&!Local(player,trip))Clear(player,trip);}
    void OnBeforeLogout(Player* player)override
    {
        Trip trip;if(!player||!player->GetSession()||!player->GetSession()->PlayerLogout()||!player->IsAlive()||
            !Read(player->GetGUID(),trip)||player->GetTransport()||player->isInFlight()||
            (player->GetVehicleBase()&&player->GetVehicleBase()->GetGUID()!=trip.vehicle))return;
        if(player->HasPendingTeleport())return; // Never replace an unrelated issued transfer.
        if(!Local(player,trip))return;
        bool departure=trip.leg==Leg::Flight||trip.leg==Leg::Exit||trip.leg==Leg::Transfer;
        Clear(player,trip);
        if(departure)
        {
            auto landing=Landing();player->GetTeleportDest().WorldRelocate(1514,landing.GetPositionX(),landing.GetPositionY(),landing.GetPositionZ(),landing.GetOrientation());
            player->SetSemaphoreTeleportFar(true);
        }
    }
    void OnLogout(Player* player)override{Trip trip;if(Read(player->GetGUID(),trip))Clear(player,trip);}
};
inline void Register()
{
    new ReturnActor();new ReturnVehicle();new PlayerEvents();
    RegisterSpellScript(spell_fuzan_return_boarding);RegisterSpellScript(spell_fuzan_return_transfer);
}
}
#endif
