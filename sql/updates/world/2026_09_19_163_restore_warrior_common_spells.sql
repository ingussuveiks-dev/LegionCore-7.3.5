-- Restore Warrior common/talent handlers whose 7.3.5 client spells contain
-- dummy/script effects that cannot perform their documented action natively.

DELETE FROM `spell_script_names` WHERE `ScriptName` IN
('spell_warr_avatar', 'spell_warr_impending_victory', 'spell_warr_victory_rush',
 'spell_warr_commanding_shout', 'spell_warr_intimidating_shout',
 'spell_warr_shockwave', 'spell_warr_storm_bolt', 'spell_warr_revenge_trigger');

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(107574, 'spell_warr_avatar'),
(202168, 'spell_warr_impending_victory'),
(34428,  'spell_warr_victory_rush'),
(97462,  'spell_warr_commanding_shout'),
(5246,   'spell_warr_intimidating_shout'),
(46968,  'spell_warr_shockwave'),
(107570, 'spell_warr_storm_bolt'),
(5301,   'spell_warr_revenge_trigger');
