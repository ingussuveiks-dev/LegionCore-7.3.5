#ifndef LEGIONCORE_PERFUMED_GUARD_ANALYSIS_H
#define LEGIONCORE_PERFUMED_GUARD_ANALYSIS_H

#include "Define.h"

class Creature;
class Player;

namespace PerfumedGuardAnalysis
{
constexpr uint16 Event = 8;
constexpr uint32 HordeQuest = 24536;
constexpr uint32 AllianceQuest = 24655;
constexpr uint32 Analyzer = 50131;
constexpr uint32 Analysis = 70192;
constexpr uint32 Perfumed = 71507;
constexpr uint32 RecentlyAnalyzed = 71508;
constexpr uint32 HordeCredit = 5735;
constexpr uint32 AllianceCredit = 46418;

bool IsCapitalGuard(Creature const& guard);
uint32 ActiveQuest(Player const* player);
bool CanAnalyze(Player const* player, Creature const* guard);
}

#endif
