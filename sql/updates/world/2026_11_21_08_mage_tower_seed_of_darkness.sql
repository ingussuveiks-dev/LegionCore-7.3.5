START TRANSACTION;

UPDATE `creature_template`
SET `AIName` = '', `ScriptName` = 'npc_mage_tower_darkness_within'
WHERE `entry` = 116840 AND `AIName` = 'SmartAI' AND `ScriptName` = '';

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
    (233248, 'spell_mage_tower_seed_target'),
    (233248, 'spell_mage_tower_seed_expiry'),
    (232991, 'spell_mage_tower_merge_darkness')
ON DUPLICATE KEY UPDATE `ScriptName` = VALUES(`ScriptName`);

-- Preserve the native Seed schedule. Its real player application and expiry
-- drive the boss aura and the three original summon casts.
UPDATE `smart_scripts` SET `link` = 0
WHERE `entryorguid` = 116839 AND `source_type` = 0 AND `id` = 2
AND `link` = 3 AND `action_type` = 11 AND `action_param1` = 233248;
DELETE FROM `smart_scripts`
WHERE `entryorguid` = 116839 AND `source_type` = 0
AND ((`id` = 3 AND `action_type` = 11 AND `action_param1` = 233164)
  OR (`id` = 4 AND `action_type` = 80 AND `action_param1` = 116839));
DELETE FROM `smart_scripts`
WHERE `entryorguid` = 116839 AND `source_type` = 9 AND `id` IN (0, 1, 2)
AND `action_type` = 86 AND `action_param1` = 233165;

COMMIT;
