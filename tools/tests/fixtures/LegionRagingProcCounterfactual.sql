-- Counterfactual TEMP-table data only; never install in the World database.
-- Data-only proc repair still sees pre-hit health/scaling and is insufficient.
-- Matching Legion7.3.5 Might206150 has14 effects and native proc flag4.
-- Raging effect2 must react to damage TAKEN; all other effects retain the
-- native proc flags, including Necrotic effect8 on a melee attack DONE.
DELETE FROM `spell_proc_event` WHERE `entry`=206150;
INSERT INTO `spell_proc_event`
(`entry`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,
 `SpellFamilyMask2`,`SpellFamilyMask3`,`procFlags`,`procEx`,`ppmRate`,
 `CustomChance`,`Cooldown`,`effectmask`)
VALUES
(206150,0,0,0,0,0,0,1048576,0,0,0,0,4),
(206150,0,0,0,0,0,0,0,0,0,0,0,16379);
