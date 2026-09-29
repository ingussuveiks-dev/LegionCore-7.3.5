-- After Allari reveals the demons, phase 7531 ends and phase 7422 contains
-- the attackers and Sylvanas. Holgar's introduction skip can leave quest
-- 44663 active when a player returns to finish the introduction, which made
-- phase 7422 fail its QUEST_NONE 44663 condition and hide all attackers.
-- Keep the existing Allari objective (40607 / 112731) requirement and allow
-- this phase until Demons Among Us is rewarded, regardless of quest 44663.
-- Preserve the other phase branches, including the demon hunter intro.
UPDATE `conditions`
SET `ConditionTypeOrReference` = 8,
    `ConditionValue1` = 40607,
    `NegativeCondition` = 1,
    `Comment` = 'Legion Start Questline - Not QUEST_REWARDED Demons Among Us after speaking to Allari'
WHERE `SourceTypeOrReferenceId` = 23 AND `SourceGroup` = 14
    AND `SourceEntry` = 1 AND `SourceId` = 0 AND `ElseGroup` = 3
    AND `ConditionTypeOrReference` = 14 AND `ConditionTarget` = 0
    AND `ConditionValue1` = 44663 AND `ConditionValue2` = 0
    AND `ConditionValue3` = 0 AND `NegativeCondition` = 0;
