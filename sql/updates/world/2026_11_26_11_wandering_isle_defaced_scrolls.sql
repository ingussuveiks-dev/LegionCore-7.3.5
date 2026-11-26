-- Forward report-use to the existing defaced scroll goober animation and spell.
UPDATE `gameobject_template`
SET `ScriptName` = 'go_wandering_isle_defaced_scroll'
WHERE `entry` IN (209656, 209660, 209661, 209663) AND `type` = 10 AND `Data1` = 29778 AND `Data10` = 104127
  AND `AIName` = '' AND `ScriptName` = '';
