-- Reconnect the original Iceflow Lake actors and the Orbital Targeting Device.
START TRANSACTION;

UPDATE `creature_template`
SET `ScriptName` = 'npc_iceflow_mekkatorque', `AIName` = ''
WHERE `entry` = 42849 AND `AIName` = 'SmartAI' AND `ScriptName` = '';

DELETE FROM `smart_scripts`
WHERE `entryorguid` = 42849 AND `source_type` = 0 AND `id` = 0
  AND `event_type` = 62 AND `event_param1` = 11662 AND `event_param2` = 0
  AND `action_type` = 33 AND `action_param1` = 42849;

UPDATE `creature_template` SET `ScriptName` = 'npc_iceflow_crushcog_enemy'
WHERE `entry` IN (42294, 42839) AND `AIName` = '' AND `ScriptName` = '';

-- 7.3.5 Vehicle 925 has the original pilot seat 8136. The existing 42494
-- accessory and 46598 boarding remain in use.
UPDATE `creature_template` SET `VehicleId` = 925 WHERE `entry` = 42839 AND `VehicleId` = 0;

UPDATE `creature_template` SET `ScriptName` = 'npc_iceflow_orbital_marker'
WHERE `entry` = 42929 AND `AIName` = '' AND `ScriptName` = '';

INSERT INTO `conditions`
(`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`,
 `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorTextId`, `ScriptName`, `Comment`)
VALUES
(15, 11662, 0, 0, 0, 9, 0, 26364, 0, 0, 0, 0, '', 'Iceflow Lake assault option while Down with Crushcog is incomplete'),
(13, 1, 79931, 0, 0, 9, 0, 26364, 0, 0, 0, 0, '', 'Razlo victory spell targets players with the active quest')
ON DUPLICATE KEY UPDATE `ConditionValue3` = VALUES(`ConditionValue3`);

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(80101, 'spell_iceflow_orbital_target'),
(80098, 'spell_iceflow_crushcog_weapons'),
(80148, 'spell_iceflow_crushcog_weapons'),
(80110, 'spell_iceflow_crushcog_weapons'),
(80220, 'spell_iceflow_crushcog_weapons'),
(79931, 'spell_iceflow_crushcog_victory')
ON DUPLICATE KEY UPDATE `spell_id` = VALUES(`spell_id`);

INSERT INTO `creature_text`
(`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `BroadcastTextID`, `comment`)
VALUES
(42849, 0, 0, '', 12, 0, 100, 5, 0, 20890, 42758, 'Mekkatorque - situation'),
(42849, 1, 0, '', 12, 0, 100, 25, 0, 20891, 42759, 'Mekkatorque - the player and the dwarves'),
(42849, 2, 0, '', 12, 0, 100, 5, 0, 20892, 42760, 'Mekkatorque - rally'),
(42849, 3, 0, '', 12, 0, 100, 5, 0, 20893, 42763, 'Mekkatorque - victory'),
(42849, 4, 0, '', 12, 0, 100, 5, 0, 20894, 42765, 'Mekkatorque - address Thermaplugg'),
(42849, 5, 0, '', 14, 0, 100, 0, 0, 0, 42829, 'Mekkatorque - cannon'),
(42849, 6, 0, '', 14, 0, 100, 0, 0, 0, 42832, 'Mekkatorque - ray'),
(42849, 7, 0, '', 14, 0, 100, 0, 0, 0, 42833, 'Mekkatorque - missiles'),
(42852, 0, 0, '', 12, 0, 100, 25, 0, 0, 42767, 'Stonegrind - advance'),
(42852, 1, 0, '', 12, 0, 100, 25, 0, 0, 42766, 'Stonegrind - victory'),
(42839, 0, 0, '', 12, 0, 100, 0, 0, 21243, 42761, 'Razlo - surprised'),
(42839, 1, 0, '', 12, 0, 100, 0, 0, 21244, 42762, 'Razlo - to arms'),
(42839, 2, 0, '', 12, 0, 100, 0, 0, 21245, 42781, 'Razlo - defeat')
ON DUPLICATE KEY UPDATE `ID` = VALUES(`ID`);

COMMIT;
