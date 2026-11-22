START TRANSACTION;

-- quest_end_status is the set that keeps the aura. Retain it while the next milestone is absent, incomplete or failed.
UPDATE `spell_area` SET `quest_end_status`=41
WHERE `area`=4720 AND `quest_end_status`=66
  AND ((`spell`=72676 AND `quest_start`=24929 AND `quest_end`=24958)
    OR (`spell`=73065 AND `quest_start`=24958 AND `quest_end`=25184));

-- The native reward's bind spell targets the cave's pre-flight checkpoint.
INSERT IGNORE INTO `spell_target_position`
    (`id`,`target_map`,`target_position_x`,`target_position_y`,`target_position_z`,`target_orientation`)
VALUES (74103,648,1176.66,1090.84,118.859,0);

-- Keep the actual encounter under its existing spawn identity. Native phase 182 contains the living boss.
UPDATE `creature`
SET `areaId`=4886, `phaseMask`=8192, `position_x`=1217.207, `position_y`=1148.594,
    `position_z`=117.9233, `orientation`=0.3141593
WHERE `guid`=242274 AND `id`=38855 AND `map`=648 AND `phaseMask`=24576;

-- The phase 183 capture contains a smoking, permanently feigning corpse at the original handoff point.
INSERT IGNORE INTO `creature`
    (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnMask`,`phaseMask`,`position_x`,`position_y`,`position_z`,
     `orientation`,`spawntimesecs`,`unit_flags`)
VALUES (3300110,38855,648,4720,4911,1,16384,1216.54,1148.701,117.9631,0.3490658,120,33555202);

INSERT IGNORE INTO `creature_addon` (`guid`,`bytes2`,`auras`)
VALUES (3300110,1,'65734 29266');

UPDATE `smart_scripts` SET `entryorguid`=-242274
WHERE `entryorguid`=38855 AND `source_type`=0 AND `id` IN (0,1) AND `action_type`=11 AND `action_param1`=73097;

-- The native 38855 template already grants 38868 to eligible death recipients, including a pet's owner.
DELETE FROM `smart_scripts`
WHERE `entryorguid`=38855 AND `source_type`=0 AND `id`=2 AND `event_type`=6 AND `action_type`=11 AND `action_param1`=73060;

COMMIT;
