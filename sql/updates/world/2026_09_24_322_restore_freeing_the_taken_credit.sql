-- World quest 42209 requires four credits for prisoner 108520. The
-- Containment Field gameobject has SmartAI actions to free its nearby
-- prisoners, but SmartGameObjectAI was never enabled and no action awarded
-- the quest credit to the player using it.
INSERT INTO `creature_template`
    (`entry`, `minlevel`, `maxlevel`, `faction`, `WorldEffects`, `PassiveSpells`)
VALUES (108520, 1, 1, 35, '', '')
ON DUPLICATE KEY UPDATE `entry` = VALUES(`entry`);

UPDATE `gameobject_template`
SET `AIName` = 'SmartGameObjectAI'
WHERE `entry` = 251190;

INSERT INTO `smart_scripts`
    (`entryorguid`, `source_type`, `id`, `link`, `event_type`,
     `event_param1`, `event_param2`, `event_param3`, `event_param4`,
     `action_type`, `action_param1`, `target_type`, `comment`)
VALUES
    (251190, 1, 2, 0, 64, 0, 0, 0, 0, 33, 108520, 7,
     'Containment Field - award Freeing the Taken prisoner credit to user')
ON DUPLICATE KEY UPDATE
    `event_type` = VALUES(`event_type`),
    `action_type` = VALUES(`action_type`),
    `action_param1` = VALUES(`action_param1`),
    `target_type` = VALUES(`target_type`),
    `comment` = VALUES(`comment`);
