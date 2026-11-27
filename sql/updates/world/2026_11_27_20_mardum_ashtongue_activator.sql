-- Enable ordinary use of the Ashtongue gateway. Keep Lock 93 and personal use.
UPDATE `gameobject_template`
SET `flags` = `flags` & ~32
WHERE `entry` = 241751 AND `type` = 10 AND `Data0` = 93 AND `ScriptName` = 'go_q40378';
