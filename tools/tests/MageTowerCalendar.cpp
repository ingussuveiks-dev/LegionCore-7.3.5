#include "../../src/server/game/Calendar/MageTowerCalendar.h"
#include <cassert>
#include <utility>
#include <vector>

int main()
{
    using Window = std::pair<time_t, time_t>;
    using Windows = std::vector<Window>;
    auto windows = [](time_t start, time_t end, unsigned period, unsigned duration, time_t first, time_t last)
    {
        Windows result;
        MageTowerCalendar::VisitWindows(start, end, period, duration, first, last,
            [&](time_t open, time_t close) { result.emplace_back(open, close); });
        return result;
    };

    // Real event 400 spans the month boundary and repeats six elapsed days later.
    constexpr time_t epoch = 1536649260, end = 2067267600;
    constexpr time_t open = 1795849260, close = 1796108400, next = 1796367660;
    assert((windows(epoch, end, 8640, 4319, open, next) ==
        Windows{{open, close}, {next, next + 4319 * 60}}));
    assert((windows(epoch, end, 8640, 4319, close, close) == Windows{{open, close}}));
    assert(windows(epoch, end, 8640, 4319, close + 1, next - 1).empty());
    // The October interval crosses Riga's daylight-saving change.
    assert((windows(epoch, end, 8640, 4319, 1792738860, 1792998000) ==
        Windows{{1792738860, 1792998000}}));
    assert((windows(open, close - 60, 8640, 4319, open, next) == Windows{{open, close - 60}}));
    assert((windows(open, next, 10, 10, open, next) == Windows{{open, next}}));
    assert(windows(epoch, end, 0, 10, open, next).empty());
    assert(windows(epoch, end, 10, 0, open, next).empty());
    assert(windows(epoch, end, 8640, 4319, end + 1, end + 60).empty());
}
