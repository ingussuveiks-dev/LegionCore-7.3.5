START TRANSACTION;

-- The matching WDB record already supplies the original image model and stats.
INSERT INTO `creature_template`
    (`entry`, `minlevel`, `maxlevel`, `faction`, `flags_extra`, `WorldEffects`, `PassiveSpells`, `ScriptName`)
VALUES (115543, 110, 110, 14, 64, '', '', 'npc_mage_tower_xylem_mirror')
ON DUPLICATE KEY UPDATE `ScriptName` = VALUES(`ScriptName`);

-- Use the native visual and duration with an arena-specific template.
INSERT INTO `areatrigger_template`
    (`entry`, `spellId`, `customEntry`, `Radius`, `RadiusTarget`, `HasDynamicShape`, `comment`)
VALUES (0, 231982, 90231982, 8, 0.8, 1, 'Closing the Eye - Arcane Barrier of Replenishment')
ON DUPLICATE KEY UPDATE `comment` = VALUES(`comment`);

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
    (231914, 'spell_mage_tower_xylem_intermission_damage'),
    (231980, 'spell_mage_tower_xylem_time_acceleration'),
    (232006, 'spell_mage_tower_xylem_darkness'),
    (232007, 'spell_mage_tower_xylem_intermission_damage'),
    (234728, 'spell_mage_tower_xylem_intermission_damage')
ON DUPLICATE KEY UPDATE `ScriptName` = VALUES(`ScriptName`);

-- The scripted controller sends these signals only after an actual transition.
UPDATE `smart_scripts`
SET `event_type` = 38, `event_phase_mask` = 0, `event_flags` = 0,
    `event_param1` = 2401, `event_param2` = 2,
    `comment` = 'Xylem - On reveal after Frost - Use Arcane phase'
WHERE `entryorguid` = 115244 AND `source_type` = 0 AND `id` = 11 AND `action_type` = 22 AND `action_param1` = 2;

INSERT INTO `smart_scripts`
    (`entryorguid`, `source_type`, `id`, `event_type`, `event_chance`,
     `event_param1`, `event_param2`, `action_type`, `action_param1`, `target_type`, `comment`)
VALUES
    (115244, 0, 33, 38, 100, 2401, 3, 22, 3, 1, 'Xylem - On Arcane Annihilation - Pause combat spells'),
    (115244, 0, 34, 38, 100, 2401, 4, 22, 4, 1, 'Xylem - On reveal after Arcane - Return to Frost phase')
ON DUPLICATE KEY UPDATE `comment` = VALUES(`comment`);

-- The returning Frost phase keeps the Frost rotation and gains Shadow Barrage.
UPDATE `smart_scripts` SET `event_phase_mask` = 9
WHERE `entryorguid` = 115244 AND `source_type` = 0 AND `id` IN (8, 9, 10)
AND `event_phase_mask` = 1;
UPDATE `smart_scripts` SET `event_phase_mask` = 10
WHERE `entryorguid` = 115244 AND `source_type` = 0 AND `id` = 13 AND `action_param1` = 231443
AND `event_phase_mask` = 2;

COMMIT;
