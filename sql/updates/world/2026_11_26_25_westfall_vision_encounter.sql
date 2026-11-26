-- The matching 26972 summon uses spell 79587 to board the original camera vehicle 916.
UPDATE `creature_template`
SET `AIName` = '', `ScriptName` = 'npc_westfall_vision_camera', `VehicleId` = 916
WHERE `entry` = 42693 AND `AIName` = 'SmartAI' AND `ScriptName` = '' AND `VehicleId` = 0;

DELETE FROM `smart_scripts`
WHERE `entryorguid` = 42693 AND `source_type` = 0 AND `id` = 0 AND `event_type` = 54
  AND `action_type` = 15 AND `action_param1` = 26320 AND `target_type` = 23;

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`)
VALUES (79586, 'spell_westfall_vision_incense');

INSERT INTO `creature_text`
(`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `BroadcastTextID`, `comment`)
VALUES
(42699, 10, 0, '', 14, 0, 100, 22, 0, 0, 42593, 'Vision: Alliance warrior challenges Edwin'),
(42699, 11, 0, '', 14, 0, 100, 397, 0, 0, 42594, 'Vision: Alliance warrior starts the encounter'),
(42699, 12, 0, '', 14, 0, 100, 5, 0, 0, 42602, 'Vision: Alliance warrior announces the victory'),
(42699, 13, 0, '', 12, 0, 100, 396, 0, 0, 42603, 'Vision: Alliance warrior announces the return'),
(42697, 10, 0, '', 14, 0, 100, 0, 0, 5780, 6061, 'Vision: Edwin answers the challenge'),
(42697, 11, 0, '', 14, 0, 100, 0, 0, 5782, 6063, 'Vision: Edwin speaks after losing one quarter of his health'),
(42697, 12, 0, '', 14, 0, 100, 0, 0, 5783, 6064, 'Vision: Edwin speaks at half health'),
(42697, 13, 0, '', 14, 0, 100, 0, 0, 5784, 6065, 'Vision: Edwin speaks at his last quarter of health'),
(42371, 10, 0, '', 12, 0, 100, 0, 0, 0, 38143, 'Vision: Vanessa reaches her father');
