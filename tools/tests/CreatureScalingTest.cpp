#include "../../src/server/game/Entities/Creature/CreatureScaling.h"

using CreatureScaling::HasValidLevelRange;
static_assert(!HasValidLevelRange(0, 0), "Duration-only rows must use normal template health");
static_assert(!HasValidLevelRange(0, 100), "Level zero cannot be a scaling endpoint");
static_assert(!HasValidLevelRange(110, 100), "Reject reversed scaling ranges");
static_assert(!HasValidLevelRange(100, 256), "Target level must fit uint8");
static_assert(HasValidLevelRange(100, 100), "Fixed level scaling is supported");
static_assert(HasValidLevelRange(100, 110), "Legion level scaling is preserved");
static_assert(HasValidLevelRange(1, 255), "Valid boundary levels are supported");

int main() { return 0; }
