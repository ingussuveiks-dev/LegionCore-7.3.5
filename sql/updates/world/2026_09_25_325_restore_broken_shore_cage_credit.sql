-- Both Legion Cage variants in the Broken Shore intro use the same model and
-- contain Argent prisoners. The second variant lacked the scenario script.
UPDATE `gameobject_template`
SET `ScriptName` = 'go_240535'
WHERE `entry` = 248819 AND `ScriptName` = '';
