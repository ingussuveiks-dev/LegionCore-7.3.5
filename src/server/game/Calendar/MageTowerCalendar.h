#pragma once

#include <algorithm>
#include <ctime>

namespace MageTowerCalendar
{
    // The game-event manager measures both the period and duration in elapsed
    // minutes. Convert to local calendar time only when serializing a marker.
    template<class Visitor>
    void VisitWindows(time_t start, time_t end, unsigned periodMinutes, unsigned durationMinutes,
        time_t first, time_t last, Visitor visit)
    {
        if (!periodMinutes || !durationMinutes || end <= start || last < first || end < first || start > last)
            return;

        if (durationMinutes >= periodMinutes)
        {
            visit(start, end);
            return;
        }

        time_t const period = time_t(periodMinutes) * 60;
        time_t const duration = time_t(durationMinutes) * 60;
        // Include the preceding cycle if its closing marker falls on the first date.
        time_t const offset = std::max<time_t>(0, first - start - duration);
        for (time_t open = start + offset / period * period; open < end && open <= last; open += period)
        {
            time_t const close = std::min(end, open + duration);
            if (close >= first)
                visit(open, close);
        }
    }
}
