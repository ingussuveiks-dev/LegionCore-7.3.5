-- A Slip of the Hand (40017) is granted directly while gathering Aethril.
-- Its SourceSpellID accidentally contained the preceding quest ID 40016,
-- rather than a spell. The parallel Legion herbalism rank-3 discovery quests
-- also have no accept-time source spell; the technique is learned on reward.
CREATE TABLE IF NOT EXISTS `_backup_20260918_invalid_quest_source_spell`
LIKE `quest_template_addon`;

REPLACE INTO `_backup_20260918_invalid_quest_source_spell`
SELECT *
FROM `quest_template_addon`
WHERE `ID` = 40017
  AND `SourceSpellID` = 40016;

UPDATE `quest_template_addon`
SET `SourceSpellID` = 0
WHERE `ID` = 40017
  AND `SourceSpellID` = 40016;
