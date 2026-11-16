-- Promote only together with the executable registering npc_warswords_helya.
-- This actor's native visibility now follows the Helya line in conversation843.
-- Keep shared phase7003: earlier scenario content also depends on it.
SET @warswords_helya_appearance_ok := (
    SELECT COUNT(*) = 1
    FROM `creature` AS c
    INNER JOIN `creature_template` AS t ON t.`entry` = c.`id`
    INNER JOIN `creature_template_wdb` AS w ON w.`Entry` = c.`id`
    WHERE c.`guid` = 361286 AND c.`id` = 99043 AND c.`map` = 1511
      AND c.`PhaseId` = 7003 AND w.`Displayid1` = 66238
      AND t.`AIName` = '' AND t.`ScriptName` IN ('', 'npc_warswords_helya')
);
SET @warswords_helya_appearance_guard := IF(@warswords_helya_appearance_ok = 1,
    'DO 0',
    'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT = ''Warswords Helya appearance: source context differs''');
PREPARE warswords_helya_appearance_check FROM @warswords_helya_appearance_guard;
EXECUTE warswords_helya_appearance_check;
DEALLOCATE PREPARE warswords_helya_appearance_check;

UPDATE `creature_template` SET `ScriptName` = 'npc_warswords_helya'
WHERE @warswords_helya_appearance_ok = 1 AND `entry` = 99043
  AND `AIName` = '' AND `ScriptName` = '';

SET @warswords_helya_appearance_ok := NULL;
SET @warswords_helya_appearance_guard := NULL;
