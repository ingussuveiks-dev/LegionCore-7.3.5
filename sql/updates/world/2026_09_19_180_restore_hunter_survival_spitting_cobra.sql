-- Spitting Cobra is a non-Pet guardian, so AnyPetAI never autocasts spell1.
-- Bind its explicit 2-second Cobra Spit casting AI and restore the 1.4 owner
-- ranged-AP coefficient measured for the 7.3.5 summon.
UPDATE `creature_template`
SET `AIName` = '', `ScriptName` = 'npc_hun_spitting_cobra'
WHERE `entry` = 104493;

UPDATE `pet_stats`
SET `ap` = -1.4, `ap_type` = 2, `spd` = 1.4, `school_mask` = 127,
    `maxspdorap` = 1, `type` = 1, `haste` = 1
WHERE `entry` = 104493;
