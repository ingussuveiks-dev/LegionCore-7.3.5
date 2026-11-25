-- Keep the placed bell's native type 22 and player-cast spell 102843.
UPDATE `gameobject_template` SET `ScriptName` = 'go_wandering_isle_training_bell'
WHERE `entry` = 209608 AND `type` = 22 AND `Data0` = 102843 AND `ScriptName` = '';
