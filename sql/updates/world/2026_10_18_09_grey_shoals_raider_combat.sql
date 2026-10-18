-- These are player artifact perks, not Helarjar Raider combat abilities:
-- 213738 belongs to the Dreadblades; 215377 belongs to the Maw of the Damned.
-- Remove only the two imported self-cast rows. Preserve Battle Rage,
-- Flurry of Steel, other creatures, source types and externally edited rows.
DELETE FROM `smart_scripts`
WHERE `entryorguid`=115025 AND `source_type`=0 AND `event_type`=0
  AND `action_type`=11 AND `target_type`=1
  AND ((`id`=2 AND `action_param1`=213738)
    OR (`id`=3 AND `action_param1`=215377));
