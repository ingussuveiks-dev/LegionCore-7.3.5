#pragma once

#include <cstdint>

// Aerylia's matching 7.3.5 destination jumps and their native arrival spells.
// Ulduar218015 is a direct teleport, not one of these jumps.
namespace SkyholdDeparture
{
    inline uint32_t Arrival(uint32_t jump)
    {
        switch (jump)
        {
            case 191473: return 191474;
            case 193334: return 193335;
            case 203921: return 203920;
            case 205813: return 205814;
            case 225115: return 225114;
            case 225163: return 225162;
            case 225220: return 225219;
            case 225233: return 225232;
            case 229808: return 236618;
            case 236624: return 234642;
            case 241928: return 241931;
            default: return 0;
        }
    }

    inline bool Matches(uint32_t jump, uint32_t arrival)
    {
        return arrival && Arrival(jump) == arrival;
    }
}
