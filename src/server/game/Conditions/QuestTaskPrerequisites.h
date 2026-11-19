#ifndef LEGION_QUEST_TASK_PREREQUISITES_H
#define LEGION_QUEST_TASK_PREREQUISITES_H

#include "Define.h"

class Player;
struct QuestV2CliTaskEntry;

// Evaluate the matching client row's completed-quest expression. Ordinary
// task admission (level, condition, rotation and storage) remains at the caller.
TC_GAME_API bool MeetsQuestTaskPrerequisites(Player const& player, QuestV2CliTaskEntry const& task);

#endif
