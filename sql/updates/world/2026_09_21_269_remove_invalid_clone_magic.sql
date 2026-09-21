-- Spell 115284 is absent from Legion 7.3.5 (26972).  Keeping it in the
-- creature template spell list only schedules a cast that can never resolve.
DELETE FROM `creature_template_spell`
WHERE `entry` = 58964 AND `spell` = 115284;
