-- The installed Banner Illidari begins ready for its native Lock 93 interaction.
-- Keep the object's personal-use flag and all other interaction data.
UPDATE `gameobject_template`
SET `flags` = `flags` & ~1
WHERE `entry` = 244898 AND `type` = 10 AND `Data0` = 93 AND `ScriptName` = 'go_q40077';
