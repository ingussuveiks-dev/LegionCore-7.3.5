-- The same T16 rule already contains Legion Holy Light (82326).  Remove the
-- duplicate condition for retired Holy Light spell 635.
DELETE FROM `spell_proc_check`
WHERE `entry` = 144624 AND `checkspell` = 635;
