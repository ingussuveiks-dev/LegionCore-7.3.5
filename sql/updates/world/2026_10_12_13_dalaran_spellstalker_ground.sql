-- Dalaran's Spellstalker uses the same ground felhound model (10950) as
-- the other Spellstalkers. Its 14 spawns are within 0.175 units of the
-- extracted city WMO floor, but this air-only row prevents settling.
-- Preserve positions, combat data, other NPCs and custom movement overrides.
UPDATE `creature_template_movement`
SET `Ground` = 1, `Flight` = 0
WHERE `CreatureId` = 114407 AND `Ground` = 0 AND `Flight` = 1;
