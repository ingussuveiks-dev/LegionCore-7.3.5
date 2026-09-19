-- Restore missing Legion 7.3.5 Fire Mage mechanics from build 26972 data.

DELETE FROM `spell_script_names`
WHERE `ScriptName` = 'spell_mage_cinderstorm';

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(198928, 'spell_mage_cinderstorm');

-- Cauterizing Blink: Blink/Shimmer procs the six-second maximum-health heal.
DELETE FROM `spell_trigger`
WHERE `spell_id` = 194318 AND `spell_trigger` = 194316;

INSERT INTO `spell_trigger`
(`spell_id`, `spell_trigger`, `spell_cooldown`, `option`, `target`, `caster`, `targetaura`, `targetaura2`,
 `bp0`, `bp1`, `bp2`, `effectmask`, `aura`, `aura2`, `chance`, `group`, `procFlags`, `procEx`,
 `check_spell_id`, `addptype`, `schoolMask`, `dummyId`, `dummyEffect`, `CreatureType`, `slot`, `randList`, `comment`)
VALUES
(194318, 194316, 0, 20, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
 0, -1, 0, 0, 0, 0, 0, '', 'Mage - Cauterizing Blink heal');

-- Phoenix Reborn already casts its damage spell through the original DB row.
-- Its second effect reduces the active Phoenix's Flames charge-recovery timer by 10 seconds.
DELETE FROM `spell_trigger`
WHERE `spell_id` = 215773 AND `spell_trigger` = 194466 AND `option` = 46;

INSERT INTO `spell_trigger`
(`spell_id`, `spell_trigger`, `spell_cooldown`, `option`, `target`, `caster`, `targetaura`, `targetaura2`,
 `bp0`, `bp1`, `bp2`, `effectmask`, `aura`, `aura2`, `chance`, `group`, `procFlags`, `procEx`,
 `check_spell_id`, `addptype`, `schoolMask`, `dummyId`, `dummyEffect`, `CreatureType`, `slot`, `randList`, `comment`)
VALUES
(215773, 194466, 0, 46, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
 0, -1, 0, 0, 0, 0, 0, '', 'Mage - Phoenix Reborn reduces Phoenix Flames charge cooldown');
