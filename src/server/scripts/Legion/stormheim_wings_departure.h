#pragma once

namespace StormheimWings
{
struct Departure {float x,y,z;};
// Authored collision-surveyed first flight points. Aleifir must leave his
// cave to the east before ascending; the other three can rise in place.
constexpr Departure Departures[] = {
 {2113.7f,2186.07f,470.226f},
 {2132.36f,2372.49f,492.706f},
 {2057.1f,2473.28f,485.957f},
 {1998.81f,2385.1f,484.035f}
};
}
