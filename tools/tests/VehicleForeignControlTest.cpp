#define VEHICLE_JOIN_FAILURE_LIBRARY
#define VEHICLE_NATIVE_SEAT_PREDICATE
#define VEHICLE_NATIVE_CHARM_ADMISSION
#include "VehicleJoinFailureTest.cpp"
#include "CanEnter.inc"
#include "CharmAdmission.inc"
int main()
{
    uint32 cases=0;
    for(uint32 ownerKind=0;ownerKind<4;++ownerKind)
        for(bool control : {false,true})
            for(uint32 kind=0;kind<4;++kind)
                for(bool recAura : {false,true})
                    for(int8 futureSeat : {int8(0),int8(1)})
                        for(bool reentrant : {false,true})
                            for(bool enterable : {false,true})
                            {
                                Reset();auto base=std::make_shared<Creature>();Vehicle vehicle(base.get());Player player;Creature creature;Unit guardian,other;
                                Unit* passenger=kind==0?static_cast<Unit*>(&player):(kind==1 || kind==3)?static_cast<Unit*>(&creature):&guardian;
                                passenger->identity=18;other.identity=19;creature.accessory=kind==3;
                                ObjectGuid original=ownerKind==0?ObjectGuid{}:ownerKind==1?ObjectGuid{18}:ownerKind==2?ObjectGuid{19}:ObjectGuid{18,1};
                                base->charmerGuid=original;vehicle.seatInfo.Flags=(control?VEHICLE_SEAT_FLAG_CAN_CONTROL:0)|(enterable?VEHICLE_SEAT_FLAG_CAN_ENTER_OR_EXIT:0);
                                vehicle.UsableSeatNum=enterable?1:0;
                                vehicle.recAura=recAura?1:0;base->controlCasters={19};if(!recAura)base->controlCasters.insert(passenger->GetGUID());
                                if(original)base->controlCasters.insert(original);
                                ObjectAccessor::units={{17,base.get()}};
                                if(reentrant)base->onRemove=[&](Unit* target,ObjectGuid caster){assert(caster==passenger->GetGUID());target->GetVehicleKit()->RemovePendingEventsForPassenger(passenger);};
                                EventProcessor processor;auto* event=Queue(processor,vehicle,*passenger,0,0);
                                assert(event->Passenger==passenger);
                                auto* future=Queue(processor,vehicle,other,futureSeat,1000);
                                processor.Update(1);
                                bool nativeFailure=kind==0 && control && ownerKind!=0;
                                bool rejected=kind==0 && control && ownerKind>=2;
                                bool handled=rejected;
#ifdef FOREIGN_CHARM_BASELINE
                                handled=false;
#endif
                                assert(base->charmerGuid==original && processor.Size()==1);
                                if(handled)
                                {
                                    assert(counters.destroyed==1 && !passenger->m_vehicle && !vehicle.Seats.at(0).Passenger.Guid);
                                    assert(vehicle.UsableSeatNum==uint32(enterable));
                                    assert(!passenger->interrupted && !passenger->mountedRemoved && !passenger->rootCalls && !passenger->splines);
                                    assert(!player.petUnsummoned && !player.cancelSent && !base->charmCalls && !scriptManager.added && !base->ai.boarded);
                                    assert(base->removedCasterGuids==std::vector<ObjectGuid>{passenger->GetGUID()});
                                    assert(base->HasAuraTypeWithCaster(236,original) && !base->HasAuraTypeWithCaster(236,passenger->GetGUID()));
                                    assert(!future->to_Abort && vehicle._pendingJoinEvents==std::list<VehicleJoinEvent*>{future});
                                    assert(fatalMessages==std::vector<std::vector<uint32>>({{105886,17,105886,18}}));
                                }
                                else if(nativeFailure)
                                {
                                    assert(counters.destroyed==0 && passenger->m_vehicle==&vehicle && vehicle.Seats.at(0).Passenger.Guid.value==18);
                                    assert(vehicle.UsableSeatNum==0);
                                    assert(player.petUnsummoned==1 && base->charmCalls==1 && !passenger->splines && !scriptManager.added);
                                    assert(base->removedCasters.empty() && future->to_Abort==(futureSeat==0));
                                    assert(fatalMessages==std::vector<std::vector<uint32>>({{105886,17,105886,18}}));
                                    delete event;
                                }
                                else
                                {
                                    assert(counters.destroyed==1 && passenger->m_vehicle==&vehicle && passenger->splines==1);
                                    assert(vehicle.UsableSeatNum==0);
                                    assert(base->charmCalls==uint32(kind==0 && control) && fatalMessages.empty());
                                    assert(scriptManager.added==1 && base->ai.boarded==1);
                                }
                                base->onRemove={};vehicle.RemovePendingEventsForPassenger(&other);processor.Update(1000);
                                assert(counters.destroyed==2 && vehicle._pendingJoinEvents.empty());
                                if(handled)
                                {
                                    base->charmerGuid={};base->controlCasters.insert(passenger->GetGUID());
                                    Queue(processor,vehicle,*passenger,0,0);processor.Update(1);
                                    assert(counters.destroyed==3 && passenger->m_vehicle==&vehicle && base->charmCalls==1 && passenger->splines==1);
                                    assert(scriptManager.added==1 && base->ai.boarded==1);
                                }
                                uint32 finished=counters.destroyed;processor.Update(1000);assert(counters.destroyed==finished);
                                ++cases;
                            }
    std::cout<<"PASS: "<<cases<<" full Execute/Abort/native queue and extracted native charm-admission-prefix cases; ";
#ifdef FOREIGN_CHARM_BASELINE
    std::cout<<"pinned30d6a929 rejects foreign charm only after publishing seat/dismissing pet/cancelling another request, then strands the event.\n";
#else
    std::cout<<"foreign control rejects before publication and side effects, preserving full owner/caster GUIDs and fatal diagnostic; non-control/non-Player paths and explicit host retry retained. Same-owner and later charm failures stay OPEN; actual binding/aura/world/C_PTR/client remain controlled or untested.\n";
#endif
}
