#ifndef STORMHEIM_HEART_ROUTE_H
#define STORMHEIM_HEART_ROUTE_H

namespace StormheimHeart
{
// The captive is inside a cave. Leave through its southern mouth before
// climbing; a vertical flight from the captive's position crosses the roof.
Position const Departure[] = {
    {2026.87f, 2629.76f, 477.371f, 0.0f},
    {2026.87f, 2617.26f, 477.371f, 0.0f},
    {2011.87f, 2597.26f, 489.871f, 0.0f},
    {2009.37f, 2592.26f, 487.371f, 0.0f},
    {2009.37f, 2567.26f, 487.371f, 0.0f},
    {2009.37f, 2559.76f, 492.371f, 0.0f},
    {2004.37f, 2557.26f, 499.871f, 0.0f},
    {1981.87f, 2552.26f, 517.371f, 0.0f},
    {1940.0f, 2520.0f, 550.0f, 0.0f},
    {1900.0f, 2500.0f, 610.0f, 0.0f},
    {1870.0f, 2470.0f, 680.0f, 0.0f}
};
constexpr size_t DepartureSize = sizeof(Departure) / sizeof(Departure[0]);
}
#endif
