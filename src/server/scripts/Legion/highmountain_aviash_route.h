#ifndef HIGHMOUNTAIN_AVIASH_ROUTE_H
#define HIGHMOUNTAIN_AVIASH_ROUTE_H

#include "Position.h"

namespace Aviash
{
// Reconstructed flight, not a recovered retail spline. Both endpoints are
// based on the 7.3.5 spawn/WorldSafeLocs data. Boarding is 3 m above the
// source ground spawn to clear the surrounding rock. Keep segments linear so
// the collision probe checks exactly the path used by MovePoint.
Position const Start = {5121.46f,4461.57f,1024.69f,0.0f};
Position const Return = {5121.46f,4461.57f,1021.69f,0.0f};
Position const Landing = {4226.380371f,4421.390625f,663.388977f,3.556257f};
Position const Route[] = {
    {5121.46f,4461.57f,1090.0f,0.0f},
    {5050.0f,4470.0f,1100.0f,0.0f},
    {4900.0f,4480.0f,1070.0f,0.0f},
    {4750.0f,4490.0f,1040.0f,0.0f},
    {4500.0f,4460.0f,1000.0f,0.0f},
    {4300.0f,4430.0f,850.0f,0.0f},
    {4226.380371f,4421.390625f,680.0f,0.0f},
    {4226.380371f,4421.390625f,665.388977f,3.556257f},
};
}
#endif
