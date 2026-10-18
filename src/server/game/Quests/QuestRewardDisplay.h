#ifndef QUEST_REWARD_DISPLAY_H
#define QUEST_REWARD_DISPLAY_H

#include "SpellInfo.h"

// Some native reward labels describe a quest-bit unlock. Their targetless
// LEARN_SPELL has no child spell; it is client presentation, not an action.
inline bool IsQuestRewardDisplayOnly(SpellInfo const* spell)
{
    if (!spell) return false;
    bool label = false;
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        if (!(spell->EffectMask & (uint32(1) << i))) continue;
        auto effect = spell->Effects[i];
        if (!effect || !effect->Effect) continue;
        if (effect->TargetA.GetTarget() || effect->TargetB.GetTarget() || effect->TriggerSpell) return false;
        if (effect->Effect == SPELL_EFFECT_LEARN_SPELL) label = true;
        else if (effect->Effect != SPELL_EFFECT_DUMMY) return false;
    }
    return label;
}

#endif
