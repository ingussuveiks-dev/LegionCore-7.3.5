-- The scoped altar preserves event 9980's original placement, delay and summon lifetime.
UPDATE `gameobject_template` SET `Data2` = 0, `ScriptName` = 'go_aquantion_altar'
WHERE `entry` = 181157 AND `type` = 10 AND `Data0` = 1637 AND `Data1` = 9174 AND `Data2` = 9980
    AND `AIName` = '' AND `ScriptName` = '';

DELETE FROM `event_scripts`
WHERE `id` = 9980 AND `delay` = 2 AND `command` = 10 AND `datalong` = 16292 AND `datalong2` = 900000 AND `dataint` = 0;

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (28226, 'spell_aquantion_medallions');
