-- Restore Legion 7.3.5 Guardian talent proc restrictions.

-- Earthwarden is granted only by the direct damage of Guardian Thrash.  The
-- existing check is attached to the resulting absorb aura (203975), so it
-- does not restrict the proc of the talent aura (203974).
DELETE FROM `spell_proc_check`
WHERE `entry` IN (203974, 203975) AND `checkspell` = 77758 AND `effectmask` = 1;

INSERT INTO `spell_proc_check`
(`entry`, `entry2`, `entry3`, `checkspell`, `hastalent`, `chance`, `target`,
 `effectmask`, `powertype`, `dmgclass`, `specId`, `spellAttr0`,
 `targetTypeMask`, `mechanicMask`, `fromlevel`, `perchp`, `spelltypeMask`,
 `combopoints`, `deathstateMask`, `hasDuration`, `comment`)
VALUES
(203974, 0, 0, 77758, 0, 0, 0, 1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 'Earthwarden: only direct Guardian Thrash damage grants a charge');
