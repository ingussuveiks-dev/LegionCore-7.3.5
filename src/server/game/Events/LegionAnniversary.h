#ifndef LEGION_ANNIVERSARY_H
#define LEGION_ANNIVERSARY_H

#include "Common.h"
#include "Util.h"

namespace LegionAnniversary
{
    constexpr uint16 Event = 693;
    constexpr uint32 Gift = 147876;
    constexpr uint32 HistorianLlore = 110034;
    constexpr uint32 HistorianJupa = 110035;

    struct Season
    {
        uint32 Year;
        time_t Start;
        time_t End;
        time_t NextStart;
    };

    inline Season GetSeason(time_t now)
    {
        std::tm local{};
        localtime_r(&now, &local);
        std::tm november{};
        november.tm_year = local.tm_year;
        november.tm_mon = 10;
        november.tm_mday = 16;
        november.tm_isdst = -1;
        time_t start = std::mktime(&november);
        november.tm_mday = 30;
        november.tm_isdst = -1;
        time_t end = std::mktime(&november);
        ++november.tm_year;
        november.tm_mday = 16;
        november.tm_isdst = -1;
        return {uint32(local.tm_year + 1900), start, end, std::mktime(&november)};
    }
}

#endif
