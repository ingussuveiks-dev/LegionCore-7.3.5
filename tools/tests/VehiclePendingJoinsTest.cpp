#include <algorithm>
#include <cassert>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <vector>
#include "EventProcessor.cpp"
using int8=int8_t;
#define TC_LOG_DEBUG(...) ((void)0)
#define TC_LOG_ERROR(...) assert(false)
enum { SPELL_AURA_CONTROL_VEHICLE=236, UNIT_MASK_ACCESSORY=1, STATUS_UNINSTALLING=1 };
struct ObjectGuid
{
    uint32 value=0;
    explicit operator bool() const {return value!=0;}
    void Clear(){value=0;}
    static ObjectGuid const Empty;
};
ObjectGuid const ObjectGuid::Empty{};
struct Unit;
struct Vehicle;
struct VehicleJoinEvent;
struct Object : std::enable_shared_from_this<Object>
{
    virtual ~Object()=default;
    virtual Unit* ToUnit(){return nullptr;}
};
struct Unit : Object
{
    uint32 identity=17,despawns=0,exits=0;
    bool inWorld=true,accessory=false;
    Vehicle* kit=nullptr;
    std::vector<uint32> removedCasters;
    uint32 removedAll=0;
    Unit* ToUnit() override {return this;}
    Unit* ToCreature(){return this;}
    std::shared_ptr<Object> get_ptr(){return shared_from_this();}
    Vehicle* GetVehicleKit(){return kit;}
    ObjectGuid GetGUID() const {return {identity};}
    uint32 GetGUIDLow() const {return identity;}
    bool IsInWorld() const {return inWorld;}
    bool HasUnitTypeMask(uint32 mask) const {assert(mask==UNIT_MASK_ACCESSORY);return accessory;}
    void RemoveAurasByType(uint32 aura,ObjectGuid caster){assert(aura==236);removedCasters.push_back(caster.value);}
    void RemoveAurasByType(uint32 aura){assert(aura==236);++removedAll;}
    void DespawnOrUnsummon(){++despawns;}
    void _ExitVehicle(){++exits;}
};
namespace ObjectAccessor {Unit* FindUnit(ObjectGuid){return nullptr;}}
struct Vehicle
{
    struct Seat {struct {ObjectGuid Guid;} Passenger;};
    using SeatMap=std::map<int8,Seat>;
    SeatMap Seats{{0,{}},{1,{}}};
    std::list<VehicleJoinEvent*> _pendingJoinEvents;
    std::mutex _lock;
    Unit* _me;
    uint32 _status=0,_creatureEntry=105886;
    explicit Vehicle(Unit* base):_me(base){base->kit=this;}
    Unit* GetBase(){return _me;}
    void AddPendingEvent(VehicleJoinEvent*);
    void RemovePendingEvent(VehicleJoinEvent*);
    void RemovePendingEventsForSeat(int8);
    void RemovePendingEventsForPassenger(Unit*);
    void RemoveAllPassengers();
};
using SeatMap=Vehicle::SeatMap;
struct Counters {uint32 executed=0,destroyed=0;} counters;
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
    bool Execute(uint64,uint32) override {++counters.executed;return true;}
};
#include "Add.inc"
#include "Remove.inc"
#include "Seat.inc"
#include "Passenger.inc"
#include "All.inc"
#include "Construct.inc"
#include "Destroy.inc"
#include "Abort.inc"
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
int main()
{
    uint32 cases=0;
    for(uint32 length=0;length<8;++length)
        for(uint32 selected=0;selected<10;++selected)
            for(bool duplicate : {false,true})
            {
                auto base=std::make_shared<Unit>();Unit passenger;
                Vehicle vehicle(base.get());
                std::vector<std::unique_ptr<TrackedJoin>> events;
                for(uint32 index=0;index<9;++index)events.emplace_back(new TrackedJoin(&vehicle,&passenger));
                for(uint32 index=0;index<length;++index)vehicle.AddPendingEvent(events[index].get());
                VehicleJoinEvent* target=selected<9?events[selected].get():nullptr;
                if(duplicate && target)vehicle.AddPendingEvent(target);
                auto expected=vehicle._pendingJoinEvents;
                expected.remove(target);
                vehicle.RemovePendingEvent(target);
#ifdef BASELINE
                assert(vehicle._pendingJoinEvents.empty());
#else
                assert(vehicle._pendingJoinEvents==expected);
#endif
                vehicle.RemovePendingEvent(target);
                for(auto const& event:events)assert(!event->to_Abort);
                vehicle._pendingJoinEvents.clear();
                ++cases;
            }
    for(uint32 cancel=0;cancel<3;++cancel)
        for(uint32 passengerKind=0;passengerKind<4;++passengerKind)
            for(bool uninstall : {false,true})
                for(bool replacement : {false,true})
                {
                    counters={};auto base=std::make_shared<Unit>();Unit first,second,third;
                    first.identity=18;second.identity=19;third.identity=20;second.accessory=passengerKind==3;
                    Vehicle vehicle(base.get());EventProcessor processor;
                    auto* previous=Queue(processor,vehicle,first,0,0);
                    auto* pending=Queue(processor,vehicle,second,1,20);
                    if(cancel==0)previous->to_Abort=true;
                    if(cancel==1)vehicle.RemovePendingEventsForPassenger(&first);
                    if(cancel==2)vehicle.RemovePendingEventsForSeat(0);
                    processor.Update(1);
                    assert(counters.destroyed==1 && counters.executed==0 && base->removedCasters==std::vector<uint32>{18});
#ifdef BASELINE
                    assert(vehicle._pendingJoinEvents.empty() && !pending->to_Abort);
#else
                    assert(vehicle._pendingJoinEvents==std::list<VehicleJoinEvent*>{pending});
#endif
                    std::unique_ptr<Vehicle> next;
                    if(replacement)
                    {
                        next=std::make_unique<Vehicle>(base.get());
                        auto* nextPending=Queue(processor,*next,third,0,40);
                        assert(nextPending->Passenger==&third);
                        vehicle.RemovePendingEvent(nullptr);
#ifndef BASELINE
                        assert(next->_pendingJoinEvents==std::list<VehicleJoinEvent*>{nextPending});
#endif
                    }
                    vehicle._status=uninstall?STATUS_UNINSTALLING:0;
                    vehicle.RemoveAllPassengers();
                    assert(base->removedAll==1);
                    processor.Update(20);
#ifdef BASELINE
                    assert(counters.executed==1 && counters.destroyed==2 && second.despawns==0);
#else
                    assert(counters.executed==0 && counters.destroyed==2 && second.despawns==uint32(second.accessory));
                    assert(base->removedCasters==std::vector<uint32>({18,19}));
#endif
                    if(next)
                    {
#ifdef BASELINE
                        assert(next->_pendingJoinEvents.empty());
#else
                        assert(next->_pendingJoinEvents.size()==1);
#endif
                        next->RemoveAllPassengers();processor.Update(20);
                        assert(counters.destroyed==3);
#ifdef BASELINE
                        assert(counters.executed==2);
#else
                        assert(counters.executed==0 && base->removedCasters==std::vector<uint32>({18,19,20}));
#endif
                    }
                    assert(vehicle._pendingJoinEvents.empty());
                    processor.Update(100);
                    assert(counters.destroyed==uint32(replacement?3:2));
                    ++cases;
                }
    std::cout<<"PASS: "<<cases<<" native pending-registry/event-queue/destructor/abort cases; ";
#ifdef BASELINE
    std::cout<<"pinnedf784d4fb erases unrelated pending joins, so later native cancellation misses them.\n";
#else
    std::cout<<"identity removal preserves other joins for native passenger/seat/uninstall cancellation; actual EventProcessor dispatch/destruction and native aura-caster abort retained. Execute/world/vehicle allocation/geometry/client are controlled or untested.\n";
#endif
}
