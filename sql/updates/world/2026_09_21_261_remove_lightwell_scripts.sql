-- Lightwell was removed from priests in patch 7.0.3. These legacy scripts depend
-- on spell records (59907 and 126150) that are not present in the 7.3.5 client.
UPDATE `creature_template`
SET `ScriptName` = ''
WHERE `ScriptName` IN ('npc_lightwell', 'npc_lightwell_mop');
