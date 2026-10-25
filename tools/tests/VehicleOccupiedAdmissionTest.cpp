#define VEHICLE_JOIN_FAILURE_LIBRARY
#define VEHICLE_NATIVE_SEAT_PREDICATE
#define TC_LOG_ERROR(...) DebugLog(__VA_ARGS__)
unsigned admissionDestroyed=0;
#include "VehicleJoinFailureTest.cpp"
#include "CanEnter.inc"
#include "Admission.inc"
int main()
{
    _set_error_mode(_OUT_TO_STDERR);
    uint32 cases=0;
    for(uint32 blocker=0;blocker<4;++blocker)
        for(uint32 kind=0;kind<4;++kind)
            for(bool enterable : {false,true})
                for(bool recAura : {false,true})
                    for(bool reentrant : {false,true})
                        for(int8 futureSeat : {int8(0),int8(1)})
                        for(bool playerBase : {false,true})
                        {
                            Reset();admissionDestroyed=0;auto creatureBase=std::make_shared<Creature>();auto ownerBase=std::make_shared<Player>();
                            std::shared_ptr<Unit> base=playerBase?std::static_pointer_cast<Unit>(ownerBase):std::static_pointer_cast<Unit>(creatureBase);Vehicle vehicle(base.get());
                            Player player,occupant;Creature creature;Unit guardian,other;
                            Unit* passenger=kind==0?static_cast<Unit*>(&player):(kind==1 || kind==3)?static_cast<Unit*>(&creature):&guardian;
                            passenger->identity=18;occupant.identity=19;other.identity=20;creature.accessory=kind==3;
                            vehicle.seatInfo.Flags=enterable?VEHICLE_SEAT_FLAG_CAN_ENTER_OR_EXIT:0x40000007;vehicle.UsableSeatNum=enterable?1:0;
                            vehicle.recAura=recAura?1:0;base->controlCasters={18,19,20};
                            vehicle.Seats.at(0).Passenger.Guid=occupant.GetGUID();
                            ObjectAccessor::units={{17,base.get()},{18,passenger},{20,&other}};
                            if(blocker!=0)ObjectAccessor::units.emplace(19,&occupant);
                            if(blocker==2)occupant.onExit=[&]{vehicle.Seats.at(0).Passenger.Guid.Clear();};
                            if(blocker==3)occupant.onExit=[&]{vehicle.RemovePendingEventsForPassenger(passenger);};
                            if(reentrant)base->onRemove=[&](Unit* target,ObjectGuid caster){assert(caster==passenger->GetGUID() || caster==other.GetGUID());target->GetVehicleKit()->RemovePendingEventsForPassenger(caster==passenger->GetGUID()?passenger:&other);};
                            auto* future=Queue(other.m_Events,vehicle,other,futureSeat,1000);
                            bool admitted=vehicle.AddPassenger(passenger,0);
                            assert(admitted==(blocker==2) && passenger->m_Events.SizeQueue()==1);
                            assert(passenger->waitOnSeat && occupant.exits==uint32(blocker!=0));
                            assert(!passenger->m_vehicle && !passenger->splines && !player.petUnsummoned && !future->to_Abort);
                            bool rejected=!admitted;
#ifdef OCCUPIED_ADMISSION_BASELINE
                            bool unmarked=blocker<=1;
#else
                            bool unmarked=false;
#endif
                            passenger->m_Events.Update(1);
                            assert(admissionDestroyed==1 && !passenger->m_Events.Size() && !passenger->m_Events.SizeQueue());
                            if(rejected && !unmarked)
                            {
                                assert(!passenger->m_vehicle && vehicle.Seats.at(0).Passenger.Guid==occupant.GetGUID());
                                assert(vehicle.UsableSeatNum==uint32(enterable) && !passenger->splines && !player.petUnsummoned);
                                assert(base->removedCasterGuids==std::vector<ObjectGuid>{passenger->GetGUID()});
                                assert(base->HasAuraTypeWithCaster(236,occupant.GetGUID()) && base->HasAuraTypeWithCaster(236,other.GetGUID()));
                                assert(!future->to_Abort && !scriptManager.added && !creatureBase->ai.boarded);
                                assert(!passenger->states && player.extraLook.empty() && ownerBase->extraLook.empty());
                            }
                            else
                            {
                                assert(passenger->m_vehicle==&vehicle && vehicle.Seats.at(0).Passenger.Guid==passenger->GetGUID());
                                assert(vehicle.UsableSeatNum==0 && passenger->splines==1 && scriptManager.added==uint32(!playerBase) && creatureBase->ai.boarded==uint32(!playerBase));
                                assert(future->to_Abort==(futureSeat==0) && base->removedCasterGuids.empty());
                                assert(!rejected || (unmarked && occupant.exits==uint32(blocker==1)));
                            }
                            assert(vehicle._pendingJoinEvents==(future->to_Abort?std::list<VehicleJoinEvent*>{}:std::list<VehicleJoinEvent*>{future}));
                            vehicle.RemovePendingEventsForPassenger(&other);other.m_Events.Update(1000);
                            assert(admissionDestroyed==2 && vehicle._pendingJoinEvents.empty());
                            if(rejected && !unmarked && kind!=3)
                            {
                                base->onRemove={};vehicle.Seats.at(0).Passenger.Guid.Clear();base->controlCasters.insert(passenger->GetGUID());
                                assert(vehicle.AddPassenger(passenger,0));passenger->m_Events.Update(1);
                                assert(admissionDestroyed==3 && passenger->m_vehicle==&vehicle && scriptManager.added==uint32(!playerBase));
                            }
                            unsigned finished=admissionDestroyed;passenger->m_Events.Update(1000);other.m_Events.Update(1000);assert(admissionDestroyed==finished);
                            ++cases;
                        }
    Reset();admissionDestroyed=0;auto base=std::make_shared<Creature>();Vehicle vehicle(base.get());Player passenger;
    passenger.identity=18;ObjectAccessor::units={{17,base.get()}};base->controlCasters={18};
    vehicle.seatInfo.Flags=VEHICLE_SEAT_FLAG_CAN_ENTER_OR_EXIT;vehicle.UsableSeatNum=2;
    assert(!vehicle.AddPassenger(&passenger,9));passenger.m_Events.Update(1);assert(admissionDestroyed==1 && !passenger.waitOnSeat);
    vehicle.Seats.at(0).Passenger.Guid={19};vehicle.Seats.at(1).Passenger.Guid={20};base->controlCasters.insert(18);
    assert(!vehicle.AddPassenger(&passenger,-1));passenger.m_Events.Update(1);assert(admissionDestroyed==2);
    vehicle.Seats.at(1).Passenger.Guid.Clear();base->controlCasters.insert(18);
    assert(vehicle.AddPassenger(&passenger,-1));passenger.m_Events.Update(1);assert(admissionDestroyed==3 && vehicle.Seats.at(1).Passenger.Guid==passenger.GetGUID());
    assert(!vehicle.AddPassenger(nullptr,0));vehicle._status=STATUS_UNINSTALLING;assert(!vehicle.AddPassenger(&passenger,0));assert(!passenger.m_Events.SizeQueue());
    assert(debugMessages.back()==std::vector<uint32>({18,105886,17,105886,0}));
    assert(counters.destroyed==0);
    std::cout<<"PASS: "<<cases<<" full AddPassenger/Execute/Abort/native queue cases plus invalid/full/auto/null/uninstall admission; ";
#ifdef OCCUPIED_ADMISSION_BASELINE
    std::cout<<"pinned07074932 returns false for a retained occupied seat but still overwrites its GUID and boards asynchronously.\n";
#else
    std::cout<<"retained occupied seat marks its pending event for native abort, preserves incumbent and other requests, supports controlled release/retry without overwriting. Admission prestate/world/actual exit/packets/client remain controlled or OPEN.\n";
#endif
}
