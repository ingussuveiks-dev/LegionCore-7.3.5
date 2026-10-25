#define VEHICLE_JOIN_FAILURE_LIBRARY
#define VEHICLE_NATIVE_SEAT_PREDICATE
#include "VehicleJoinFailureTest.cpp"
#include "CanEnter.inc"
int main()
{
    uint32 cases=0;
    for(bool usable : {false,true})
        for(uint32 count : {0u,1u,2u})
            for(uint32 kind=0;kind<4;++kind)
                for(bool reentrant : {false,true})
                    for(int8 futureSeat : {int8(0),int8(1)})
                        for(bool recAura : {false,true})
                        {
                            Reset();auto base=std::make_shared<Creature>();Vehicle vehicle(base.get());Player player;Creature creature;Unit guardian,other;
                            Unit* passenger=kind==0?static_cast<Unit*>(&player):(kind==1 || kind==3)?static_cast<Unit*>(&creature):&guardian;
                            passenger->identity=18;other.identity=19;creature.accessory=kind==3;
                            vehicle.seatInfo.Flags=usable?VEHICLE_SEAT_FLAG_CAN_ENTER_OR_EXIT:0x40000007;vehicle.UsableSeatNum=count;
                            vehicle.recAura=recAura?1:0;base->controlCasters={19};if(!recAura)base->controlCasters.insert(18);
                            ObjectAccessor::units={{17,base.get()}};
                            if(reentrant)base->onRemove=[&](Unit* target,ObjectGuid caster){if(caster.value==18)target->GetVehicleKit()->RemovePendingEventsForPassenger(passenger);};
                            EventProcessor processor;auto* event=Queue(processor,vehicle,*passenger,0,0);
                            assert(event->Passenger==passenger && event->Seat->first==0);
                            auto* future=Queue(processor,vehicle,other,futureSeat,1000);
                            processor.Update(1);
                            bool rejected=usable && count==0;
#ifdef USABLE_SEAT_BASELINE
                            if(rejected)
                            {
                                assert(counters.destroyed==0 && passenger->m_vehicle==&vehicle && vehicle.Seats.at(0).Passenger.Guid.value==18);
                                assert(base->removedCasters.empty() && !passenger->despawns && !passenger->splines);
                                assert(vehicle.UsableSeatNum==0 && future->to_Abort==(futureSeat==0));
                                assert(vehicle._pendingJoinEvents.size()==uint32(futureSeat!=0));
                                delete event;
                            }
                            else
#endif
                            if(rejected)
                            {
                                assert(counters.destroyed==1 && !passenger->m_vehicle && !vehicle.Seats.at(0).Passenger.Guid);
                                assert(base->removedCasters==std::vector<uint32>{18} && passenger->despawns==uint32(kind==3));
                                assert(vehicle.UsableSeatNum==0 && !passenger->splines && !passenger->rootCalls && !passenger->interrupted);
                                assert(!player.petUnsummoned && !scriptManager.added && !base->ai.boarded);
                                assert(vehicle._pendingJoinEvents==std::list<VehicleJoinEvent*>{future} && !future->to_Abort);
                            }
                            else
                            {
                                assert(counters.destroyed==1 && passenger->m_vehicle==&vehicle && vehicle.Seats.at(0).Passenger.Guid.value==18);
                                assert(vehicle.UsableSeatNum==count-uint32(usable) && passenger->splines==1 && base->removedCasters.empty());
                                assert(scriptManager.added==1 && base->ai.boarded==1);
                            }
                            assert(base->controlCasters.count(19) && processor.Size()==1);
                            base->onRemove={};vehicle.RemovePendingEventsForPassenger(&other);processor.Update(1000);
                            assert(counters.destroyed==2 && vehicle._pendingJoinEvents.empty());
#ifndef USABLE_SEAT_BASELINE
                            if(rejected)
                            {
                                vehicle.UsableSeatNum=1;base->controlCasters.insert(18);
                                Queue(processor,vehicle,*passenger,0,0);processor.Update(1);
                                assert(counters.destroyed==3 && passenger->m_vehicle==&vehicle && passenger->splines==1 && vehicle.UsableSeatNum==0);
                            }
#endif
                            uint32 finished=counters.destroyed;processor.Update(1000);assert(counters.destroyed==finished);
                            ++cases;
                        }
    std::cout<<"PASS: "<<cases<<" full native Execute/Abort/EventProcessor usable-seat admission cases; ";
#ifdef USABLE_SEAT_BASELINE
    std::cout<<"pinned321206b1 publishes a zero-count seat then strands its event without cleanup.\n";
#else
    std::cout<<"zero count rejects before publication/cancellation; normal counted and non-enterable seats, unrelated requests, caster cleanup and explicit retry retained. World/aura/C_PTR/client controlled or untested; late charm failure remains open.\n";
#endif
}
