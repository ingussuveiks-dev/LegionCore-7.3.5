-- SpellArea::IsFitToRequirements treats quest_end_status as an ALLOWED mask.
-- 74 admitted active/completed/rewarded The Sixth, excluding new warlocks.
-- Native spell 204860 summons the personal questgiver 103506 and a 5-min aura.
UPDATE `spell_area` SET `quest_end_status`=1
WHERE `spell`=204860 AND `area`=7502 AND `quest_start`=0
  AND `quest_end`=40716 AND `classmask`=256 AND `quest_end_status`=74;

-- Move Like No Other's event objects were migrated to Smart source type 13,
-- but their quest/objective conditions still used source type 10. Thus even
-- warlocks without the quest triggered Lan'dalock's Spectral Sight dialogue.
INSERT IGNORE INTO `conditions`
(`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,
 `ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`ConditionValue2`,
 `ConditionValue3`,`NegativeCondition`,`ErrorTextId`,`ScriptName`,`Comment`)
SELECT `SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,13,`ElseGroup`,
 `ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`ConditionValue2`,
 `ConditionValue3`,`NegativeCondition`,`ErrorTextId`,`ScriptName`,`Comment`
FROM `conditions`
WHERE `SourceTypeOrReferenceId`=22 AND `SourceGroup`=1 AND `SourceId`=10
  AND `SourceEntry` BETWEEN 136 AND 140 AND `ConditionValue1`=42594
  AND `ConditionTypeOrReference` IN (9,41);
DELETE FROM `conditions`
WHERE `SourceTypeOrReferenceId`=22 AND `SourceGroup`=1 AND `SourceId`=10
  AND `SourceEntry` BETWEEN 136 AND 140 AND `ConditionValue1`=42594
  AND `ConditionTypeOrReference` IN (9,41);

-- Ritssyn's greeting incorrectly used Akazamzarak's coin solicitation.
-- Use the existing 7.3.5 BroadcastText line/voice; preserve custom replacements.
UPDATE `creature_text`
SET `Text`='A moment of your time, warlock. The Council of the Black Harvest requests your assistance.',
    `BroadcastTextID`=106111, `Sound`=71324
WHERE `CreatureID`=103506 AND `GroupID`=0 AND `ID`=0 AND `BroadcastTextID`=105637;

-- The existing quest-accept SmartAI action references absent text group 1.
INSERT INTO `creature_text`
(`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Sound`,`BroadcastTextID`,`comment`)
SELECT 103506,1,0,
 'Join us at the Circle of Wills in Dalaran\'s underbelly, warlock. My portal will take you there directly. We will be waiting.',
 12,0,100,63919,104179,'Ritssyn - The Sixth accepted (native BroadcastText)'
WHERE NOT EXISTS (SELECT 1 FROM `creature_text` WHERE `CreatureID`=103506 AND `GroupID`=1);
