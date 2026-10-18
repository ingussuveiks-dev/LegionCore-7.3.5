-- Player artifact perks imported into the Vrekt automatic creature spell pool:
-- Dreadblades213738 and Maw of the Damned215377 are not NPC abilities.
-- Keep the existing AggressorAI path and every other spell/difficulty row.
DELETE FROM `creature_template_spell`
WHERE `entry` IN(115846,116180,116600,116601)
  AND `spell` IN(213738,215377);
