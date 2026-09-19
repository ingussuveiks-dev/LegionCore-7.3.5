-- Restore Legion 7.3.5 Survival active abilities and server-side proc filters.
DELETE FROM `spell_script_names`
WHERE (`spell_id` = 204315 AND `ScriptName` = 'spell_hun_animal_instincts')
   OR (`spell_id` = 201078 AND `ScriptName` = 'spell_hun_snake_hunter')
   OR (`spell_id` = 200163 AND `ScriptName` = 'spell_hun_throwing_axes')
   OR (`spell_id` = 201075 AND `ScriptName` = 'spell_hun_mortal_wounds')
   OR (`spell_id` = 201082 AND `ScriptName` = 'spell_hun_way_of_the_moknathal')
   OR (`spell_id` = 224764 AND `ScriptName` = 'spell_hun_bird_of_prey')
   OR (`spell_id` = 203755 AND `ScriptName` = 'spell_hun_aspect_of_the_skylord')
   OR (`spell_id` = 203757 AND `ScriptName` = 'spell_hun_eagles_bite_trait')
   OR (`spell_id` = 238125 AND `ScriptName` = 'spell_hun_echoes_of_ohnara')
   OR (`spell_id` = 203563 AND `ScriptName` = 'spell_hun_talon_strike');

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(204315, 'spell_hun_animal_instincts'),
(201078, 'spell_hun_snake_hunter'),
(200163, 'spell_hun_throwing_axes'),
(201075, 'spell_hun_mortal_wounds'),
(201082, 'spell_hun_way_of_the_moknathal'),
(224764, 'spell_hun_bird_of_prey'),
(203755, 'spell_hun_aspect_of_the_skylord'),
(203757, 'spell_hun_eagles_bite_trait'),
(238125, 'spell_hun_echoes_of_ohnara'),
(203563, 'spell_hun_talon_strike');

-- These generic links only cast one axe or attempt to cast Mongoose Bite on
-- the hunter. The scripts above restore three projectiles and spell charges.
DELETE FROM `spell_linked_spell`
WHERE (`spell_trigger` = 200163 AND `spell_effect` = 200167)
   OR (`spell_trigger` = 201078 AND `spell_effect` = 190928)
   OR (`spell_trigger` = 201091 AND `spell_effect` = 190928);

-- Animal Instincts must choose only among abilities currently recovering;
-- handle that selection in C++ rather than the unfiltered generic proc chain.
DELETE FROM `spell_trigger`
WHERE `spell_id` = 204315 AND `spell_trigger` = 232646;

DELETE FROM `spell_linked_spell`
WHERE `spell_trigger` = 232646
  AND `spell_effect` IN (190925, 202800);

-- Kill callbacks provide the exact behavior for these two artifact traits and
-- avoid duplicate cooldown changes from the old generic trigger rows.
DELETE FROM `spell_trigger`
WHERE (`spell_id` = 203749 AND `spell_trigger` = 109304)
   OR (`spell_id` = 203754 AND `spell_trigger` = 190925);
