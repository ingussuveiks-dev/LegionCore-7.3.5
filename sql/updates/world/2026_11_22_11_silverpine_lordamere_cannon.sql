-- Keep the three existing dock placements. Native Rocket Blast uses a
-- vehicle action and must not inherit the unrelated Demolisher bar.
UPDATE `creature_template`
SET `npcflag` = `npcflag` | 16777216, `VehicleId` = 1094, `spell1` = 84575, `ScriptName` = 'npc_lordamere_horde_cannon'
WHERE `entry` = 45263;

UPDATE `creature_template`
SET `ScriptName` = 'npc_lordamere_hillsbrad_worgen'
WHERE `entry` = 45270;

UPDATE `npc_spellclick_spells`
SET `spell_id` = 46598
WHERE `npc_entry` = 45263 AND `spell_id` = 84549 AND `cast_flags` = 1 AND `user_type` = 0;

INSERT INTO `conditions`
    (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`,
     `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorTextId`, `ScriptName`, `Comment`)
SELECT 18, 45263, 46598, 0, 0, 9, 0, 27232, 0, 0, 0, 0, '', 'Horde Cannon: The Waters Run Red must be in progress'
WHERE NOT EXISTS (
    SELECT 1 FROM `conditions` WHERE `SourceTypeOrReferenceId` = 18 AND `SourceGroup` = 45263 AND `SourceEntry` = 46598
      AND `SourceId` = 0 AND `ElseGroup` = 0 AND `ConditionTypeOrReference` = 9 AND `ConditionTarget` = 0
      AND `ConditionValue1` = 27232 AND `ConditionValue2` = 0 AND `ConditionValue3` = 0 AND `NegativeCondition` = 0
);

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
    (84574, 'spell_lordamere_rocket_blast'),
    (84575, 'spell_lordamere_rocket_blast')
ON DUPLICATE KEY UPDATE `ScriptName` = VALUES(`ScriptName`);
