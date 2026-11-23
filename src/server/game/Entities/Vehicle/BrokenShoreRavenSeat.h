#ifndef BrokenShoreRavenSeat_h__
#define BrokenShoreRavenSeat_h__

#include "DB2Structure.h"
#include "Vehicle.h"

namespace AssaultBrokenShore
{
    inline bool IsArcaneRavenRider(Unit const& rider)
    {
        Vehicle const* vehicle = rider.GetVehicle();
        if (!rider.IsPlayer() || !vehicle || vehicle->GetCreatureEntry() != 118517 ||
            vehicle->GetVehicleInfo()->ID != 5416 || rider.GetMapId() != 1666)
            return false;

        VehicleSeatEntry const* seat = vehicle->GetSeatForPassenger(&rider);
        return seat && seat->ID == 18005;
    }
}

#endif
