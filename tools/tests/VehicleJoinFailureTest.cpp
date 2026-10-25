#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <vector>
#include "EventProcessor.cpp"
using int8=int8_t;
static_assert(_ITERATOR_DEBUG_LEVEL==2,"Vehicle join regression requires checked iterators");
#define TC_LOG_DEBUG(...) DebugLog(__VA_ARGS__)
#define TC_LOG_ERROR(...) assert(false)
std::vector<std::vector<uint32>> debugMessages;
template<typename... Arguments> void DebugLog(char const*,char const*,Arguments... arguments)
{
    debugMessages.push_back({uint32(arguments)...});
}

enum { SPELL_AURA_CONTROL_VEHICLE=236, SPELL_AURA_MOUNTED=78, UNIT_MASK_ACCESSORY=0x200, STATUS_UNINSTALLING=2,
    UNIT_FIELD_FLAGS=0x61, UNIT_FIELD_NPC_FLAGS=0x7d, UNIT_FLAG_NOT_SELECTABLE=0x2000000, MOVEMENTFLAG_DISABLE_GRAVITY=0x200,
    UNIT_NPC_FLAG_PLAYER_VEHICLE=0x2000000, UNIT_NPC_FLAG_SPELLCLICK=0x1000000, VEHICLE_SEAT_FLAG_CAN_CONTROL=0x800,
    VEHICLE_SEAT_FLAG_DISABLE_GRAVITY=4, VEHICLE_SEAT_FLAG_HIDE_PASSENGER=0x200, VEHICLE_SEAT_FLAG_PASSENGER_NOT_SELECTABLE=0x100000,
    VEHICLE_SEAT_FLAG_B_KEEP_PET=0x20000, CREATURE_FLAG_EXTRA_VEHICLE_ATTACKABLE_PASSENGERS=0x1000000, UNIT_STATE_ONVEHICLE=0x80000,
    UNIT_STATE_ROOT=0x400, CHARM_TYPE_VEHICLE=2 };
struct ObjectGuid
{
    uint32 value=0;
    uint32 GetGUIDLow() const {return value;}
    explicit operator bool() const {return value!=0;}
    void Clear(){value=0;}
    static ObjectGuid const Empty;
};
ObjectGuid const ObjectGuid::Empty{};
struct Unit;
struct Player;
struct Creature;
struct Vehicle;
struct VehicleJoinEvent;
struct Position
{
    float m_positionX=0,m_positionY=0,m_positionZ=0;
    Position()=default;
    Position(float offsetX,float offsetY,float offsetZ):m_positionX(offsetX),m_positionY(offsetY),m_positionZ(offsetZ){}
    void Relocate(float offsetX,float offsetY,float offsetZ){*this=Position(offsetX,offsetY,offsetZ);}
};
struct Object : std::enable_shared_from_this<Object>
{
    virtual ~Object()=default;
    virtual Unit* ToUnit(){return nullptr;}
};
struct Unit : Object
{
    uint32 identity=17,despawns=0,exits=0,flags=0,states=0,movementFlags=0,interrupted=0,mountedRemoved=0;
    uint32 breakTargets=0,splines=0,visibilityUpdates=0,charmCalls=0,rootCalls=0;
    bool inWorld=true,accessory=false,charmResult=true;
    Vehicle* kit=nullptr;
    Vehicle* m_vehicle=nullptr;
    std::vector<uint32> removedCasters;
    uint32 removedAll=0;
    std::set<uint32> controlCasters;
    std::function<void(Unit*,ObjectGuid)> onRemove;
    struct {struct {Position Pos;uint32 MoveTime=99;int8 VehicleSeatIndex=-1;ObjectGuid Guid;uint32 VehicleRecID=0;} transport;} m_movementInfo;
    Unit* ToUnit() override {return this;}
    virtual Player* ToPlayer(){return nullptr;}
    virtual Creature* ToCreature(){return nullptr;}
    bool IsCreature(){return ToCreature()!=nullptr;}
    bool IsPlayer(){return ToPlayer()!=nullptr;}
    std::shared_ptr<Object> get_ptr(){return shared_from_this();}
    Vehicle* GetVehicleKit(){return kit;}
    ObjectGuid GetGUID() const {return {identity};}
    uint32 GetGUIDLow() const {return identity;}
    uint32 GetEntry() const {return 105886;}
    bool IsInWorld() const {return inWorld;}
    bool HasUnitTypeMask(uint32 mask) const {assert(mask==UNIT_MASK_ACCESSORY);return accessory;}
    bool HasFlag(uint32 field,uint32 flag) const {assert(field==UNIT_FIELD_FLAGS);return (flags&flag)!=0;}
    bool HasUnitMovementFlag(uint32 flag) const {return (movementFlags&flag)!=0;}
    bool HasAuraTypeWithCaster(uint32 aura,ObjectGuid caster){assert(aura==236);return controlCasters.count(caster.value)!=0;}
    void RemoveAurasByType(uint32 aura,ObjectGuid caster)
    {
        assert(aura==236);removedCasters.push_back(caster.value);
        if(controlCasters.erase(caster.value) && onRemove)onRemove(this,caster);
    }
    void RemoveAurasByType(uint32 aura){if(aura==236)++removedAll;else {assert(aura==78);++mountedRemoved;}}
    void DespawnOrUnsummon(){++despawns;}
    void _ExitVehicle(){++exits;}
    void RemoveFlag(uint32 field,uint32 flag){assert(field==UNIT_FIELD_NPC_FLAGS);flags&=~flag;}
    void SetFlag(uint32 field,uint32 flag){assert(field==UNIT_FIELD_FLAGS);flags|=flag;}
    void InterruptNonMeleeSpells(bool delayed){assert(!delayed);++interrupted;}
    void SetDisableGravity(bool disabled){assert(disabled);movementFlags|=MOVEMENTFLAG_DISABLE_GRAVITY;}
    void AddUnitState(uint32 state){states|=state;}
    bool SetCharmedBy(Unit* passenger,uint32 type){assert(passenger && type==CHARM_TYPE_VEHICLE);++charmCalls;return charmResult;}
    void SendBreakTarget(Unit* target){assert(target);++breakTargets;}
    void SetControlled(bool apply,uint32 state){assert(apply && state==UNIT_STATE_ROOT);++rootCalls;states|=state;}
    void UpdateObjectVisibility(bool forced){assert(forced);++visibilityUpdates;}
};
struct Battleground {uint32 dropped=0;void EventPlayerDroppedFlag(Player* player){assert(player);++dropped;}};
struct Player : Unit
{
    uint32 charmStopped=0,sightStopped=0,cancelSent=0,petUnsummoned=0,collisionSent=0;
    Battleground* battleground=nullptr;
    Player* ToPlayer() override {return this;}
    Battleground* GetBattleground(){return battleground;}
    void StopCastingCharm(){++charmStopped;}
    void StopCastingBindSight(){++sightStopped;}
    void SendOnCancelExpectedVehicleRideAura(){++cancelSent;}
    void UnsummonPetTemporaryIfAny(){++petUnsummoned;}
    float GetCollisionHeight(){return 2.0f;}
    void SendMovementSetCollisionHeight(float height,uint32 reason){assert(height==2.0f && reason==2);++collisionSent;}
};
struct CreatureAI
{
    uint32 boarded=0;
    void PassengerBoarded(Unit* passenger,int8 seat,bool apply){assert(passenger && passenger->m_vehicle && seat>=0 && apply);++boarded;}
};
struct Creature : Unit
{
    bool IsAIEnabled=true,accessoryInit=false;
    struct {uint32 flags_extra=0;} creatureTemplate;
    CreatureAI ai;
    Creature* ToCreature() override {return this;}
    auto GetCreatureTemplate(){return &creatureTemplate;}
    CreatureAI* AI(){return &ai;}
    bool onVehicleAccessoryInit(){return accessoryInit;}
    void SetVehicleAccessoryInit(bool initialized){accessoryInit=initialized;}
};
namespace ObjectAccessor
{
    std::map<uint32,Unit*> units;
    Unit* GetUnit(Unit&,ObjectGuid guid){auto found=units.find(guid.value);return found==units.end()?nullptr:found->second;}
    Unit* FindUnit(ObjectGuid guid){auto found=units.find(guid.value);return found==units.end()?nullptr:found->second;}
}
struct VehicleSeatEntry
{
    uint32 ID=16558,Flags=0,FlagsB=0;
    bool usable=false;
    struct {float X=0,Y=0,Z=0;} AttachmentOffset;
    bool CanEnterOrExit() const {return usable;}
};
struct Vehicle
{
    struct Seat {struct {ObjectGuid Guid;bool IsUnselectable=false,IsGravityDisabled=false;} Passenger;VehicleSeatEntry const* SeatInfo=nullptr;};
    using SeatMap=std::map<int8,Seat>;
    VehicleSeatEntry seatInfo;
    SeatMap Seats;
    std::list<VehicleJoinEvent*> _pendingJoinEvents;
    std::mutex _lock;
    Unit* _me;
    uint32 _status=0,_creatureEntry=105886,UsableSeatNum=0,recAura=0,resets=0;
    struct {uint32 ID=4726;} vehicleInfo;
    explicit Vehicle(Unit* base):_me(base){base->kit=this;Seats.emplace(int8(0),Seat{{},&seatInfo});Seats.emplace(int8(1),Seat{{},&seatInfo});}
    Unit* GetBase(){return _me;}
    uint32 GetRecAura(){return recAura;}
    auto GetVehicleInfo(){return &vehicleInfo;}
    void Reset(){++resets;}
    void AddPendingEvent(VehicleJoinEvent*);
    void RemovePendingEvent(VehicleJoinEvent*);
    void RemovePendingEventsForSeat(int8);
    void RemovePendingEventsForPassenger(Unit*);
    void RemoveAllPassengers();
};
using SeatMap=Vehicle::SeatMap;
struct VehicleAttachmentOffset {Position Pos;};
struct ObjectManager
{
    bool hasAttachment=false;
    VehicleAttachmentOffset attachment{{4,5,6}};
    VehicleAttachmentOffset const* GetVehicleAttachmentOffset(Vehicle*,int8){return hasAttachment?&attachment:nullptr;}
} objectManager;
ObjectManager* sObjectMgr=&objectManager;
struct ScriptManager
{
    uint32 added=0,installed=0;
    void OnAddPassenger(Vehicle* vehicle,Unit* passenger,int8 seat){assert(passenger->m_vehicle==vehicle && vehicle->Seats.at(seat).Passenger.Guid.value==passenger->identity);++added;}
    void OnInstallAccessory(Vehicle* vehicle,Creature* passenger){assert(vehicle && passenger);++installed;}
} scriptManager;
ScriptManager* sScriptMgr=&scriptManager;
namespace Movement
{
    struct MoveSplineInit
    {
        Unit& passenger;
        Position destination;
        bool transformed=true,enter=false;
        explicit MoveSplineInit(Unit& unit):passenger(unit){}
        void DisableTransportPathTransformations(){transformed=false;}
        void MoveTo(float offsetX,float offsetY,float offsetZ,bool generate,bool force){assert(!generate && force);destination.Relocate(offsetX,offsetY,offsetZ);}
        void SetFacing(float angle){assert(angle==0);}
        void SetTransportEnter(){enter=true;}
        void Launch(){assert(!transformed && enter);++passenger.splines;}
    };
}
struct Counters {uint32 destroyed=0;} counters;
struct VehicleJoinEvent : BasicEvent
{
    std::shared_ptr<Object> ptr;
    Vehicle* vehicle;
    Unit* Target;
    Unit* Passenger;
    SeatMap::iterator Seat;
    ObjectGuid targetGuid;
    VehicleJoinEvent(Vehicle*,Unit*);
    ~VehicleJoinEvent();
    void Abort(uint64) override;
    bool Execute(uint64,uint32) override;
};
#include "Add.inc"
#include "Remove.inc"
#include "Seat.inc"
#include "Passenger.inc"
#include "All.inc"
#include "Construct.inc"
#include "Destroy.inc"
#include "Abort.inc"
#include "Execute.inc"
struct TrackedJoin : VehicleJoinEvent
{
    TrackedJoin(Vehicle* vehicle,Unit* passenger):VehicleJoinEvent(vehicle,passenger){}
    ~TrackedJoin() override {++counters.destroyed;}
};
static TrackedJoin* Queue(EventProcessor& processor,Vehicle& vehicle,Unit& passenger,int8 seat,uint32 delay)
{
    auto* event=new TrackedJoin(&vehicle,&passenger);
    event->Seat=vehicle.Seats.find(seat);
    assert(event->Seat!=vehicle.Seats.end());
    vehicle.AddPendingEvent(event);
    processor.AddEvent(event,processor.CalculateTime(delay));
    return event;
}
static void Reset()
{
    counters={};scriptManager={};objectManager={};ObjectAccessor::units.clear();debugMessages.clear();
}
struct RequeuedEvent : BasicEvent
{
    EventProcessor& processor;
    uint32& executions;
    uint32& destructions;
    RequeuedEvent(EventProcessor& owner,uint32& calls,uint32& deleted):processor(owner),executions(calls),destructions(deleted){}
    ~RequeuedEvent() override {++destructions;}
    bool Execute(uint64,uint32) override
    {
        if(++executions==1){processor.AddEvent(this,processor.CalculateTime(10));return false;}
        return true;
    }
};
int main()
{
    uint32 cases=0;
    for(uint32 failure=0;failure<5;++failure)
        for(uint32 kind=0;kind<4;++kind)
            for(bool auraPresent : {false,true})
                for(bool reentrant : {false,true})
                    {
                        Reset();auto base=std::make_shared<Creature>();Vehicle vehicle(base.get());
                        Player player;Creature creature,other;Unit guardian;
                        Unit* passenger=kind==0?static_cast<Unit*>(&player):(kind==1 || kind==3)?static_cast<Unit*>(&creature):&guardian;
                        passenger->identity=18;passenger->states=UNIT_STATE_ONVEHICLE;creature.accessory=kind==3;other.identity=19;
                        base->controlCasters={19};if(auraPresent)base->controlCasters.insert(18);
                        ObjectAccessor::units={{17,base.get()}};
                        if(reentrant)base->onRemove=[&](Unit* target,ObjectGuid caster){if(!target->GetVehicleKit())return;target->GetVehicleKit()->RemovePendingEventsForPassenger(passenger);assert(caster.value==18);passenger->_ExitVehicle();};
                        EventProcessor processor;
                        auto* failed=Queue(processor,vehicle,*passenger,0,0);
                        auto* unrelated=Queue(processor,vehicle,other,1,1000);
                        if(failure==0)passenger->inWorld=false;
                        if(failure==1)ObjectAccessor::units.clear();
                        if(failure==2)base->inWorld=false;
                        if(failure==3){vehicle.RemovePendingEvent(failed);base->kit=nullptr;}
                        if(failure==4)base->controlCasters.erase(18);
                        processor.Update(1);
                        assert(processor.Size()==1 && processor.SizeQueue()==0);
                        assert(passenger->m_vehicle==nullptr && !vehicle.Seats.at(0).Passenger.Guid);
                        assert(!passenger->interrupted && !passenger->splines && !passenger->rootCalls && !passenger->mountedRemoved);
                        assert(passenger->states==UNIT_STATE_ONVEHICLE);
                        assert(!scriptManager.added && !scriptManager.installed && !base->ai.boarded);
                        assert(!player.petUnsummoned && !player.cancelSent && !player.collisionSent && !base->charmCalls);
#ifdef BASELINE
                        assert(counters.destroyed==0 && base->removedCasters.empty() && passenger->despawns==0);
                        assert(vehicle._pendingJoinEvents.size()==(failure==3?1u:2u) && !failed->to_Abort);
                        vehicle.RemovePendingEvent(failed);delete failed;
#else
                        assert(counters.destroyed==1 && base->removedCasters==std::vector<uint32>{18});
                        assert(passenger->despawns==uint32(kind==3 && failure!=0));
                        assert(passenger->exits==uint32(reentrant && auraPresent && failure!=3 && failure!=4));
                        assert(vehicle._pendingJoinEvents==std::list<VehicleJoinEvent*>{unrelated});
#endif
                        assert(!unrelated->to_Abort && base->controlCasters.count(19));
                        base->kit=&vehicle;base->onRemove={};
                        vehicle.RemovePendingEventsForPassenger(&other);processor.Update(1000);
                        assert(counters.destroyed==2 && processor.Size()==0 && processor.SizeQueue()==0);
                        assert(other.despawns==0 && vehicle._pendingJoinEvents.empty());
                        processor.Update(1000);assert(counters.destroyed==2);
                        passenger->inWorld=true;base->inWorld=true;base->controlCasters.insert(18);
                        ObjectAccessor::units={{17,base.get()}};
                        Queue(processor,vehicle,*passenger,0,0);processor.Update(1);
                        assert(counters.destroyed==3 && passenger->m_vehicle==&vehicle && passenger->splines==1);
                        assert(base->ai.boarded==1 && scriptManager.added==1 && vehicle._pendingJoinEvents.empty());
                        processor.Update(100);assert(counters.destroyed==3 && scriptManager.added==1);
                        ++cases;
                    }
    for(uint32 kind=0;kind<4;++kind)
    {
        Reset();auto base=std::make_shared<Creature>();Vehicle vehicle(base.get());Player player;Creature creature;Unit guardian;
        Unit* passenger=kind==0?static_cast<Unit*>(&player):(kind==1 || kind==3)?static_cast<Unit*>(&creature):&guardian;
        passenger->identity=18;creature.accessory=kind==3;
        EventProcessor processor;auto* failed=Queue(processor,vehicle,*passenger,0,0);
        vehicle.RemovePendingEvent(failed);failed->Seat=vehicle.Seats.end();failed->ptr.reset();
        processor.Update(1);
#ifdef BASELINE
        assert(counters.destroyed==0 && debugMessages.empty());delete failed;
#else
        assert(counters.destroyed==1 && debugMessages==std::vector<std::vector<uint32>>({{18,105886,17}}));
        assert(passenger->despawns==uint32(kind==3));
#endif
        assert(!passenger->m_vehicle && !passenger->splines && !scriptManager.added && vehicle._pendingJoinEvents.empty());
        processor.Update(100);assert(counters.destroyed==1 && processor.Size()==0 && processor.SizeQueue()==0);
        ++cases;
    }
    for(uint32 kind=0;kind<3;++kind)
        for(bool usable : {false,true})
            for(bool control : {false,true})
                for(bool recAura : {false,true})
                    for(bool keepPet : {false,true})
                    {
                        Reset();auto base=std::make_shared<Creature>();Vehicle vehicle(base.get());
                        Player player;Creature creature;Unit guardian;
                        Unit* passenger=kind==0?static_cast<Unit*>(&player):kind==1?static_cast<Unit*>(&creature):&guardian;
                        passenger->identity=18;passenger->flags=UNIT_FLAG_NOT_SELECTABLE;passenger->movementFlags=MOVEMENTFLAG_DISABLE_GRAVITY;
                        creature.accessory=true;creature.accessoryInit=true;
                        vehicle.seatInfo.usable=usable;vehicle.UsableSeatNum=uint32(usable);vehicle.recAura=recAura?1:0;
                        vehicle.seatInfo.Flags=(control?VEHICLE_SEAT_FLAG_CAN_CONTROL:0)|VEHICLE_SEAT_FLAG_DISABLE_GRAVITY|VEHICLE_SEAT_FLAG_HIDE_PASSENGER|VEHICLE_SEAT_FLAG_PASSENGER_NOT_SELECTABLE;
                        vehicle.seatInfo.FlagsB=keepPet?VEHICLE_SEAT_FLAG_B_KEEP_PET:0;
                        if(!recAura)base->controlCasters.insert(18);
                        Battleground battleground;player.battleground=&battleground;
                        ObjectAccessor::units={{17,base.get()}};EventProcessor processor;
                        Queue(processor,vehicle,*passenger,0,0);processor.Update(1);
                        assert(counters.destroyed==1 && processor.Size()==0 && vehicle._pendingJoinEvents.empty());
                        assert(passenger->m_vehicle==&vehicle && vehicle.Seats.at(0).Passenger.Guid.value==18);
                        assert(vehicle.Seats.at(0).Passenger.IsUnselectable && vehicle.Seats.at(0).Passenger.IsGravityDisabled);
                        assert(vehicle.UsableSeatNum==0 && passenger->interrupted==1 && passenger->mountedRemoved==1);
                        assert(passenger->splines==1 && passenger->rootCalls==1 && passenger->breakTargets==1);
                        assert(passenger->m_movementInfo.transport.Guid.value==17 && passenger->m_movementInfo.transport.VehicleSeatIndex==0);
                        assert(passenger->m_movementInfo.transport.VehicleRecID==4726 && passenger->m_movementInfo.transport.MoveTime==0);
                        assert(scriptManager.added==1 && base->ai.boarded==1 && scriptManager.installed==uint32(kind==1));
                        assert(base->removedCasters.empty() && !passenger->despawns);
                        assert(player.petUnsummoned==uint32(kind==0 && !keepPet) && player.collisionSent==uint32(kind==0));
                        assert(base->charmCalls==uint32(kind==0 && control) && battleground.dropped==uint32(kind==0));
                        processor.Update(100);assert(counters.destroyed==1 && passenger->splines==1 && scriptManager.added==1);
                        ++cases;
                    }
    for(uint32 lateFailure=0;lateFailure<2;++lateFailure)
    {
        Reset();auto base=std::make_shared<Creature>();Vehicle vehicle(base.get());Player player;player.identity=18;
        base->controlCasters.insert(18);ObjectAccessor::units={{17,base.get()}};
        if(lateFailure==0){vehicle.seatInfo.usable=true;vehicle.UsableSeatNum=0;}
        else {vehicle.seatInfo.Flags=VEHICLE_SEAT_FLAG_CAN_CONTROL;base->charmResult=false;}
        EventProcessor processor;auto* event=Queue(processor,vehicle,player,0,0);processor.Update(1);
        assert(counters.destroyed==0 && processor.Size()==0 && processor.SizeQueue()==0);
        assert(vehicle._pendingJoinEvents.empty() && player.m_vehicle==&vehicle && vehicle.Seats.at(0).Passenger.Guid.value==18);
        assert(!player.splines && !scriptManager.added && !base->ai.boarded);
        assert(player.petUnsummoned==lateFailure && base->charmCalls==lateFailure);
        delete event;
        ++cases;
    }
    {
        EventProcessor processor;uint32 executions=0,destructions=0;
        processor.AddEvent(new RequeuedEvent(processor,executions,destructions),0);
        processor.Update(1);assert(executions==1 && destructions==0 && processor.SizeQueue()==1);
        processor.Update(10);assert(executions==2 && destructions==1);
        ++cases;
    }
    std::cout<<"PASS: "<<cases<<" full extracted Execute/native Abort/destructor/EventProcessor controlled cases; ";
#ifdef BASELINE
    std::cout<<"pinned6355ef63 strands prepublication failures outside the native queue without abort or destruction.\n";
#else
    std::cout<<"early failures abort and finish once without publication or unrelated cancellation; success and explicit requeue semantics retained. Two late postpublication failures remain reproduced OPEN, not repaired; world/aura services/C_PTR/client are controlled or untested.\n";
#endif
}
