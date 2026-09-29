-- Keep Holgar's normal database gossip; add recovery for players who already
-- embarked on an unfinished Battle for the Broken Shore (42740).
UPDATE `creature_template` SET `ScriptName` = 'npc_broken_shore_return'
WHERE `entry` = 4311 AND `ScriptName` IN ('', 'npc_broken_shore_return');

-- Custom service creature 230005 has six existing spawns but no WDB template.
-- The original name/model is absent in the source dump as well. Use the same
-- locally available human model as the neighbouring custom Quest Repair NPC.
INSERT INTO `creature_template_wdb`
    (`Entry`, `Name1`, `Type`, `Displayid1`, `HpMulti`, `PowerMulti`, `VerifiedBuild`)
VALUES (230005, 'Service NPC', 7, 308, 1, 1, 26972)
ON DUPLICATE KEY UPDATE
    `Displayid1` = IF(`Displayid1` = 0 AND `Displayid2` = 0 AND `Displayid3` = 0 AND `Displayid4` = 0,
        VALUES(`Displayid1`), `Displayid1`);
