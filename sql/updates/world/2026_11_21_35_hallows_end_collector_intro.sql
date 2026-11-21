-- These two seasonal spawns are the corpse already present when the scene begins.
UPDATE `creature_template` SET `ScriptName` = 'npc_hallows_end_ren'
WHERE `entry` = 54205 AND `AIName` = '' AND `ScriptName` = '';

-- Use the native force cast for the explosion and validate the actual Sanath scene on its summon.
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(100668, 'spell_hallows_end_summon_void'),
(100669, 'spell_hallows_end_void_explosion');

UPDATE `creature_text` SET `Text` = 'My master owes you nothing. You will leave here empty-handed.',
    `Type` = 12, `comment` = 'Hallow End - Sanath denies the payment'
WHERE `CreatureID` = 54072 AND `GroupID` = 0 AND `ID` = 0 AND `comment` = 'Sanath Lim-yo - unleashed void';

INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Probability`, `comment`) VALUES
(54072, 1, 0, 'Someone followed you. Time to find out what this crate is capable of.', 12, 100, 'Hallow End - Sanath notices the witness'),
(54072, 2, 0, 'No! I cannot control it!', 12, 100, 'Hallow End - Sanath retreats');
