-- Quest 13545 "Cleansing the Afflicted": Blessed Herb Bundle summons a
-- Spirit of Corruption. The furbolg must become friendly and despawn only
-- after that spirit dies and sends SetData(0, 1) back to its host.
CREATE TABLE IF NOT EXISTS `_backup_20260918_corrupted_blackwood_cleanse_chain` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260918_corrupted_blackwood_cleanse_chain`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` = 33044
  AND `source_type` = 0;

-- Keep the existing summon as the only immediate SpellHit action. Its former
-- self-link incorrectly ran the cleanse result before the summoned spirit died.
UPDATE `smart_scripts`
SET `link` = 0,
    `comment` = 'Corrupted Blackwood - On Blessed Herb Bundle SpellHit - Summon Spirit of Corruption'
WHERE `entryorguid` = 33044
  AND `source_type` = 0
  AND `id` = 1
  AND `link` = 1
  AND `event_type` = 8
  AND `event_param1` = 62092
  AND `action_type` = 12
  AND `action_param1` = 33000;

-- Spirit of Corruption (33000) already sends SetData(0, 1) to the nearest
-- Corrupted Blackwood on death. Reuse the former premature faction action as
-- that completion handler and link the post-kill presentation from it.
UPDATE `smart_scripts`
SET `id` = 2,
    `link` = 3,
    `event_type` = 38,
    `event_phase_mask` = 0,
    `event_chance` = 100,
    `event_flags` = 0,
    `event_param1` = 0,
    `event_param2` = 1,
    `event_param3` = 0,
    `event_param4` = 0,
    `event_param5` = 0,
    `action_type` = 2,
    `action_param1` = 35,
    `action_param2` = 0,
    `action_param3` = 0,
    `action_param4` = 0,
    `action_param5` = 0,
    `action_param6` = 0,
    `target_type` = 1,
    `target_param1` = 0,
    `target_param2` = 0,
    `target_param3` = 0,
    `target_param4` = 0,
    `target_x` = 0,
    `target_y` = 0,
    `target_z` = 0,
    `target_o` = 0,
    `comment` = 'Corrupted Blackwood - On Spirit Death Data Set - Become Friendly'
WHERE `entryorguid` = 33044
  AND `source_type` = 0
  AND `id` = 1
  AND `link` = 2
  AND `event_type` = 8
  AND `event_param1` = 62092
  AND `action_type` = 2
  AND `action_param1` = 35;

-- The former premature manual kill-credit row becomes the thank-you line.
-- Killing Spirit of Corruption itself supplies objective 33000 credit.
UPDATE `smart_scripts`
SET `id` = 3,
    `link` = 4,
    `event_type` = 61,
    `event_phase_mask` = 0,
    `event_chance` = 100,
    `event_flags` = 0,
    `event_param1` = 0,
    `event_param2` = 0,
    `event_param3` = 0,
    `event_param4` = 0,
    `event_param5` = 0,
    `action_type` = 1,
    `action_param1` = 0,
    `action_param2` = 0,
    `action_param3` = 0,
    `action_param4` = 0,
    `action_param5` = 0,
    `action_param6` = 0,
    `target_type` = 1,
    `target_param1` = 0,
    `target_param2` = 0,
    `target_param3` = 0,
    `target_param4` = 0,
    `target_x` = 0,
    `target_y` = 0,
    `target_z` = 0,
    `target_o` = 0,
    `comment` = 'Corrupted Blackwood - Linked - Say Thank You'
WHERE `entryorguid` = 33044
  AND `source_type` = 0
  AND `id` = 1
  AND `link` = 3
  AND `event_type` = 8
  AND `event_param1` = 62092
  AND `action_type` = 33
  AND `action_param1` = 33000;

UPDATE `smart_scripts`
SET `id` = 4,
    `event_flags` = 0,
    `action_param1` = 5500,
    `comment` = 'Corrupted Blackwood - Linked - Despawn After Thank You'
WHERE `entryorguid` = 33044
  AND `source_type` = 0
  AND `id` = 2
  AND `link` = 0
  AND `event_type` = 61
  AND `action_type` = 41;

-- Reset the template faction when the database spawn returns.
INSERT IGNORE INTO `smart_scripts`
(`entryorguid`,`source_type`,`id`,`link`,`Difficulties`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,
 `event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,
 `action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,
 `target_type`,`target_param1`,`target_param2`,`target_param3`,`target_param4`,
 `target_x`,`target_y`,`target_z`,`target_o`,`comment`)
VALUES
(33044,0,5,0,'',11,0,100,0,
 0,0,0,0,0,
 2,2319,0,0,0,0,0,
 1,0,0,0,0,
 0,0,0,0,'Corrupted Blackwood - On Respawn - Restore Hostile Faction');
