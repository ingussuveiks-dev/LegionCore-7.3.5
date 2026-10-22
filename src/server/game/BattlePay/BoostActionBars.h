#ifndef BOOST_ACTION_BARS_H
#define BOOST_ACTION_BARS_H

#include <algorithm>
#include <cstdint>
#include <set>
#include <utility>
#include <vector>

// CUSTOM Legion starter layouts, not an automatic rotation. Optional talents
// and replacement spells are included only if active and visible on this player.
namespace BoostActionBars
{
inline std::vector<uint32_t> Priority(uint32_t spec)
{
    switch (spec)
    {
        case 71: return {167105,12294,163201,772,7384,1464,1680,845,1719,227847,209577,107574};
        case 72: return {23881,85288,184367,100130,5308,190411,1719,184364,205545,118000};
        case 73: return {23922,6572,6343,20243,2565,190456,1160,12975,871,6552};
        case 65: return {20473,82326,19750,183998,53563,223306,85222,35395,20271,31842,633,498};
        case 66: return {31935,53595,20271,53600,26573,184092,31850,86659,96231,853};
        case 70: return {184575,20271,35395,85256,53385,205273,231895,31884,215661,210191};
        case 253: return {34026,217200,120679,193455,131894,2643,19574,193530,207068,201430,109248};
        case 254: return {185358,19434,185901,2643,204147,194386,131894,193526,198670};
        case 255: return {190928,186270,185855,187708,194277,212436,191433,201078,186289,203415};
        case 259: return {703,1329,1943,32645,192759,79140,5171,1856,1766,51723};
        case 260: return {193315,2098,199804,193316,185763,202665,51690,13750,1856,1766,2094};
        case 261: return {185438,53,196819,195452,197835,212283,185313,5171,1856,1766};
        case 256: return {17,194509,186263,47540,585,589,204197,129250,214621,62618,33206,47536};
        case 257: return {2050,2061,2060,139,33076,34861,596,88625,585,14914,64843,47788};
        case 258: return {34914,589,8092,15407,32379,228260,205351,205385,10060,47585,15487};
        case 250: return {195182,206930,49998,50842,43265,195292,49028,55233,48792,48707,47528,56222};
        case 251: return {49184,49020,49143,196770,51271,152279,194913,207256,43265,47528,48792,48707};
        case 252: return {77575,85948,55090,207311,47541,63560,42650,49206,43265,47528,48792,48707};
        case 262: return {188389,51505,188196,8042,61882,188443,114050,192249,191634,16166,198067};
        case 263: return {17364,60103,187874,193786,193796,196834,197211,51533,187837};
        case 264: return {61295,77472,8004,1064,73920,73685,5394,108280,98008,79206,188196,188389};
        case 62: return {30451,5143,44425,1449,114923,153626,12042,12051,1953,2139,110959,45438};
        case 63: return {133,108853,11366,2948,2120,44457,153561,190319,31661,122,2139,45438};
        case 64: return {116,30455,44614,84714,190356,12472,153595,205021,122,2139,11426,45438};
        case 265: return {980,172,30108,198590,27243,48181,205179,205180,688,697,712,691};
        case 266: return {686,104316,105174,193440,157695,193396,211714,196277,1122,18540};
        case 267: return {348,17962,29722,116858,80240,5740,17877,196447,196448,1122,18540};
        case 268: return {121253,100780,100784,115181,205523,115308,119582,115399,115203,122278,116705};
        case 269: return {100780,100784,107428,113656,101546,152175,115080,137639,152173,123904,116705};
        case 270: return {115151,116670,124682,191837,115310,116680,115175,115450,115313,100780,100784,107428};
        case 102: return {8921,93402,190984,194153,78674,191034,202767,202768,202771,194223,202425,78675};
        case 103: return {5217,1822,5221,1079,22568,106785,202028,106830,106951,210722,22570,106839};
        case 104: return {33917,77758,8921,213771,192081,22842,6807,80313,61336,22812,99,6795};
        case 105: return {774,33763,8936,18562,48438,145205,5185,102342,740,22812,8921,5176};
        case 577: return {162243,232893,162794,188499,198013,185123,195072,198793,191427,179057,183752,198589,196718};
        case 581: return {203782,209795,228477,204596,207407,203720,204021,187827,202137,183752,189110};
        default: return {};
    }
}

inline std::vector<uint32_t> Utility(uint32_t classId)
{
    switch (classId)
    {
        case 1: return {6552,100,6544,18499,23920,5246,118038,97462,34428};
        case 2: return {96231,853,1044,633,642,1022,19750,190784,20066,4987,7328};
        case 3: return {147362,109304,186265,781,186257,5384,34477,187650,187698,5116,109248,136,982,883,2641,6991,1515,1543};
        case 4: return {1766,1856,5277,31224,1966,185311,2983,36554,195457,408,1833,6770,921,1784};
        case 5: return {586,73325,528,527,32375,8122,19236,15487,1706,2006};
        case 6: return {47528,49576,48707,48792,48265,61999,46584,56222,108194,108199};
        case 7: return {57994,108271,2645,51514,51886,77130,546,2008,192058,2484};
        case 8: return {2139,1953,212653,45438,122,31687,130,190336,80353};
        case 9: return {19647,119910,6789,30283,104773,108416,111400,20707,6201,698,48018,48020,111771,688,697,712,691,1122,18540};
        case 10: return {116705,115078,119381,109132,115008,101643,115203,122278,116849,115546,115178};
        case 11: return {22812,61336,1850,106898,20484,2782,88423,339,102401,5215,768,5487,783};
        case 12: return {183752,185245,195072,189110,179057,217832,188501};
        default: return {};
    }
}

struct Action
{
    uint8_t button;
    uint64_t action;
    uint8_t type;
};

// Plan completely before mutating: an overfull bar must retain every old action.
// The six ordinary pages are separate from druid/rogue stance pages 72..131.
template<class Eligible>
bool Build(uint32_t spec, uint32_t classId, std::vector<uint32_t> known,
    std::vector<Action> const& old, Eligible eligible, std::vector<Action>& result)
{
    auto priority = Priority(spec);
    if (priority.empty())
        return false;
    auto utility = Utility(classId);
    priority.insert(priority.end(), utility.begin(), utility.end());
    std::sort(known.begin(), known.end());
    priority.insert(priority.end(), known.begin(), known.end());
    std::set<uint32_t> seen;
    std::vector<Action> planned;
    auto slot = [](size_t index) -> uint8_t
    {
        static uint8_t const pages[] = {0,60,48,24,36,12};
        return uint8_t(pages[index / 12] + index % 12);
    };
    for (uint32_t spell : priority)
        if (eligible(spell) && seen.insert(spell).second)
        {
            if (planned.size() == 72)
                return false;
            planned.push_back({slot(planned.size()), spell, 0});
        }
    for (auto const& action : old)
        if (action.button < 72 && action.type != 0)
        {
            if (planned.size() == 72)
                return false;
            planned.push_back({slot(planned.size()), action.action, action.type});
        }
    if (planned.empty())
        return false;
    auto formPage = [&](uint8_t page, uint32_t formSpec)
    {
        // Leave even a partially customized stance page intact.
        for (auto const& action : old)
            if (action.button >= page && action.button < page + 12)
                return;
        std::set<uint32_t> formSeen;
        uint8_t index = 0;
        for (auto spell : Priority(formSpec))
            if (eligible(spell) && formSeen.insert(spell).second)
            {
                planned.push_back({uint8_t(page + index), spell, 0});
                if (++index == 12)
                    break;
            }
    };
    // Native SpellShapeshiftForm BonusActionBar: Cat/Stealth=1,
    // Shadow Dance=2, Bear=3, Moonkin=4; bonus page 1 starts at slot 72.
    if (classId == 11)
    {
        if (eligible(768))
        {
            formPage(72, 103);
            formPage(84, 103); // prowling cat page
        }
        if (eligible(5487))
            formPage(96, 104);
        if (eligible(24858))
            formPage(108, 102);
    }
    if (classId == 4)
    {
        if (eligible(1784))
            formPage(72, spec);
        if (eligible(185313))
            formPage(84, spec);
    }
    result = std::move(planned);
    return true;
}
}
#endif
